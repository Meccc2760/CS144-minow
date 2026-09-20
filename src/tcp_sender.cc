#include "tcp_sender.hh"
#include "debug.hh"
#include "tcp_config.hh"

using namespace std;

// This function is for testing only; don't add extra state to support it.
uint64_t TCPSender::sequence_numbers_in_flight() const
{
  return this->next_seqno_ - this->acked_no_;
}

// This function is for testing only; don't add extra state to support it.
uint64_t TCPSender::consecutive_retransmissions() const
{
  return this->consecutive_retransmissions_;
}

void TCPSender::push( const TransmitFunction& transmit )
{
  while(true){
    std::string payload;
    bool is_syn = (this->next_seqno_ == 0);
    uint64_t inflight = this->next_seqno_ - this->acked_no_;
    uint64_t available_window = this->receiver_available_capa_ == 0 ? 1 : this->receiver_available_capa_;

    if(inflight >= available_window){
      return;
    }

    uint64_t len = available_window - inflight;
    read(this->reader(), min(len - is_syn, TCPConfig::MAX_PAYLOAD_SIZE), payload);

    bool is_fin = this->reader().is_finished() && !this->fin_sent && len > is_syn + payload.size();
    if(is_fin){
      this->fin_sent = true;
    }
    TCPSenderMessage sendmsg {Wrap32::wrap(this->next_seqno_, this->isn_), is_syn, payload, is_fin, this->reader().has_error()};
    if(sendmsg.sequence_length() == 0){
      return;
    }
    outstanding_buffer_.push({sendmsg, this->next_seqno_, sendmsg.sequence_length()});
    transmit(sendmsg);
    this->next_seqno_ += sendmsg.sequence_length();
  }
}

TCPSenderMessage TCPSender::make_empty_message() const
{
  return TCPSenderMessage{Wrap32::wrap(this->next_seqno_, this->isn_), false, "", false, this->reader().has_error()};
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  if (msg.RST){
    this->reader().set_error();
  }
  this->receiver_available_capa_ = msg.window_size;

  if (msg.ackno.has_value()){
    uint64_t abs_ackno = msg.ackno.value().unwrap(this->isn_, this->next_seqno_);
    if(this->acked_no_ < abs_ackno && abs_ackno <= this->next_seqno_){
      this->acked_no_ = abs_ackno;
      this->curr_RTO_ms_ = this->initial_RTO_ms_;
      this->RTO_elapsed_ = 0;
      this->consecutive_retransmissions_ = 0;
      while (!outstanding_buffer_.empty()){
        const OutstandingSegment& segment = outstanding_buffer_.front();
        if (segment.first_no + segment.size <= this->acked_no_){
          outstanding_buffer_.pop();
        } else {
          break;
        }
      }
    }
  }
}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{
  if(this->outstanding_buffer_.empty()){
    return;
  }

  this->RTO_elapsed_ += ms_since_last_tick;
  if (this->RTO_elapsed_ >= this->curr_RTO_ms_){
    transmit(outstanding_buffer_.front().msg);

    if(this->receiver_available_capa_ > 0){
      this->consecutive_retransmissions_ += 1;
      this->curr_RTO_ms_ *= 2;
    }
    this->RTO_elapsed_ = 0; 
  }
  
}
