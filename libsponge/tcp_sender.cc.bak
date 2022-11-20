#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}                 // 初始超时时间，该值后续会变化
    , _stream(capacity)
    , _timer(retx_timeout) {}

// 返回已发送但是未确认的报文数量
uint64_t TCPSender::bytes_in_flight() const { 
    return _next_seqno - _recent_ack;
}

// TCP sender writes all of the fields of the TCPSegment 
// that were relevant to the TCPReceiver
// [the sequence number], 
// [the SYN flag], 
// [the payload], 
// [the FIN flag]. 

// 创建并发送报文段，只要条件允许
void TCPSender::fill_window() {

    // 连接已结束
    if (_FIN_is_sent) {
        return;
    }

    // window_size 实际是包含那些正在飞的字节
    // topdown 中的定义是：rwnd = RecBuffer - (LastRead - LastRcvd)
    int64_t receiver_spare_space = static_cast<int64_t>(_receiver_window_size) - bytes_in_flight();

    // 这里不可以加
    // 因为强行将 0 窗口大小赋值为 1 是接收方告知窗口为 0 时
    // 如果自己用完了是不会赋值为 0 的
    // 反正有已发送但是未确认的包，等对方回复就行了
    // if (payload_length == 0) {
    //     // cout << "reset" << endl;
    //     payload_length = 1;
    // }

    // cout << "_receiver_window_size: " << _receiver_window_size << endl;
    // cout << "bytes_in_fight(): " << bytes_in_flight() << endl;
    // cout << "receiver_spare_space = _receiver_window_size - bytes_in_flight(): " << receiver_spare_space << endl;
    // cout << "payload_length: " << payload_length << endl;
    

    while (!_SYN_is_sent || (receiver_spare_space > 0 && !_FIN_is_sent)) {
        TCPSegment tcpSegment;
        int64_t payload_length = std::min(static_cast<int64_t>(TCPConfig::MAX_PAYLOAD_SIZE), receiver_spare_space);
        
        if (!_SYN_is_sent) {
            // 未建立连接
            tcpSegment.header().syn = true;
            _SYN_is_sent = true;
            // _receiver_window_size -= 1;
            payload_length -= 1;
            cout << "write SYN" << endl;
        } else {
            // 上层没有数据 && 没结束
            if (!stream_in().buffer_size() && !stream_in().eof()) {
                cout << "return" << endl;
                return;
            }

            cout << "debug1" << endl;
            
            // 仍有数据未发送
            // cout << "stream_size: " << stream_in().buffer_size() << endl << endl;
            if (!stream_in().buffer_empty()) {
                // cout << "payload_length: " << payload_length << endl;
                tcpSegment.payload() = stream_in().read(payload_length);
                payload_length -= static_cast<int64_t>(tcpSegment.length_in_sequence_space());
                //_receiver_window_size -= tcpSegment.length_in_sequence_space();
                cout << "debug2" << endl;
            } else if (stream_in().eof() == false){
                // 对方有窗口，我方没数据 &&
                // 还没发送 FIN（我方有数据但是未到达，我方数据已到达但是还没发完）
                cout << "debug3" << endl;
                return;
            }

            // 如果遭遇 eof && 上层没有数据了 && 报文负载能够承担
            cout << "=== is fin ? ===" << endl;
            cout << stream_in().eof() << " " << stream_in().buffer_empty() << " " << payload_length << endl;
            if (stream_in().eof()
                && stream_in().buffer_empty()
                && tcpSegment.length_in_sequence_space() + 1 <= static_cast<size_t>(receiver_spare_space) /*payload_length >= 1*/ /*tcpSegment.length_in_sequence_space() + 1 <= payload_length*/) {

                cout << "debug4" << endl;

                //cout << "Sender set FIN = true !!!" << endl;
                tcpSegment.header().fin = true;
                payload_length -= 1;
                //_receiver_window_size -= 1;
                _FIN_is_sent = true;

            }
        }

        //cout << "after this transmission, payload_length = " << payload_length << " and data = " << tcpSegment.payload().str() << endl;

        tcpSegment.header().seqno = next_seqno();
        _next_seqno += tcpSegment.length_in_sequence_space();
        _segments_out.emplace(tcpSegment);
        _segment_outstanding.emplace(tcpSegment);

        receiver_spare_space = static_cast<int64_t>(_receiver_window_size) - bytes_in_flight();

        if (tcpSegment.payload().size() != 0) {
            cout << "send data: " << tcpSegment.payload().str() << endl;
        } else {
            if (tcpSegment.header().fin) {
                cout << "send fin" << endl;
            } else if (tcpSegment.header().syn){
                cout << "send syn" << endl;
            }
        }
        

        // 当上层交付报文准备发送 && 定时器未开启
        // 设置定时器
        if (_timer.is_running() == false) {
            _timer.restart();
        }

        // cout << "data: " << tcpSegment.payload().str() << endl << endl;
        // cout << "after transmit: " << "payload_length: " << payload_length << endl;
    }   
}


//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size

// 收到 ack
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) { 

    // 更新窗口
    _receiver_window_size = (window_size == 0) ?  1 :  window_size;
    _no_window = (window_size == 0) ? true : false;

    // cout << "---" << endl; 
    // cout << "window_size: " << window_size << endl;
    // cout << "receiver_window_size: " <<  _receiver_window_size << endl;
    
    // 来自对方的 ack 转换为本地 uint64_t
    uint64_t absolute_ack = unwrap(ackno, _isn, _recent_ack);
    cout << "Get ack: " << absolute_ack << endl;

    // 排除无效 ack
    if (absolute_ack <= _recent_ack || absolute_ack > next_seqno_absolute()) {
        return;
    }

    // 合法 ack 先更新了再说
    // _next_seqno = absolute_ack;

    // 收到有效 ack，检查是否可以清除本地缓存
    while (!_segment_outstanding.empty()) {
        TCPSegment tcpSegment = _segment_outstanding.front();
        size_t segment_length = tcpSegment.length_in_sequence_space();
        uint64_t segment_start_seqno = unwrap(tcpSegment.header().seqno, _isn, _recent_ack);
        uint64_t segment_end_seqno = segment_start_seqno + segment_length - 1;
        if (absolute_ack > segment_end_seqno) {
            _segment_outstanding.pop();
            _recent_ack = absolute_ack;
        } else {
            // 这里必须要退出
            // 不然会死循环
            // 不满足条件直接结束
            break;
        }
    }

    // 收到 ack 和新的 windows_size 之后要及时去发送数据包
    // cout << "recent_ack: " << _recent_ack << endl;
    // cout << "next_seqno: " << _next_seqno << endl;
    // cout << "bytes_in_flight: " << bytes_in_flight() << endl << endl;
    // fill_window();


    // --- 定时器 --- 

    // 1. Set the RTO back to its “initial value.”
    _timer.reset_to_intial();

    //2. If the sender has any outstanding data, 
    if (_segment_outstanding.empty() == false) {
        // restart the retransmission timer so that it
        // will expire after RTO milliseconds
        // (for the current value of RTO)
        _timer.restart();
    } else {
        _timer.stop();
    }

    // 3. Reset the count of 
    // “consecutive retransmissions” 
    // back to zero.
    _consecutive_retx_segments = 0;
    //cout << "Get ack and _consecuative_retx_segments = " << _consecutive_retx_segments << endl; 
}

//! \param[in] ms_since_last_tick ---> the number of milliseconds since the last call to this method
// 通知 TCPSender 距离上次 tick 过了多久
void TCPSender::tick(const size_t ms_since_last_tick) { 

    // 更新定时器
    _timer.set_alarm(ms_since_last_tick);

    cout << "current alarm: " << _timer.get_alarm() << endl;

    //cout << "current_alarm_left when tick: " << _timer.get_alarm() << endl;
    
    // If tick is called
    // && the retransmission timer has expired
    if (_timer.is_running() &&  _timer.is_expired()) {
        // Retransmit the earliest (lowest sequence number) 
        // segment that hasn’t been fully
        // acknowledged by the TCP receiver.
        TCPSegment earliest_seg = _segment_outstanding.front();
        _segments_out.emplace(earliest_seg);

        // 如果接收方窗口不为 0
        if (!_no_window) {
            // 记录连续重传报文数量
            ++ _consecutive_retx_segments;
            //cout << "consecutive_retx_segments: " << _consecutive_retx_segments << endl;
            _timer.double_RTO();
        }

        _timer.restart();
    }

    //cout << "current_alarm_left after tick: " << _timer.get_alarm() << endl;
}

// 返回连续重传事件发生的次数
unsigned int TCPSender::consecutive_retransmissions() const { 
    return _consecutive_retx_segments;
}

// 发送一个空的报文段
void TCPSender::send_empty_segment() {

}
