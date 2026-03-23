/*
Some processors might perform that sort of increment in an
atomic manner, but you can’t count on it. But a full locking regime seems like overhead for a simple integer value. For cases like this, the kernel provides an atomic
integer type called atomic_t, defined in <asm/atomic.h>.
*/
#include <asm/atomic.h>

