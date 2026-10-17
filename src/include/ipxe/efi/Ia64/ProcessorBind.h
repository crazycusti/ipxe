#pragma once
FILE_LICENCE ( GPL2_OR_LATER );
#define MDE_CPU_IPF
#define EFIAPI
typedef unsigned long long UINT64;
typedef long long INT64;
typedef unsigned int UINT32;
typedef int INT32;
typedef unsigned short UINT16;
typedef short INT16;
typedef unsigned char UINT8;
typedef signed char INT8;
typedef unsigned short CHAR16;
typedef char CHAR8;
typedef unsigned char BOOLEAN;
typedef unsigned long UINTN;
typedef long INTN;
#define MAX_BIT 0x8000000000000000UL
#define MAX_2_BITS 0xc000000000000000UL
#define MAX_ADDRESS 0xffffffffffffffffUL
#define MAX_ALLOC_ADDRESS MAX_ADDRESS
#define MAX_INTN ((INTN)0x7fffffffffffffffUL)
#define MAX_UINTN ((UINTN)-1)
#define DEFAULT_PAGE_ALLOCATION_GRANULARITY 0x1000
#define RUNTIME_PAGE_ALLOCATION_GRANULARITY 0x2000
#define CPU_STACK_ALIGNMENT 16
#define FUNCTION_ENTRY_POINT(f) (*(VOID **)(f))
#define ASM_GLOBAL .globl
