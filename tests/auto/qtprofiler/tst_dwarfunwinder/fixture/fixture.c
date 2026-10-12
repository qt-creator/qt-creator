// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

// Writes what perf records of a "--call-graph dwarf" sample taken in leaf():
// the user registers, by perf's register numbers, and the stack from the
// stack pointer up. The program counter is taken where the other registers
// are, so that all of them are the state at one instruction. Built without
// frame pointers, so that only the CFI tells how to get from one frame to the
// next. See generate.sh.

#define _GNU_SOURCE
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#if defined(__aarch64__)
enum { RegisterCount = 33, StackPointer = 31, ProgramCounter = 32 };
#elif defined(__x86_64__)
enum { RegisterCount = 24, StackPointer = 7, ProgramCounter = 8 };
#endif

static uint64_t registers[RegisterCount];
static uint8_t stack[8192];
static uint64_t stackSize;
volatile int sink;

__attribute__((noinline)) static void leaf(void)
{
#if defined(__aarch64__)
    __asm__ volatile("stp x0, x1, [%0, #0]\n\t"
                     "stp x2, x3, [%0, #16]\n\t"
                     "stp x4, x5, [%0, #32]\n\t"
                     "stp x6, x7, [%0, #48]\n\t"
                     "stp x8, x9, [%0, #64]\n\t"
                     "stp x10, x11, [%0, #80]\n\t"
                     "stp x12, x13, [%0, #96]\n\t"
                     "stp x14, x15, [%0, #112]\n\t"
                     "stp x16, x17, [%0, #128]\n\t"
                     "stp x18, x19, [%0, #144]\n\t"
                     "stp x20, x21, [%0, #160]\n\t"
                     "stp x22, x23, [%0, #176]\n\t"
                     "stp x24, x25, [%0, #192]\n\t"
                     "stp x26, x27, [%0, #208]\n\t"
                     "stp x28, x29, [%0, #224]\n\t"
                     "str x30, [%0, #240]\n\t"
                     "mov x9, sp\n\t"
                     "str x9, [%0, #248]\n\t"
                     "adr x9, .\n\t"
                     "str x9, [%0, #256]\n\t"
                     :
                     : "r"(registers)
                     : "x9", "memory");
#elif defined(__x86_64__)
    __asm__ volatile("movq %%rax, 0(%0)\n\t"
                     "movq %%rbx, 8(%0)\n\t"
                     "movq %%rcx, 16(%0)\n\t"
                     "movq %%rdx, 24(%0)\n\t"
                     "movq %%rsi, 32(%0)\n\t"
                     "movq %%rdi, 40(%0)\n\t"
                     "movq %%rbp, 48(%0)\n\t"
                     "movq %%rsp, 56(%0)\n\t"
                     "movq %%r8, 128(%0)\n\t"
                     "movq %%r9, 136(%0)\n\t"
                     "movq %%r10, 144(%0)\n\t"
                     "movq %%r11, 152(%0)\n\t"
                     "movq %%r12, 160(%0)\n\t"
                     "movq %%r13, 168(%0)\n\t"
                     "movq %%r14, 176(%0)\n\t"
                     "movq %%r15, 184(%0)\n\t"
                     "leaq 0(%%rip), %%rax\n\t"
                     "movq %%rax, 64(%0)\n\t"
                     :
                     : "r"(registers)
                     : "rax", "memory");
#endif
    const uint64_t sp = registers[StackPointer];
    pthread_attr_t attr;
    void *stackAddress = NULL;
    size_t stackLength = 0;
    pthread_getattr_np(pthread_self(), &attr);
    pthread_attr_getstack(&attr, &stackAddress, &stackLength);
    const uint64_t stackEnd = (uint64_t)(uintptr_t)stackAddress + stackLength;
    stackSize = stackEnd - sp < sizeof(stack) ? stackEnd - sp : sizeof(stack);
    memcpy(stack, (const void *)(uintptr_t)sp, stackSize);
    sink = 1;
}

// Each keeps a frame of its own, and does something after the call, so that
// none of the calls becomes a jump.
__attribute__((noinline)) static int level3(int value)
{
    volatile int local[8] = {value};
    leaf();
    return local[0] + sink;
}

__attribute__((noinline)) static int level2(int value)
{
    volatile int local[16] = {value};
    return level3(local[0] * 3) + local[1] + sink;
}

__attribute__((noinline)) static int level1(int value)
{
    volatile int local[4] = {value};
    return level2(local[0] + 5) * 2 + sink;
}

int main(int argc, char **argv)
{
    const int result = level1(argc);
    if (argc < 2)
        return 1;
    FILE *out = fopen(argv[1], "wb");
    if (!out)
        return 1;
    const uint32_t count = RegisterCount;
    fwrite(&count, sizeof(count), 1, out);
    fwrite(registers, sizeof(registers[0]), RegisterCount, out);
    fwrite(&stackSize, sizeof(stackSize), 1, out);
    fwrite(stack, 1, stackSize, out);
    return fclose(out) == 0 && result != 0 ? 0 : 1;
}
