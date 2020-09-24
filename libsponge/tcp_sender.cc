#include "tcp_sender.hh"
#include "tcp_config.hh"
#include <random>
#include <algorithm>
#include <iostream>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _init_retx_timeout{retx_timeout}
    , _stream(capacity)
    , _segments_fly()
    , _retx_timeout(retx_timeout){}


uint64_t TCPSender::bytes_in_flight() const {
    uint64_t ans = 0;
    for (auto it : _segments_fly)
        ans += it.segment.length_in_sequence_space();
    return ans;
}

void TCPSender::fill_window() {
    size_t send_size = 0;
    TCPSegment segment;
    TCPHeader &header = segment.header();
    header.seqno = wrap(_next_seqno, _isn);
    if (_cur_window_size>0 && _next_seqno==0) {
        header.syn = true;
        send_size++;
    }
    size_t payload_size = min(min(TCPConfig::MAX_PAYLOAD_SIZE-send_size, _stream.buffer_size()), _cur_window_size-send_size);
    segment.payload() = Buffer(_stream.peek_output(payload_size));
    _stream.pop_output(payload_size);
    send_size += payload_size;
    if (!_eof && _stream.eof() && _cur_window_size > send_size) {
        header.fin = true;
        send_size++;
        _eof = true;
    }
    if (send_size==0) return;
    _segments_out.push(segment);
    _segments_fly.emplace_back(_curr_timestamp, segment);
    _next_seqno += send_size;
    _cur_window_size -= send_size;
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
//! \returns `false` if the ackno appears invalid (acknowledges something the TCPSender hasn't sent yet)
bool TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    _retx_timeout = _init_retx_timeout;
    _consec_retx_count = 0;
    uint64_t temp = unwrap(ackno, _isn, _next_seqno);
    if (temp > _next_seqno) {
        return false;
    } else {
        _cur_window_size = window_size;
        while (!_segments_fly.empty() &&
                unwrap(_segments_fly.front().segment.header().seqno, _isn, _next_seqno) < temp) {
            _segments_fly.pop_front();
        }
        return true;
    }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    _curr_timestamp += ms_since_last_tick;
    if (_segments_fly.front().time_stamp + _retx_timeout <= _curr_timestamp) {
        _segments_out.push(_segments_fly.front().segment);
        _segments_fly.front().time_stamp = _curr_timestamp;
        _retx_timeout *= 2;
        _consec_retx_count++;
    }
}

unsigned int TCPSender::consecutive_retransmissions() const {
    return _consec_retx_count;
}

void TCPSender::send_empty_segment() {
    TCPSegment segment;
    segment.header().seqno = wrap(_next_seqno, _isn);
    _segments_out.push(segment);
}
