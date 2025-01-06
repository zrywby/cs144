#pragma once

#include "byte_stream.hh"
#include "tcp_receiver_message.hh"
#include "tcp_sender_message.hh"

#include <cstdint>
#include <functional>
#include <list>
#include <memory>
#include <optional>
#include <queue>
#include <map>

class TCPSender
{
public:
  TCPSender( ByteStream&& input, Wrap32 isn, uint64_t initial_RTO_ms )
    : input_( std::move( input ) ), isn_( isn ), initial_RTO_ms_( initial_RTO_ms ), raw_RTO_ms(initial_RTO_ms)
    , currentSeqNum_(isn), last_Ack_Seq(isn)
    , window_size_(2)
    , unAckedSegments()
  {}

  TCPSenderMessage make_empty_message() const;

  void receive( const TCPReceiverMessage& msg );

  using TransmitFunction = std::function<void( const TCPSenderMessage& )>;

  void push( const TransmitFunction& transmit );

  void tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit );

  uint64_t sequence_numbers_in_flight() const;
  uint64_t consecutive_retransmissions() const;
  Writer& writer() { return input_.writer(); }
  const Writer& writer() const { return input_.writer(); }

  const Reader& reader() const { return input_.reader(); }

private:
  void handleWindowProbe(const TransmitFunction& transmit);

  bool handleInitialSYN(TCPSenderMessage& message);

  void handlePayload(TCPSenderMessage& message);

  void handleSqeno(TCPSenderMessage& message);

  bool handleFIN(TCPSenderMessage& message);

  void resetRTO();

  void processACK(const TCPReceiverMessage& msg);

  void handle_ack();

  void handle_RTO();

  ByteStream input_;
  Wrap32 isn_;
  uint64_t initial_RTO_ms_;
  uint64_t raw_RTO_ms;

  Wrap32 currentSeqNum_;
  std::optional<Wrap32> last_Ack_Seq;

  uint16_t window_size_;

  std::map<uint64_t, TCPSenderMessage> unAckedSegments;
  bool is_SYN_ACK = false;

  uint64_t unAckedSegmentsNums = 0;
  uint64_t checkout = 0;
  uint64_t push_checkout = 0;

  uint64_t since_last_send = 0;
  bool is_RTO_double = false;

  bool isSYNSent_ = false;
  bool isFINSent_ = false;
};

void print(TCPSenderMessage message);
