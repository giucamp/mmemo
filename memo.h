

/*! \mainpage Memo: overwiew and usage
 *
 * \section whats What's Memo
 * Memo is an open source C++ library that provides data-driven and object-oriented memory management.
 * The classic scenario of dynamic memory allocation consists of a program requesting randomly dynamic storage to a black-box allocator (implementing a set of malloc\realloc\free functions) which doesn’t know and can’t predict anything about the requests of the program. Implementing a good black-box is a well known difficult problem.  
 * The main point of memo is not providing memory allocation algorithms, but rather adding a layer between the allocator and the program, to allow to select the best memory allocation strategy with the best tuning for every part of the program. Provided that the key functions in the source code are tagged with contexts, Memo allows to select and tune a different allocator for any context without altering the code, but just editing a memory configuration file.
 */

#ifndef MEMO_INCLUDED
#define MEMO_INCLUDED

#include <stdint.h>
#include <vector>
#include <unordered_map>

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
	class DataStack; /** manages a raw memory buffer providing LIFO ordered memory block allocation. 
			Any non-POD object allocated in it must be manually destroyed before being freed. */
	class ObjectStack; /** manages a raw memory buffer providing LIFO ordered memory block allocation.
			Unlike DataStack, ObjectStack keeps a deallocation callback for every memory block that 
			can be used to destroy non-POD objects. */
	class LifoAllocator;
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

#include "allocation_functions.h"
#include "address_functions.h"
#include "std_allocator.h"
#include "std_containers.h"
#include "allocators\allocators.h"
#include "lifo\lifo.h"
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

#endif // #ifndef MEMO_INCLUDED
