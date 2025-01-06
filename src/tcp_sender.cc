#include "tcp_sender.hh"
#include "tcp_config.hh"
#include<iostream>

using namespace std;

uint64_t TCPSender::sequence_numbers_in_flight() const
{
  return unAckedSegmentsNums;
}

uint64_t TCPSender::consecutive_retransmissions() const
{
  uint64_t exponent = 0;
  uint64_t number = initial_RTO_ms_ / raw_RTO_ms;
  while (number > 1) {
      number /= 2;
      exponent++;
  }

  return exponent + is_RTO_double;
}

TCPSenderMessage TCPSender::make_empty_message() const
{
  TCPSenderMessage message;
  message.FIN = false;
  message.RST = input_.has_error();
  message.SYN = false;
  message.payload = "";
  message.seqno = currentSeqNum_;
  return message;
}

void TCPSender::push( const TransmitFunction& transmit )
{
  uint64_t windowSize = window_size_==0?1:window_size_;

  uint64_t bytes_to_send = input_.reader().bytes_buffered();
  uint64_t payload_len = min({input_.reader().bytes_buffered()
                            , static_cast<uint64_t>(TCPConfig::MAX_PAYLOAD_SIZE)
                            , static_cast<uint64_t>(windowSize) - sequence_numbers_in_flight()});

  TCPSenderMessage message = make_empty_message();

  if(!handleInitialSYN(message)){
    return;
  }

  do{
    if(static_cast<uint64_t>(windowSize) - sequence_numbers_in_flight() == 0){
      return ;
    }

    if(message.RST){
        transmit(message);
        return;
    }

    handlePayload(message);

    if(handleFIN(message)){
      return ;
    }
    
    handleSqeno(message);
    
    unAckedSegments[push_checkout] = message;

    push_checkout += payload_len;

    bytes_to_send -= payload_len;

    is_RTO_double = 0;

    transmit(message);

  }while(bytes_to_send >0);
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  if (!msg.ackno.has_value()) {
      if(!msg.window_size){
        input_.set_error();
      }
      return;     
  }    

  if(msg.RST){
      input_.set_error();
  }

  if(msg.ackno > currentSeqNum_ ){

    return;
  }

  if(last_Ack_Seq.has_value()){
    if(last_Ack_Seq >= msg.ackno && window_size_ == msg.window_size){
      return;
    }
  }
  is_SYN_ACK = true;

  last_Ack_Seq = msg.ackno.value();

  checkout = msg.ackno.value().unwrap(isn_,checkout);

  handle_ack();

  window_size_ = msg.window_size;

  unAckedSegmentsNums = currentSeqNum_.distance(msg.ackno.value());

  resetRTO();

  return;
}

void TCPSender::tick(uint64_t ms_since_last_tick, const TransmitFunction& transmit)
{
    since_last_send += ms_since_last_tick;

    if(window_size_ != 0 ){
      handle_RTO();
    }

    if (since_last_send >= initial_RTO_ms_) {
      since_last_send = 0;

      is_RTO_double = true;     

      if (!unAckedSegments.empty()) {
          transmit(unAckedSegments.begin()->second);
      }
    }
}

void TCPSender::handleWindowProbe(const TransmitFunction& transmit) {
    if (!window_size_) {
        TCPSenderMessage message = make_empty_message();

        handlePayload(message);

        if(handleFIN(message)){
          return ;
        }
        
        handleSqeno(message);
        print(message);
        transmit(message);
    }
}

bool TCPSender::handleInitialSYN(TCPSenderMessage& message) {
  if (isSYNSent_ && !input_.reader().bytes_buffered() && !input_.writer().is_closed()) {
      return false;  
  }

  if(!is_SYN_ACK && isSYNSent_){
    return false;  
  }

  if (!isSYNSent_) {
    message.SYN = true;
    isSYNSent_ = true;
  }
  return true;
}

bool TCPSender::handleFIN(TCPSenderMessage& message){  
  if(isFINSent_){
    return true;
  }     

  if (input_.writer().is_closed() &&
      !input_.reader().bytes_buffered() &&
      (window_size_ == 0 ? 1 : window_size_) - message.sequence_length() > 0){
      message.FIN = true;
      isFINSent_ = true;
  }

  return false; 
}

void TCPSender::handlePayload(TCPSenderMessage& message){
  uint64_t payload_len = min({input_.reader().bytes_buffered()
                            , static_cast<uint64_t>(TCPConfig::MAX_PAYLOAD_SIZE)
                            , static_cast<uint64_t>(window_size_ == 0?1:window_size_) - sequence_numbers_in_flight()});

  message.payload = std::string(input_.reader().peek().substr(0, payload_len));
  input_.reader().pop(payload_len);
}

void TCPSender::handleSqeno(TCPSenderMessage& message){    
    message.seqno = currentSeqNum_;

    currentSeqNum_ = currentSeqNum_ + message.sequence_length();

    unAckedSegmentsNums += message.sequence_length();
}

void TCPSender::resetRTO() {
    is_RTO_double = false;
    initial_RTO_ms_ = raw_RTO_ms;
    since_last_send = 0;
}

void TCPSender::processACK(const TCPReceiverMessage& msg) {
    if (!msg.ackno.has_value()) {
        return;
    }

    if (msg.RST) {
        input_.set_error();
    }

    if (msg.ackno > currentSeqNum_) {
        return;
    }

    if (last_Ack_Seq.has_value()) {
        if (last_Ack_Seq >= msg.ackno && window_size_ == msg.window_size) {
            return;
        }
    }
}

void TCPSender::handle_ack() {
    auto it = unAckedSegments.begin();
    while (it != unAckedSegments.end()) {
        uint64_t seq_no = it->first;
        uint64_t end_seq_no = seq_no + it->second.sequence_length();
        if (end_seq_no < checkout) {  
            it = unAckedSegments.erase(it);
        } else {
            ++it;
        }
    }
}

void TCPSender::handle_RTO(){
    if(is_RTO_double){
        initial_RTO_ms_ *=2;
        is_RTO_double = false;
    }
}

void print(TCPSenderMessage message){
//    std::cout << "Current Sequence Number: " << message.seqno.getuint32_t() << std::endl;
//    std::cout << "Current Sequence Number: " << static_cast<uint32_t>(message.seqno) << std::endl;
  
  std::cout << "Current Sequence Number: " << message.seqno.get() << std::endl;
    std::cout << "SYN: " << (message.SYN ? "true" : "false") << std::endl;
    std::cout << "payload: " << message.payload << std::endl;
    std::cout << "FIN: " << (message.FIN ? "true" : "false") << std::endl;
    std::cout << "RST: " << (message.RST ? "true" : "false") << std::endl;
    std::cout << "sequence_length: " << message.sequence_length() << std::endl;
}

