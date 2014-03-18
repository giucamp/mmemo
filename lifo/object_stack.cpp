
namespace memo
{
	// ObjectLifoAllocator::set_buffer
	void ObjectLifoAllocator::set_buffer( void * i_buffer_start_address, size_t i_buffer_length )
	{
		// any allocated block is freed
		m_start_address = m_curr_address = i_buffer_start_address;
		m_end_address = address_add( i_buffer_start_address, i_buffer_length );

		#if MEMO_LIFO_ALLOC_DEBUG
			m_dbg_allocations.clear();
			memset( i_buffer_start_address, s_dbg_initialized_mem, i_buffer_length );
		#endif
	}

	// ObjectLifoAllocator::alloc
	void * ObjectLifoAllocator::alloc( size_t i_size, size_t i_alignment, size_t i_alignment_offset, DeallocationCallback i_deallocation_callback )
	{
		MEMO_ASSERT( m_start_address != nullptr ); // no buffer assigned?
		MEMO_ASSERT( is_integer_power_of_2( i_alignment ) );
		MEMO_ASSERT( i_alignment_offset <= i_size );
		MEMO_ASSERT( (i_size & (i_alignment-1)) == 0 );

		void * const block = upper_align( m_curr_address, i_alignment, i_alignment_offset );
		void * const end_of_block = address_add( block, i_size );

		Footer * const footer = static_cast< Footer * >( upper_align( end_of_block, MEMO_ALIGNMENT_OF( Footer ) ) ); 

		void * const new_pos = footer + 1;
		if( new_pos > m_end_address )
			return nullptr;

		#if MEMO_LIFO_ALLOC_DEBUG
			m_dbg_allocations.push_back( block );
			memset( m_curr_address, s_dbg_allocated_mem, address_diff( new_pos, m_curr_address ) );
		#endif

		footer->m_prev_pos = m_curr_address;
		footer->m_block = block;
		footer->m_deallocation_callback = i_deallocation_callback;

		m_curr_address = new_pos;

		return block;
	}

	// ObjectLifoAllocator::free_to_bookmark
	void ObjectLifoAllocator::free_to_bookmark( void * i_bookmark )
	{
		void * current_pos = m_curr_address;

		if( current_pos > i_bookmark ) do {

			// the footer is just before current_pos
			Footer * footer = static_cast<Footer *>( current_pos ) - 1;

			// call the destruction callback
			if( footer->m_deallocation_callback )
				(*footer->m_deallocation_callback)( footer->m_block );

			// update m_dbg_allocations and clear the memory with s_dbg_freed_mem
			#if MEMO_LIFO_ALLOC_DEBUG
				MEMO_ASSERT( m_dbg_allocations.size() > 0 && m_dbg_allocations.back() == footer->m_block );
				m_dbg_allocations.pop_back();
				memset( footer->m_block, s_dbg_freed_mem, address_diff( footer, footer->m_block ) );
			#endif

			current_pos = footer->m_prev_pos;

		} while( current_pos > i_bookmark );

		// store the current pos
		m_curr_address = current_pos;
	}

	#if MEMO_LIFO_ALLOC_DEBUG
			
		// ObjectLifoAllocator::dbg_get_curr_block_count
		size_t ObjectLifoAllocator::dbg_get_curr_block_count() const
		{
			return m_dbg_allocations.size();
		}

		// ObjectLifoAllocator::dbg_get_block_on_top
		void * ObjectLifoAllocator::dbg_get_block_on_top() const
		{
			if( m_dbg_allocations.empty() )
				return nullptr;
			else
				return m_dbg_allocations.back();
		}

	#endif

				/// test session ///

	#if MEMO_ENABLE_TEST
			
		// ObjectLifoAllocator::TestSession::constructor
		ObjectLifoAllocator::TestSession::TestSession( size_t i_buffer_size )
		{
			m_buffer = memo::unaligned_alloc( i_buffer_size ); 
			
			m_lifo_allocator = static_cast<ObjectLifoAllocator*>( memo::alloc( sizeof(ObjectLifoAllocator), MEMO_ALIGNMENT_OF(ObjectLifoAllocator), 0 ) );
			new( m_lifo_allocator ) ObjectLifoAllocator( m_buffer, i_buffer_size );
		}

		// ObjectLifoAllocator::TestSession::check_val
		void ObjectLifoAllocator::TestSession::check_val( const void * i_address, size_t i_size, uint8_t i_value )
		{
			const uint8_t * first = static_cast<const uint8_t *>( i_address );
			for( size_t index = 0; index < i_size; index++ )
			{
				MEMO_ASSERT( first[ index ] == i_value );
			}
		}

		// ObjectLifoAllocator::TestSession::allocate
		bool ObjectLifoAllocator::TestSession::allocate()
		{
			// try to allocate
			void * memory_block;
			size_t alloc_size;
			if( (generate_rand_32() & 15) == 13 )
				alloc_size = 0;
			else
				alloc_size = static_cast<size_t>( generate_rand_32() & 0xFF );
			for(;;)
			{
				const size_t alloc_alignment = static_cast<size_t>( 1 ) << ( generate_rand_32() & 5 );
				alloc_size &= ~(alloc_alignment - 1);
				const size_t alloc_alignment_offset = std::min( alloc_size, static_cast<size_t>( generate_rand_32() & 31 ) );

				memory_block = m_lifo_allocator->alloc( alloc_size, alloc_alignment, alloc_alignment_offset, nullptr );
				if( memory_block == nullptr )
				{
					if( alloc_size > 0 )
					{
						alloc_size--; // decrement the requested size and retry
						continue;
					}
					else
						break; // the size is zero, the allocation failed
				}
				else
				{
					MEMO_ASSERT( is_aligned( address_add( memory_block, alloc_alignment_offset ), alloc_alignment ) );
					break;
				}
			}

			if( memory_block == nullptr )
				return false;


			memset( memory_block, static_cast<int>( m_allocations.size() & 0xFF ), alloc_size );

			Allocation alloc;
			alloc.m_block = memory_block;
			alloc.m_block_size = alloc_size;
			m_allocations.push_back( alloc );

			return true;
		}
				
		// ObjectLifoAllocator::TestSession::free
		bool ObjectLifoAllocator::TestSession::free()
		{
			if( m_allocations.size() == 0 )
			{
				// the allocator must be empty
				MEMO_ASSERT( m_lifo_allocator->get_buffer_size() == m_lifo_allocator->get_free_space() );
				return false;
			}

			Allocation alloc = m_allocations[ m_allocations.size() - 1 ]; 
			m_allocations.erase( m_allocations.begin() + ( m_allocations.size() - 1 ) );

			check_val( alloc.m_block, alloc.m_block_size, static_cast<uint8_t>( m_allocations.size() & 0xFF ) );

			m_lifo_allocator->free( alloc.m_block );

			return true;
		}

		// ObjectLifoAllocator::TestSession::fill_and_empty_test
		void ObjectLifoAllocator::TestSession::fill_and_empty_test()
		{
			size_t alloc_count = 0;
			size_t max_alloc_count = 0;
			size_t fill_iterations = 0;
			size_t empty_iterations = 0;

			// fill the buffer
			for(;;)
			{
				fill_iterations++;
				max_alloc_count = std::max( max_alloc_count, alloc_count );

				const uint32_t rand = generate_rand_32();
				if( (rand & 7) == 3 )
				{
					if( free() )
					{
						MEMO_ASSERT( alloc_count > 0 );
						alloc_count--;
					}
				}
				else
				{
					if( allocate() )
						alloc_count++;
					else
						break;
				}
			}

			// empty the buffer
			for(;;)
			{
				empty_iterations++;
				max_alloc_count = std::max( max_alloc_count, alloc_count );

				const uint32_t rand = generate_rand_32();
				if( (rand & 7) == 3 )
				{
					if( allocate() )
					{
						alloc_count++;
					}
				}
				else
				{
					if( free() )
					{
						MEMO_ASSERT( alloc_count > 0 );
						alloc_count--;
					}
					else
						break;
				}
			}			

			MEMO_ASSERT( m_lifo_allocator->get_buffer_size() == m_lifo_allocator->get_free_space() );
		}

		// ObjectLifoAllocator::TestSession::destructor
		ObjectLifoAllocator::TestSession::~TestSession()
		{
			m_lifo_allocator->~ObjectLifoAllocator();
			memo::free( m_lifo_allocator );

			memo::unaligned_free( m_buffer );
		}

	#endif

} // namespace memo
