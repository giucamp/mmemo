
namespace memo
{
	/**	\class UntypedPool
		Class providing efficient fixed-size allocation services for a specific type. When initialized, the pool allocates a 
		buffer with the default allocator. This buffer is large enough to contain the number of elements specified during the
		initialization (see UntypedPool::init).
		The pool keeps a linked list of free slots. So both allocations and deallocation are constant time and very fast.
		There is not space overhead, nor any fragmentation. The only drawback is that the pool must be initialized with a size.
		When the pool is full, this class can allocate transparently using the default allocator.
		If you want to use a pool not for a specific type (with the only constraint of the fixed size), you may use memo::UntypedPool.
		This class is not thread safe.
	*/
	template <typename TYPE>
		class TypedPool
	{
	public:

		TypedPool() { }

		TypedPool( size_t i_object_count )
		{ 
			init( i_object_count );
		}

		/** Allocates a buffer with the default allocator, and formats it to be used for fixed size allocations.
			@params i_object_count number of objects in the pool
			@return true if the buffer was successfully allocated and formatted, false otherwise */
		bool init( size_t i_object_count )
		{
			const UntypedPool::Config config( sizeof(TYPE), MEMO_ALIGNMENT_OF(TYPE), i_object_count );
			return m_pool.init( config );
		}

		/** Deallocates the buffer used by the pool. This method should be called only when there are no living blocks 
			allocated by the pool. It's legal to call this method before init or twice. */
		void uninit()								{ m_pool.uninit(); }
		
		/** Allocates a block of memory. The size and the alignment of the block are those of TYPE.
			If there is not a free sot in the pool, this method allocates using the default allocator.
			This method allocates a block without constructing any object. You may use the new in-place to construct an object:
			\code{.cpp}
				new ( my_pool.alloc() ) MY_TYPE( arg1, arg2m ... )
			\endcode
			@return address of the newly allocated block, or nullptr if both the pool and the default allocator could not allocate the block. */
		void * alloc()								{ return m_pool.alloc(); }

		void * alloc_slot()							{ return m_pool.alloc_slot(); }

		void free( void * i_address )				{ m_pool.free( i_address ); }

		void free_slot( void * i_address )			{ m_pool.free_slot( i_address ); }

		TYPE * create_object()								{ return new( m_pool.alloc() ) TYPE; }

		TYPE * create_object( const TYPE & i_source )		{ return new( m_pool.alloc() ) TYPE( i_source ); }

		void destroy_object( TYPE * i_object )				{ i_object->~TYPE(); m_pool.free( i_object ); }

	private:
		UntypedPool m_pool;
 	};
}

