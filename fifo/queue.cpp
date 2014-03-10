

namespace memo
{
	Queue::Queue()
		: m_buffer_start( nullptr ), m_buffer_end( nullptr ), m_start( nullptr ), m_end( nullptr )
	{

	}

	Queue::Queue( void * i_buffer_start_address, size_t i_buffer_length )
		: m_buffer_start( nullptr ), m_buffer_end( nullptr ), m_start( nullptr ), m_end( nullptr )
	{
		set_buffer( i_buffer_start_address, i_buffer_length );
	}
	
	void Queue::set_buffer( void * i_buffer_start_address, size_t i_buffer_length )
	{
		MEMO_ASSERT( i_buffer_length > sizeof(_Header) * 2 ); // buffer too small?

		// the buffer start must be aligned like an _Header
		m_buffer_start = upper_align( i_buffer_start_address, MEMO_ALIGNMENT_OF( _Header ) );
		const size_t alignment_padding = address_diff( m_buffer_start, i_buffer_start_address );
		if( alignment_padding < i_buffer_length )
		{
			m_buffer_end = address_add( i_buffer_start_address, i_buffer_length - alignment_padding );
			m_start = m_buffer_start;
			m_end = m_buffer_start;
		}
		else
		{
			// the buffer is too small
			m_buffer_start = nullptr;
			m_buffer_end = nullptr;
			m_start = nullptr;
			m_end = nullptr;
		}
	}

	void * Queue::alloc( size_t i_size, size_t i_alignment, size_t i_alignment_offset )
	{
		bool wrapped = false;
		
		for(;;)
		{
			_Header * header = static_cast< _Header * >( m_end );
			MEMO_ASSERT( header + 1 <= m_buffer_end );

			// get the block for the user aligning as requested
			void * new_block = upper_align( header + 1, i_alignment, i_alignment_offset );

			// offset by the size of the block, and align to get an header
			void * new_next_block = upper_align( address_add( new_block, i_size ), MEMO_ALIGNMENT_OF( _Header ) );

			// new_next_block must have enough space to store the next header
			if( static_cast< _Header * >( new_next_block ) + 1 <= m_buffer_end )
			{
				if( (new_next_block >= m_start) == (m_end >= m_start) )
				{
					// done
					header->m_next_header_offset = address_diff( new_next_block, m_end );
					header->m_user_block_offset = address_diff( new_block, header );
					#if MEMO_ENABLE_ASSERT
						const size_t buffer_size = address_diff( m_buffer_end, m_buffer_start );
						MEMO_ASSERT( header->m_next_header_offset <= buffer_size );
						MEMO_ASSERT( header->m_user_block_offset <= buffer_size );
					#endif
					m_end = new_next_block;
					return new_block;
				}
				else
					return nullptr; // new_next_block crossed m_start, allocation failed
			}
			else
			{
				if( wrapped )
					return nullptr; // wrapping twice, allocation failed

				if( m_start == m_buffer_start )
					return nullptr; // out of space, allocation failed

				// mark the current header as a wrap header
				header->m_next_header_offset = std::numeric_limits<size_t>::max();

				// restart from m_buffer_start
				m_end = m_buffer_start;
				wrapped = true;
			}
		}
	}

	void * Queue::get_first_block()
	{
		_Header * header = static_cast< _Header * >( m_start );
		if( header != m_end )
		{			
			if( header->m_next_header_offset == std::numeric_limits<size_t>::max() )
			{
				header = static_cast< _Header * >( m_buffer_start );
				if( header == m_end )
					return nullptr;
			}

			return address_add( header, header->m_user_block_offset );
		}
		else
		{
			return nullptr;
		}
	}

	void Queue::free_first( void * i_first_block )
	{
		MEMO_ASSERT( i_first_block == get_first_block() );

		MEMO_UNUSED( i_first_block );

		_Header * header = static_cast< _Header * >( m_start );
		
		MEMO_ASSERT( header != m_end ); // no block available?

		if( header->m_next_header_offset == std::numeric_limits<size_t>::max() )
			header = static_cast< _Header * >( m_buffer_start );

		MEMO_ASSERT( header != m_end ); // no block available?

		void * new_first_block = address_add( header, header->m_next_header_offset );

		// check for a wrap header
		if( static_cast<_Header *>( new_first_block )->m_next_header_offset == std::numeric_limits<size_t>::max() )
		{
			new_first_block = m_buffer_start;
		}

		#if MEMO_ENABLE_ASSERT
			const size_t buffer_size = address_diff( m_buffer_end, m_buffer_start );
			if( static_cast<_Header *>( m_start )->m_next_header_offset != std::numeric_limits<size_t>::max() )
			{
				MEMO_ASSERT( static_cast<_Header *>( m_start )->m_next_header_offset <= buffer_size );
				MEMO_ASSERT( static_cast<_Header *>( m_start )->m_user_block_offset <= buffer_size );
			}
			if( new_first_block != m_end )
			{
				MEMO_ASSERT( static_cast<_Header *>( new_first_block )->m_next_header_offset <= buffer_size );
				MEMO_ASSERT( static_cast<_Header *>( new_first_block )->m_user_block_offset <= buffer_size );
			}
		#endif

		m_start = new_first_block;
	}

	void Queue::clear()
	{
		m_start = m_buffer_start;
		m_end = m_buffer_start;
	}

	bool Queue::is_empty() const
	{
		return m_start == m_end;
	}


	Queue::Iterator::Iterator()
		: m_queue( nullptr ), m_curr_header( nullptr )
	{

	}

	void Queue::Iterator::start_iteration( const Queue & i_queue )
	{
		m_queue = &i_queue;
		_Header * curr_header = static_cast< _Header * >( i_queue.m_start );
		if( curr_header != m_queue->m_end && curr_header->m_next_header_offset == std::numeric_limits<size_t>::max() )
			m_curr_header = static_cast< _Header * >( m_queue->m_buffer_start );
		else			
			m_curr_header = curr_header;
	}

	bool Queue::Iterator::is_over() const
	{
		return m_curr_header == m_queue->m_end;
	}

	void Queue::Iterator::operator ++ ( int )
	{
		_Header * curr_header = static_cast< _Header * >( m_curr_header );
		curr_header = static_cast< _Header * >( address_add( curr_header, curr_header->m_next_header_offset ) );
		if( curr_header != m_queue->m_end && curr_header->m_next_header_offset == std::numeric_limits<size_t>::max() )
			m_curr_header = static_cast< _Header * >( m_queue->m_buffer_start );
		else			
			m_curr_header = curr_header;
	}

	void * Queue::Iterator::curr_block() const
	{
		return address_add( m_curr_header, static_cast< _Header * >( m_curr_header )->m_user_block_offset );
	}


				/// test session ///

	#if MEMO_ENABLE_TEST
			
		// Queue::TestSession::constructor
		Queue::TestSession::TestSession( size_t i_buffer_size )
		{
			m_buffer = memo::unaligned_alloc( i_buffer_size ); 
			
			m_fifo_allocator = static_cast<Queue*>( memo::alloc( sizeof(Queue), MEMO_ALIGNMENT_OF(Queue), 0 ) );
			new( m_fifo_allocator ) Queue( m_buffer, i_buffer_size );
		}

		// Queue::TestSession::check_val
		void Queue::TestSession::check_val( const void * i_address, size_t i_size, uint8_t i_value )
		{
			const uint8_t * first = static_cast<const uint8_t *>( i_address );
			for( size_t index = 0; index < i_size; index++ )
			{
				MEMO_ASSERT( first[ index ] == i_value );
			}
		}

		// Queue::TestSession::allocate
		bool Queue::TestSession::allocate()
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

				memory_block = m_fifo_allocator->alloc( alloc_size, alloc_alignment, alloc_alignment_offset );
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

			Allocation alloc;
			alloc.m_block = memory_block;
			alloc.m_block_size = alloc_size;
			m_allocations.push_back( alloc );

			memset( memory_block, static_cast<int>( alloc_size & 0xFF ), alloc_size );

			return true;
		}
				
		// Queue::TestSession::free
		bool Queue::TestSession::free()
		{
			if( m_allocations.size() == 0 )
			{
				// the allocator must be empty
				//MEMO_ASSERT( m_fifo_allocator->get_buffer_size() == m_fifo_allocator->get_free_space() );
				return false;
			}

			Allocation alloc = m_allocations.front(); 

			check_val( alloc.m_block, alloc.m_block_size, static_cast<uint8_t>( alloc.m_block_size & 0xFF ) );

			m_allocations.pop_front();

			void * block = m_fifo_allocator->get_first_block();

			MEMO_ASSERT( alloc.m_block == block );

			m_fifo_allocator->free_first( block );

			return true;
		}

		void Queue::TestSession::check_consistency()
		{
			size_t count = 0;
			for( Iterator it( *m_fifo_allocator ); !it.is_over(); it++ )
			{
				MEMO_ASSERT( count < m_allocations.size() );
				Allocation & alloc = m_allocations[count];

				void * block = it.curr_block();

				MEMO_ASSERT( alloc.m_block == block );

				count++;
			}

			MEMO_ASSERT( m_allocations.size() == count );
		}

		// Queue::TestSession::fill_and_empty_test
		void Queue::TestSession::fill_and_empty_test()
		{
			size_t alloc_count = 0;
			size_t max_alloc_count = 0;
			size_t fill_iterations = 0;
			size_t empty_iterations = 0;

			// fill the buffer
			for(;;)
			{
				check_consistency();

				fill_iterations++;
				max_alloc_count = std::max( max_alloc_count, alloc_count );

				MEMO_ASSERT( m_allocations.empty() == m_fifo_allocator->is_empty() );

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
				check_consistency();
				
				empty_iterations++;
				max_alloc_count = std::max( max_alloc_count, alloc_count );

				MEMO_ASSERT( m_allocations.empty() == m_fifo_allocator->is_empty() );

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

			// MEMO_ASSERT( m_fifo_allocator->get_buffer_size() == m_fifo_allocator->get_free_space() );
		}

		// Queue::TestSession::destructor
		Queue::TestSession::~TestSession()
		{
			m_fifo_allocator->~Queue();
			memo::free( m_fifo_allocator );

			memo::unaligned_free( m_buffer );
		}

	#endif

}
