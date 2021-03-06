
namespace memo
{
	// ObjectStack::constructor
	ObjectStack::ObjectStack()
		: m_last_page( nullptr ), m_target_allocator( nullptr ), m_page_size( 0 )
	{

	}

	// ObjectStack::init
	bool ObjectStack::init( IAllocator & i_allocator, size_t i_first_page_size, size_t i_other_pages_size )
	{
		// clear
		uninit();
		
		m_target_allocator = &i_allocator;
		m_page_size = i_other_pages_size;

		// try to create the first page
		size_t page_size = i_first_page_size;
		while( !new_page( page_size ) )
		{
			if( page_size < sizeof( PageHeader ) * 4 )
			{
				// failed, undo the changes
				m_target_allocator = nullptr;
				m_page_size = 0;
				return false;
			}

			// halve the size and retry
			page_size /= 2;
		}

		// succeeded
		return true;
	}

	// ObjectStack::uninit
	void ObjectStack::uninit()
	{
		PageHeader * curr = m_last_page;
		while( curr != nullptr )
		{
			PageHeader * prev = curr->m_prev_page;
			destroy_page( curr );
			curr = prev;
		}
		m_last_page = nullptr;
		m_target_allocator = nullptr;
	}

	// ObjectStack::new_page - internal service
	bool ObjectStack::new_page( size_t i_min_size )
	{	
		size_t size = std::max( i_min_size, sizeof(PageHeader) * 4 );

		// allocate the page
		PageHeader * header = static_cast< PageHeader * >( m_target_allocator->unaligned_alloc( size ) );
		if( header == nullptr )
			return false;

		// initialize the page
		::new( header ) PageHeader();
		header->m_prev_page = m_last_page;
		header->m_size = size;
		header->m_lifo_allocator.set_buffer( header + 1, size - sizeof(PageHeader) );

		// succeeded
		m_last_page = header;				
		return true;	
	}

	// ObjectStack::destroy_page
	void ObjectStack::destroy_page( PageHeader * i_page )
	{
		i_page->~PageHeader();
		m_target_allocator->unaligned_free( i_page );
	}

	// ObjectStack::alloc
	void * ObjectStack::alloc( size_t i_size, size_t i_alignment, size_t i_alignment_offset, DeallocationCallback i_deallocation_callback )
	{
		MEMO_ASSERT( m_last_page != nullptr ); // the stack must be initialized first

		// try to allocate in the last page
		void * result = m_last_page->m_lifo_allocator.alloc( i_size, i_alignment, i_alignment_offset, i_deallocation_callback );
		if( result != nullptr )
			return result;

		// try to allocate a new page
		const size_t min_page_size = i_size + (i_alignment + ( sizeof( PageHeader ) * 2 + MEMO_ALIGNMENT_OF( PageHeader ) ));
		const size_t page_size = std::max( min_page_size, m_page_size );
		if( !new_page( page_size ) )
			return nullptr;

		/* allocate the block in the new page - the first allocation in the page cannot be zero-sized, otherwise 
			ObjectStack::free would destroy the page before freeing all the blocks. */
		result = m_last_page->m_lifo_allocator.alloc( i_size > 0 ? i_size : 1, i_alignment, i_alignment_offset, i_deallocation_callback );
		MEMO_ASSERT( result != nullptr ); // page_size should be enough to allocate the block
		return result;
	}

	// ObjectStack::free
	void ObjectStack::free( void * i_address )
	{
		MEMO_ASSERT( m_last_page != nullptr ); // the stack must be initialized first

		PageHeader * const last_page = m_last_page;

		last_page->m_lifo_allocator.free( i_address );

		if( last_page->m_lifo_allocator.get_used_space() == 0 )
		{
			// the last page is empty, if it's not the first it can be freed
			PageHeader * const prev = last_page->m_prev_page;
			if( prev != nullptr )
			{
				destroy_page( last_page );
				m_last_page = prev;
			}
		}
	}

	// ObjectStack::free_all
	void ObjectStack::free_all()
	{
		MEMO_ASSERT( m_last_page != nullptr ); // the datastack must be initialized

		PageHeader * curr = m_last_page;
		while( curr->m_prev_page != nullptr )
		{
			PageHeader * prev = curr->m_prev_page;
			destroy_page( curr );
			curr = prev;
		}
	}

	// ObjectStack::StateInfo::constructor
	ObjectStack::StateInfo::StateInfo()
	{
		reset();
	}

	// ObjectStack::StateInfo::reset
	void ObjectStack::StateInfo::reset()
	{
		#if MEMO_LIFO_ALLOC_DEBUG
			m_dbg_block_count = 0;
		#endif
		m_total_used_space = 0;
		m_page_count = 0;
		m_pages_total_space = 0;
	}

	// ObjectStack::get_state_info
	void ObjectStack::get_state_info( ObjectStack::StateInfo & o_info ) const
	{
		o_info.reset();

		PageHeader * curr = m_last_page;
		while( curr != nullptr )
		{
			#if MEMO_LIFO_ALLOC_DEBUG
				o_info.m_dbg_block_count += curr->m_lifo_allocator.dbg_get_curr_block_count();
			#endif
			o_info.m_total_used_space += curr->m_lifo_allocator.get_used_space();
			o_info.m_page_count++;
			o_info.m_pages_total_space += curr->m_size;
			curr = curr->m_prev_page;
		}
	}

} // namespace memo
