#include "bitmap.h"            //函数定义
#include "global.h"            //global.h 不清楚
#include "string.h"            //memset函数要用
#include "interrupt.h"         
#include "print.h"             
#include "debug.h"

#define BITMAP_MASK 1

void bitmap_init(struct bitmap* btmp)
{
    memset(btmp->bits,0,btmp->btmp_bytes_len);
    return;
}

bool bitmap_scan_test(struct bitmap* btmp,uint32_t bit_idx)  //一个8位的数 bit_idx/8 找数组下标 %得索引下的具体位置
{
    uint32_t byte_idx = bit_idx/8;
    uint32_t byte_pos = bit_idx%8;
    return (btmp->bits[byte_idx] & (BITMAP_MASK << byte_pos));
}

/*
   这个函数写很多的原因 是因为刚开始先用一个字节的快速扫描
   看是否每个字节中存在 位为0的位置 然后紧接着再看连续位 挺有意思 自己先写写看
*/

int bitmap_scan(struct bitmap* btmp, uint32_t cnt) {
   uint32_t idx_byte = 0;	 // 用于记录空闲位所在的字节
/* 先逐字节比较,蛮力法 */
   while (( 0xff == btmp->bits[idx_byte]) && (idx_byte < btmp->btmp_bytes_len)) {
/* 1表示该位已分配,所以若为0xff,则表示该字节内已无空闲位,向下一字节继续找 */
      idx_byte++;
   }

   ASSERT(idx_byte < btmp->btmp_bytes_len);
   if (idx_byte == btmp->btmp_bytes_len) {  // 若该内存池找不到可用空间		
      return -1;
   }

 /* 若在位图数组范围内的某字节内找到了空闲位，
  * 在该字节内逐位比对,返回空闲位的索引。*/
   int idx_bit = 0;
 /* 和btmp->bits[idx_byte]这个字节逐位对比 */
   while ((uint8_t)(BITMAP_MASK << idx_bit) & btmp->bits[idx_byte]) { 
	 idx_bit++;
   }
	 
   int bit_idx_start = idx_byte * 8 + idx_bit;    // 空闲位在位图内的下标
   if (cnt == 1) {
      return bit_idx_start;
   }

   uint32_t bit_left = (btmp->btmp_bytes_len * 8 - bit_idx_start);   // 记录还有多少位可以判断
   uint32_t next_bit = bit_idx_start + 1;
   uint32_t count = 1;	      // 用于记录找到的空闲位的个数

   bit_idx_start = -1;	      // 先将其置为-1,若找不到连续的位就直接返回
   while (bit_left-- > 0) {
      if (!(bitmap_scan_test(btmp, next_bit))) {	 // 若next_bit为0
	 count++;
      } else {
	 count = 0;
      }
      if (count == cnt) {	    // 若找到连续的cnt个空位
	 bit_idx_start = next_bit - cnt + 1;
	 break;
      }
      next_bit++;          
   }
   return bit_idx_start;
}

void bitmap_set(struct bitmap* btmp,uint32_t bit_idx,int8_t value)
{
    ASSERT(value == 1 || value == 0);
    uint32_t byte_idx = bit_idx/8;
    uint32_t byte_pos = bit_idx%8;
    if(value)	btmp->bits[byte_idx] |=  (BITMAP_MASK << byte_pos);
    else	btmp->bits[byte_idx] &= ~(BITMAP_MASK << byte_pos);
    return;
}

