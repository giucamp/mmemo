
namespace memo
{
	extern DefaultAllocator * _g_default_allocator;

	struct DefaultAllocator::AlignmentHeader
	{
		void * m_block;
	};
	
	MEMO_INLINE DefaultAllocator & get_default_allocator()
	{
		MEMO_ASSERT( _g_default_allocator != nullptr ); /* if this assert fails, you
			are calling get_default_allocator during the construction of the global 
			variables. Use safe_get_default_allocator(). */
		return *_g_default_allocator;
	}

	MEMO_INLINE DefaultAllocator & safe_get_default_allocator()
	{
		if( _g_default_allocator == nullptr )
		{
			_g_default_allocator = new( DefaultAllocator::s_unaligned_alloc( sizeof( _g_default_allocator ) ) ) DefaultAllocator();
		}
		return *_g_default_allocator;
	}

	// DefaultAllocator::s_free
	MEMO_INLINE void DefaultAllocator::s_free( void * i_address )
	{
		MEMO_ASSERT( i_address != nullptr );

		AlignmentHeader & header = *( static_cast<AlignmentHeader*>( i_address ) - 1 );
		::free( header.m_block );
	}

	// DefaultAllocator::s_dbg_check
	MEMO_INLINE void DefaultAllocator::s_dbg_check( void * i_address )
	{
		#if MEMO_ENABLE_ASSERT
			AlignmentHeader & header = *( static_cast<AlignmentHeader*>( i_address ) - 1 );
			MEMO_ASSERT( i_address >= header.m_block && header.m_block != nullptr );
		#else
			MEMO_UNUSED( i_address );
		#endif
	}

	// DefaultAllocator::s_unaligned_alloc
	MEMO_INLINE void * DefaultAllocator::s_unaligned_alloc( size_t i_size )
	{
		void * result = ::malloc( i_size );
		if( result == nullptr && i_size == 0 )
		{
			/* some implementation may return a null pointer if the requested size is zero. */
			return ::malloc( 1 );
		}
		return result;
	}

	// DefaultAllocator::s_unaligned_realloc
	MEMO_INLINE void * DefaultAllocator::s_unaligned_realloc( void * i_address, size_t i_new_size )
	{
		MEMO_ASSERT( i_address != nullptr );

		return ::realloc( i_address, std::max<size_t>( i_new_size, 1 ) );
	}

	// DefaultAllocator::s_unaligned_free
	MEMO_INLINE void DefaultAllocator::s_unaligned_free( void * i_address )
	{
		::free( i_address );
	}

	// DefaultAllocator::s_unaligned_dbg_check
	MEMO_INLINE void DefaultAllocator::s_unaligned_dbg_check( void * /*i_address*/ )
	{

	}


} // namespace memo
