
namespace memo
{	
	// This struct is put in front of dynamic arrays to keep the size
	struct _ArrayHeader
	{
		size_t m_size;
	};

	/** \endcond  */

	/** _typed_alloc<TYPE>() - allocates an object of a given type with the current allocator of the thread. No constructor is called. 
		This function is an internal service, and is not supposed to be called directly. Use MEMO_NEW instead. */
	template <typename TYPE> inline void * _typed_alloc()
	{
		if( MEMO_ALIGNMENT_OF( TYPE ) <= MEMO_MIN_ALIGNMENT )
			return unaligned_alloc( sizeof(TYPE) );
		else
			return alloc( sizeof(TYPE), MEMO_ALIGNMENT_OF( TYPE ), 0 );
	}

	/** _typed_alloc<TYPE>( i_allocator ) - allocates an object of a given type with the specified allocator. No constructor is called.
		This function is an internal service, and is not supposed to be called directly. Use MEMO_NEW_ALLOC instead. */
	template <typename TYPE> inline void * _typed_alloc( IAllocator & i_allocator )
	{
		if( MEMO_ALIGNMENT_OF( TYPE ) <= MEMO_MIN_ALIGNMENT )
			return i_allocator.unaligned_alloc( sizeof(TYPE) );
		else
			return i_allocator.alloc( sizeof(TYPE), MEMO_ALIGNMENT_OF( TYPE ), 0 );
	}

	/** _delete( i_pointer ) - destroys and deallocates an object of a given type with the allocator used to allocate it.
		This function is an internal service, and is not supposed to be called directly. Use MEMO_DELETE instead. */
	template <typename TYPE> inline void _delete( TYPE * i_pointer )
	{
		i_pointer->~TYPE();
		if( MEMO_ALIGNMENT_OF( TYPE ) <= MEMO_MIN_ALIGNMENT )
			memo::unaligned_free( i_pointer );
		else
			memo::free( i_pointer );
	}

	/** _delete( i_allocator, i_pointer ) - destroys and deallocates an object of a given type with the specified allocator.
		This function is an internal service, and is not supposed to be called directly. Use MEMO_DELETE_ALLOC instead. */
	template <typename TYPE> inline void _delete( IAllocator & i_allocator, TYPE * i_pointer )
	{
		i_pointer->~TYPE();
		if( MEMO_ALIGNMENT_OF( TYPE ) <= MEMO_MIN_ALIGNMENT )
			i_allocator.unaligned_free( i_pointer );
		else
			i_allocator.free( i_pointer );
	}

	/** _lifo_delete( i_pointer ) - destroys and deallocates an object of a given type with the lifo allocator.
		This function is an internal service, and is not supposed to be called directly. Use MEMO_LIFO_DELETE instead. */
	template <typename TYPE> inline void _lifo_delete( TYPE * i_pointer )
	{
		i_pointer->~TYPE();
		lifo_free( i_pointer );
	}

	/** _new_array<TYPE>( i_size ) - allocates and constructs an array of objects with the current allocator of the thread.
		This function is an internal service, and is not supposed to be called directly. Use MEMO_NEW_ARRAY instead. */ 
	template <typename TYPE> inline void * _new_array( size_t i_size )
	{
		_ArrayHeader * header;

		if( MEMO_ALIGNMENT_OF( TYPE ) <= MEMO_MIN_ALIGNMENT )
			header = static_cast<_ArrayHeader>( unaligned_alloc( sizeof(TYPE) * i_size + sizeof(_ArrayHeader) ) );
		else
			header = static_cast<_ArrayHeader>( alloc( sizeof(TYPE) * i_size + sizeof(_ArrayHeader), MEMO_ALIGNMENT_OF( TYPE ), sizeof(_ArrayHeader) ) );
		
		if( header != nullptr )
		{
			header->m_size = i_size;

			TYPE * result = reinterpret_cast< TYPE * >( header + 1 );
			for( size_t index = 0; index < i_size; index++ )
				new ( result[index] ) TYPE;
		}

		return result;
	}

	/** _new_array<TYPE>( i_allocator, i_size ) - allocates and constructs an array of objects with the specified allocator.
		This function is an internal service, and is not supposed to be called directly. Use MEMO_NEW_ARRAY instead. */ 
	template <typename TYPE> inline void * _new_array( IAllocator & i_allocator, size_t i_size )
	{
		_ArrayHeader * header;

		if( MEMO_ALIGNMENT_OF( TYPE ) <= MEMO_MIN_ALIGNMENT )
			header = static_cast<_ArrayHeader>( i_allocator.unaligned_alloc( sizeof(TYPE) * i_size + sizeof(_ArrayHeader) ) );
		else
			header = static_cast<_ArrayHeader>( i_allocator.alloc( sizeof(TYPE) * i_size + sizeof(_ArrayHeader), MEMO_ALIGNMENT_OF( TYPE ), sizeof(_ArrayHeader) ) );
		
		if( header != nullptr )
		{
			header->m_size = i_size;

			TYPE * result = reinterpret_cast< TYPE * >( header + 1 );
			for( size_t index = 0; index < i_size; index++ )
				new ( result[index] ) TYPE;
		}

		return result;
	}

	/** _new_array<TYPE>( i_size, i_source ) - allocates and copy-constructs an array of objects with the current allocator of the thread.
		This function is an internal service, and is not supposed to be called directly. Use MEMO_NEW_ARRAY_SRC instead. */ 
	#if MEMO_ENABLE_RVALUE_REFERENCES
		template <typename TYPE> inline TYPE * _new_array( size_t i_size, const TYPE && i_source )
	#else
		template <typename TYPE> inline TYPE * _new_array( size_t i_size, const TYPE & i_source )
	#endif
	{
		_ArrayHeader * header;

		if( MEMO_ALIGNMENT_OF( TYPE ) <= MEMO_MIN_ALIGNMENT )
			header = static_cast<_ArrayHeader>( unaligned_alloc( sizeof(TYPE) * i_size + sizeof(_ArrayHeader) ) );
		else
			header = static_cast<_ArrayHeader>( alloc( sizeof(TYPE) * i_size + sizeof(_ArrayHeader), MEMO_ALIGNMENT_OF( TYPE ), sizeof(_ArrayHeader) ) );
		
		if( header != nullptr )
		{
			header->m_size = i_size;

			TYPE * result = reinterpret_cast< TYPE * >( header + 1 );
			for( size_t index = 0; index < i_size; index++ )
				new ( result[index] ) TYPE( i_source );
		}

		return static_cast<TYPE*>( result );
	}

	/** _new_array<TYPE>( i_allocator, i_size, i_source ) - allocates and constructs an array of objects with the specified allocator.
		This function is an internal service, and is not supposed to be called directly. Use MEMO_NEW_ARRAY instead. */ 
	#if MEMO_ENABLE_RVALUE_REFERENCES
		template <typename TYPE> inline TYPE * _new_array( IAllocator & i_allocator, size_t i_size, const TYPE && i_source )
	#else
		template <typename TYPE> inline TYPE * _new_array( IAllocator & i_allocator, size_t i_size, const TYPE & i_source )
	#endif
	{
		_ArrayHeader * header;

		if( MEMO_ALIGNMENT_OF( TYPE ) <= MEMO_MIN_ALIGNMENT )
			header = static_cast<_ArrayHeader>( i_allocator.unaligned_alloc( sizeof(TYPE) * i_size + sizeof(_ArrayHeader) ) );
		else
			header = static_cast<_ArrayHeader>( i_allocator.alloc( sizeof(TYPE) * i_size + sizeof(_ArrayHeader), MEMO_ALIGNMENT_OF( TYPE ), sizeof(_ArrayHeader) ) );
		
		if( header != nullptr )
		{
			header->m_size = i_size;

			TYPE * result = reinterpret_cast< TYPE * >( header + 1 );
			for( size_t index = 0; index < i_size; index++ )
				new ( result[index] ) TYPE( i_source );
		}

		return static_cast<TYPE*>( result );
	}

	/** _delete( i_pointer ) - destroys and deallocates an array of a given type with the allocator used to allocate it.
		This function is an internal service, and is not supposed to be called directly. Use MEMO_DELETE_ARRAY instead. */
	template <typename TYPE> inline void _delete_array( TYPE * i_pointer )
	{
		_ArrayHeader * header = reinterpret_cast< _ArrayHeader * >( i_pointer ) - 1;
		
		TYPE * curr = i_pointer + header->m_size;
		while( curr >= i_pointer )
		{
			curr->~TYPE();
			curr--;
		}

		if( MEMO_ALIGNMENT_OF( TYPE ) <= MEMO_MIN_ALIGNMENT )
			return unaligned_free( header );
		else
			return free( header );
	}

	/** _delete( i_allocator, i_pointer ) - destroys and deallocates an array of a given type with with the specified allocator.
		This function is an internal service, and is not supposed to be called directly. Use MEMO_DELETE_ARRAY instead. */
	template <typename TYPE> inline void _delete_array( IAllocator & i_allocator, TYPE * i_pointer )
	{
		_ArrayHeader * header = reinterpret_cast< _ArrayHeader * >( i_pointer ) - 1;
		
		TYPE * curr = i_pointer + header->m_size;
		while( curr >= i_pointer )
		{
			curr->~TYPE();
			curr--;
		}

		if( MEMO_ALIGNMENT_OF( TYPE ) <= MEMO_MIN_ALIGNMENT )
			return i_allocator.unaligned_free( header );
		else
			return i_allocator.free( header );
	}

} // namespace memo

