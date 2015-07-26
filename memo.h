

#ifndef MEMO_INCLUDED
#define MEMO_INCLUDED

#include <stdint.h>
#include <vector>
#include <unordered_map>
#include <deque>
#include <queue>
#include <memory>

#ifdef _MSC_VER
	#pragma warning( push )
	#pragma warning( disable: 4127 ) // conditional expression is constant
#endif

namespace memo // classes' forward declarations 
{
	// allocators
	class IAllocator; /** base interface for classes providing dynamic memory allocation services */
		class DefaultAllocator; /** provides allocation services using malloc\free of C standard library */
		class DecoratorAllocator; /** abstract base for classes that add functionality to other allocators */
			class DebugAllocator; /** adds debug functionalities to another allocator */
			class StatAllocator; /** collects statistics about the usage of another allocator */
		class RegionAllocator;
			class TlsfAllocator;

	template <typename TYPE> class StdAllocator; /** implements a standard library allocator wrapping 
														memo allocation functions. */

	// lifo
	class LifoAllocator; /** manages a raw memory buffer providing LIFO ordered memory block allocation. 
			Any non-POD object allocated in it must be manually destroyed before being freed. */
	class ObjectLifoAllocator; /** manages a raw memory buffer providing LIFO ordered memory block allocation.
			Unlike LifoAllocator, ObjectLifoAllocator keeps a deallocation callback for every memory block that 
			can be used to destroy non-POD objects. */
	class ObjectStack;
	class LifoObjectAllocator;

	typedef void (*DeallocationCallback)( void * i_memory_block );

	template <typename TYPE> 
		void default_destructor_callback( void * i_memory_block )
			{ static_cast<TYPE*>( i_memory_block )->~TYPE(); }
	
	// contexts
	class StaticName;
	class ThreadRoot;
	class AllocatorConfigFactory;
	class MemoryManager;

	// serialization
	namespace serialization
	{
		enum Result
		{
			eSuccessful,
			eCantOpenStream,
			eBadFormed,
			eWrongContent,
			eUnrecognizedProperty,
			eSomeUnrecognizedToken
		};
		class IConfigReader;
	}

} // namespace memo

#include "memo_externals.h"

namespace memo
{
	class MutexLock
	{
	public:
		MutexLock( memo_externals::Mutex & i_mutex ) : m_mutex( i_mutex ) { m_mutex.lock(); }
		~MutexLock() { m_mutex.unlock(); }

	private:
		MutexLock( const MutexLock & ); // unimplemented
		MutexLock & operator = ( const MutexLock & ); // unimplemented

	private:
		memo_externals::Mutex & m_mutex;
	};

} // namespace memo

#include "allocation_functions.h"
#include "allocation_dispatcher.h"
#include "address_functions.h"
#include "std_allocator.h"
#include "std_containers.h"
#include "allocators\allocators.h"
#include "lifo\lifo.h"
#include "fifo\fifo.h"
#include "pool\pool.h"
#include "management\management.h"

namespace memo
{
	/** Converts a memory size to a null-terminated string, expressing big sizes in kilobytes, megabytes gigabytes, and terabytes units.
		@param i_dest_buffer Pointer to the destination buffer 
		@param i_dest_buffer_size Size of the destination buffer
		@param i_mem_size Memory Size to be converted to string
		@return Pointer to the first character of the converted string, which is equal or bigger then i_dest_buffer. The result 
			string in contained in the destination buffer, but starts in an unspecified position. If the destination buffer is not
			large enough, the return value is nullptr.

		Examples:
			5 -> "5"
			1024 * 6 = 6144 -> "6K"
			1024 * 900 = 921600 -> "0.87M"
			1024 * 1024 * 66 = 69206016 -> "66M"
			1024 * 1024 * (1024 * 3 + 500) = 3745513472 -> "3.48G" */
	char * mem_size_to_string( char * i_dest_buffer, size_t i_dest_buffer_size, size_t i_mem_size );

	/** converts the input memory size with memo::mem_size_to_string and then writes it in the output 
		with memo_externals::output_message. */
	void output_mem_size( size_t i_mem_size );

	void output_integer( size_t i_mem_size );

	uint32_t generate_rand_32( uint32_t & io_rand_seed );

	#if MEMO_ENABLE_TEST

		void test();
				
		uint32_t generate_rand_32();

	#endif

} // namespace memo

#include "allocation_functions.inl" // this source contains function templates, so it must be included anyway

/* if MEMO_ENABLE_INLINE is true then .inl files are included in memo.h, otherwise 
	they are included in memo.cpp, and inline expansion is disabled */
#if MEMO_ENABLE_INLINE

	#ifdef MEMO_INLINE
		#error MEMO_INLINE is already defined
	#endif
	#define MEMO_INLINE inline

	#include "memo.inl"

	#undef MEMO_INLINE

#endif


#ifdef _MSC_VER
	#pragma warning( pop )
#endif

#endif // #ifndef MEMO_INCLUDED

/*! \mainpage Memo: overwiew and usage

\section whats What's Memo
Memo is an open source C++ library that provides data-driven and object-oriented memory management.
The classic scenario of dynamic memory allocation consists of a program requesting randomly dynamic storage to a black-box allocator that implements a set of malloc/realloc/free functions, and that doesn't know and can't predict anything about the requests of the program.
Memo adds a layer between the allocator and the program, and allows the program to select the best memory allocation strategy with the best tuning for every part of the program.

In Memo an allocation algorithm is wrapped by a class implementing the interface memo::IAllocator. 
Here is some common allocators:
-	memo::DefaultAllocator, which wraps the system malloc/free
-	memo::DebugAllocator, which decorates another allocator and adds no man's land around memory blocks and initializes memory to help to catch uninitialized variables and dangling pointers
-	memo::StatAllocator, which decorates another allocator to keep tracks of: total memory allocated, total block count, and allocation peaks
-	memo::TlsfAllocator, which wraps the two level segregate allocator written by Matthew Conte (http://tlsf.baisoku.org)

![Allocator Hierarchy](classmemo_1_1_i_allocator__inherit__graph.png)

\section usage Usage
Just as one can expect, Memo provides a set of global functions to allocate memory, similar to the ones of the standard C library, and a set of macros to allocate C++ objects:

\code{.cpp}
void * buffer = memo::alloc( buffer_length, buffer_alignment, 0/ *offset* / );
// ...
memo::free( buffer );

Dog * bell = MEMO_NEW( Dog, "Bell" );
// ...
MEMO_DELETE( bell );

\endcode

The allocation requests are redirected to an object implementing the interface IAllocator. But which one is used? Every thread has its own current allocator, that is used to allocate new memory blocks. Anyway, when a realloc or free is requested, the operation is performed by the allocator that allocated the block, regardless or the current allocator of the thread.
The memory configuration file can associate a startup allocator to every thread, otherwise the default allocator is assigned.

Memo allows to change the thread's current allocator, but it's not recommended. The best practice is opening contexts on the callstack:

\code{.cpp}
const memo::StaticName g_graphics( "graphics" );

void load_archive( const char * i_file_name )
{
	memo::Context context( g_graphics );
	// ...
	void * buffer = memo::alloc( buffer_length, buffer_alignment, 0/ *offset* / );
	// ...
}
\endcode

The context label is pushed on the calling thread when the object Context is constructed, and popped when it goes out of scope. Of course, contexts can be nested:

\code{.cpp}
const memo::StaticName g_zoo( "zoo" );
const memo::StaticName g_robots( "robots" );

void load_zoo( const char * i_file_name )
{
	memo::Context context( g_zoo );
	load_archive( i_file_name );
}

void load_robots( const char * i_file_name )
{
	memo::Context context( g_robots );
	load_archive( i_file_name );
}
\endcode

The Context objects pushed on the call stack form a context path. The function load_archive, called from load_animals, sets on the calling thread the context with the path "zoo/graphics". If the same function is called  from load_robots, the path of the context is "robots/graphics".

The memory configuration file can assign and tune an allocator for:
-	the context "robots", and all its child context
	-	the context "zoo/graphics"
	-	the context "robots/graphics"

\section overhead Space and execution overhead
If you request memory directly to an memo::IAllocator object, you don't have any space overhead. Anyway every allocation\deallocation has a time overhead due to the virtual call. If you use the global function memo::alloc or the macro MEMO_NEW, then memo will add, at the beginning of the memory block, a pointer to the allocator used to allocate.
Anyway, you may want to use memo just to analyze the memory usage in the debug builds of your program. In this case you can define, in the release builds, in the header memo_externals.h the macro MEMO_ONLY_DEFAULT_ALLOCATOR as 1. In this way, memo::alloc and friends will resolve to a static call to malloc and friends.

\section pool Pools
Memo supports efficient fixed-size allocations with pools (memo::TypedPool amd memo::UntypedPool). The pool is initialized with a capacity,
and can allocate objects up to its capacity. When the pool is full, it uses the default allocator, but this mechanism
is transparent for the user. The pool has no space overhead, and does not fragment the memory.
You can use the pool directly, or you can enable automatic pooling for type by using the macro MEMO_ENABLE_POOL:
\code{.cpp}
MEMO_ENABLE_POOL( MyClass, 2000 );
\endcode
This declarations tells memo to redirect every MEMO_NEW and MEMO_DELETE of MyClass to pool whose capacity is by default 2000. You can override
this capacity in the memory configuration file.

\section lifoallocator Data Stack and lifo allocations
Memo allows to use a thread specific data stack, to perform efficient lifo (last-in, first-out) allocations. The lifo constraint is suited to:
- blocks allocated when the program starts, and released when the program exits
- temporary dynamic storage needed to a function or scope, like in this example:

\code{.cpp}

const size_t required_size = strlen(str1) + strlen(str2) + 1;

char * buffer = static_cast< char * >( memo::lifo_alloc( sizeof(char) * required_size, MEMO_ALIGNMENT_OF(char), 0, nullptr ) );

strcpy( buffer, str1 );
strcat( buffer, str2 );

printf( buffer );

memo::lifo_free( buffer );
\endcode

Allocations in the data stack are very fast, do not fragment the memory, and don't waste any space, as the memory blocks are "packed"
together one after the other in a single (or in a few) memory buffer. The lifo order must be respected, otherwise the memory gets corrupted. 
In a debug build, a mismatch is reported with an assert.
See memo::ObjectStack, memo::LifoAllocator and memo::ObjectLifoAllocator for details.

\section fifoallocator Queues and fifo allocations
Memo provides support for fifo (first-in, first-out) ordered allocations too. This kind of ordering is suited for the producer-consumer problem, allowing dealing with items of variable 
size and alignment allocated in-place in a single memory buffer.
As example of this scenario consider a command queue of a thread, with every command being a struct or class with different data members, that are the parameters of the command. 
A second and similar use case may be a command buffer for a graphic renderer.
See memo::FifoAllocator and memo::Queue for details.

\section corruptiondetector Detecting memory corruption
Memo includes a special allocator to help to find bugs in the code that causes wrong memory access and memory corruption. CorruptionDetectorAllocator can detect:
- read accesses to memory that was allocated but never written (i.e. uninitialized memory usage)
- read or write access to memory outside allocated blocks

CorruptionDetectorAllocator runs an external debugger, "memo_debugger.exe", and communicates with it.
memo_debugger tracks every read or write access inside the memory managed by CorruptionDetectorAllocator,
so the access to this memory is extremely slow.
Currently this allocator is defined only for windows, as it uses windows specific APIs. Internally
CorruptionDetectorAllocator uses the tlsf allocator implemented by Matthew Conte (http://tlsf.baisoku.org).
		
When memo_debugger detects an invalid memory read or write operation, it breaks the execution of the program,
writes about the error on its console window, and allows the user to choose one if these actions: 
	- detaching the target process, to allow another debugger to attach to it
	- resuming the target process, ignoring the error
	- quitting, after resuming the target process
	- saving a dump that can be opened in a compatible debugger, such visual studio or windbg.
	- dumping the memory content around the address being wrongly accessed
	- saving or copying all the output of the debugger
		
Here is an example of the output of the debugger:

		\verbatim

		 *********** ERROR: attempt to write unwritable address 0x00790C9C ***********

		*** STACK TRACE ***
		->0x0096047A memo::ContextTest::test_CorruptionDetectorAllocator (231)
		  0x0095FA95 memo::ContextTest::test (247)
		  0x0095F974 memo::test (262)
		  0x0094B5A6 main (82)
		  0x0098E7FF __tmainCRTStartup (555)
		  0x0098E62F mainCRTStartup (371)
		  0x75D5ED5C BaseThreadInitThunk (371)
		  0x778B37EB RtlInitializeExceptionChain (371)
		  0x778B37BE RtlInitializeExceptionChain (371)

		*** SOURCE CODE AROUND 0x0096047A ***
		module: D:\GitHub\mmemo\test\Debug\test.exe
		source file: d:\github\mmemo\memo_test.cpp
		   229: 			int * array = MEMO_NEW_ARRAY( int, 5 );
		   230: 			for( int i = 1; i <= 5; i++ )
		-> 231: 				array[i] = i;
		   232: 			MEMO_DELETE_ARRAY( array );
		   233: 

		*** MEMORY CONTENT AROUND 0x00790C9C grouped by 4 byte(s) ***
		  0x00790C7C: 0x00790000 - not allocated
		  0x00790C80: 0x0045842c - readable/writable
		  0x00790C84: 0x00000005 - readable/writable
		  0x00790C88: 0x40400000 - writable
		  0x00790C8C: 0x00000001 - readable/writable
		  0x00790C90: 0x00000002 - readable/writable
		  0x00790C94: 0x00000003 - readable/writable
		  0x00790C98: 0x00000004 - readable/writable
		->0x00790C9C: 0x00790c70 - not allocated
		  0x00790CA0: 0x000ff359 - not allocated
		  0x00790CA4: 0x00790000 - not allocated
		  0x00790CA8: 0x00790000 - not allocated
		  0x00790CAC: 0x00000000 - not allocated
		  0x00790CB0: 0x00000000 - not allocated
		  0x00790CB4: 0x00000000 - not allocated
		  0x00790CB8: 0x00000000 - not allocated
		  0x00790CBC: 0x00000000 - not allocated

		Type a command:
		  detach: detach the target process, to allow another debugger to attach to it
		  ignore: resume the target process, ignoring the error
		  quit: resume the target process, and quit
		  minidump [file=test_memo_mini.dmp]: save a minidump
		  dump [file=test_memo.dmp]: save a complete dump
		  mem [1|2|4|8]: dump memory content around 0x00790C9C
		  save [file=test_memo_output.txt]: save all the output of this program
		  copy: copy all the output of this program into the clipboard
		>> 


		\endverbatim
*/
