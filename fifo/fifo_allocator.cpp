
namespace memo
{
	const size_t Queue::s_min_page_size = sizeof( Queue::PageHeader ) * 4 + MEMO_ALIGNMENT_OF( PageHeader );

	// Queue::constructor
	Queue::Queue()
		: m_first_page( nullptr ), m_last_page( nullptr ), 
		  m_put_page( nullptr ), m_peek_page( nullptr ),
		  m_target_allocator( nullptr ), m_page_size( 0 )
	{

	}

	Queue::~Queue()
	{
		uninit();
	}

	// Queue::init
	bool Queue::init( IAllocator & i_allocator, size_t i_first_page_size, size_t i_other_pages_size )
	{
		// clear
		uninit();
		
		m_target_allocator = &i_allocator;
		m_page_size = i_other_pages_size;

		// try to create the first page
		PageHeader * new_page = create_page( i_first_page_size );
		if( new_page == nullptr )
		{
			// failed, undo the changes
			m_target_allocator = nullptr;
			m_page_size = 0;
			return false;
		}

		// succeeded
		new_page->m_next_page = new_page;
		m_first_page = new_page;
		m_last_page = new_page;
		m_put_page = new_page;
		m_peek_page = new_page;
		return true;
	}

	// Queue::uninit
	void Queue::uninit()
	{
		PageHeader * curr = m_first_page;
		if( m_first_page != nullptr )
		{
			do {
				PageHeader * next = curr->m_next_page;
				destroy_page( curr );
				curr = next;
			} while( curr != m_first_page );
		}

		m_first_page = nullptr;
		m_last_page = nullptr;
		m_put_page = nullptr;
		m_peek_page = nullptr;
		m_target_allocator = nullptr;
	}

	// Queue::create_page - internal service
	Queue::PageHeader * Queue::create_page( size_t i_min_size )
	{	
		size_t size = std::max( i_min_size, s_min_page_size );

		// allocate the page
		PageHeader * header;
		while( header = static_cast< PageHeader * >( m_target_allocator->unaligned_alloc( size ) ), header == nullptr )
		{
			// halve the size and retry
			size /= 2;

			if( size < i_min_size )
				return nullptr; // failed
		}

		// initialize the page
		::new( header ) PageHeader();
		header->m_next_page = nullptr;
		header->m_size = size;
		header->m_queue.set_buffer( header + 1, size - sizeof(PageHeader) );

		// succeeded
		return header;
	}

	// Queue::destroy_page
	void Queue::destroy_page( PageHeader * i_page )
	{
		i_page->~PageHeader();
		m_target_allocator->unaligned_free( i_page );
	}

	// Queue::alloc
	void * Queue::alloc( size_t i_size, size_t i_alignment, size_t i_alignment_offset )
	{
		MEMO_ASSERT( m_first_page != nullptr ); // the allocator must be initialized

		// try to allocate in m_put_page
		void * result = m_put_page->m_queue.alloc( i_size, i_alignment, i_alignment_offset );
		if( result != nullptr )
			return result;

		// move m_put_page to the next page
		PageHeader * next_page = m_put_page->m_next_page; 
		if( next_page == m_peek_page )
		{
			// m_put_page is reaching reached m_peek_page, create a new page
			const size_t requiredSize = ( i_size + i_alignment + s_min_page_size + 1 + sizeof(PageHeader) ) & ~(i_alignment + 1); 
			next_page = create_page( std::max( m_page_size, requiredSize ) );
			if( next_page == nullptr )
				return nullptr; // failed

			// insert the new page in the linked list
			next_page->m_next_page = m_put_page->m_next_page;
			m_put_page->m_next_page = next_page;
			if( m_put_page == m_last_page )
				m_last_page = next_page; 
		}
		m_put_page = next_page;
		
		// retry to allocate
		result = m_put_page->m_queue.alloc( i_size, i_alignment, i_alignment_offset );
		MEMO_ASSERT( result != nullptr ); // page_size should be enough to allocate the block
		return result;
	}

	// Queue::get_first_block
	void * Queue::get_first_block()
	{
		MEMO_ASSERT( m_first_page != nullptr ); // the allocator must be initialized

		return m_peek_page->m_queue.get_first_block();
	}

	// Queue::free_first
	void Queue::free_first( void * i_address )
	{
		MEMO_ASSERT( m_last_page != nullptr ); // the allocator must be initialized

		m_peek_page->m_queue.free_first( i_address );

		while( m_peek_page->m_queue.is_empty() && m_peek_page != m_put_page )
		{
			PageHeader * next_page = m_peek_page->m_next_page;
						
			// the peek page is empty, if it's not the first it can be freed
			//if( m_peek_page != m_first_page )
			{
				remove_page( m_peek_page );
				destroy_page( m_peek_page );
			}

			m_peek_page = next_page;
		}
	}

	// Queue::remove_page
	void Queue::remove_page( PageHeader * i_page )
	{
		MEMO_ASSERT( i_page != nullptr );
		MEMO_ASSERT( m_first_page != nullptr );
		//MEMO_ASSERT( i_page != m_first_page );

		PageHeader * curr_page = m_first_page;
		while( curr_page->m_next_page != i_page )
		{
			curr_page = curr_page->m_next_page;
		}

		curr_page->m_next_page = i_page->m_next_page;
		if( i_page == m_last_page )
			m_last_page = curr_page;
		if( i_page == m_first_page )
			m_first_page = i_page->m_next_page;
		if( i_page == m_put_page )
			m_put_page = i_page->m_next_page;
	}


	#if MEMO_ENABLE_TEST

		Queue::TestSession::TestSession()
		{
			m_fifo_allocator = MEMO_NEW( Queue );
			m_fifo_allocator->init( memo::get_default_allocator(), 1024, 512 );
		}

		Queue::TestSession::~TestSession()
		{
			MEMO_DELETE( m_fifo_allocator );
		}

		void Queue::TestSession::allocate()
		{	
			const size_t size = generate_rand_32() & 31;
			const size_t alignment = std::max<size_t>( 1 << (generate_rand_32() & 7), MEMO_ALIGNMENT_OF( int ) );
			const size_t alignment_offset = std::min( size, (generate_rand_32() & 31) );
			
			memo::std_vector< int >::type vect;
			vect.reserve( size );
							
			int * alloc = static_cast<int*>( m_fifo_allocator->alloc( size * sizeof(int), alignment, alignment_offset ) );

			for( size_t i = 0; i < size; i++ )
			{
				vect.push_back( i );
				alloc[i] = i;					
			}

			m_test_queue.push_back( vect );
		}

		bool Queue::TestSession::free()
		{
			if( m_test_queue.size() == 0 )
				return false;

			memo::std_vector< int >::type vect = m_test_queue.front();
			m_test_queue.pop_front();

			int * alloc = static_cast<int*>( m_fifo_allocator->get_first_block() );
			
			for( size_t i = 0; i < vect.size(); i++ )
			{
				MEMO_ASSERT( vect[i] == alloc[i] );					
			}

			for( size_t i = 0; i < vect.size(); i++ )
			{
				alloc[i] = ~i;					
			}

			m_fifo_allocator->free_first( alloc );

			return true;
		}

		void Queue::TestSession::fill_and_empty_test( size_t i_iterations )
		{
			size_t alloc_count = 0;
			size_t max_alloc_count = 0;
			size_t fill_iterations = 0;
			size_t empty_iterations = 0;

			allocate();

			allocate();

			free();

			// fill
			for( size_t iteration = 0; iteration < i_iterations; iteration++ )
			{
				fill_iterations++;
				max_alloc_count = std::max( max_alloc_count, m_test_queue.size() );

				const uint32_t rand = generate_rand_32();
				if( (rand & 7) == 3 )
					free();
				else
					allocate();
			}

			// empty
			for( size_t iteration = 0; iteration < i_iterations; iteration++ )
			{
				empty_iterations++;
				max_alloc_count = std::max( max_alloc_count, m_test_queue.size() );

				const uint32_t rand = generate_rand_32();
				if( (rand & 7) != 3 )
					free();
				else
					allocate();
			}
		}

	#endif // #if MEMO_ENABLE_TEST
}