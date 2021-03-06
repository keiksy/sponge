#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

using namespace std;

bool TCPReceiver::segment_received(const TCPSegment &seg) {
    size_t old_window_size = window_size(), old_reassem_size = _reassembler.stream_out().bytes_written();
    const TCPHeader &header = seg.header();
    if (header.syn) {
        if (!_isn_valid) {
            _isn_valid = true;
            _isn = header.seqno;
        } else return false;
    }
    if (header.fin) {
        if (_fin_valid || !_isn_valid) return false;
        else _fin_valid = true;
    }
    uint64_t abs_seq_no = header.syn ? unwrap(header.seqno, _isn, _ckp) : unwrap(header.seqno, _isn, _ckp)-1;
    if (abs_seq_no >= _capacity) return false;
    // 减一是要减去syn占的一个字节
    _reassembler.push_substring(seg.payload().copy(), abs_seq_no, header.fin);
    _lower_edge += _reassembler.stream_out().bytes_written() - old_reassem_size + header.syn + header.fin;
    _ckp = _lower_edge - 1;
    return window_size() < old_window_size;
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    if (!_isn_valid) {
        return {};
    } else {
        return wrap(_lower_edge, _isn);
    }
}

size_t TCPReceiver::window_size() const {
    return _capacity - _reassembler.receive_bytes_count() - _isn_valid - _fin_valid;
}