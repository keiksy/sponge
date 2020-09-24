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
    if (data.empty()) {
        if (eof) _output.end_input();
        return;
    }
    if (eof) _eof_idx = index + data.size();
    // 如果这次是对以前插入过字符串的一次重传
    if (_wait_idx > index) {
        // 如果新字符串的位置和上一次插入字符串的位置一样，那就执行比较或者追加操作
        // 比如上一次是在1位置插入了"ab"，这一次是要在1位置插入"abc"，那么就要在结尾追加一个"c"
        if (_wait_idx - _last_data.size() == index) {
            size_t p_idx = prefix_idx(data, _last_data);
            //上一次插入的字符串是新字符串的子串，执行追加操作
            if (p_idx >= _last_data.size() && p_idx < data.size()) {
                _output.write(data.substr(p_idx));
                _wait_map[_wait_idx - _last_data.size()] = data;
                _receive_bytes += data.size() - _last_data.size();
                _last_data = data;
                _wait_idx += data.size() - _last_data.size();
            }
        } else return;
    } else {
        vector<size_t> del;
        // 在map中清除掉所有为当前插入字符串的子串
        // 例如当前插入的是1，"abbc"，map中有一个2，"bb"，就和当前插入的产生了重合
        for (auto &i : _wait_map) {
            // datae是当前插入字符串的结尾的位置
            size_t datae = index+data.size(), cure = i.second.size()+i.first;
            if (index >= i.first && datae<=cure && i.second.find(data) != string::npos) return;
            if (i.first >= index && cure <= datae && data.find(i.second) != string::npos) del.push_back(i.first);
        }
        for (size_t key : del) {
            _wait_map.erase(key);
        }
        // 如果当前字符串的插入位置不是最想要的位置，那就先保存起来。然后直接返回
        _wait_map[index] = data;
        _receive_bytes += data.size();
        if (_wait_idx < index) {
            return;
        }
        // 当前字符串的插入位置正好是想要的位置，那就追加上去
        // 当前串追加上去以后，新的最想要的位置可能已经保存在map中了，所以要一直追加到map中没有符合要求的串为止
        // 比如当前map有：5，"bbb"，8，"ccc"，当前最想要的位置是2，当前插入的是2，"aaa"，
        // 把aaa插入以后，_wait_idx变成5，那就直接顺便把map里的两个串全部追加了，追加之后记得把该串从map中清除
        while (_wait_idx < _eof_idx && in_map(_wait_idx)) {
            _output.write(_wait_map[_wait_idx]);
            _last_data = _wait_map[_wait_idx];
            _wait_map.erase(_wait_idx);
            _wait_idx += _last_data.size();
        }
        // 该eof了
        if (_wait_idx >= _eof_idx) _output.end_input();
    }
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