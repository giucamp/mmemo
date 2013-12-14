
namespace memo
{
	#if MEMO_ENABLE_TEST

		// AllocatorTester::Allocation::_fill_mem
		void AllocatorTester::Allocation::_fill_mem( void * i_start_address, size_t i_size ) const
		{
			unsigned char * dest = static_cast< unsigned char * >( i_start_address );
			unsigned char * end = static_cast< unsigned char * >( address_add( i_start_address, i_size ) );

			uintptr_t value = (m_size + 1) ^ (m_alignment * 11);

			while( dest < end )
			{
				const unsigned char curr_value = ~static_cast<unsigned char>( ( ~reinterpret_cast< uintptr_t >( dest ) ^ value ) & std::numeric_limits<unsigned char>::max() );
				*dest = curr_value;
				dest++;
			}
		}

		// AllocatorTester::Allocation::_check_mem
		void AllocatorTester::Allocation::_check_mem( const void * i_start_address, size_t i_size ) const
		{
			const unsigned char * dest = static_cast< const unsigned char * >( i_start_address );
			const unsigned char * end = static_cast< const unsigned char * >( address_add( i_start_address, i_size ) );

			uintptr_t value = (m_size + 1) ^ (m_alignment * 11);

			while( dest < end )
			{
				const unsigned char curr_value = ~static_cast<unsigned char>( ( ~reinterpret_cast< uintptr_t >( dest ) ^ value ) & std::numeric_limits<unsigned char>::max() );
				MEMO_ASSERT( *dest == curr_value );
				dest++;
			}	
		}

		// AllocatorTester::do_alloc
		bool AllocatorTester::do_alloc()
		{
			const bool aligned = ( generate_rand_32() & 1 ) == 0;

			Allocation alloc;
			alloc.m_size = generate_rand_32() % m_max_allocation_size;
			if( aligned )
			{
				alloc.m_alignment = static_cast<size_t>(1) << (generate_rand_32() & 5 );
				alloc.m_size &= ~( alloc.m_alignment - 1 );
				alloc.m_offset = alloc.m_size > 0 ? ( ( generate_rand_32() % alloc.m_size ) & ~( MEMO_MIN_ALIGNMENT - 1 ) ) : 0;
				alloc.m_block = m_allocator.alloc( alloc.m_size, alloc.m_alignment, alloc.m_offset );
				if( alloc.m_block != nullptr )
				{
					MEMO_ASSERT( is_aligned( alloc.m_block, MEMO_MIN_ALIGNMENT ) );
					MEMO_ASSERT( is_aligned( address_add( alloc.m_block, alloc.m_offset ), alloc.m_alignment ) );
				}
			}
			else
			{
				alloc.m_alignment = 0;
				alloc.m_offset = 0;
				alloc.m_block = m_allocator.unaligned_alloc( alloc.m_size );
				if( alloc.m_block != nullptr )
				{
					MEMO_ASSERT( is_aligned( alloc.m_block, MEMO_MIN_ALIGNMENT ) );
				}			
			}		

			if( alloc.m_block == nullptr )
			{
				m_failed_allocations++;
				return false;
			}

			m_allocations.push_back( alloc );

			alloc._fill_mem( alloc.m_block, alloc.m_size );
		
			m_allocated_memory += alloc.m_size;
			m_max_allocated_memory = std::max( m_max_allocated_memory, m_allocated_memory );
			m_max_allocation_count = std::max( m_max_allocation_count, m_allocations.size() );

			if( aligned )
				m_allocator.dbg_check( alloc.m_block );
			else
				m_allocator.unaligned_dbg_check( alloc.m_block );

			return true;
		}

		// AllocatorTester::do_realloc
		bool AllocatorTester::do_realloc()
		{
			if( m_allocations.size() == 0 )
			{
				m_failed_reallocations++;
				return false;
			}

			// choose an allocation
			const size_t allocation_index = generate_rand_32() % m_allocations.size();
			Allocation alloc = m_allocations[ allocation_index ];
			const bool aligned = alloc.m_alignment > 0;

			// check the content
			if( aligned )
				m_allocator.dbg_check( alloc.m_block );
			else
				m_allocator.unaligned_dbg_check( alloc.m_block );
			alloc._check_mem( alloc.m_block, alloc.m_size );

			// realloc the memory
			const size_t prev_alloc_size = alloc.m_size;
			alloc.m_size = generate_rand_32() % m_max_allocation_size;
			if( aligned )
			{
				alloc.m_size &= ~( alloc.m_alignment - 1 );
				alloc.m_size = std::max( alloc.m_size, alloc.m_offset );
			}
			if( aligned )
			{
				alloc.m_block = m_allocator.realloc( alloc.m_block, alloc.m_size, alloc.m_alignment, alloc.m_offset );
				if( alloc.m_block != nullptr )
				{
					MEMO_ASSERT( is_aligned( alloc.m_block, MEMO_MIN_ALIGNMENT ) );
					MEMO_ASSERT( is_aligned( address_add( alloc.m_block, alloc.m_offset ), alloc.m_alignment ) );
				}
			}
			else
			{
				alloc.m_block = m_allocator.unaligned_realloc( alloc.m_block, alloc.m_size );
				if( alloc.m_block != nullptr )
				{
					MEMO_ASSERT( is_aligned( alloc.m_block, MEMO_MIN_ALIGNMENT ) );
				}
			}

			if( alloc.m_block == nullptr )
			{
				m_failed_reallocations++;
				return false;
			}

			m_allocations[ allocation_index ] = alloc;

			// fill & check again the content
			alloc._fill_mem( alloc.m_block, alloc.m_size );
			if( aligned )
				m_allocator.dbg_check( alloc.m_block );
			else
				m_allocator.unaligned_dbg_check( alloc.m_block );

			// done
			MEMO_ASSERT( m_allocated_memory >= prev_alloc_size );
			m_allocated_memory -= prev_alloc_size;
			m_allocated_memory += alloc.m_size;
			m_max_allocated_memory = std::max( m_max_allocated_memory, m_allocated_memory );
			return true;
		}

		// AllocatorTester::do_free
		bool AllocatorTester::do_free()
		{
			if( m_allocations.size() == 0 )
			{
				m_failed_frees++;
				return false;
			}

			// choose an allocation
			const size_t allocation_index = generate_rand_32() % m_allocations.size();
			const Allocation & alloc = m_allocations[ allocation_index ];
			const bool aligned = alloc.m_alignment > 0;
		
			// check the content
			if( aligned )
				m_allocator.dbg_check( alloc.m_block );
			else
				m_allocator.unaligned_dbg_check( alloc.m_block );
			alloc._check_mem( alloc.m_block, alloc.m_size );

			// free the memory
			if( aligned )
				m_allocator.free( alloc.m_block );
			else
				m_allocator.unaligned_free( alloc.m_block );

			// done
			MEMO_ASSERT( m_allocated_memory >= alloc.m_size );
			m_allocated_memory -= alloc.m_size;
			m_allocations.erase( m_allocations.begin() + allocation_index );
			return true;
		}

		AllocatorTester::AllocatorTester( IAllocator & i_allocator )
			: m_allocator( i_allocator ), m_max_allocation_size( 333 ),
			  m_allocated_memory(0), m_max_allocated_memory(0), m_max_allocation_count(0),
			  m_failed_allocations(0), m_failed_reallocations(0), m_failed_frees(0)
		{

		}

		// AllocatorTester::do_test_session
		void AllocatorTester::do_test_session( size_t i_iterations )
		{
			size_t iterations = 0;
			size_t allocations = 0;
			size_t reallocations = 0;
			size_t frees = 0;

			m_allocator.unaligned_free( m_allocator.unaligned_alloc( 22 ) );

			m_allocator.free( m_allocator.alloc(48, 4, 4 ) );

			// fill the buffer
			for(; iterations < i_iterations; iterations++ )
			{		
				size_t operation = generate_rand_32() & 7;

				if( operation == 3 )
				{
					if( do_free() )
						frees++;
				}
				else if( operation == 6 )
				{
					if( do_realloc() )
						reallocations++;
				}
				else 
				{
					if( do_alloc() )
						allocations++;
				}

				/*for( size_t index = 0; index < m_allocations.size(); index++ )
				{
					const Allocation & alloc = m_allocations[ index ];
					if( alloc.m_alignment > 0 )
						m_allocator.dbg_check( alloc.m_block );
					else
						m_allocator.unaligned_dbg_check( alloc.m_block );
					alloc._check_mem( alloc.m_block, alloc.m_size );
				}*/
			}

			memo_externals::output_message( "allocations: " );
			output_integer( m_allocations.size() );
			memo_externals::output_message( ", " );
			output_mem_size( m_allocated_memory );
			memo_externals::output_message( "; max allocations: " );
			output_integer( m_max_allocation_count );
			memo_externals::output_message( ", " );
			output_mem_size( m_max_allocated_memory );
			memo_externals::output_message( "\n" );

			while( do_free() )
			{
				frees++;
			}

			MEMO_ASSERT( m_allocated_memory == 0 );
			MEMO_ASSERT( m_allocations.size() == 0 );
		}

	#endif

} // namespace memo

