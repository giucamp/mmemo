

namespace memo
{
	// RegionAllocator::constructor
	RegionAllocator::RegionAllocator( const Config & i_config, size_t i_minimum_buffer_size ) 
		: m_extern_allocator( &safe_get_default_allocator() )
	{
		m_buffer_size = std::max( i_config.m_buffer_size, i_minimum_buffer_size );
		m_buffer = m_extern_allocator->unaligned_alloc( m_buffer_size );
		if( m_buffer == nullptr )
		{
			memo_externals::output_message( "region allocation failed\n" );
			memo_externals::debug_break();
		}
	}

	// RegionAllocator::destructor
	RegionAllocator::~RegionAllocator()
	{
		if( m_buffer != nullptr )
			m_extern_allocator->unaligned_free( m_buffer );
	}

	// RegionAllocator::Config::configure_allocator
	IAllocator * RegionAllocator::Config::configure_allocator( IAllocator * i_new_allocator ) const
	{
		MEMO_ASSERT( i_new_allocator != nullptr ); /* RegionAllocator is an abstract class,
			so the allocator must be created by the derived class */

		IAllocator::Config::configure_allocator( i_new_allocator );

		return i_new_allocator;
	}

	// RegionAllocator::Config::try_recognize_property
	bool RegionAllocator::Config::try_recognize_property( serialization::IConfigReader & i_config_reader )
	{
		if( i_config_reader.try_recognize_property( "size" ) )
		{
			if( !i_config_reader.curr_property_vakue_as_uint( &m_buffer_size ) )
				i_config_reader.output_message( serialization::eWrongContent );

			return true;
		}	

		return false;
	}

	// RegionAllocator::dump_state
	void RegionAllocator::dump_state( StateWriter & i_state_writer )
	{
		i_state_writer.write_mem_size( "buffer_size", m_buffer_size );
	}

} // namespace memo
