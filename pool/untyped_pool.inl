

namespace memo
{
	MEMO_INLINE void * UntypedPool::alloc_slot()
	{
		MEMO_ASSERT( m_buffer_start != nullptr ); // call init first

		if( m_first_free != nullptr )
		{
			void * result = m_first_free;
			m_first_free = m_first_free->m_next;
			MEMO_ASSERT( is_aligned( result, m_config.m_element_alignment ) );
			return result;
		}
		else
		{
			return nullptr;
		}
	}

	MEMO_INLINE void UntypedPool::free_slot( void * i_element )
	{
		MEMO_ASSERT( m_buffer_start != nullptr ); // call init first

		MEMO_ASSERT( i_element >= m_buffer_start && i_element < m_buffer_end );

		FreeSlot * new_free_slot = static_cast<FreeSlot *>( i_element );
		new_free_slot->m_next = m_first_free;
		m_first_free = new_free_slot;
	}

	MEMO_INLINE void * UntypedPool::alloc()
	{
		MEMO_ASSERT( m_buffer_start != nullptr ); // call init first

		if( m_first_free != nullptr )
		{
			void * result = m_first_free;
			m_first_free = m_first_free->m_next;
			MEMO_ASSERT( is_aligned( result, m_config.m_element_alignment ) );
			return result;
		}
		else
		{
			return memo::alloc( m_config.m_element_size, m_config.m_element_alignment, 0 );
		}
	}

	MEMO_INLINE void UntypedPool::free( void * i_element )
	{
		MEMO_ASSERT( m_buffer_start != nullptr ); // call init first

		MEMO_ASSERT( i_element != nullptr );
		MEMO_ASSERT( is_aligned( i_element, m_config.m_element_alignment ) );

		if( i_element >= m_buffer_start && i_element < m_buffer_end )
		{
			FreeSlot * new_free_slot = static_cast<FreeSlot *>( i_element );
			new_free_slot->m_next = m_first_free;
			m_first_free = new_free_slot;
		}
		else
		{
			memo::free( i_element );
		}
	}

} // namespace memo
