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

uint64_t TCPSender::bytes_in_flight() const { 
    return _next_seqno - _recent_ack;
}

void TCPSender::fill_window() {

    if (_FIN_is_sent) {
        return;
    }

    int64_t receiver_spare_space = static_cast<int64_t>(_receiver_window_size) - bytes_in_flight();

    while (!_SYN_is_sent || (receiver_spare_space > 0 && !_FIN_is_sent)) {
        TCPSegment tcpSegment;
        int64_t payload_length = std::min(static_cast<int64_t>(TCPConfig::MAX_PAYLOAD_SIZE), receiver_spare_space);
        
        if (!_SYN_is_sent) {
            tcpSegment.header().syn = true;
            _SYN_is_sent = true;
            payload_length -= 1;
            // cerr << "SYN send" << endl;
        } else {
            if (stream_in().buffer_empty() && !stream_in().eof()) {
                return;
            }

            if (!stream_in().buffer_empty()) {
                tcpSegment.payload() = stream_in().read(payload_length);
                payload_length -= static_cast<int64_t>(tcpSegment.length_in_sequence_space());
            }
            
            if (stream_in().eof()
                && stream_in().buffer_empty()
                && tcpSegment.length_in_sequence_space() + 1 <= static_cast<size_t>(receiver_spare_space)) {
                
                tcpSegment.header().fin = true;
                payload_length -= 1;
                _FIN_is_sent = true;

                // cout << "end_input" << endl;
            }
            // cout << "payload left: " << payload_length << endl;
        }

        // cout << "fill window succeed" << endl;

        tcpSegment.header().seqno = next_seqno();
        _next_seqno += tcpSegment.length_in_sequence_space();
        _segments_out.emplace(tcpSegment);
        _segment_outstanding.emplace(tcpSegment);
        receiver_spare_space = static_cast<int64_t>(_receiver_window_size) - bytes_in_flight();

        if (_timer.is_running() == false) {
            _timer.restart();
        }
    }   
}


//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size

void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) { 

    // cerr << "ACK received" << endl;

    _receiver_window_size = (window_size == 0) ?  1 :  window_size;
    _no_window = (window_size == 0) ? true : false;

    uint64_t absolute_ack = unwrap(ackno, _isn, _recent_ack);

    if (absolute_ack <= _recent_ack || absolute_ack > next_seqno_absolute()) {
        return;
    }

    while (!_segment_outstanding.empty()) {
        TCPSegment tcpSegment = _segment_outstanding.front();
        size_t segment_length = tcpSegment.length_in_sequence_space();
        uint64_t segment_start_seqno = unwrap(tcpSegment.header().seqno, _isn, _recent_ack);
        uint64_t segment_end_seqno = segment_start_seqno + segment_length - 1;
        if (absolute_ack > segment_end_seqno) {
            _segment_outstanding.pop();
            _recent_ack = absolute_ack;
        } else {
            break;
        }
    }

    fill_window();

    // --- 定时器 --- 

    _timer.reset_to_intial();
    if (_segment_outstanding.empty() == false) {
        _timer.restart();
    } else {
        _timer.stop();
    }
    _consecutive_retx_segments = 0;
}

//! \param[in] ms_since_last_tick ---> the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) { 
    _timer.set_alarm(ms_since_last_tick);

    if (_timer.is_running() &&  _timer.is_expired()) {
        TCPSegment earliest_seg = _segment_outstanding.front();
        _segments_out.emplace(earliest_seg);

        if (!_no_window) {
            ++ _consecutive_retx_segments;
            _timer.double_RTO();
        }

        _timer.restart();
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { 
    return _consecutive_retx_segments;
}

void TCPSender::send_empty_segment() {
    TCPSegment empty_tcpSegment;
    empty_tcpSegment.header().seqno = next_seqno();
    _segments_out.emplace(empty_tcpSegment);
    // _segment_outstanding.emplace(empty_tcpSegment);
}
