require_extension('M');
word_t lhs = zext_xlen(RS1);
word_t rhs = zext_xlen(RS2);
if(rhs == 0)
  WRITE_RD(reg_t(UINT64_MAX));
else
  WRITE_RD(reg_t(sext_xlen(lhs / rhs)));
