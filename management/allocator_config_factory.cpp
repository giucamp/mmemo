


namespace memo
{
	// AllocatorConfigFactory::register_config_creater
	void AllocatorConfigFactory::register_config_creater( const char * i_name, CreaterConfigFunction i_creater_function )
	{
		const StaticName name( i_name );
		if( !m_registry.insert( std::make_pair( name, i_creater_function ) ).second )
		{
			memo_externals::output_message( "allocator config name collision, registration has failed: " );
			memo_externals::output_message( i_name );
			memo_externals::output_message( "\n" );
			memo_externals::debug_break();
		}
	}
	
	// AllocatorConfigFactory::create_allocator_config
	IAllocator::Config * AllocatorConfigFactory::create_allocator_config( const char * i_name )
	{
		const StaticName name( i_name );

		Map::const_iterator it = m_registry.find( name );
		if( it != m_registry.end() )
		{
			return (**it->second)();
		}
		else
		{
			memo_externals::output_message( "unknown allocator name, creation has failed: " );
			memo_externals::output_message( i_name ); 
			memo_externals::output_message( "\n" );
			memo_externals::debug_break();
			return nullptr;
		}
	}

} // namespace memo

