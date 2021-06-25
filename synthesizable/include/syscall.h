#include "portability.h"

// Functions for memory accesses
void stb(const CORE_UINT(32) addr, const CORE_INT(8) value);
void sth(const CORE_UINT(32) addr, const CORE_INT(16) value);
void stw(const CORE_UINT(32) addr, const CORE_INT(32) value);
void _std(const CORE_UINT(32) addr, const CORE_INT(64) value);

CORE_INT(8) ldb(const CORE_UINT(32) addr);
CORE_INT(16) ldh(const CORE_UINT(32) addr);
CORE_INT(32) ldw(const CORE_UINT(32) addr);
CORE_INT(32) ldd(const CORE_UINT(32) addr);

CORE_INT(32) doRead(CORE_UINT(32) file, CORE_UINT(32) bufferAddr, CORE_UINT(32) size);
CORE_INT(32) doWrite(CORE_UINT(32) file, CORE_UINT(32) bufferAddr, CORE_UINT(32) size);
CORE_INT(32) doOpen(CORE_UINT(32) name, CORE_UINT(32) flags, CORE_UINT(32) mode);
CORE_INT(32) doOpenat(CORE_UINT(32) dir, CORE_UINT(32) name, CORE_UINT(32) flags, CORE_UINT(32) mode);
CORE_INT(32) doLseek(CORE_UINT(32) file, CORE_UINT(32) ptr, CORE_UINT(32) dir);
CORE_INT(32) doClose(CORE_UINT(32) file);
CORE_INT(32) doStat(CORE_UINT(32) filename, CORE_UINT(32) ptr);
CORE_INT(32) doSbrk(CORE_UINT(32) value);
CORE_INT(32) doGettimeofday(CORE_UINT(32) timeValPtr);
CORE_INT(32) doUnlink(CORE_UINT(32) path);
CORE_INT(32) doFstat(CORE_UINT(32) file, CORE_UINT(32) stataddr);

CORE_UINT(32) solveSysCall(CORE_UINT(32) syscallId, CORE_UINT(32) arg1, CORE_UINT(32) arg2,CORE_UINT(32) arg3,
 CORE_UINT(32) arg4, CORE_UINT(2) *sys_status);