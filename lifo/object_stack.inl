
namespace memo
{

							/// allocation services ///

	// ObjectLifoAllocator::default constructor
	MEMO_INLINE ObjectLifoAllocator::ObjectLifoAllocator()
		: m_curr_address( nullptr ), m_start_address( nullptr ), m_end_address( nullptr )
	{
	}

	// ObjectLifoAllocator::destructor
	MEMO_INLINE ObjectLifoAllocator::~ObjectLifoAllocator()
	{
		free_all();
	}

	// ObjectLifoAllocator::constructor
	MEMO_INLINE ObjectLifoAllocator::ObjectLifoAllocator( void * i_buffer_start_address, size_t i_buffer_length )
	{
		set_buffer( i_buffer_start_address, i_buffer_length );
	}

	// ObjectLifoAllocator::free
	MEMO_INLINE void ObjectLifoAllocator::free( void * i_address )
	{
		MEMO_ASSERT( i_address >= m_start_address && i_address < m_end_address );

		#if MEMO_ENABLE_ASSERT
			Footer * footer = static_cast<Footer *>( m_curr_address ) - 1;
			MEMO_ASSERT( i_address == footer->m_block );
		#endif

		free_to_bookmark( i_address );
	}

	// ObjectLifoAllocator::free_all
	MEMO_INLINE void ObjectLifoAllocator::free_all()
	{
		free_to_bookmark( m_start_address );

		#if MEMO_LIFO_ALLOC_DEBUG
			MEMO_ASSERT( m_dbg_allocations.size() == 0 );
		#endif
	}



					/// getters ///

	// ObjectLifoAllocator::get_bookmark
	MEMO_INLINE void * ObjectLifoAllocator::get_bookmark() const
	{
		return m_curr_address;
	}

	// ObjectLifoAllocator::get_buffer_start
	MEMO_INLINE const void * ObjectLifoAllocator::get_buffer_start() const
	{
		return m_start_address;
	}

	// ObjectLifoAllocator::get_buffer_size
	MEMO_INLINE size_t ObjectLifoAllocator::get_buffer_size() const
	{
		return address_diff( m_end_address, m_start_address );
	}

	// ObjectLifoAllocator::get_free_space
	MEMO_INLINE size_t ObjectLifoAllocator::get_free_space() const
	{
		return address_diff( m_end_address, m_curr_address );
	}

	// ObjectLifoAllocator::get_used_space
	MEMO_INLINE size_t ObjectLifoAllocator::get_used_space() const
	{
		return address_diff( m_curr_address, m_start_address );
	}

} // namespace memo
