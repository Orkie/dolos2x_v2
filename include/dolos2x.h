#ifndef __DOLOS2X_H__
#define __DOLOS2X_H__
#include <stdint.h>

typedef void (*mem_callback)(uint32_t, int, void*);
typedef void (*add_mem_callback)(uint32_t, mem_callback);

typedef struct {
  int (*init)(add_mem_callback, add_mem_callback);
  const char* name;
} dolos_peripheral;

extern dolos_peripheral peri_nand;

#endif

