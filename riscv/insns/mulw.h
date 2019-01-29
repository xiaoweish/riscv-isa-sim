require_extension('M');
require_rv64;
WRITE_RD(sext32(RS1 * RS2));

STATE.mcycle += INST_MULW_CYCLES-1;
