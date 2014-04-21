
/*! \file pool.h
*/

#include "untyped_pool.h"
#include "typed_pool.h"

/** \def MEMO_ENABLE_POOL( TYPE, DEFAULT_CAPACITY )
	 Enables a memory pool for the type TYPE, with capacity DEFAULT_CAPACITY, or with the capacity specified in the memory configuration file.
	 MEMO_ENABLE_POOL specializes a AllocationDispatcher that uses a memo::TypedPool. Only the macro MEMO_NEW and MEMO_DELETE are dispatched to
	 the pool: array allocations are performed with the default dispatcher (that is, the thread current allocator is used).
	 TypedPool is not thread safe, but the AllocationDispatcher defined by this macro protects the pool with a mutex.
	 The pool allocates a memory buffer the first time it is created, and places in it objects up to the capacity. When the capacity is over,
	 TypedPool uses the default allocator.
	This macro must be used in the global namespace, only once, and after TYPE has been defined. */
#define MEMO_ENABLE_POOL( TYPE, DEFAULT_CAPACITY ) namespace memo {																		\
	template <>	class AllocationDispatcher<TYPE> : public memo::PoolDispatcher< TYPE, AllocationDispatcher<TYPE > >	\
	{																																	\
	public:																																\
		static const char * type_name() { return #TYPE; }																				\
		static size_t get_pool_capacity()																								\
		{																																\
			size_t capacity = DEFAULT_CAPACITY;																							\
			MemoryManager::get_instance().get_pool_object_count( type_name(), &capacity );												\
			return capacity;																											\
		}																																\
	}; }

namespace memo
{
	/** Generic class template PoolDispatcher - dispatch the allocation of single objects to a pool,
		and allocation of arrays to the DefaultAllocationDispatcher. 
		You can define a specialization of AllocationDispatcher that derives from PoolDispatcher
		to enable pooling of a specific type.
		Allocation of the type TYPE made with MEMO_NEW will be performed with a memo::TypedPool sized
		with DEFAULT_OBJECT_COUNT. If the pool is full, TypedPool allocates with the default allocator.
		This class is thread-safe. */
	template < typename TYPE, typename COUNT_GETTER >
		class PoolDispatcher : public DefaultAllocationDispatcher<TYPE>
	{
	private:

		struct Data
		{
			memo_externals::Mutex m_mutex;
			TypedPool< TYPE> m_pool;
			Data()
			{
				const size_t capacity = COUNT_GETTER::get_pool_capacity();
				
				memo_externals::output_message( COUNT_GETTER::type_name() );
				memo_externals::output_message( ": using a pool with capacity " );
				memo::output_integer( capacity );
				memo_externals::output_message( ", size: " );
				memo::output_mem_size( capacity * sizeof( TYPE ) );
				memo_externals::output_message( "\n" );

				m_pool.init( capacity );
			}
		};

		static Data & get_data()
			{ static Data s_data; return s_data; }

	public:

		/** typed_alloc<TYPE>() - allocates an object of a given type with the current allocator of the thread. No constructor is called. 
			This function is an internal service, and is not supposed to be called directly. Use MEMO_NEW instead. */
		static void * typed_alloc()
		{
			Data & data = get_data();
			MutexLock lock( data.m_mutex );
			return data.m_pool.alloc();
		}

		/** _delete( i_pointer ) - destroys and deallocates an object of a given type with the allocator used to allocate it.
			This function is an internal service, and is not supposed to be called directly. Use MEMO_DELETE instead. */
		static void _delete( TYPE * i_pointer )
		{
			i_pointer->~TYPE();

			Data & data = get_data();
			MutexLock lock( data.m_mutex );
			data.m_pool.free( i_pointer );
		}
	};
}