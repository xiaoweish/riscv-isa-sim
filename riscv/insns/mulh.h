require_extension('M');
if (xlen == 64)
  WRITE_RD(mulh(RS1, RS2));
else
  WRITE_RD(sext32((sext32(RS1) * sext32(RS2)) >> 32));

STATE.mcycle += INST_MULH_CYCLES-1;
