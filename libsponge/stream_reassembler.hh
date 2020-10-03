#ifndef SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH
#define SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH

#include "byte_stream.hh"

#include <iostream>
#include <cstdint>
#include <unordered_set>
#include <string>
#include <vector>

using namespace std;

//! \brief A class that assembles a series of excerpts from a byte stream (possibly out of order,
//! possibly overlapping) into an in-order byte stream.
class StreamReassembler {
  private:
    // 等待接收的下一个字节的绝对序列号
    size_t _wait_idx;
    //最后一个字节的序列号+1
    size_t _eof_idx;
    ByteStream _output;  //!< The reassembled in-order byte stream
    size_t _capacity;    //!< The maximum number of bytes
    // 收到的字节总数
    size_t _receive_bytes{0};
    // 收到但未重整的字节总数
    std::size_t _unassem_bytes{0};
    // 用来记忆收到但未重整的字节的数据结构，比如当前需要序列号为3的字节，来了一个index=5，data="abc"的串
    // 这个串无法被放到输出中，所以要先记忆起来，将这个串复制到_wait_vec中下标为5开始的三个字节中。
    vector<char> _wait_vec;
    // 将无法重整的串放到_wait_vec中的方法，返回此次复制中新加入字节的个数
    // 例如当前_wait_vec中现有的以4开始的2个字节是"bc"，来了一个index=3，data="abcd"的串，新串的"bc"部分
    // 和原有的产生了重复，因此不用重复写入
    std::size_t copy_to_vec(const string &data, std::size_t begin_idx);
    // 获取_wait_vec中以begin开始，以end结束的字符串
    string string_of_range(std::size_t begin, std::size_t end);
    // 保存_wait_vec中已经被占据的字节的序列号，用来识别字符串的中断
    // 例如来了一个index=3，data="abc"的串，那就将3，4，5放到_used_seq中
    // 这样做是为了在使用上一个方法提取字符串时用来识别字符串是否已经中断
    unordered_set<size_t> _used_seq;

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
