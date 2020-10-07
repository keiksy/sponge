#include "tcp_connection.hh"

#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

using namespace std;

size_t TCPConnection::remaining_outbound_capacity() const {
    return _sender.stream_in().remaining_capacity();
}

size_t TCPConnection::bytes_in_flight() const {
    return _sender.bytes_in_flight();
}

size_t TCPConnection::unassembled_bytes() const {
    return _receiver.unassembled_bytes();
}

size_t TCPConnection::time_since_last_segment_received() const {
    return _curr_timestamp - _last_receive_segment_time;
}

void TCPConnection::segment_received(const TCPSegment &seg) {
    //从seg中提取出sender所需要的信息，然后将segment传给receiver
    if (!_sender.syn_sent()) {
        if (seg.header().syn) connect();
        else return;
    }
    _last_receive_segment_time = _curr_timestamp;
    if (seg.header().rst) {
        _receiver.stream_out().set_error();
        _sender.stream_in().set_error();
        _connected = false;
    }
    bool ack_ok = true;
    if (seg.header().ack && !_sender.ack_received(seg.header().ackno, seg.header().win)) ack_ok = false;
    if (!_sender.fin_sent() && seg.header().fin) {
        _linger_after_streams_finish = false;
    }
    if (_sender.fin_sent() && seg.header().ack && seg.header().ackno == _fin_seqno && _receiver.fin_received()
        && (!_linger_after_streams_finish)) {
        _connected = false;
    }
    if (ack_ok && seg.length_in_sequence_space()==0) return;
    _receiver.segment_received(seg);
    if (_receiver.ackno().has_value()) {
        if (_segments_out.empty()) {
            _sender.send_empty_segment();
            fill_outbound_queue();
        }
        _segments_out.front().header().ack = true;
        _segments_out.front().header().ackno = _receiver.ackno().value();
    }
}

bool TCPConnection::active() const {
    return _connected;
}

size_t TCPConnection::write(const string &data) {
    //先将data写入发送方的缓冲区，然后再让发送方包装成tcp报文，然后再将发送方的tcp报文取出，加上接收方的ack和窗口大小等信息，再发送出去
     size_t ans = _sender.stream_in().write(data);
     _sender.fill_window();
     fill_outbound_queue();
     if (_sender.consecutive_retransmissions() > _cfg.MAX_RETX_ATTEMPTS)
         _segments_out.front().header().rst = true;
     if (_receiver.ackno().has_value())  {
         _segments_out.front().header().ack = true;
         _segments_out.front().header().ackno = _receiver.ackno().value();
     }
     _segments_out.front().header().win = _receiver.window_size();
     return ans;
}

void TCPConnection::fill_outbound_queue() {
    std::queue<TCPSegment> &q = _sender.segments_out();
    while (!q.empty()) {
        TCPSegment& temp = q.front();
        if (temp.header().fin) {
            _fin_seqno = temp.header().seqno + temp.length_in_sequence_space();
        }
        q.pop();
        _segments_out.push(temp);
    }
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) {
    _curr_timestamp += ms_since_last_tick;
    _sender.tick(ms_since_last_tick);
    write("");
    if (time_since_last_segment_received() >= 10 * _cfg.rt_timeout
        && _receiver.unassembled_bytes() == 0 && _receiver.stream_out().eof()
        && _sender.bytes_in_flight() == 0 && _sender.stream_in().eof() && _sender.fin_sent()) {
        _connected = false;
    }
}

void TCPConnection::end_input_stream() {
    _sender.stream_in().end_input();
    write("");
}

void TCPConnection::connect() {
    write("");
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";
            if (_segments_out.empty()) write("");
            _segments_out.front().header().rst = true;
            _connected = false;
            _receiver.stream_out().set_error();
            _sender.stream_in().set_error();
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}