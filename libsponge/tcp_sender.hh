#ifndef SPONGE_LIBSPONGE_TCP_SENDER_HH
#define SPONGE_LIBSPONGE_TCP_SENDER_HH

#include "byte_stream.hh"
#include "tcp_config.hh"
#include "tcp_segment.hh"
#include "wrapping_integers.hh"
#include "util/timer.hh"

#include <functional>
#include <queue>
#include <algorithm>
#include <cassert>

//! \brief The "sender" part of a TCP implementation.

//! Accepts a ByteStream, divides it up into segments and sends the
//! segments, keeps track of which segments are still in-flight,
//! maintains the Retransmission Timer, and retransmits in-flight
//! segments if the retransmission timer expires.
//! 追踪已发送未确认的分组
//! 维护超时重传的定时器，做到超时重传

class TCPSender {
  private:
    //! our initial sequence number, the number for our SYN.
    WrappingInt32 _isn;

    //! outbound queue of segments that the TCPSender wants sent
    // 将要送出去的报文队列
    std::queue<TCPSegment> _segments_out{};

    //! retransmission timer for the connection
    //! 初始 RTO
    unsigned int _initial_retransmission_timeout;

    //! outgoing stream of bytes that have not yet been sent
    // 数据流
    ByteStream _stream;

    //! the (absolute) sequence number for the next byte to be sent
    // 下一个要发送的 seqno
    // 发送方绝对 seq (uint64) -> 网络相对 seq (uint32) ---> 对方绝对 seq (uint64)
    uint64_t _next_seqno{0};


    //! --- User add variables ---
    std::queue<TCPSegment> _segment_outstanding{};  // 已发送未确认的报文队列
    bool      _SYN_is_sent{false};            // 是否已经握手
    bool      _FIN_is_sent{false};            // 是否请求结束建立连接
    uint16_t  _receiver_window_size{0};       // 对方接收窗口的大小
    uint64_t  _recent_ack{0};                 // 最近一次 ack
    int       _consecutive_retx_segments{0};  // 连续的重传次数
    // uint64_t  _last_retx_seg{0};              // 上一个重传的报文的结尾 seqno
    int       _retx_cnt{0};                   // 连续重传的报文数量
    bool      _no_window{false};              // 对方窗口大小是否为 0
    Timer     _timer;                         // 定时器


    //! ---       end          ---

  public:
    //! Initialize a TCPSender
    TCPSender(const size_t capacity = TCPConfig::DEFAULT_CAPACITY,
              const uint16_t retx_timeout = TCPConfig::TIMEOUT_DFLT,
              const std::optional<WrappingInt32> fixed_isn = {});

    //! \name "Input" interface for the writer
    //!@{
    ByteStream &stream_in() { return _stream; }
    const ByteStream &stream_in() const { return _stream; }
    //!@}

    //! \name Methods that can cause the TCPSender to send a segment
    //!@{

    //! \brief A new acknowledgment was received
    //! 收到接收方发出的 ack
    void ack_received(const WrappingInt32 ackno, const uint16_t window_size);

    //! \brief Generate an empty-payload segment (useful for creating empty ACK segments)
    // 生成一个不包含负载的报文段
    void send_empty_segment();

    //! \brief create and send segments to fill as much of the window as possible
    // 生成并发送报文去填充接收方的窗口
    void fill_window();

    //! \brief Notifies the TCPSender of the passage of time
    void tick(const size_t ms_since_last_tick);
    //!@}

    //! \name Accessors
    //!@{

    //! \brief How many sequence numbers are occupied by segments sent but not yet acknowledged?
    //! \note count is in "sequence space," i.e. SYN and FIN each count for one byte
    //! (see TCPSegment::length_in_sequence_space())
    size_t bytes_in_flight() const;

    //! \brief Number of consecutive retransmissions that have occurred in a row
    unsigned int consecutive_retransmissions() const;

    //! \brief TCPSegments that the TCPSender has enqueued for transmission.
    //! \note These must be dequeued and sent by the TCPConnection,
    //! which will need to fill in the fields that are set by the TCPReceiver
    //! (ackno and window size) before sending.
    std::queue<TCPSegment> &segments_out() { return _segments_out; }
    //!@}

    //! \name What is the next sequence number? (used for testing)
    //!@{

    //! \brief absolute seqno for the next byte to be sent
    // 期待的 ackno 的本地绝对序号
    uint64_t next_seqno_absolute() const { return _next_seqno; }

    //! \brief relative seqno for the next byte to be sent
    // 将期待的 ackno 转换为网络中的相对序号
    WrappingInt32 next_seqno() const { return wrap(_next_seqno, _isn); }
    //!@}

    uint64_t Get_recent_ack() { return _recent_ack; }
    uint64_t Get_next_seqno() { return _next_seqno; }


};

#endif  // SPONGE_LIBSPONGE_TCP_SENDER_HH
