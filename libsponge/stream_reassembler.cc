#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : 
    _output(capacity), _capacity(capacity),
    _first_unread(0), _first_unasb(0), 
    _cache(), 
    _meet_eof(false), _eof_idx(0)
    {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {

#if 0
    cout << "分组内容   " << data << endl;
    cout << "分组编号   " << index << endl;
    if (eof) cout << "结尾了" << endl;
    else cout << "没有结尾" <<   endl;
#endif

    // 读取起点 (first_unassembled || 窗口内且在分组范围内)
    size_t start_idx = max(_first_unasb, index);

    // 窗口的起点和终点 
    size_t capacity_end   = _first_unread + _capacity;

    // 读取的终点 (分组的末尾 || 窗口的末尾)
    size_t end_idx = min(index + data.size(), capacity_end);

    if (eof) {
        _meet_eof = true;
        _eof_idx = index + data.size();
    }

    // 缓存乱序分组
    // start_idx ~ end_idx 之间的数据是合法的可读取的
    for (size_t i = start_idx; i < end_idx; ++ i)
        _cache[i] = data[i - index];
    
    // uodate first unassembled pointer
    while (_cache.find(_first_unasb) != _cache.end())
        ++ _first_unasb;

    // assemble the contiguous bags
    string assembled_data;

    // 合法的剩余空间
    size_t legal_write = min (_first_unasb - _first_unread,       // all 
                              _output.remaining_capacity());    // space

    // write ordered bytes into  a new string       
    while (legal_write) {
        assembled_data.push_back(_cache[_first_unread]);
        _cache.erase(_first_unread);
        ++ _first_unread;
        -- legal_write;
    }

    // write into bytestream
    _output.write(assembled_data);

    if (_meet_eof && empty())
        _output.end_input();
    
#if 0
    cout << "map: " << _cache.size() << endl;
    cout << "stream buffer: " << _capacity - _output.remaining_capacity() << endl;
    cout << "===============结尾标记: " << (empty() == true) << endl; 
    cout << endl;
#endif
}

size_t StreamReassembler::unassembled_bytes() const { 
    return _cache.size() - (_first_unasb - _first_unread);
}

bool StreamReassembler::empty() const { 
    return _meet_eof && unassembled_bytes() == 0 && _cache.size() == 0; 
}
