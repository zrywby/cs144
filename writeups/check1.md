### Checkpoint 1 Writeup

**My name:** [朱润宇]

**My SUNet ID:** [211220126]

**I collaborated with:** [NULL]

**I would like to credit/thank these classmates for their help:** [NULL]

**This lab took me about [10] hours to do. I [did] attend the lab session.**

**My secret code from section 2.1 was:** [http://cs144.keithw.org/hello]

**I was surprised by or edified to learn that:** [重叠数据片段的处理比预期要复杂，数据片段需要精细地管理，尤其是字节流中的状态控制。]

**Describe Reassembler implementation:**

在 `Reassembler` 的实现中，我的设计目的是将流中的重叠片段精确地组合并输出。这个实现由以下几个关键部分组成：

### 设计思路

#### 1. 状态管理

`Reassembler` 类使用了一些状态变量来跟踪重组数据流的状态：

* `next_index_`：表示下一个要写入的字节索引。

* `last_index_`：用于记录最后一个字节的索引，当接收到标记为最后的数据片段时更新。

* `is_last_substring_`：标记是否已经接收到最后一个数据片段。

* `buffered_bytes_`：用于追踪当前未写入但已接收的缓存字节数。

#### 2. 插入数据的逻辑

* **数据插入的条件控制**：如果插入的数据片段超出了可接受的范围（例如，起始索引大于当前可用的容量），则该数据片段会被直接丢弃。

* **重叠数据的处理**：在插入数据片段时，如果部分数据已经被写入，程序会调整数据的起始位置，仅保留还未写入的部分。如果新数据片段与已有的缓冲区数据部分重叠，需要合并并去除重复部分，只保留一个副本。

* **容量控制**：计算当前可写入的最大字节数，确保数据不会超过 `ByteStream` 的剩余容量。如果缓冲区中还有可写入的部分，则应尽快写入。

* **缓存的更新**：在数据写入后，剩余未写入的数据会被记录在缓存中，并更新 `buffered_bytes_`，以便后续读取操作可以精确反映当前状态。

* **最后数据片段的处理**：当接收到标记为最后的数据片段时，更新 `is_last_substring_` 和 `last_index_`。如果所有数据都已写入，调用 `writer().close()` 来关闭流。

#### 3. 关闭逻辑

* **流的关闭**：在接收到最后一个数据片段且所有数据都被写入时，调用 `writer().close()` 来关闭流，确保字节流的完整性和正确的结束。

### 实验细节

在本次实验中，我们需要确保每个接收到的子字符串能够被尽快插入到 `ByteStream` 中，这意味着在插入过程中要尽可能减少延迟。一旦有新的子字符串到达且可以写入，就应立即进行处理。此外，`buffer` 中的子字符串应该是唯一的且不重叠的，因此需要处理子字符串的合并以减少重复和浪费。

在实现中还需要注意的是，并不涉及 `read()` 的调用，所有逻辑集中于处理接收和插入的子字符串。

### 代码的优势与权衡

1. **优势**：
   
   * **状态管理清晰**：使用多个状态变量有效地跟踪数据的接收、缓存以及流的关闭状态，确保数据的完整性。
   
   * **处理重叠插入的能力**：实现中对数据的重叠部分进行了精确处理，确保不会重复写入已经存在的数据，保证了流的可靠性和有序性。

2. **不足**：
   
   * **复杂度高**：在插入时需要频繁地检查和调整数据的范围，使得代码在逻辑上变得更加复杂，特别是在处理重叠片段和缓存更新时。
   
   * **可能的性能瓶颈**：每次插入操作都需要检查并调整数据片段，可能会在大数据量情况下产生性能瓶颈。此外，处理合并重叠片段的逻辑也可能导致额外的计算开销。

### Optional Sections

* **I had unexpected difficulty with:** [对于数据重组过程中的重叠处理和边界情况感到有些挑战，特别是如何精确跟踪未写入的字节数。]

* **I think you could make this lab better by:** [提供更多关于如何设计高效数据重组器的提示，特别是关于边界条件的处理。]

* **I'm not sure about:** [是否所有的边界情况都已经被处理得当，例如，当插入的数据与现有数据流部分重叠时的具体处理逻辑。]

* **I contributed a new test case that catches a plausible bug not otherwise caught:** [provide Pull Request URL]
