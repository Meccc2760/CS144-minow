#include "wrapping_integers.hh"
#include "debug.hh"

using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  // Your code here.
  return Wrap32 { zero_point + (n & 0xFFFFFFFF) };
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  uint64_t iter = checkpoint >> 32;
  uint32_t offset = this->raw_value_ - zero_point.raw_value_;
  uint64_t seqno = (iter << 32) + offset;

  if (seqno + (1ULL << 31) < checkpoint)
    seqno += (1ULL << 32);
  else if (seqno > checkpoint + (1ULL << 31) && seqno >= (1ULL << 32))
    seqno -= (1ULL << 32);

  return seqno;
}
