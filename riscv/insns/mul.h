require_extension('M');
WRITE_RD(sext_xlen(RS1 * RS2));

STATE.mcycle += INST_MUL_CYCLES-1;
