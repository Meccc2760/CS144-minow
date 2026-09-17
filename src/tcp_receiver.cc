#include "tcp_receiver.hh"
#include "debug.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{
  // Your code here.
  if(message.RST){
    this->reader().set_error(); 
  }
  if(!this->isn_.has_value()){
    if(!message.SYN){
      return;
    }
    this->isn_ = message.seqno;
  }

  uint64_t checkpoint = this->writer().bytes_pushed();
  uint64_t abs_seqno = message.seqno.unwrap(this->isn_.value(), checkpoint);
  uint64_t stream_idx = abs_seqno+ message.SYN - 1;

  this->reassembler_.insert(stream_idx, message.payload, message.FIN);
}

TCPReceiverMessage TCPReceiver::send() const
{
  // Your code here.
  uint16_t window_size = this->writer().available_capacity() > UINT16_MAX ? UINT16_MAX : this->writer().available_capacity();
  std::optional<Wrap32> ackno;
  if(this->isn_.has_value()){
    ackno = this->isn_.value()+this->writer().bytes_pushed() + 1;
    if(this->writer().is_closed()){
      ackno = ackno.value() + 1;
    }
  }
  return {ackno, window_size, this->writer().has_error()};
}
