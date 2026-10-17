/* Function pointer identity must survive ABI relocation and aliases. */
FILE_LICENCE ( GPL2_OR_LATER );
#undef NDEBUG
#include <ipxe/test.h>

static unsigned int function_calls;
void abi_function_target ( void ) { function_calls++; }
void abi_function_alias ( void ) __attribute__((alias("abi_function_target")));
static void ( * volatile function_pointer ) ( void ) = abi_function_target;
static void ( * volatile alias_pointer ) ( void ) = abi_function_alias;

static __attribute__((noinline)) void ( * function_address ( void ) ) ( void ) {
	return abi_function_target;
}

static void function_test_exec ( void ) {
	void ( *function ) ( void ) = function_pointer;
	void ( *alias ) ( void ) = alias_pointer;
	ok ( function == function_address() );
	ok ( alias == function );
	function_calls = 0;
	function();
	ok ( function_calls == 1 );
	alias();
	ok ( function_calls == 2 );
}

struct self_test function_test __self_test = {
	.name = "function",
	.exec = function_test_exec,
};
