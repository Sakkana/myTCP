#include "byte_stream.hh"
#include <algorithm>

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}


// 模拟字节流服务
ByteStream::ByteStream(const size_t capacity) : 
                        m_buffer(""), m_capacity(capacity), 
                        m_total_write(0), m_total_read(0),
                        m_is_end(false), _error(false)
                        {}

//! Write a string of bytes into the stream. Write as many
//! as will fit, and return how many were written.
//! \returns the number of bytes accepted into the stream
size_t ByteStream::write(const string &data) {
    size_t legal_write_size 
        = min(data.size(), m_capacity - m_buffer.size());   // 合法写入数量

    m_buffer += data.substr(0, legal_write_size);   // 写入缓冲区

    m_total_write += legal_write_size;              // 写入计数

    return legal_write_size;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    size_t legal_peek_size = min(m_buffer.size(), len);     // 合法观察数量
    string ret = m_buffer.substr(0, legal_peek_size);       // 拷贝数据
    return ret;
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
    size_t legal_pop_size = min(m_buffer.size(), len);     // 合法读出数量
    m_buffer = m_buffer.substr(legal_pop_size);         // 删除数据
    m_total_read += legal_pop_size;     // 读出计数
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    string ret = peek_output(len);
    pop_output(len);
    return ret;
}

//! Signal that the byte stream has reached its ending
void ByteStream::end_input() {
    m_is_end = true;
}

// //! \returns `true` if the stream input has ended
bool ByteStream::input_ended() const {
    return m_is_end;
}

//! \returns the maximum amount that can currently be read from the stream
size_t ByteStream::buffer_size() const {
    return m_buffer.size();
}

//! \returns `true` if the buffer is empty
bool ByteStream::buffer_empty() const { 
    return m_buffer.empty();    
}

//! \returns `true` if the output has reached the ending
bool ByteStream::eof() const {
    return m_buffer.empty() && input_ended();
}

//! Total number of bytes written
size_t ByteStream::bytes_written() const {
    return m_total_write;
}

//! Total number of bytes popped
size_t ByteStream::bytes_read() const {
    return m_total_read;
}

//! \returns the number of additional bytes that the stream has space for
size_t ByteStream::remaining_capacity() const { 
    return m_capacity - m_buffer.size();
}
