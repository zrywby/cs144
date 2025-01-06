#include "wrapping_integers.hh"
using namespace std;
/**
 * 绝对转相对
  @param  n           绝对序列号
  @param zero_point   初始序列号
  @return Wrap32      相对序列号
  static_cast<uint32_t>(n) 相当于n取模2^32（转换过程取低32位）
*/
Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  uint32_t relative_ = static_cast<uint32_t>( n ) + zero_point.raw_value_;
  return Wrap32 { relative_ };
}
 

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  uint64_t upper = static_cast<uint64_t>( UINT32_MAX ) + 1; 
 
  uint32_t ckpt_mod = Wrap32::wrap( checkpoint, zero_point ).raw_value_;
 
  uint32_t distance = raw_value_ - ckpt_mod;
 
  if ( distance <= ( upper >> 1 ) || checkpoint + distance < upper ) {
    return checkpoint + distance;
  }
  return checkpoint + distance - upper;
}
