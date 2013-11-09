

namespace memo
{
	DefaultAllocator * _g_default_allocator;
	
	// DefaultAllocator::Config::configure_allocator
	IAllocator * DefaultAllocator::Config::configure_allocator( IAllocator * i_new_allocator ) const
	{
		DefaultAllocator * allocator;
		if( i_new_allocator != nullptr )
			allocator = static_cast< DefaultAllocator * >( i_new_allocator );
		else
			allocator = MEMO_NEW( DefaultAllocator );

		IAllocator::Config::configure_allocator( i_new_allocator );
		
		return allocator;
	}

	void * DefaultAllocator::s_alloc( size_t i_size, size_t i_alignment, size_t i_alignment_offset )
	{
		// check the parameters
		MEMO_ASSERT( i_alignment > 0 && is_integer_power_of_2( i_alignment ) );
		MEMO_ASSERT( i_alignment_offset <= i_size );

		// make the allocation
		const size_t extra_size = ( i_alignment >= sizeof( AlignmentHeader ) ? i_alignment : sizeof( AlignmentHeader ) );
		const size_t actual_size = i_size + extra_size;
		void * block = ::malloc( actual_size );
		if( block == nullptr )
		{
			return nullptr;
		}

		// setup the header
		void * aligned_address = lower_align( address_add( block, extra_size ), i_alignment, i_alignment_offset );
		AlignmentHeader & header = *( static_cast<AlignmentHeader*>( aligned_address ) - 1 );
		MEMO_ASSERT( &header >= block );
		header.m_block = block;

		// done
		return aligned_address;
	}

	void * DefaultAllocator::s_realloc( void * i_address, size_t i_new_size, size_t i_alignment, size_t i_alignment_offset )
	{
		// check the parameters
		MEMO_ASSERT( i_address != nullptr );
		MEMO_ASSERT( i_alignment > 0 && is_integer_power_of_2( i_alignment ) );
		MEMO_ASSERT( i_alignment_offset <= i_new_size );

		// realloc
		AlignmentHeader & old_header = *( static_cast<AlignmentHeader*>( i_address ) - 1 );
		const size_t extra_size = ( i_alignment >= sizeof( AlignmentHeader ) ? i_alignment : sizeof( AlignmentHeader ) );
		const size_t new_actual_size = i_new_size + extra_size;
		void * new_block = ::realloc( old_header.m_block, new_actual_size );
		if( new_block == nullptr )
			return nullptr;

		// setup the header
		void * aligned_address = lower_align( address_add( new_block, extra_size ), i_alignment, i_alignment_offset );
		AlignmentHeader & new_header = *( static_cast<AlignmentHeader*>( aligned_address ) - 1 );
		new_header.m_block = new_block;

		// done
		return aligned_address;
	}

	// DefaultAllocator::dump_state
	void DefaultAllocator::dump_state( StateWriter & i_state_writer )
	{
		i_state_writer.write( "type", "default" );

	}

} // namespace memo