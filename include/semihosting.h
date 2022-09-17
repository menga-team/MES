#ifndef MES_SEMIHOSTING_H
#define MES_SEMIHOSTING_H

#ifdef SEMIHOSTING
extern void initialise_monitor_handles(void);
#endif

#ifdef SEMIHOSTING
#define sh_printf(...) printf(__VA_ARGS__)
#else
#define sh_printf(...)
#endif


#endif