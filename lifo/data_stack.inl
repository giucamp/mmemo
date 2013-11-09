
namespace memo
{

							/// allocation services ///

	// DataStack::default constructor
	MEMO_INLINE DataStack::DataStack()
		: m_curr_address( nullptr ), m_start_address( nullptr ), m_end_address( nullptr )
	{
	}

	// DataStack::destructor
	MEMO_INLINE DataStack::~DataStack()
	{
	}

	// DataStack::constructor
	MEMO_INLINE DataStack::DataStack( void * i_buffer_start_address, size_t i_buffer_length )
	{
		set_buffer( i_buffer_start_address, i_buffer_length );
	}

	// DataStack::set_buffer
	MEMO_INLINE void DataStack::set_buffer( void * i_buffer_start_address, size_t i_buffer_length )
	{
		// any allocated block is freed
		m_start_address = m_curr_address = i_buffer_start_address;
		m_end_address = address_add( i_buffer_start_address, i_buffer_length );

		#if MEMO_LIFO_ALLOC_DEBUG
			m_dbg_allocations.clear();
			memset( i_buffer_start_address, s_dbg_initialized_mem, i_buffer_length );
		#endif
	}

	// DataStack::alloc
	MEMO_INLINE void * DataStack::alloc( size_t i_size, size_t i_alignment, size_t i_alignment_offset )
	{
		MEMO_ASSERT( m_start_address != nullptr ); // no buffer assigned?

		void * result = upper_align( m_curr_address, i_alignment, i_alignment_offset );

		void * new_curr_address = address_add( result, i_size );
		if( new_curr_address > m_end_address )
			return nullptr;

		#if MEMO_LIFO_ALLOC_DEBUG
			m_dbg_allocations.push_back( result );
			memset( m_curr_address, s_dbg_allocated_mem, address_diff( new_curr_address, m_curr_address ) );
		#endif

		m_curr_address = new_curr_address;
		return result;
	}

	// DataStack::realloc
	MEMO_INLINE bool DataStack::realloc( void * i_address, size_t i_new_size )
	{
		MEMO_ASSERT( m_start_address != nullptr ); // no buffer assigned?

		MEMO_ASSERT( i_address >= m_start_address && i_address <= m_end_address );

		#if MEMO_LIFO_ALLOC_DEBUG
			MEMO_ASSERT( m_dbg_allocations.size() > 0 && m_dbg_allocations.back() == i_address );
		#endif

		void * new_curr_address = address_add( i_address, i_new_size );
		if( new_curr_address > m_end_address )
			return false;

		m_curr_address = new_curr_address;

		return true;
	}

	// DataStack::free
	MEMO_INLINE void DataStack::free( void * i_address )
	{
		MEMO_ASSERT( i_address >= m_start_address && i_address <= m_end_address );

		#if MEMO_LIFO_ALLOC_DEBUG
			MEMO_ASSERT( m_dbg_allocations.size() > 0 && m_dbg_allocations.back() == i_address );
			m_dbg_allocations.pop_back();

			memset( i_address, s_dbg_freed_mem, address_diff( m_curr_address, i_address ) );
		#endif

		m_curr_address = i_address;
	}

	// DataStack::free_all
	MEMO_INLINE void DataStack::free_all()
	{
		#if MEMO_LIFO_ALLOC_DEBUG
			m_dbg_allocations.clear();
			memset( m_start_address, s_dbg_freed_mem, address_diff( m_curr_address, m_start_address ) );
		#endif

		m_curr_address = m_start_address;
	}



					/// getters ///

	// DataStack::get_buffer_start
	MEMO_INLINE const void * DataStack::get_buffer_start() const
	{
		return m_start_address;
	}

	// DataStack::get_buffer_size
	MEMO_INLINE size_t DataStack::get_buffer_size() const
	{
		return address_diff( m_end_address, m_start_address );
	}

	// DataStack::get_free_space
	MEMO_INLINE size_t DataStack::get_free_space() const
	{
		return address_diff( m_end_address, m_curr_address );
	}

	// DataStack::get_used_space
	MEMO_INLINE size_t DataStack::get_used_space() const
	{
		return address_diff( m_curr_address, m_start_address );
	}

} // namespace memo
