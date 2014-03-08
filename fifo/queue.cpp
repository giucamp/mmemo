

namespace memo
{
	Queue::Queue()
		: m_buffer_start( nullptr ), m_buffer_end( nullptr ), m_first_block( nullptr ), m_next_block( nullptr )
	{

	}

	Queue::Queue( void * i_buffer_start_address, size_t i_buffer_length )
		: m_buffer_start( nullptr ), m_buffer_end( nullptr ), m_first_block( nullptr ), m_next_block( nullptr )
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
			m_first_block = m_buffer_start;
			m_next_block = m_buffer_start;
		}
		else
		{
			// the buffer is too small
			m_buffer_start = nullptr;
			m_buffer_end = nullptr;
			m_first_block = nullptr;
			m_next_block = nullptr;
		}
	}

	void * Queue::alloc( size_t i_size, size_t i_alignment, size_t i_alignment_offset )
	{
		const size_t actual_size = i_size + sizeof(_Header);
		const size_t actual_alignment_offset = i_alignment_offset + sizeof(_Header);

		bool wrapped = false;
		
		for(;;)
		{
			_Header * header = static_cast< _Header * >( m_next_block );
			MEMO_ASSERT( header + 1 <= m_buffer_end );

			// get the block for the user aligning as requested
			void * new_block = upper_align( header + 1, i_alignment, actual_alignment_offset );

			// offset by the size of the block, and align to get an header
			void * new_next_block = upper_align( address_add( new_block, actual_size ), MEMO_ALIGNMENT_OF( _Header ) );

			// new_next_block must have enough space to store the next header
			if( static_cast< _Header * >( new_next_block ) + 1 <= m_buffer_end )
			{
				if( (new_next_block >= m_first_block) == (m_next_block >= m_first_block) )
				{
					// done
					header->m_next_header_offset = address_diff( new_next_block, m_next_block );
					header->m_user_block_offset = address_diff( new_block, header );
					m_next_block = new_next_block;
					return new_block;
				}
				else
					return nullptr; // new_next_block crossed m_first_block, allocation failed
			}
			else
			{
				if( wrapped )
					return nullptr; // wrapping twice, allocation failed

				// mark the current header as a wrap header
				header->m_next_header_offset = std::numeric_limits<size_t>::max();

				// restart from m_buffer_start
				m_next_block = m_buffer_start;
				wrapped = true;
			}
		}
	}

	void * Queue::get_first_block()
	{
		_Header * header = static_cast< _Header * >( m_first_block );

		// check for a wrap header
		if( header->m_next_header_offset == std::numeric_limits<size_t>::max() )
		{
			header = static_cast< _Header * >( m_buffer_start );
		}

		return address_add( header, header->m_user_block_offset );
	}

	void Queue::free_first( void * i_first_block )
	{
		MEMO_ASSERT( i_first_block == get_first_block() );

		_Header * header = static_cast< _Header * >( i_first_block );

		m_first_block = address_add( header, header->m_next_header_offset );
	}

	void Queue::clear()
	{
		m_first_block = m_buffer_start;
		m_next_block = m_buffer_start;
	}

	bool Queue::is_empty() const
	{
		return m_first_block == m_next_block;
	}
}
