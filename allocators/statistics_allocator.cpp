
namespace memo
{
	struct StatAllocator::Header
	{
		size_t m_block_size;
	};
	
	// StatAllocator::Config::configure_allocator
	IAllocator * StatAllocator::Config::configure_allocator( IAllocator * i_new_allocator ) const
	{
		StatAllocator * allocator;
		if( i_new_allocator != nullptr )
			allocator = static_cast< StatAllocator * >( i_new_allocator );
		else
			allocator = MEMO_NEW( StatAllocator );

		DecoratorAllocator::Config::configure_allocator( allocator );
		
		return allocator;
	}

	StatAllocator::Statistics::Statistics()
		: m_allocation_count( 0 ), m_allocation_count_peak( 0 ),
		 m_total_allocated( 0 ), m_total_allocated_peak( 0 ),
		 m_min_address( ), m_max_address( reinterpret_cast<void*>( std::numeric_limits<uintptr_t>::max() ) )
	{
	}

	StatAllocator::StatAllocator()
	{
	}
	
	void StatAllocator::get_stats( Statistics & o_dest )
	{
		MutexLock lock( m_mutex );
		o_dest = m_stats;
	}

	
	size_t StatAllocator::get_size( void * i_address )
	{
		Header * header = static_cast<Header*>( i_address ) - 1;
		return header->m_block_size;
	}
		

	void * StatAllocator::alloc( size_t i_size, size_t i_alignment, size_t i_alignment_offset )
	{
		IAllocator & dest_allocator = this->dest_allocator();

		// alloc
		Header * header = static_cast<Header *>( dest_allocator.alloc( i_size + sizeof( Header ), i_alignment, i_alignment_offset + sizeof( Header ) ) );
		if( header == nullptr )
			return nullptr;

		// setup header
		header->m_block_size = i_size;
		void * result = header + 1;

		// update stats
		MutexLock lock( m_mutex );
		m_stats.m_allocation_count++;
		m_stats.m_total_allocated += i_size;
		m_stats.m_allocation_count_peak = std::max( m_stats.m_allocation_count_peak, m_stats.m_allocation_count );
		m_stats.m_total_allocated_peak = std::max( m_stats.m_total_allocated_peak, m_stats.m_total_allocated );
		m_stats.m_min_address = std::min( m_stats.m_min_address, result );
		m_stats.m_max_address = std::max( m_stats.m_max_address, address_add( result, i_size ) );

		// done
		return result;
	}


	void * StatAllocator::realloc( void * i_address, size_t i_new_size, size_t i_alignment, size_t i_alignment_offset )
	{
		Header * const prev_header = static_cast<Header*>( i_address ) - 1;
		const size_t prev_size = prev_header->m_block_size;

		// reallocate
		IAllocator & dest_allocator = this->dest_allocator();
		void * result = dest_allocator.realloc( prev_header, i_new_size + sizeof( Header ), i_alignment, i_alignment_offset + sizeof( Header ) );
		if( result == nullptr )
			return nullptr;
		Header * new_header = static_cast<Header *>( result ); 
		new_header->m_block_size = i_new_size;
		void * new_user_block = new_header + 1;

		// update stats
		MutexLock lock( m_mutex );
		MEMO_ASSERT( m_stats.m_total_allocated >= prev_size );
		m_stats.m_total_allocated -= prev_size;
		m_stats.m_total_allocated += i_new_size;
		m_stats.m_total_allocated_peak = std::max( m_stats.m_total_allocated_peak, m_stats.m_total_allocated );
		m_stats.m_min_address = std::min( m_stats.m_min_address, new_user_block );
		m_stats.m_max_address = std::max( m_stats.m_max_address, address_add( new_user_block, i_new_size ) );

		return new_user_block;	
	}

	void StatAllocator::free( void * i_address )
	{
		if( i_address != nullptr )
			return;

		Header * const header = static_cast<Header*>( i_address ) - 1;
		const size_t size = header->m_block_size;

		IAllocator & dest_allocator = this->dest_allocator();
		dest_allocator.free( header );

		// update stats
		MutexLock lock( m_mutex );
		MEMO_ASSERT( m_stats.m_allocation_count > 0 );
		MEMO_ASSERT( m_stats.m_total_allocated >= size );
		m_stats.m_total_allocated -= size;
		m_stats.m_allocation_count--;
	}

	void StatAllocator::dbg_check( void * i_address )
	{
		IAllocator & dest_allocator = this->dest_allocator();

		Header * const header = static_cast<Header*>( i_address ) - 1;

		dest_allocator.dbg_check( header );
	}

	void * StatAllocator::unaligned_alloc( size_t i_size )
	{
		IAllocator & dest_allocator = this->dest_allocator();

		// alloc
		Header * header = static_cast<Header *>( dest_allocator.unaligned_alloc( i_size + sizeof( Header ) ) );
		if( header == nullptr )
			return nullptr;

		// setup header
		header->m_block_size = i_size;
		void * result = header + 1;

		// update stats
		MutexLock lock( m_mutex );
		m_stats.m_allocation_count++;
		m_stats.m_total_allocated += i_size;
		m_stats.m_allocation_count_peak = std::max( m_stats.m_allocation_count_peak, m_stats.m_allocation_count );
		m_stats.m_total_allocated_peak = std::max( m_stats.m_total_allocated_peak, m_stats.m_total_allocated );
		m_stats.m_min_address = std::min( m_stats.m_min_address, result );
		m_stats.m_max_address = std::max( m_stats.m_max_address, address_add( result, i_size ) );

		// done
		return result;
	}

	void * StatAllocator::unaligned_realloc( void * i_address, size_t i_new_size )
	{
		Header * const prev_header = static_cast<Header*>( i_address ) - 1;
		const size_t prev_size = prev_header->m_block_size;

		// allocate
		IAllocator & dest_allocator = this->dest_allocator();
		void * result = dest_allocator.unaligned_realloc( prev_header, i_new_size + sizeof( Header ) );
		if( result == nullptr )
			return nullptr;
		Header * new_header = static_cast<Header *>( result ); 
		new_header->m_block_size = i_new_size;
		void * new_user_block = new_header + 1;

		// update stats
		MutexLock lock( m_mutex );
		MEMO_ASSERT( m_stats.m_total_allocated >= prev_size );
		m_stats.m_total_allocated -= prev_size;
		m_stats.m_total_allocated += i_new_size;
		m_stats.m_total_allocated_peak = std::max( m_stats.m_total_allocated_peak, m_stats.m_total_allocated );
		m_stats.m_min_address = std::min( m_stats.m_min_address, new_user_block );
		m_stats.m_max_address = std::max( m_stats.m_max_address, address_add( new_user_block, i_new_size ) );

		return new_user_block;
	}

	void StatAllocator::unaligned_free( void * i_address )
	{
		Header * const header = static_cast<Header*>( i_address ) - 1;
		const size_t size = header->m_block_size;

		IAllocator & dest_allocator = this->dest_allocator();
		dest_allocator.unaligned_free( header );

		// update stats
		MutexLock lock( m_mutex );
		MEMO_ASSERT( m_stats.m_allocation_count > 0 );
		MEMO_ASSERT( m_stats.m_total_allocated >= size );
		m_stats.m_total_allocated -= size;
		m_stats.m_allocation_count--;
	}

	void StatAllocator::unaligned_dbg_check( void * i_address )
	{
		IAllocator & dest_allocator = this->dest_allocator();

		Header * const header = static_cast<Header*>( i_address ) - 1;

		dest_allocator.unaligned_dbg_check( header );
	}

	// StatAllocator::dump_state
	void StatAllocator::dump_state( StateWriter & i_state_writer )
	{
		Statistics statistics;
		get_stats( statistics );

		i_state_writer.write( "type", "stats" );
		i_state_writer.write_uint( "allocation_count", statistics.m_allocation_count );
		i_state_writer.write_mem_size( "total_allocated", statistics.m_total_allocated );
		i_state_writer.write_uint( "allocation_count_peak", statistics.m_allocation_count_peak );
		i_state_writer.write_mem_size( "total_allocated_peak", statistics.m_total_allocated_peak );

		DecoratorAllocator::dump_state( i_state_writer );
	}

} // namespace memo
