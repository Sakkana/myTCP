#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

/**
 * TCPReceiver::segment received() will be called 
 * each time a new segment is received from the peer.
 */
void TCPReceiver::segment_received(const TCPSegment &seg) {
    // Set the Initial Sequence Number if necessary

    // 拆解头部
    auto seg_header = seg.header();

#if 0
    // 错误代码
    if (!this->SYN && seg_header.syn) {

    }
#endif

    // 如果没有建立过连接, 那么这个报文可能是 syn 报文
    if (!this->SYN) {
        // 如果接收到的报文没有携带 syn
        if (!seg_header.syn) {
            return;
        }

        // 若携带了 syn, 正式建立连接
        this->SYN = true;
        this->ISN = seg_header.seqno;
    }

    // 期望报文本地序列号
    uint64_t checkpoint = this->_reassembler.stream_out().bytes_written() + 1;

    // 网络报文本地序列号
    uint64_t seqno_64 = unwrap(seg_header.seqno, this->ISN, checkpoint);

    // Push any data, or end-of-stream marker, to the StreamReassembler

    // 如果是初次连接, 在字节流中数据是从 0 开始的
    // 如果是后续的数据传输, 在字节流中该段是从 seqno 开始的
    // 但是由于 syn 也占据了发送方 index 中的一个序号, 因此实际上数据是从 seqno = 1 开始的
    // 因此后续需要往字节流中写入 seqno - 1

    // bug 传进去的 index 不对 ---> SYN 那里的条件判断写错了，导致 ISN 被修改了
    auto index = seg_header.syn ? 0 : seqno_64 - 1;
    // cout << "index   " << index << endl << endl;
    this->_reassembler.push_substring(seg.payload().copy(), index, seg_header.fin);
}

/**
 * Returns an optional<WrappingInt32> containing 
 * the sequence number of the first byte that 
 * the receiver doesn’t already know. 
 * This is the windows’s left edge: 
 * the first byte the receiver is interested in receiving. 
 * If the ISN hasn’t been set yet, return an empty optional.
 */

optional<WrappingInt32> TCPReceiver::ackno() const { 
    // 如果未建立连接
    if (!this->SYN) {
        return nullopt;
    }

    uint64_t first_unassemable = this->_reassembler.stream_out().bytes_written() + 1;

    // std::cout << first_unassemable << std::endl;

    // 如果请求断开连接
    if (this->_reassembler.stream_out().input_ended()) {
        ++ first_unassemable;
    }

    return wrap(first_unassemable, this->ISN);
}

/**
 * Returns the distance between 
 * the “first unassembled” index 
 * (the index corresponding to the ackno) 
 * and the “first unacceptable” index
 * 
 * 当前窗口大小: 可用大小
 */ 

size_t TCPReceiver::window_size() const { 
    return this->_capacity - this->_reassembler.stream_out().buffer_size();
 }
