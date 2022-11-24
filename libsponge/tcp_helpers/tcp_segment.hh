#ifndef SPONGE_LIBSPONGE_TCP_SEGMENT_HH
#define SPONGE_LIBSPONGE_TCP_SEGMENT_HH

#include "buffer.hh"
#include "tcp_header.hh"

#include <cstdint>

//! \brief [TCP](\ref rfc::rfc793) segment
class TCPSegment {
  private:
    TCPHeader _header{};
    Buffer _payload{};

  public:
    //! \brief Parse the segment from a string
    // 将一个 string 解析为报文段
    ParseResult parse(const Buffer buffer, const uint32_t datagram_layer_checksum = 0);

    //! \brief Serialize the segment to a string
    // 将一个报文段序列化为一个 string
    BufferList serialize(const uint32_t datagram_layer_checksum = 0) const;

    //! \name Accessors
    //!@{
    const TCPHeader &header() const { return _header; }
    TCPHeader &header() { return _header; }

    const Buffer &payload() const { return _payload; }
    Buffer &payload() { return _payload; }
    //!@}

    //! \brief Segment's length in sequence space
    //! \note    if SYN is set, plus one byte if FIN is set
    /**
     * which calculates how many sequence numbers 
     * a segment occupies (including the fact that 
     * the SYN and FIN flags each occupy one sequence 
     * number, along with each byte of the payload).
     */
    size_t length_in_sequence_space() const;
};

#endif  // SPONGE_LIBSPONGE_TCP_SEGMENT_HH
