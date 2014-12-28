
namespace memo
{
	/**	\class TypedPool
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
				new ( my_pool.alloc() ) MY_TYPE( arg1, arg2 ... )
			\endcode
			@return address of the newly allocated block, or nullptr if both the pool and the default allocator could not allocate the block. */
		void * alloc()								{ return m_pool.alloc(); }

		/** Allocates a block of memory. The size and the alignment of the block are those of TYPE.
			Unlike TypedPool::alloc, this method allocates only in the pool.
			This method allocates a block without constructing any object. You may use the new in-place to construct an object:
			\code{.cpp}
				new ( my_pool.alloc() ) MY_TYPE( arg1, arg2 ... )
			\endcode
			@return address of the newly allocated block, or nullptr if the pool could not allocate the block. */
		void * alloc_slot()							{ return m_pool.alloc_slot(); }

		/** Frees a block of memory, allocated with alloc or alloc_slot. If the block is outside the pool, this method uses the default allocator
			to free the block. This method does not call the destructor of the object. Use destroy_object to emulate a full delete.
			@param i_element address of the block to free. Can't be nullptr. */
		void free( void * i_address )				{ m_pool.free( i_address ); }

		/** Frees a block of memory, allocated with alloc or alloc_slot. Unlike TypedPool::free, this method can free only blocks allocated in the pool.
			Use free_slot only if the block was allocated with alloc_slot.
			This method does not call the destructor of the object. Use destroy_object to emulate a full delete.
			@param i_element address of the block to free. Can't be nullptr. */
		void free_slot( void * i_address )			{ m_pool.free_slot( i_address ); }

		/** Allocates a block of memory, and default-construct an object on it.
			If there is not a free sot in the pool, this method allocates using the default allocator.
			This method allocates a block constructing the object. Use alloc if you just want to allocate the memory.
			@return pointer to the new object, or nullptr if both the pool and the default allocator could not allocate the block. */
		TYPE* create_object()								{ return new( m_pool.alloc() ) TYPE; }
		
		/** Allocates a block of memory, and copy-construct on object on it.
			If there is not a free sot in the pool, this method allocates using the default allocator.
			This method allocates a block constructing the object. Use alloc if you just want to allocate the memory.
			@param i_source source object to copy
			@return pointer to the new object, or nullptr if both the pool and the default allocator could not allocate the block. */
		TYPE * create_object( const TYPE & i_source )		{ return new( m_pool.alloc() ) TYPE( i_source ); }

		/** Destroys the specified object and deallocates it. If the block is outside the pool, this method uses the default allocator
			to free the block. This method calls the destructor of the object. Use free to just deallocate the memory.
			@param i_object pointer to the object to delete. Can't be nullptr. */
		void destroy_object( TYPE * i_object )				{ i_object->~TYPE(); m_pool.free( i_object ); }

	private:
		UntypedPool m_pool;
 	};
}

