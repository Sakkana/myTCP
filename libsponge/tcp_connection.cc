#include "tcp_connection.hh"

#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

// the number of `bytes` that can be written right now.
size_t TCPConnection::remaining_outbound_capacity() const { 
    return _sender.stream_in().remaining_capacity();
}

size_t TCPConnection::bytes_in_flight() const { 
    return _sender.bytes_in_flight();
}

size_t TCPConnection::unassembled_bytes() const { 
    return _receiver.unassembled_bytes();
}

// Number of milliseconds since the last segment was received
// 
size_t TCPConnection::time_since_last_segment_received() const { 
    return _time_since_last_seg_receive;
}

// the TCPConnection receives TCPSegments 
// from the Internet 
// when its segment received method is called.
// 小飞棍来咯
// 我是一个 endpoint，我需要收信也需要发信
void TCPConnection::segment_received(const TCPSegment &seg) { 

    // cout << "segment received" << endl;

    if (!active()) {
        return;
    }

    _time_since_last_seg_receive = 0;

    const TCPHeader header = seg.header();

    // --- RST SET ---

    if (header.rst) {
        unclean_shutdown();
        return;
    }

    // --- RST NOT SET ---

    // 还未建立连接就收到 ack
    if (!seg.header().syn && !_receiver.ackno().has_value()) {
        return;
    }

    // 是否为携带 ack
    if (header.ack) {
        // 内部会调用 fill_window 去生成报文
        // cout << "inform the receiver" << endl;
        _sender.ack_received(header.ackno, header.win);
    }

    // 将报文交给 receiver
    // 对方更新了 ack 和 window size
    _receiver.segment_received(seg);

    // 赶紧根据对方 window size 多产生一些报文
    _sender.fill_window();

    // 如果我方没有可发送的东西，产生空白报文
    // 针对对方 syn / 我方已 fin，对方发送 fin + ack 的情况
    // 因为这时候数据流里可能没数据，有可能因为 fin 我方 sender 不发送数据
    // 因此需要空数据包来回复对方
    if (_sender.segments_out().empty() && (seg.length_in_sequence_space() > 0)) {
        // cout << "pruduce empty segment" << endl;
        _sender.send_empty_segment();
    }

    // 存活确认报文
    if (_receiver.ackno().has_value() && seg.length_in_sequence_space() == 0 && seg.header().seqno == _receiver.ackno().value() - 1) {
        _sender.send_empty_segment();
    }
    
    // TCPConnection 赶紧去发送报文
    send_segments();

    // 检查是否要 linger
    clean_shutdown();
}

bool TCPConnection::active() const { 
    return _tcp_is_active;
}

// Write data to the outbound byte stream, and send it over TCP if possible
size_t TCPConnection::write(const string &data) {
    size_t len = data.size();
    size_t already_written = 0;
    while (already_written < len) {
        auto success_written = _sender.stream_in().write(data.substr(already_written));
        _sender.fill_window();
        already_written += success_written;
    }
    return already_written;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) {
    
    // 可能 tick 多次
    _time_since_last_seg_receive += ms_since_last_tick;

    _sender.tick(ms_since_last_tick);

    // cout << _time_since_last_seg_receive << endl;

    // 如果超过 linger 时间就干净关闭连接
    if (_sender.consecutive_retransmissions() > _cfg.MAX_RETX_ATTEMPTS) {
        unclean_shutdown();
        send_segments();
    } else {
        send_segments();
        clean_shutdown();
    }
}

// 允许接收字节，但是关闭往外发送的字节流
void TCPConnection::end_input_stream() {
    _sender.stream_in().end_input();
    _sender.fill_window();
    send_segments();
}

// 通过发送 SYN 建立连接
void TCPConnection::connect() {
    _sender.fill_window();
    send_segments();
}

TCPConnection::~TCPConnection() {
    unclean_shutdown();
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";

            // Your code here: need to send a RST segment to the peer
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}

// 发送
void TCPConnection::send_segments() {
    // cout << "send segments" << endl;
    TCPSegment tcpSegment;
    while (!_sender.segments_out().empty()) {
        tcpSegment = _sender.segments_out().front();
        if (_receiver.ackno().has_value()) {
            tcpSegment.header().ack = true;
            tcpSegment.header().ackno = _receiver.ackno().value();
        }
        if (_sender.stream_in().error() || _receiver.stream_out().error()) {
            tcpSegment.header().rst = true;
        }
        tcpSegment.header().win = _receiver.window_size();
        _segments_out.push(tcpSegment);
        _sender.segments_out().pop();
    }
}

void TCPConnection::unclean_shutdown() {
    _sender.stream_in().set_error();
    _receiver.stream_out().set_error();
    _tcp_is_active = false;
    // if (_sender.stream_in().buffer_empty()) {
    //     _sender.send_empty_segment();
    // }
    // send_segments();
}

void TCPConnection::clean_shutdown() {
    // If the inbound stream ends before the TCPConnection
    // has reached EOF on its outbound stream, 
    // this variable needs to be set to false.
    // 对方已经传输结束，我不需要 linger
    // linger 就是怕对方要传输东西过来
    // cout << "_receiver.stream_out().input_ended() == " << (_receiver.stream_out().input_ended() == true) << endl;
    // cout << "_sender.stream_in().input_ended() == " << (_sender.stream_in().input_ended() == true) << endl;    
    if (_receiver.stream_out().input_ended() && !_sender.stream_in().input_ended()) {
        // cout << "_linger_after_streams_finish = false" << endl;
        _linger_after_streams_finish = false;
    }

    // if linger after streams finish is false
    // && prerequisites #1 through #3 are satisfied, 
    // ---> the connection is “done” (and active() should return false) 
    
    if ((_sender.stream_in().input_ended() && _receiver.stream_out().input_ended() && !_sender.bytes_in_flight())) {
            // cout << "_tcp_is_active = false" << endl;
            if (!_linger_after_streams_finish || _time_since_last_seg_receive >= _cfg.rt_timeout * 10) {
                _tcp_is_active = false;
            }
        }
}