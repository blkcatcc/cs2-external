#ifndef MEM_H
#define MEM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

int mem_open(pid_t pid);
void mem_close(void);
bool mem_read_raw(uintptr_t addr, void *out, size_t n);
uint8_t mem_u8(uintptr_t addr);
uint32_t mem_u32(uintptr_t addr);
int32_t mem_i32(uintptr_t addr);
uintptr_t mem_u64(uintptr_t addr);
float mem_f32(uintptr_t addr);

int mem_mouse_button(int button, int down);

#endif
