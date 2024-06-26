#include <cpu/ifetch.h>
#include <isa-all-instr.h>

def_all_THelper();

static uint32_t get_instr(Decode *s) {
  return s->isa.instr.val;
}

// decode operand helper
#define def_DopHelper(name) void concat(decode_op_, name) (Decode *s, Operand *op, word_t val, bool flag)

static inline def_DopHelper(i) 
{
  op->imm = val;
}

static inline def_DopHelper(r) 
{
    bool is_write = flag;
    static word_t zero_null = 0;
    op->preg = (is_write && val == 0) ? &zero_null : &gpr(val);
}

static inline def_DHelper(I) 
{
  decode_op_r(s, id_src1, s->isa.instr.i.rs1, false);
  decode_op_i(s, id_src2, (sword_t)s->isa.instr.i.simm11_0, false);
  decode_op_r(s, id_dest, s->isa.instr.i.rd, true);
}

static inline def_DHelper(U) 
{
    decode_op_i(s, id_src1, (sword_t)s->isa.instr.u.simm31_12 << 12, true);
    decode_op_r(s, id_dest, s->isa.instr.u.rd, true);
}

static inline def_DHelper(S) 
{
    decode_op_r(s, id_src1, s->isa.instr.s.rs1, false);
    sword_t simm = (s->isa.instr.s.simm11_5 << 5) | s->isa.instr.s.imm4_0;
    decode_op_i(s, id_src2, simm, false);
    decode_op_r(s, id_dest, s->isa.instr.s.rs2, false);
}

static inline def_DHelper(SS) 
{
    decode_op_r(s, id_src1, s->isa.instr.s.rs1, false);
    decode_op_r(s, id_src2, s->isa.instr.s.rs2, false);
    decode_op_r(s, id_dest, s->isa.instr.s.imm4_0, true);
}

def_THelper(load) 
{
    def_INSTR_TAB("??????? ????? ????? 011 ????? ????? ??", ld);
    def_INSTR_TAB("??????? ????? ????? 001 ????? ????? ??", lh);
    def_INSTR_TAB("??????? ????? ????? 010 ????? ????? ??", lw);
    def_INSTR_TAB("??????? ????? ????? 000 ????? ????? ??", lb);
    def_INSTR_TAB("??????? ????? ????? 100 ????? ????? ??", lbu);
    def_INSTR_TAB("??????? ????? ????? 101 ????? ????? ??", lhu);

    return EXEC_ID_inv;
}

def_THelper(store) 
{
  def_INSTR_TAB("??????? ????? ????? 011 ????? ????? ??", sd);
  def_INSTR_TAB("??????? ????? ????? 001 ????? ????? ??", sh);
  def_INSTR_TAB("??????? ????? ????? 000 ????? ????? ??", sb);
  def_INSTR_TAB("??????? ????? ????? 010 ????? ????? ??", sw);

  return EXEC_ID_inv;
}

def_THelper(binop) 
{
    def_INSTR_TAB("0000000 ????? ????? 000 ????? ????? ??", add);
    def_INSTR_TAB("0100000 ????? ????? 000 ????? ????? ??", sub);
    def_INSTR_TAB("0000000 ????? ????? 111 ????? ????? ??", and);
    def_INSTR_TAB("0000000 ????? ????? 110 ????? ????? ??", or);
    def_INSTR_TAB("0000000 ????? ????? 001 ????? ????? ??", sll);
    def_INSTR_TAB("0000000 ????? ????? 010 ????? ????? ??", slt);
    def_INSTR_TAB("0000000 ????? ????? 011 ????? ????? ??", sltu);
    def_INSTR_TAB("0000000 ????? ????? 100 ????? ????? ??", xor);

    def_INSTR_TAB("0000001 ????? ????? 000 ????? ????? ??", mul);
    def_INSTR_TAB("0000001 ????? ????? 100 ????? ????? ??", div);
    def_INSTR_TAB("0000001 ????? ????? 101 ????? ????? ??", divu);
    def_INSTR_TAB("0000001 ????? ????? 110 ????? ????? ??", rem);
    def_INSTR_TAB("0000001 ????? ????? 111 ????? ????? ??", remu);

    return EXEC_ID_inv;
}

def_THelper(binopiw) 
{
    def_INSTR_TAB("??????? ????? ????? 000 ????? ????? ??", addiw);
    def_INSTR_TAB("0000000 ????? ????? 001 ????? ????? ??", slliw);
    def_INSTR_TAB("0000000 ????? ????? 101 ????? ????? ??", srliw);
    def_INSTR_TAB("0100000 ????? ????? 101 ????? ????? ??", sraiw);

    return EXEC_ID_inv;
}

def_THelper(binopimm)
{
  def_INSTR_TAB("??????? ????? ????? 000 ????? ????? ??", addi);
  def_INSTR_TAB("??????? ????? ????? 010 ????? ????? ??", slti);
  def_INSTR_TAB("??????? ????? ????? 011 ????? ????? ??", sltiu);
  def_INSTR_TAB("000000? ????? ????? 001 ????? ????? ??", slli);
  def_INSTR_TAB("010000? ????? ????? 101 ????? ????? ??", srai);
  def_INSTR_TAB("000000? ????? ????? 101 ????? ????? ??", srli);
  def_INSTR_TAB("??????? ????? ????? 111 ????? ????? ??", andi);
  def_INSTR_TAB("??????? ????? ????? 100 ????? ????? ??", xori);
  def_INSTR_TAB("??????? ????? ????? 110 ????? ????? ??", ori);

  return EXEC_ID_inv;
}

def_THelper(binopw) 
{
    def_INSTR_TAB("0000000 ????? ????? 000 ????? ????? ??", addw);
    def_INSTR_TAB("0100000 ????? ????? 000 ????? ????? ??", subw);
    def_INSTR_TAB("0000000 ????? ????? 001 ????? ????? ??", sllw);
    def_INSTR_TAB("0000000 ????? ????? 101 ????? ????? ??", srlw);
    def_INSTR_TAB("0100000 ????? ????? 101 ????? ????? ??", sraw);
    def_INSTR_TAB("0000001 ????? ????? 000 ????? ????? ??", mulw);
    def_INSTR_TAB("0000001 ????? ????? 100 ????? ????? ??", divw);
    def_INSTR_TAB("0000001 ????? ????? 101 ????? ????? ??", divuw);
    def_INSTR_TAB("0000001 ????? ????? 110 ????? ????? ??", remw);
    def_INSTR_TAB("0000001 ????? ????? 111 ????? ????? ??", remuw);

    return EXEC_ID_inv;
}

def_THelper(branch) 
{
    def_INSTR_TAB("??????? ????? ????? 000 ????? ????? ??", beq);
    def_INSTR_TAB("??????? ????? ????? 001 ????? ????? ??", bne);
    def_INSTR_TAB("??????? ????? ????? 101 ????? ????? ??", bge);
    def_INSTR_TAB("??????? ????? ????? 111 ????? ????? ??", bgeu);
    def_INSTR_TAB("??????? ????? ????? 100 ????? ????? ??", blt);
    def_INSTR_TAB("??????? ????? ????? 110 ????? ????? ??", bltu);

    return EXEC_ID_inv;
}

def_THelper(csrop) 
{
    def_INSTR_TAB("??????? ????? ????? 010 ????? ????? ??", csrrs);
    def_INSTR_TAB("??????? ????? ????? 001 ????? ????? ??", csrrw);
    def_INSTR_TAB("0000000 00000 00000 000 00000 11100 11", ecall);
    def_INSTR_TAB("0011000 00010 00000 000 00000 11100 11", mret);

    return EXEC_ID_inv;
}

def_THelper(main) {
    def_INSTR_IDTAB("??????? ????? ????? ??? ????? 00000 11", I, load);
    def_INSTR_IDTAB("??????? ????? ????? ??? ????? 01000 11", S, store);
    def_INSTR_IDTAB("??????? ????? ????? ??? ????? 00101 11", U, auipc);
    def_INSTR_TAB("??????? ????? ????? ??? ????? 11010 11",      nemu_trap);

    // extended instructions.
    def_INSTR_IDTAB("??????? ????? ????? ??? ????? 00100 11", I, binopimm);
    def_INSTR_IDTAB("??????? ????? ????? ??? ????? 00110 11", I, binopiw);
    def_INSTR_IDTAB("??????? ????? ????? ??? ????? 01100 11", SS, binop);
    def_INSTR_IDTAB("??????? ????? ????? ??? ????? 01110 11", SS, binopw);
    def_INSTR_IDTAB("??????? ????? ????? ??? ????? 11000 11", S, branch);
    def_INSTR_IDTAB("??????? ????? ????? ??? ????? 11100 11", I, csrop);

    def_INSTR_IDTAB("??????? ????? ????? ??? ????? 11001 11", I, jalr);
    def_INSTR_IDTAB("??????? ????? ????? ??? ????? 11011 11", U, jal);
    def_INSTR_IDTAB("??????? ????? ????? ??? ????? 01101 11", U, lui);
    
    return table_inv(s);
};

// 返回一个编号idx, 用于对g_exec_table这一数组进行索引
int isa_fetch_decode(Decode *s) 
{  
  s->isa.instr.val = instr_fetch(&s->snpc, 4);
  int idx = table_main(s);
  return idx;
}
