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
    if(message.SYN){
      this->isn_ = message.seqno;
    }else{
      return;
    }
  }

  uint64_t checkpoint = this->writer().bytes_pushed();
  uint64_t abs_seqno = message.seqno.unwrap(this->isn_.value(), checkpoint);
  uint64_t stream_idx = abs_seqno+ message.SYN - 1;

  std::string data = message.payload;
  this->reassembler_.insert(stream_idx, data, message.FIN);
}

TCPReceiverMessage TCPReceiver::send() const
{
  // Your code here.
  return {};
}
