#include "tcp_connection.hh"

#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

using namespace std;

size_t TCPConnection::remaining_outbound_capacity() const {
    return _cfg.DEFAULT_CAPACITY - _used_space;
}

size_t TCPConnection::bytes_in_flight() const {
    //可能会忽略掉一些ack字段
    return _sender.bytes_in_flight();
}

size_t TCPConnection::unassembled_bytes() const {
    return _receiver.unassembled_bytes();
}

size_t TCPConnection::time_since_last_segment_received() const {
    return {};
}

void TCPConnection::segment_received(const TCPSegment &seg) {
    //从seg中提取出sender所需要的信息，然后将segment传给receiver
    if (seg.header().ack) _sender.ack_received(seg.header().ackno, seg.header().win);
    _receiver.segment_received(seg);
}

bool TCPConnection::active() const { return {}; }

size_t TCPConnection::write(const string &data) {
     size_t ans = _sender.stream_in().write(data);
     std::queue<TCPSegment> &q = _sender.segments_out();
     while (!q.empty()) {
         TCPSegment temp = q.front();
         q.pop();
         _segments_out.push(temp);
     }
     if (_receiver.ackno().has_value()) {
         _segments_out.front().header().ackno = _receiver.ackno().value();
     }
     _segments_out.front().header().win = _receiver.window_size();
     return ans;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) {
    _curr_timestamp += ms_since_last_tick;
    _sender.tick(ms_since_last_tick);
}

void TCPConnection::end_input_stream() {
    _sender.stream_in().end_input();
}

void TCPConnection::connect() {
    _sender.fill_window();
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";

            // Your code here: need to send a RST segment to the peer
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}
