
namespace memo
{
	ContextConfig::ContextConfig()
		: m_name_string( nullptr ), m_allocator_config( nullptr )
	{

	}

	ContextConfig::ContextConfig( const char * i_name )
		: m_allocator_config( nullptr )
	{
		const size_t name_length = strlen( i_name );
		m_name_string = static_cast< char* >( memo::unaligned_alloc( (name_length + 1) * sizeof(char) ) );
		memcpy( m_name_string, i_name, name_length + 1 );

		m_name = StaticName( m_name_string );
	}

	void ContextConfig::set_name( const char * i_name )
	{
		if( m_name_string != nullptr )
			memo::unaligned_free( m_name_string );

		const size_t name_length = strlen( i_name );
		m_name_string = static_cast< char* >( memo::unaligned_alloc( (name_length + 1) * sizeof(char) ) );
		memcpy( m_name_string, i_name, name_length + 1 );

		m_name = StaticName( m_name_string );
	}

	ContextConfig::~ContextConfig()
	{
		if( m_allocator_config != nullptr )
			MEMO_DELETE( m_allocator_config );

		if( m_name_string != nullptr )
			memo::unaligned_free( m_name_string );
	}

	void ContextConfig::load( serialization::IConfigReader & i_config_reader )
	{
		if( m_allocator_config != nullptr )
			MEMO_DELETE( m_allocator_config );

		while( i_config_reader.read_next_property() )
		{
			if( i_config_reader.try_recognize_property( "allocator" ) )
			{
				const char * allocator_name = i_config_reader.curr_property_vakue_as_string();
				
				m_allocator_config = MemoryManager::get_instance().allocator_config_factor().create_allocator_config( allocator_name );
				if( m_allocator_config == nullptr )
				{
					i_config_reader.output_message( serialization::eWrongContent );
					break;
				}

				i_config_reader.tab();

				m_allocator_config->load( i_config_reader );

				i_config_reader.untab();
			}
			else if( i_config_reader.try_recognize_property( "context" ) )
			{
				const char * context_name = i_config_reader.curr_property_vakue_as_string();

				ContextConfig * new_context = MEMO_NEW( ContextConfig, context_name );

				m_inner_contexts.push_back( new_context  );

				i_config_reader.tab();

				new_context->load( i_config_reader );

				i_config_reader.untab();
			}
			else
			{
				i_config_reader.output_message( serialization::eUnrecognizedProperty );
				break;
			}
		}
	}

}

