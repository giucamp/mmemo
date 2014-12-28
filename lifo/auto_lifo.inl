
namespace memo
{
	MEMO_INLINE AutoLifo::AutoLifo()
		: m_thread_stack( get_lifo_allocator() ), m_buffer( nullptr ), m_size( 0 ), m_alignment( 0 ), m_deallocation_callback( nullptr )
	{

	}

	MEMO_INLINE AutoLifo::~AutoLifo()
	{
		free();
	}

	MEMO_INLINE bool AutoLifo::is_allocated() const
	{
		return m_buffer != nullptr;
	}

	MEMO_INLINE void * AutoLifo::buffer() const
	{
		return m_buffer;
	}

	MEMO_INLINE size_t AutoLifo::alignment() const
	{
		return m_alignment;
	}

	MEMO_INLINE size_t AutoLifo::size() const
	{
		return m_size;
	}

	MEMO_INLINE void * AutoLifo::alloc( size_t i_size, size_t i_alignment, DeallocationCallback i_deallocation_callback )
	{
		if( m_buffer != nullptr )
		{
			if( m_deallocation_callback != nullptr )
			{
				(*m_deallocation_callback)( m_buffer );
			}

			m_deallocation_callback = i_deallocation_callback;

			if( m_size >= i_size && m_alignment >= i_alignment )
			{
				return m_buffer;
			}

			m_thread_stack.free( m_buffer );
		}

		m_deallocation_callback = i_deallocation_callback;
		m_size = i_size;
		m_alignment = i_alignment;
		m_buffer = m_thread_stack.alloc( i_size, i_alignment, 0, nullptr );
		return m_buffer;
	}

	MEMO_INLINE void AutoLifo::free()
	{
		if( m_buffer != nullptr )
		{
			if( m_deallocation_callback != nullptr )
			{
				(*m_deallocation_callback)( m_buffer );
			}
			m_thread_stack.free( m_buffer );
			m_buffer = nullptr;
		}
	}

} // namespace memo
