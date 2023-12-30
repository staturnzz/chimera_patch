#import <Foundation/Foundation.h>
#include <stdio.h>
#include <stdlib.h>
#include <mach-o/dyld.h>
#include <mach/mach.h>
#include "time_saved.h"

#define IMAGE_BASE 0x100000000

uint64_t *internel_tfp0 = NULL;
uint64_t *internel_slide = NULL;

void set_x21(uint64_t val);
void set_x22(uint64_t val);

uint64_t get_base(void) {
    intptr_t slide = _dyld_get_image_vmaddr_slide(0);
    return IMAGE_BASE + slide;
}

mach_port_t time_saved(uint64_t *tfp0, uint64_t *slide) {
	mach_port_t tfpzero = start_time_saved();
	if (tfpzero == MACH_PORT_NULL) {
		NSLog(@"[chimera_patch] failed to get tfp0");
		return MACH_PORT_NULL;
	}

	uint64_t kernel_slide = kernel_base - 0xFFFFFFF007004000;
	*slide = kernel_slide;
	*tfp0 = tfpzero;

	NSLog(@"[chimera_patch] tfp0: 0x%x", tfpzero);
	NSLog(@"[chimera_patch] kernel base: 0x%llx", kernel_base);
	NSLog(@"[chimera_patch] kernel slide: 0x%llx", kernel_slide);

	set_x21((uint64_t)slide);
	set_x22((uint64_t)&kern_struct_task);
	return tfpzero;
}

__attribute__ ((constructor))
static void ctor(void) {
	NSLog(@"[chimera_patch] chimera_patch started (v1.0b1)");
	internel_tfp0 = (uint64_t*)(get_base() + 0x5BA48);
	internel_slide = (uint64_t*)(get_base() + 0x5B778);

	NSLog(@"[chimera_patch] internel_tfp0: 0x%llx", (uint64_t)internel_tfp0);
	NSLog(@"[chimera_patch] internel_slide: 0x%llx", (uint64_t)internel_slide);
	*internel_slide = (uint64_t)&time_saved;
	*internel_tfp0 = 0;
}
