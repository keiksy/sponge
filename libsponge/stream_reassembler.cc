#include "stream_reassembler.hh"

#include <vector>

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) :
_wait_idx(0), _eof_idx(INT64_MAX), _output(capacity), _capacity(capacity), _last_data(), _receive_bytes(0), _wait_map(){}

// 两个字符串的最长相同前缀的长度
size_t prefix_idx (const string &a, const string &b) {
    size_t loop_time = min(a.size(), b.size()), i = 0;
    while (i < loop_time) {
        if (a[i]==b[i]) i++;
        else break;
    }
    return i;
}

void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    if (eof) _eof_idx = index + data.size();
    string to_write = data;
    if (!data.empty()) {
        if (_wait_idx > index) {
            if (_wait_idx-_last_data.size()==index && data.find(_last_data)==0 &&
                    data.size() > _last_data.size()) {
                to_write = data.substr(_last_data.size());
                _wait_idx -= _last_data.size();
                _wait_map[_wait_idx] = data;
                _receive_bytes += to_write.size();
            } else return;
        } else {
            for (auto i = _wait_map.begin(); i!=_wait_map.end();) {
                size_t datae = index + to_write.size(), cure = i->second.size() + i->first;
                if (index == i->first && i->second.find(to_write)==0) {to_write = i->second; break;}
                if (i->first <= index && cure >= datae && i->second.find(to_write) != string::npos) return;
                if (i->first >= index && cure <= datae && to_write.find(i->second) != string::npos) i = _wait_map.erase(i);
                else i++;
            }
            _wait_map[index] = to_write;
            _receive_bytes += to_write.size();
            if (_wait_idx < index) return;
        }
        while (_wait_idx < _eof_idx) {
            _output.write(to_write);
            _last_data = _wait_map[_wait_idx];
            _wait_map.erase(_wait_idx);
            _wait_idx += to_write.size();
            if (_wait_map.count(_wait_idx)) to_write = _wait_map[_wait_idx];
            else break;
        }
    }
    if (_wait_idx >= _eof_idx) _output.end_input();
}

bool StreamReassembler::in_map(size_t index) const {
    auto got = _wait_map.find(index);
    return !(got == _wait_map.end());
}

size_t StreamReassembler::unassembled_bytes() const {
    size_t size = 0;
    for (const auto& i : _wait_map) {
        size += i.second.size();
    }
    return size;
}

bool StreamReassembler::empty() const {
    return _wait_idx>=_eof_idx;
}