#include "wrapping_integers.hh"

// Dummy implementation of a 32-bit wrapping integer

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! Transform an "absolute" 64-bit sequence number (zero-indexed) into a WrappingInt32
//! \param n The input absolute 64-bit sequence number
//! \param isn The initial sequence number
//! absolute seqno → seqno.
WrappingInt32 wrap(uint64_t n, WrappingInt32 isn) {
    uint32_t seqno = (static_cast<uint32_t>(n) + isn.raw_value()) % (1ul << 32);
    return WrappingInt32(seqno);
}

//! Transform a WrappingInt32 into an "absolute" 64-bit sequence number (zero-indexed)
//! \param n The relative sequence number
//! \param isn The initial sequence number
//! \param checkpoint A recent absolute 64-bit sequence number
//! \returns the 64-bit sequence number that wraps to `n` and is closest to `checkpoint`
//!
//! \note Each of the two streams of the TCP connection has its own ISN. One stream
//! runs from the local TCPSender to the remote TCPReceiver and has one ISN,
//! and the other stream runs from the remote TCPSender to the local TCPReceiver and
//! has a different ISN.
//! seqno → absolute seqno
uint64_t unwrap(WrappingInt32 n, WrappingInt32 isn, uint64_t checkpoint) {
    // n 当前网络报文序号
    // isn 第一个网络报文序号
    // checkpoint 最近一次的报文本地绝对序号

    // 把期望转换成网络报文序号
    // 看看这次的报文起始点距离期望差多少
    int32_t offset = n  - wrap(checkpoint, isn);
    
    // 把这段差距加到期望上
    int64_t res = checkpoint + offset;

    // if (n.raw_value() == 1 && isn.raw_value() == 0 && checkpoint == 1)  cout << offset << "   " << res << endl;

    if (res >= 0) {
        return res;
    } else {
        return res + (1ul << 32);
    }

}


// 之前的实验版本

#if 0
    // 获取高位数量级掩码
    uint64_t level = checkpoint & 0xffffff00000000;

    // 当前 seqno 和 isn 的偏移
    uint64_t offset = n - isn;
    
    uint64_t abs_seqno = level | offset;

    auto ckp_seqno = wrap(checkpoint, isn);
    if (n.raw_value() < ckp_seqno.raw_value()) {
        abs_seqno += 1ul << 32;
    }

    return abs_seqno;
#endif

#if 0
    // checkpoint 的 seqno
    auto cp_seqno = wrap(checkpoint, isn);
    
    // 当前 sqeno 和 cp_seqno 的 偏移
    // 类内部有重载减号，返回 int32_t
    int32_t offset = n - cp_seqno;

    // 当前报文序号的绝对序号
    int64_t abs_seqno = checkpoint + offset;
    if (abs_seqno < 0) {
        abs_seqno += (1ul << 32);;
    }

    return abs_seqno;
#endif
