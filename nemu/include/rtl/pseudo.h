#ifndef __RTL_PSEUDO_H__
#define __RTL_PSEUDO_H__

#ifndef __RTL_RTL_H__
#error "Should be only included by <rtl/rtl.h>"
#endif

/* RTL pseudo instructions */

static inline def_rtl(li, rtlreg_t* dest, const rtlreg_t imm) { // 立即数读入
  rtl_addi(s, dest, rz, imm);
}

static inline def_rtl(mv, rtlreg_t* dest, const rtlreg_t *src1) {  // 寄存器传输
  rtl_addi(s, dest, src1, 0);
}

static inline def_rtl(not, rtlreg_t *dest, const rtlreg_t* src1) {  // 按位取反
  // dest <- ~src1
  rtl_xori(s, dest, src1, ~(word_t)0);
}

static inline def_rtl(neg, rtlreg_t *dest, const rtlreg_t* src1) { 
  // dest <- -src1
  rtl_sub(s, dest, rz, src1) ;
}

static inline def_rtl(sext, rtlreg_t* dest, const rtlreg_t* src1, int width) {  // 符号拓展
  // dest <- signext(src1[(width * 8 - 1) .. 0])
  int shift = (sizeof(word_t) - width) * 8;
  rtl_slli(s, t0, src1, shift);
  rtl_srai(s, dest, t0, shift);
}

static inline def_rtl(zext, rtlreg_t* dest, const rtlreg_t* src1, int width) { // 零拓展
  // dest <- zeroext(src1[(width * 8 - 1) .. 0])
  int shift = (sizeof(word_t) - width) * 8 ;
  rtl_slli(s, t0, src1, shift);
  rtl_srli(s, dest, t0, shift);
}

static inline def_rtl(msb, rtlreg_t* dest, const rtlreg_t* src1, int width) {  // 最高位符号赋值
  // dest <- src1[width * 8 - 1]
  rtl_zext(s, dest, src1, width);
}
#endif
