#ifndef kernel_memory_h
#define kernel_memory_h

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <mach/mach.h>
#include "offsets.h"

extern uint64_t task_self;

kern_return_t mach_vm_allocate(vm_map_t target, mach_vm_address_t *address, mach_vm_size_t size, int flags);
kern_return_t mach_vm_read_overwrite(vm_map_t target_task, mach_vm_address_t address, mach_vm_size_t size, mach_vm_address_t data, mach_vm_size_t *outsize);
kern_return_t mach_vm_write(vm_map_t target_task, mach_vm_address_t address, vm_offset_t data, mach_msg_type_number_t dataCnt);
kern_return_t mach_vm_deallocate(vm_map_t target, mach_vm_address_t address, mach_vm_size_t size);;
kern_return_t mach_vm_read(vm_map_t target_task, mach_vm_address_t address, mach_vm_size_t size, vm_offset_t *data, mach_msg_type_number_t *dataCnt);
kern_return_t mach_vm_map(vm_map_t target_task, mach_vm_address_t *address, mach_vm_size_t size, mach_vm_offset_t mask, int flags, mem_entry_name_port_t object, memory_object_offset_t offset, boolean_t copy, vm_prot_t cur_protection, vm_prot_t max_protection, vm_inherit_t inheritance);
kern_return_t mach_vm_region_recurse(vm_map_t target_task, mach_vm_address_t *address, mach_vm_size_t *size, natural_t *nesting_depth, vm_region_recurse_info_t info, mach_msg_type_number_t *infoCnt);

void init_kernel_memory(mach_port_t tfp0, uint64_t our_port_addr);

size_t kread(uint64_t where, void *p, size_t size);
uint32_t rk32(uint64_t where);
uint64_t rk64(uint64_t where);

size_t kwrite(uint64_t where, const void *p, size_t size);
void wk32(uint64_t where, uint32_t what);
void wk64(uint64_t where, uint64_t what);

void kfree(mach_vm_address_t address, vm_size_t size);
uint64_t kalloc(vm_size_t size);

int kstrcmp(uint64_t string1, uint64_t string2);
int kstrcmp_u(uint64_t string1, char *string2);
unsigned long kstrlen(uint64_t string);

uint64_t find_port(mach_port_name_t port);

uint32_t rk32_via_fakeport(mach_port_t fakeport, uint64_t *bsd_info, uint64_t address);
uint64_t rk64_via_fakeport(mach_port_t fakeport, uint64_t *bsd_info, uint64_t address);

#define kr32(addr) rk32_via_fakeport(fakeport, read_addr_ptr, addr)
#define kr64(addr) rk64_via_fakeport(fakeport, read_addr_ptr, addr)
#endif /* kernel_memory_h */
