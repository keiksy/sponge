#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

using namespace std;

bool TCPReceiver::segment_received(const TCPSegment &seg) {
    size_t old_window_size = window_size(), old_reasmbler_size = _reassembler.receive_bytes_count();
    const TCPHeader &header = seg.header();
    if (!_isn_valid) {
        if (header.syn) {
            _isn_valid = true;
            _isn = header.seqno;
            _lower_edge++;
        } else return false;
    }
    uint64_t abs_seq_no = header.syn ? unwrap(header.seqno, _isn, _ckp) : unwrap(header.seqno, _isn, _ckp)-1;
    if (abs_seq_no >= _capacity) {
        int a = 4;
        cout<<a;
        return false;
    }
    // 减一是要减去syn占的一个字节
    _reassembler.push_substring(seg.payload().copy(), abs_seq_no, header.fin);
    _lower_edge += _reassembler.receive_bytes_count() - old_reasmbler_size;
    _ckp = _lower_edge-1;
    if (header.fin) {
        _lower_edge++;
        _fin_valid = true;
    }
    return window_size() < old_window_size || _reassembler.receive_bytes_count() > old_reasmbler_size;
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    if (!_isn_valid) {
        return {};
    } else {
        uint64_t ans = _reassembler.ackno();
        if (_fin_valid) ans++;
        if (_isn_valid) ans++;
        return wrap(ans, _isn);
    }
}

size_t TCPReceiver::window_size() const {
    return _capacity - _lower_edge;
}