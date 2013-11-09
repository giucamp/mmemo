
namespace memo
{
	#if MEMO_LIFO_ALLOC_DEBUG
			
		// DataStack::dbg_get_curr_block_count
		size_t DataStack::dbg_get_curr_block_count() const
		{
			return m_dbg_allocations.size();
		}

		// DataStack::dbg_get_block_on_top
		void * DataStack::dbg_get_block_on_top() const
		{
			if( m_dbg_allocations.empty() )
				return nullptr;
			else
				return m_dbg_allocations.back();
		}

	#endif

				/// test session ///

	#if MEMO_ENABLE_TEST
			
		// DataStack::TestSession::constructor
		DataStack::TestSession::TestSession( size_t i_buffer_size )
		{
			m_buffer = memo::unaligned_alloc( i_buffer_size ); 
			
			m_lifo_allocator = static_cast<DataStack*>( memo::alloc( sizeof(DataStack), MEMO_ALIGNMENT_OF(DataStack), 0 ) );
			new( m_lifo_allocator ) DataStack( m_buffer, i_buffer_size );
		}

		// DataStack::TestSession::check_val
		void DataStack::TestSession::check_val( const void * i_address, size_t i_size, uint8_t i_value )
		{
			const uint8_t * first = static_cast<const uint8_t *>( i_address );
			for( size_t index = 0; index < i_size; index++ )
			{
				MEMO_ASSERT( first[ index ] == i_value );
			}
		}

		// DataStack::TestSession::allocate
		bool DataStack::TestSession::allocate()
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

				memory_block = m_lifo_allocator->alloc( alloc_size, alloc_alignment, alloc_alignment_offset );
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

			memset( memory_block, static_cast<int>( m_allocations.size() & 0xFF ), alloc_size );

			return true;
		}

		// DataStack::TestSession::reallocate
		bool DataStack::TestSession::reallocate()
		{
			if( m_allocations.size() == 0 )
			{
				// the allocator must be empty
				//MEMO_ASSERT( m_lifo_allocator->get_buffer_size() == m_lifo_allocator->get_free_space() );
				return false;
			}

			Allocation & alloc = m_allocations[ m_allocations.size() - 1 ]; 
			check_val( alloc.m_block, alloc.m_block_size, static_cast<uint8_t>( m_allocations.size() & 0xFF ) );

			// try to reallocate
			bool result;
			size_t alloc_size = static_cast<size_t>( generate_rand_32() & 0xFF );
			for(;;)
			{
				result = m_lifo_allocator->realloc( alloc.m_block, alloc_size );
				if( !result && alloc_size > 0 )
				{
					if( alloc_size > 0 )
					{
						alloc_size--; // decrement the requested size and retry
						continue;
					}
				}
				break;
			}

			if( !result )
				return false;

			memset( alloc.m_block, static_cast<int>( m_allocations.size() & 0xFF ), alloc_size );

			alloc.m_block_size = alloc_size;
			
			return true;
		}
		
		// DataStack::TestSession::free
		bool DataStack::TestSession::free()
		{
			if( m_allocations.size() == 0 )
			{
				// the allocator must be empty
				//MEMO_ASSERT( m_lifo_allocator->get_buffer_size() == m_lifo_allocator->get_free_space() );
				return false;
			}

			Allocation alloc = m_allocations[ m_allocations.size() - 1 ]; 

			check_val( alloc.m_block, alloc.m_block_size, static_cast<uint8_t>( m_allocations.size() & 0xFF ) );

			m_allocations.erase( m_allocations.begin() + ( m_allocations.size() - 1 ) );

			m_lifo_allocator->free( alloc.m_block );

			return true;
		}

		// DataStack::TestSession::fill_and_empty_test
		void DataStack::TestSession::fill_and_empty_test()
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
				else if( (rand & 7) == 4 )
				{
					reallocate();
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
				else if( (rand & 7) == 4 )
				{
					reallocate();
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

			// MEMO_ASSERT( m_lifo_allocator->get_buffer_size() == m_lifo_allocator->get_free_space() );
		}

		// DataStack::TestSession::destructor
		DataStack::TestSession::~TestSession()
		{
			m_lifo_allocator->~DataStack();
			memo::free( m_lifo_allocator );

			memo::unaligned_free( m_buffer );
		}

	#endif

} // namespace memo
