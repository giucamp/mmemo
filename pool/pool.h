

#include "untyped_pool.h"
#include "typed_pool.h"

namespace memo
{
	/** Generic class template PoolDispatcher - dispatch the allocation of single objects to a pool,
		and allocation of arrays to the DefaultAllocationDispatcher. 
		You can define a specialization of AllocationDispatcher that derives from PoolDispatcher
		to enable pooling of a specific type.
		This class is thread-safe. */
	template < typename TYPE, size_t DEFAULT_OBJECT_COUNT >
		class PoolDispatcher : public DefaultAllocationDispatcher<TYPE>
	{
	private:

		struct Data
		{
			memo_externals::Mutex m_mutex;
			TypedPool< TYPE> m_pool;
			Data() : m_pool( DEFAULT_OBJECT_COUNT ) { }
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