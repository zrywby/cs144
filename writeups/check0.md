Checkpoint 0 Writeup
====================

My name: [朱润宇]

My SUNet ID: [211220126]

I collaborated with: [NULL]

I would like to credit/thank these classmates for their help: [NULLe]

This lab took me about [8] hours to do. I [did] attend the lab session.

My secret code from section 2.1 was: [[cs144.keithw.org/hello](http://cs144.keithw.org/hello)]

I was surprised by or edified to learn that: [还能这么发邮件！HTTP 请求的构建有点麻烦，结尾细节不能写错。C++中也有try catch类似Java。很多意料之外的报错信息。]

Describe ByteStream implementation. [在这个 `ByteStream` 实现中，我们定义了两个类 `Writer` 和 `Reader`，分别用于向字节流中写入数据和从字节流中读取数据。`ByteStream` 类本身是一个基础类，封装了字节流的容量、缓冲区、以及跟踪状态的各种变量。`Reader` 和 `Writer` 类继承自 `ByteStream`，并实现了各自的具体功能。

### 设计思路

#### 1. 数据结构的选择

考虑使用队列作为stream buffer的底层数据结构，因为字节流需要先进先出。

#### 2. 状态管理

`ByteStream` 类中定义了一些状态变量来跟踪字节流的状态：

* `_pushedBytes`：记录累积的写入字节数。
* `_poppedBytes`：记录累积的读取字节数。
* `_isClosed`：标记字节流是否已经关闭，表示不再有更多数据写入。
* `_isError`：标记是否发生了错误。

#### 3. 功能实现

`Writer` 和 `Reader` 类分别实现了字节流的写入和读取功能：

* **`Writer`**：
  
  * **`void set_error()`**
    
    * 函数用于设置错误状态。当调用该函数时，它会将流的错误状态标志设为 `true`，表示在写入过程中发生了错误。
  
  * **`void push(std::string data)`**
    
    * 函数将数据推送到 `ByteStream` 中，但只允许在可用容量范围内写入。如果流的缓冲区中还有剩余空间（由 `available_capacity()` 决定），则它会将传入的字符串数据逐个字节写入到流中。如果没有足够的空间，则多余的数据将被丢弃。
  
  * **`void close()`**
    
    * 函数表示写入操作已经完成，流已经关闭。调用该函数会将 `_isClosed` 标志设为 `true`，表明不会再有新的数据被写入。
  
  * **`bool is_closed() const`**
    
    * 函数用于检查流是否已经关闭。它返回一个布尔值 `true` 或 `false`，表示写入端是否调用了 `close()` 函数，确认流不再写入任何数据。
  
  * **`uint64_t available_capacity() const`**
    
    * 函数返回流当前的剩余容量，即当前缓冲区中还可以容纳多少字节数据。它通过计算 `capacity - _buf.size()` 来得出结果。
  
  * **`uint64_t bytes_pushed() const`**
    
    * 函数返回总共写入的字节数，即累积写入到流中的数据量。这个值不会减少，它是流生命周期中所有写入数据的总和。

* **`Reader`**：
  
  * 1. **`std::string_view peek() const`**
       
       * 函数用于查看流中下一个将要读取的字节。它不会移除该字节，而是通过 `std::string_view` 返回当前队列中的第一个字节。如果缓冲区为空，返回空 `{}`。
    
    2. **`void pop(uint64_t len)`**
       
       * 函数用于从流中移除指定数量的字节。它从缓冲区中逐个字节地移除，直到达到 `len` 指定的数量或缓冲区为空时停止。每移除一个字节，会更新累积弹出的字节数 `_poppedBytes`。
    
    3. **`bool has_error() const`**
       
       * 这个函数用于检查流是否发生了错误。它返回一个布尔值，表示流是否处于错误状态。错误状态通常由 `set_error()` 函数在 `Writer` 端设置。
    
    4. **`bool is_finished() const`**
       
       * 函数用于检查流是否已经完成，即所有数据都被读取并且写入端已经关闭。
    
    5. **`uint64_t bytes_buffered() const`**
       
       * 函数返回当前缓冲区中尚未被读取的字节数。它表示缓冲区中可以供读取的数据量，计算方式为 `_buf.size()`。
    
    6. **`uint64_t bytes_popped() const`**
       
       * 函数返回总共弹出的字节数，即累积从流中移除的数据量。这个值不会减少，是从流开始到当前为止所有读取操作移除的字节总和。

### 代码的优势与权衡

1. **优势**：
   
   * **简单清晰**：基于 `queue` 使得数据的进出流十分简单，代码易于理解和维护。
   * **性能稳定**：`std::queue` 的操作在时间复杂度上都是常数时间 `O(1)`，对于频繁的读写操作效率较高。
   * **状态管理明确**：通过 `isClosed` 和 `isError` 等标志位可以很方便地处理流的关闭和错误情况。

2. **不足**：
   
   * **固定容量**：虽然设计时采用了 `available_capacity` 进行容量控制，但如果流中要写入的数据量较大，仍然可能遇到需要处理超容量的情况。在实际应用中，需要额外设计策略来处理超出容量的写入请求。
   * **内存占用**：`std::queue` 会对每个元素进行单独的内存分配，对于小量数据可能会带来一些内存开销。
- Optional: I had unexpected difficulty with: [不熟悉的代码，常用语法错误，虚拟机使用较为麻烦。]

- Optional: I think you could make this lab better by: [不了解TCPSocket和HTTP。]

- Optional: I'm not sure about: [有时候我的代码运行时间较长，响应不够迅速。]

- Optional: I contributed a new test case that catches a plausible bug
  not otherwise caught: [provide Pull Request URL]
