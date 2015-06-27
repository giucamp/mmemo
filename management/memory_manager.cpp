

namespace memo
{
	// MemoryManager::get_instance
	MemoryManager & MemoryManager::get_instance()
	{
		static MemoryManager s_instance;
		return s_instance;
	}

	// MemoryManager::get_allocator
	IAllocator * MemoryManager::get_allocator( NamePath i_context_path ) const
	{
		ContextMap::const_iterator it = m_contexts_map.find( i_context_path );
		if( it != m_contexts_map.end() )
			return it->second.m_allocator;
		else
			return nullptr;
	}

	// MemoryManager::constructor
	MemoryManager::MemoryManager()
	{
		m_allocator_config_factory.register_allocator<DefaultAllocator>();
		m_allocator_config_factory.register_allocator<DebugAllocator>();
		m_allocator_config_factory.register_allocator<StatAllocator>();
		#if MEMO_ENABLE_TLSF
			m_allocator_config_factory.register_allocator<TlsfAllocator>();
			#if defined( _WIN32 )
				m_allocator_config_factory.register_allocator<CorruptionDetectorAllocator>();
			#endif
		#endif

		memo_externals::register_custom_allocators( m_allocator_config_factory );

		if( serialization::load_config_file( m_config, memo_externals::g_config_file_name ) != serialization::eSuccessful )
		{
			memo_externals::output_message( "could not load the configuration file: " );
			memo_externals::output_message( memo_externals::g_config_file_name );
			memo_externals::output_message( "\n" );
		}

		NamePath empty_path;
		_build_context_maps( m_config.m_root_context, empty_path );
	}

	// MemoryManager::_build_context_maps
	void MemoryManager::_build_context_maps( const ContextConfig & i_context, const NamePath & i_context_path )
	{
		// setup the entry
		ContextEntry entry;
		entry.m_config = &i_context;
		IAllocator * allocator = nullptr;
		if( i_context.allocator_config() != nullptr )
		{			
			allocator = i_context.allocator_config()->create_allocator();
		}
		entry.m_allocator = allocator;

		// insert it in the map 
		if( !m_contexts_map.insert( std::make_pair( i_context_path, entry ) ).second )
		{
			memo_externals::output_message( "hash collision in the context map: " );
			memo_externals::output_message( i_context.name().name() );
			memo_externals::output_message( "\n" );
			memo_externals::debug_break();
		}

		const ContextConfig::ContextVector & inner_contexts = i_context.inner_contexts();
		for( size_t index = 0; index < inner_contexts.size(); index++ )
		{
			const ContextConfig & child_config = *inner_contexts[ index ];
			
			NamePath new_path( i_context_path );
			new_path.append_context( child_config.name() );
			_build_context_maps( child_config, new_path );
		}
		
	}

	// MemoryManager::dump_contexts
	bool MemoryManager::dump_contexts( const NamePath & i_path, IAllocator::StateWriter & i_state_writer ) const
	{
		ContextMap::const_iterator it = m_contexts_map.find( i_path );
		if( it == m_contexts_map.end() )
			return false;

		const ContextEntry & entry = it->second;

		i_state_writer.tab( "context" );
		i_state_writer.write( "name", entry.m_config->name().name() );

		entry.m_allocator->dump_state( i_state_writer );

		const ContextConfig::ContextVector & inner_contexts = entry.m_config->inner_contexts();
		const size_t context_count = inner_contexts.size();
		for( size_t context_index = 0; context_index < context_count; context_index++ )
		{
			const ContextConfig * inner_context = inner_contexts[ context_index ];

			NamePath path( i_path );
			path.append_context( inner_context->name() );

			bool result = dump_contexts( path, i_state_writer );
			MEMO_ASSERT( result );
			#if !MEMO_ENABLE_ASSERT
				MEMO_UNUSED(result);
			#endif
		}

		i_state_writer.untab();
		return true;
	}

	void MemoryManager::Config::load( serialization::IConfigReader & i_config_reader )
	{
		while( i_config_reader.read_next_property() )
		{
			if( i_config_reader.try_recognize_property( "context" ) )
			{
				const char * context_name = i_config_reader.curr_property_vakue_as_string();

				ContextConfig * new_context = MEMO_NEW( ContextConfig, context_name );

				m_root_context.add_inner_context( new_context );

				i_config_reader.tab();

				new_context->load( i_config_reader );

				i_config_reader.untab();
			}
			else if( i_config_reader.try_recognize_property( "global_allocator" ) )
			{
				const char * allocator_name = i_config_reader.curr_property_vakue_as_string();

				IAllocator::Config * global_allocator = MemoryManager::get_instance().allocator_config_factor().create_allocator_config( allocator_name );
				if( global_allocator == nullptr )
				{
					i_config_reader.output_message( serialization::eWrongContent );
					break;
				}

				m_root_context.set_allocator_config( global_allocator );

				i_config_reader.tab();

				global_allocator->load( i_config_reader );

				i_config_reader.untab();
			}
			else
			{
				i_config_reader.output_message( serialization::eUnrecognizedProperty );
				break;
			}
		}
	}

	bool MemoryManager::get_pool_object_count( const char * i_type_name, size_t * o_result ) const
	{
		MEMO_UNUSED( i_type_name );
		MEMO_UNUSED( o_result ); 
		// to do
		return false;
	}

} // namespace memo