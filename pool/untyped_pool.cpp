
namespace memo
{
	UntypedPool::UntypedPool()
		: m_buffer_start( nullptr ), m_buffer_end( nullptr ), m_first_free( nullptr )
	{

	}

	bool UntypedPool::init( const Config & i_config )
	{
		MEMO_ASSERT( i_config.m_element_size > 0 && i_config.m_element_alignment > 0 && i_config.m_element_count > 0 );

		uninit();

		m_config = i_config;
		m_config.m_element_size = std::max( m_config.m_element_size, sizeof(FreeSlot) );
		m_config.m_element_alignment = std::max( m_config.m_element_alignment, MEMO_ALIGNMENT_OF(FreeSlot) );

		m_first_free = nullptr;
		const size_t buffer_size = m_config.m_element_size * m_config.m_element_count;
		m_buffer_start = static_cast<FreeSlot*>( get_default_allocator().alloc( buffer_size, m_config.m_element_alignment, 0 ) );
		if(m_buffer_start == nullptr )
		{
			m_buffer_end = nullptr;
			return false;
		}
		else
		{
			m_buffer_end = static_cast<FreeSlot*>( address_add( m_buffer_start, buffer_size ) );
			format_free_space();
			return true;
		}		
	}

	void UntypedPool::uninit()
	{
		if( m_buffer_start != nullptr )
		{
			get_default_allocator().free( m_buffer_start );
			m_buffer_start = nullptr;
			m_buffer_end = nullptr;
			m_first_free = nullptr;
		}
	}

	void UntypedPool::format_free_space()
	{
		MEMO_ASSERT( m_buffer_start != nullptr && m_buffer_end != nullptr && m_buffer_start <= m_buffer_end );
		
		FreeSlot * curr = m_buffer_start, * next;
		
		if( curr < m_buffer_end )
		{
			m_first_free = curr;
		}
		else
		{
			m_first_free = nullptr;
		}

		while( curr < m_buffer_end )
		{
			next = static_cast<FreeSlot *>( address_add( curr, m_config.m_element_size ) );
			if( next < m_buffer_end )
				curr->m_next = next;
			else
				curr->m_next = nullptr;

			curr = next;
		}

		MEMO_ASSERT( curr == m_buffer_end ); // the size of the buffer should be aligned to the size of an element
	}
}