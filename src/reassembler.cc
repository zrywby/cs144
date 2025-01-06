#include "reassembler.hh"

using namespace std;

void Reassembler::insert(uint64_t first_index, std::string data, bool is_last_substring) {
    if (is_last_substring) {
        is_last_substring_ = true;
        last_index_ = first_index + data.size();
    }

    // 如果插入的数据在可接受范围之外，则丢弃
    if (first_index >= next_index_ + writer().available_capacity()) {
        return;
    }

    // 如果数据的一部分已经被写入，丢弃该部分数据
    if (first_index + data.size() <= next_index_) {
        if (is_last_substring_) {
            writer().close();
        }
        return;
    }

    // 跳过已经写入的部分，调整数据起始位置
    if (first_index < next_index_) {
        data = data.substr(next_index_ - first_index);
        first_index = next_index_;
    }

    // 计算可以写入的字节数
    uint64_t capacity = writer().available_capacity();
    uint64_t write_length = std::min(data.size(), capacity);

    // 写入数据
    if (write_length > 0) {
        writer().push(data.substr(0, write_length));
        next_index_ += write_length;
    }

    // 更新缓冲字节数
    if (write_length < data.size()) {
        buffered_bytes_ += data.size() - write_length;
    } else {
        buffered_bytes_ = 0; // 所有数据已写入，重置缓冲字节数
    }

    // 如果是最后一个子串且所有字节已写入，关闭流
    if (is_last_substring_ && next_index_ >= last_index_) {
        writer().close();
    }
}

uint64_t Reassembler::bytes_pending() const {
    // 返回未写入的数据字节数，缓冲中的数据和还未写入的剩余数据
    return buffered_bytes_ + (last_index_ > next_index_ ? last_index_ - next_index_ : 0);
}


