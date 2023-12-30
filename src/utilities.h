#ifndef utilities_h
#define utilities_h

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <mach/mach.h>
#include <sched.h>
#include <sys/utsname.h>
#include "io.h"

#define OSArray_objectCount(a) rk32(a + 24)
#define OSArray_objectAtIndex(a,b) rk64(rk64(a + 32) + b * 8)
#define OSData_buffer(a) rk64(a + 24)
#define OSData_setBuffer(a,b) wk64(a + 24, b)
#define OSData_length(a) rk32(a + 16)
#define OSData_setLength(a,b) wk32(a + 16, b)
#define msg_sz_kalloc(a) ((3 * a) / 4) - 0x74

enum {
    kOSSerializeDictionary          = 0x01000000U,
    kOSSerializeArray               = 0x02000000U,
    kOSSerializeSet                 = 0x03000000U,
    kOSSerializeNumber              = 0x04000000U,
    kOSSerializeSymbol              = 0x08000000U,
    kOSSerializeString              = 0x09000000U,
    kOSSerializeData                = 0x0a000000U,
    kOSSerializeBoolean             = 0x0b000000U,
    kOSSerializeObject              = 0x0c000000U,
    kOSSerializeTypeMask            = 0x7F000000U,
    kOSSerializeDataMask            = 0x00FFFFFFU,
    kOSSerializeEndCollection       = 0x80000000U,
    kOSSerializeBinarySignature     = 0x000000d3U,
};

uint32_t transpose(uint32_t val);
uint64_t OSDictionary_objectForKey(uint64_t dict, char *key);
uint64_t address_of_property_key(mach_port_t IOSurfaceRootUserClient, uint32_t key);
mach_port_t kalloc_fill(mach_port_t target, int count);
mach_port_t new_mach_port(void);
mach_port_t new_port(void);

kern_return_t send_message(mach_port_t destination, void *buffer, mach_msg_size_t size);
struct simple_msg* receive_message(mach_port_t source, mach_msg_size_t size);
int send_ool_ports(mach_port_t where, int count);

//mach_msg_size_t msg_sz_kalloc(mach_msg_size_t kalloc_size);
void trigger_gc(void);


#endif /* utilities_h */
