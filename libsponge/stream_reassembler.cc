#include "stream_reassembler.hh"

#include <vector>

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) :
_wait_idx(0), _eof_idx(INT64_MAX), _output(capacity), _capacity(capacity), _wait_vec(capacity) , _used_seq(){}

std::size_t StreamReassembler::copy_to_vec(const string &data, size_t begin_idx) {
    size_t dup = 0;
    for (size_t i = 0; i < data.size(); i++) {
        _used_seq.insert(begin_idx+i);
        if (data[i] == _wait_vec[begin_idx+i]) dup++;
        else _wait_vec[begin_idx+i] = data[i];
    }
    return data.size() - dup;
}

string StreamReassembler::string_of_range(std::size_t begin, std::size_t end) {
    string ans;
    for (size_t i = begin; i < end && _used_seq.count(i); i++)
        ans += _wait_vec[i];
    return ans;
}

void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    if (eof) _eof_idx = index + data.size();
    string to_write = data;
    if (!data.empty()) {
        if (_wait_idx > index) {
            string last = string_of_range(index, _wait_idx);
            if (data.find(last)==0 && data.size()>last.size()) {
                to_write = data.substr(last.size());
                _unassem_bytes += copy_to_vec(to_write, _wait_idx);
                _receive_bytes += to_write.size();
            } else return;
        } else {
            _unassem_bytes += copy_to_vec(to_write, index);
            _receive_bytes += to_write.size();
            if (_wait_idx < index) return;
        }
        while (_wait_idx < _eof_idx) {
            size_t write_num = _output.write(to_write);
            _wait_idx += write_num;
            _unassem_bytes -= write_num;
            if (write_num==0 || !_used_seq.count(_wait_idx)) break;
            to_write = string_of_range(_wait_idx, _capacity);
        }
    }
    if (_wait_idx >= _eof_idx) _output.end_input();
}

size_t StreamReassembler::unassembled_bytes() const {
    return _unassem_bytes;
}

bool StreamReassembler::empty() const {
    return _wait_idx>=_eof_idx;
}