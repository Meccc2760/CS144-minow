#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ), buffer(""), pushed_size(0), poped_size(0), isclosed(false) {}

void Writer::push( string data )
{
  // Your code here.
  if(not isclosed){
    uint64_t accept_size = min(available_capacity(), data.size());
    this->buffer += data.substr(0, accept_size);
    this->pushed_size += accept_size;
  }
}

void Writer::close()
{
  // Your code here.
  this->isclosed = true;
}

bool Writer::is_closed() const
{
  return isclosed; // Your code here.
}

uint64_t Writer::available_capacity() const
{
  return capacity_ - buffer.size(); // Your code here.
}

uint64_t Writer::bytes_pushed() const
{
  return pushed_size; // Your code here.
}

string_view Reader::peek() const
{
  return buffer; // Your code here.
}

void Reader::pop( uint64_t len )
{
  // Your code here.
  uint64_t buffer_size = bytes_buffered();
  uint64_t poping_size = min(buffer_size, len);
  buffer = buffer.substr(len, buffer_size-poping_size);
  poped_size += poping_size;
  
}

bool Reader::is_finished() const
{
  return isclosed && buffer.size() == 0; // Your code here.
}

uint64_t Reader::bytes_buffered() const
{
  return static_cast<uint64_t>(buffer.size()); // Your code here.
}

uint64_t Reader::bytes_popped() const
{
  return poped_size; // Your code here.
}
