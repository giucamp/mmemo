
namespace memo
{
	// IAllocator::Config::create_allocator
	IAllocator * IAllocator::Config::create_allocator() const
	{
		IAllocator * result = configure_allocator( nullptr );
		MEMO_ASSERT( result != nullptr ); // configure_allocator can't return nullptr
		return result;
	}

	// IAllocator::Config::configure_allocator
	IAllocator * IAllocator::Config::configure_allocator( IAllocator * i_new_allocator ) const
	{
		return i_new_allocator;
	}

	// IAllocator::Config::load
	void IAllocator::Config::load( serialization::IConfigReader & i_config_reader )
	{
		while( i_config_reader.read_next_property() )
		{
			if( !try_recognize_property( i_config_reader ) )
			{
				i_config_reader.output_message( serialization::eUnrecognizedProperty );
				break;
			}
		}
	}

	// IAllocator::StateWriter::write_uint
	void IAllocator::StateWriter::write_uint( const char * i_property_name, size_t i_value )
	{
		const size_t buffer_size = 64;
		char buffer[ buffer_size ];
		const char * value = double_to_string( buffer, buffer_size, static_cast<double>( i_value ), 0 );
		write( i_property_name, value );
	}

	// IAllocator::StateWriter::write_mem_size
	void IAllocator::StateWriter::write_mem_size( const char * i_property_name, size_t i_mem_size )
	{
		const size_t buffer_size = 64;
		char buffer[ buffer_size ];
		const char * value = mem_size_to_string( buffer, buffer_size, i_mem_size );
		write( i_property_name, value );
	}

} // namespace memo

