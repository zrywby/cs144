#pragma once

#include "byte_stream.hh"

class Reassembler {
public:
    // 构造函数接受左值引用或右值引用
    explicit Reassembler(ByteStream&& output)
        : output_(std::move(output)), next_index_(0), last_index_(0), is_last_substring_(false), buffered_bytes_(0) {}

    // 插入新数据片段
    void insert(uint64_t first_index, std::string data, bool is_last_substring);

    // 返回未写入的字节数
    uint64_t bytes_pending() const;

    // 返回 ByteStream 的 reader（非 const 版本）
    Reader& reader() { return output_.reader(); }

    // 返回 ByteStream 的 reader（const 版本）
    const Reader& reader() const { return output_.reader(); }

    // 返回 ByteStream 的 writer（非 const 版本）
    Writer& writer() { return output_.writer(); }

    // 返回 ByteStream 的 writer（const 版本）
    const Writer& writer() const { return output_.writer(); }

private:
    ByteStream output_;  // ByteStream 现在存储为成员变量
    uint64_t next_index_;  // 下一个需要写入的字节索引
    uint64_t last_index_;  // 最后一个字节的索引
    bool is_last_substring_;  // 标记是否为最后一个数据片段
    uint64_t buffered_bytes_;  // 用于追踪缓存的字节数
};

