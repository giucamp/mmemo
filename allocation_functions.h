

/** \def MEMO_NEW( TYPE, ... ) 
		Creates a new instance of TYPE using the current allocator of the calling thread */
#define MEMO_NEW( TYPE, ... )						new ( ::memo::_typed_alloc<TYPE>() ) TYPE( __VA_ARGS__ )

/** \def MEMO_DELETE( TYPE * pointer ) 
		Destroys an object created with MEMO_NEW, using the allocator that allocated the object */
#define MEMO_DELETE( pointer )						::memo::_delete( pointer )

/** \def MEMO_NEW_ARRAY( TYPE, size_t size ) 
		Creates a dynamic array of TYPE objects using the current allocator of the calling thread */
#define MEMO_NEW_ARRAY( TYPE, size )				::memo::_new_array<TYPE>( size )

/** \def MEMO_NEW_ARRAY_SRC( TYPE, size_t size, const TYPE & source ) 
		Creates a dynamic array of TYPE objects, copy-constructing every element from the object source.
		The current allocator of the calling thread is used. */
#define MEMO_NEW_ARRAY_SRC( TYPE, size, source )	::memo::_new_array<TYPE>( size, source )

/** \def MEMO_DELETE_ARRAY( TYPE * pointer ) 
		Destroys an array created with MEMO_NEW_ARRAY or MEMO_NEW_ARRAY_SRC, using the allocator that allocated the object. */
#define MEMO_DELETE_ARRAY( pointer )				::memo::_delete_array( pointer )



/** \def MEMO_NEW_ALLOC( memo::IAllocator & allocator, TYPE, ... ) 
		Creates a new instance of TYPE using the specified allocator. */
#define MEMO_NEW_ALLOC( allocator, TYPE, ... )						new ( ::memo::_typed_alloc<TYPE>( allocator ) ) TYPE( __VA_ARGS__ )

/** \def MEMO_DELETE_ALLOC( memo::IAllocator & allocator, TYPE * pointer ) 
		Destroys an object created with MEMO_NEW_ALLOC, using the specified allocator. */
#define MEMO_DELETE_ALLOC( allocator, pointer )						::memo::_delete( allocator, pointer )

/** \def MEMO_NEW_ARRAY_ALLOC( memo::IAllocator & allocator, TYPE, size_t size ) 
		Creates a dynamic array of TYPE objects using the specified allocator. */
#define MEMO_NEW_ARRAY_ALLOC( allocator, TYPE, size )				::memo::_new_array<TYPE>( allocator, size )

/** \def MEMO_NEW_ARRAY_SRC( memo::IAllocator & allocator, TYPE, size_t size, const TYPE & source ) 
		Creates a dynamic array of TYPE objects using the specified allocator and copy-constructing every 
		element from the object source. */
#define MEMO_NEW_ARRAY_SRC_ALLOC( allocator, TYPE, size, source )	::memo::_new_array<TYPE>( allocator, size, source )

/** \def MEMO_DELETE_ARRAY_ALLOC( memo::IAllocator & allocator, TYPE * pointer ) 
		Destroys an array created with MEMO_NEW_ARRAY_ALLOC or MEMO_NEW_ARRAY_SRC_ALLOC, using the specified allocator. */
#define MEMO_DELETE_ARRAY_ALLOC( allocator, pointer )				::memo::_delete_array( allocator, pointer )


#define MEMO_LIFO_NEW( TYPE, ... )									new ( ::memo::lifo_alloc( sizeof(TYPE), MEMO_ALIGNMENT_OF( TYPE ), 0, &memo::default_destructor_callback<TYPE> ) ) TYPE( __VA_ARGS__ )

#define MEMO_LIFO_DELETE( object )									::memo::_lifo_delete( object );

namespace memo
{
			///// allocator ////

	IAllocator & get_current_allocator();



			///// LIFO allocations /////

	ObjectStack & get_lifo_allocator();

	void * lifo_alloc( size_t i_size, size_t i_alignment, size_t i_alignment_offset, DeallocationCallback i_deallocation_callback );

	void lifo_free( void * i_address );


			///// allocations /////


	/** allocates a new memory block using the current allocator of the calling thread. 
		The address of the block with the specified offset respects the specified alignment. 
		Anyway the first byte of the block is aligned such that it can be used to store any int 
		or pointer variable.
		If the request size is zero the return value is a non-null address that points to a 
		zero-sized memory block.
		The content of the newly allocated block is undefined. 
		@param i_size size of the block in bytes
		@param i_alignment alignment requested for the block. It must be an integer power of 2
		@param i_alignment_offset offset from beginning of the block of the address that respects the alignment
		@return the address of the first byte in the block, or nullptr if the allocation fails
	*/
	void * alloc( size_t i_size, size_t i_alignment, size_t i_alignment_offset );

	/** changes the size of the memory block allocated by memo::alloc, using the allocator that 
		allocated it. The block is possibly moved it in a new location.
		If the memory block can be resized, the same address is returned. Otherwise the block is 
		allocated in a new location, and the content is copied to the new location. Then the old 
		block is freed.
		The content of the memory block is preserved up to the lesser of the new size and the old 
		size, while the content of the newly allocated portion is undefined. 
		If the request size is zero the return value is a non-null address that points to a 
		zero-sized memory block.
		The alignment and the offset of the alignment of an existing memory block cannot be changed
		by realloc.
		If the reallocation fails nullptr is returned, and the memory block is left unchanged. 
		@param i_address address of the memory block to reallocate. This parameter cannot be nullptr.
		@param i_new_size new size of the block in bytes
		@param i_alignment alignment requested for the block. It must be an integer power of 2
		@param i_alignment_offset offset from the address that respects the alignment
		@return the new address of the first byte in the block, or nullptr if the reallocation fails
	*/
	void * realloc( void * i_address, size_t i_new_size, size_t i_alignment, size_t i_alignment_offset );

	/** deallocates a memory block allocated by memo::alloc or memo::realloc, using the allocator that 
		allocated it.
		@param i_address address of the memory block to free. It cannot be nullptr.
	*/
	void free( void * i_address );

	/** performs some checks on a new memory block allocated by memo::alloc or memo::realloc, using the 
		allocator that allocated it. The actual checks depend on the allocator and the configuration 
		being compiled.
		@param i_address address of the memory block to check
	*/
	void dbg_check( void * i_address );




				///// unaligned allocations /////

	/** allocates a new memory block using the current allocator of the calling thread. 
		The first byte of the block is aligned such that it can be used to store any int 
		or pointer variable.
		If the request size is zero the return value is a non-null address that points to a 
		zero-sized memory block.
		The content of the newly allocated block is undefined. 
		@param i_size size of the block in bytes
		@return the address of the first byte in the block, or nullptr if the allocation fails
	*/
	void * unaligned_alloc( size_t i_size );

	/** changes the size of the memory block allocated by memo::unaligned_alloc, using the allocator
		that allocated it. The block is possibly moved it in a new location.
		If the memory block can be resized, the same address is returned. Otherwise the block is 
		allocated in a new location, and the content is copied to the new location. Then the old 
		block is freed.
		The content of the memory block is preserved up to the lesser of the new size and the old 
		size, while the content of the newly allocated portion is undefined. 
		If the request size is zero the return value is a non-null address that points to a 
		zero-sized memory block.
		If the reallocation fails nullptr is returned, and the memory block is left unchanged. 
		@param i_address address of the memory block to reallocate. This parameter cannot be nullptr.
		@param i_new_size new size of the block in bytes
		@param i_alignment alignment requested for the block. It must be an integer power of 2
		@param i_alignment_offset offset from the address that respects the alignment
		@return the new address of the first byte in the block, or nullptr if the reallocation fails
	*/
	void * unaligned_realloc( void * i_address, size_t i_new_size );

	/** deallocates a memory block allocated by memo::unaligned_alloc or memo::unaligned_realloc, using 
		the allocator that allocated it.
		@param i_address address of the memory block to free. It cannot be nullptr.
	*/
	void unaligned_free( void * i_address );

	/** performs some checks on a new memory block allocated by memo::unaligned_alloc or memo::unaligned_realloc,
		using the allocator that allocated it. The actual checks depend on the allocator and the 
		configuration being compiled.
		@param i_address address of the memory block to check
	*/
	void unaligned_dbg_check( void * i_address );

} // namespace memo

