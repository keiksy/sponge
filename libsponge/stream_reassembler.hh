#ifndef SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH
#define SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH

#include "byte_stream.hh"

#include <iostream>
#include <cstdint>
#include <unordered_map>
#include <string>

using namespace std;

//! \brief A class that assembles a series of excerpts from a byte stream (possibly out of order,
//! possibly overlapping) into an in-order byte stream.
class StreamReassembler {
  private:
    size_t _wait_idx;
    size_t _eof_idx;
    ByteStream _output;  //!< The reassembled in-order byte stream
    size_t _capacity;    //!< The maximum number of bytes
    string _last_data;
    // _receive_bytes统计StreamReassembler收到的字节总数，和byte_stream中的num_written变量不同，
    // 这个变量不仅仅统计了写入缓冲区的字节数，也统计了保存在_wait_map中的字节数，设置这个变量是为了告知发送方
    // 发送方所发送的字符串是否被收到（不管这个字符串是被写入了缓冲区还是被保存在了_wait_map中）
    size_t _receive_bytes;
    unordered_map<size_t, string> _wait_map;

    bool in_map(size_t index) const;

  public:
    //! \brief Construct a `StreamReassembler` that will store up to `capacity` bytes.
    //! \note This capacity limits both the bytes that have been reassembled,
    //! and those that have not yet been reassembled.
    StreamReassembler(const size_t capacity);

    //! \brief Receives a substring and writes any newly contiguous bytes into the stream.
    //!
    //! If accepting all the data would overflow the `capacity` of this
    //! `StreamReassembler`, then only the part of the data that fits will be
    //! accepted. If the substring is only partially accepted, then the `eof`
    //! will be disregarded.
    //!
    //! \param data the string being added
    //! \param index the index of the first byte in `data`
    //! \param eof whether or not this segment ends with the end of the stream
    void push_substring(const std::string &data, const uint64_t index, const bool eof);

    //! \name Access the reassembled byte stream
    //!@{
    const ByteStream &stream_out() const { return _output; }
    ByteStream &stream_out() { return _output; }
    //!@}

    //! The number of bytes in the substrings stored but not yet reassembled
    //!
    //! \note If the byte at a particular index has been submitted twice, it
    //! should only be counted once for the purpose of this function.
    size_t unassembled_bytes() const;

    //! \brief Is the internal state empty (other than the output stream)?
    //! \returns `true` if no substrings are waiting to be assembled
    bool empty() const;

    size_t receive_bytes_count() const {
        return _receive_bytes;
    }

    size_t ackno() const {
        return _wait_idx;
    }
};

#endif  // SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH
