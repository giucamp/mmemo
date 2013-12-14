	
#if defined( _MSC_VER ) && defined( MEMO_ENABLE_TLSF )

namespace memo
{
	// CorruptionDetectorAllocator::Config::configure_allocator
	IAllocator * CorruptionDetectorAllocator::Config::configure_allocator( IAllocator * i_new_allocator ) const
	{
		CorruptionDetectorAllocator * allocator;
		if( i_new_allocator != nullptr )
			allocator = static_cast< CorruptionDetectorAllocator * >( i_new_allocator );
		else
			allocator = MEMO_NEW( CorruptionDetectorAllocator, *this );

		return allocator;
	}

	// CorruptionDetectorAllocator::constructor
	CorruptionDetectorAllocator::CorruptionDetectorAllocator( const CorruptionDetectorAllocator::Config & i_config )
		: m_config( i_config )
	{
		SYSTEM_INFO system_info;
		::GetSystemInfo( &system_info );

		// the size should be multiple of the memory page size
		m_size = (i_config.m_region_size + system_info.dwPageSize - 1 ) / system_info.dwPageSize;
		m_size *= system_info.dwPageSize;
		m_buffer = ::VirtualAlloc( NULL, m_size, MEM_COMMIT, PAGE_READWRITE );
		if( m_buffer == nullptr )
		{
			memo_externals::output_message( "VirtualAlloc in CorruptionDetectorAllocator failed\n" );
			memo_externals::debug_break();
			m_tlsf = nullptr;
			return;
		}

		m_tlsf = tlsf_create( m_buffer, m_size );

		int tries = 0;
		while( IsDebuggerPresent() )
		{
			if( tries == 0 )
				DebugBreak(); // please detach the debugger
			Sleep( 200 );
			tries++;
		}

		char commnand_line[ 128 ];
		char memo_debugger_name[ 4096 ];
		PROCESS_INFORMATION process_information;
		ZeroMemory( &process_information, sizeof( process_information ) );
		STARTUPINFOA startup_info;
		ZeroMemory( &startup_info, sizeof( startup_info ) );
		startup_info.cb = sizeof( startup_info );
		strcpy_s( memo_debugger_name, i_config.m_memo_debugger_name.c_str() );
		sprintf_s( commnand_line, "%d", GetCurrentProcessId() );
		BOOL result = CreateProcessA( memo_debugger_name, commnand_line, 
			NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &startup_info, &process_information );

		while( !IsDebuggerPresent() )
			Sleep( 200 );

		ULONG_PTR parameters[] = { reinterpret_cast<ULONG_PTR>( m_buffer ), static_cast<ULONG_PTR>( m_size ), i_config.m_check_granularity };
		RaiseException( EXC_SET_BUFFER, 0, 3, parameters );
	}

	CorruptionDetectorAllocator::~CorruptionDetectorAllocator()
	{
		unprotect();

		RaiseException( EXC_STOP_DEBUGGING, 0, 0, NULL);

		::VirtualFree( m_buffer, m_size, MEM_RELEASE );
	}

	// CorruptionDetectorAllocator::notify_allocation
	void CorruptionDetectorAllocator::notify_allocation( void * i_address, size_t i_size )
	{
		ULONG_PTR parameters[] = { reinterpret_cast<ULONG_PTR>( i_address ), static_cast<ULONG_PTR>( i_size ) };
		RaiseException( EXC_ALLOCATION, 0, 2, parameters );
	}

	// CorruptionDetectorAllocator::notify_deallocation
	void CorruptionDetectorAllocator::notify_deallocation( void * i_address, size_t i_size )
	{
		ULONG_PTR parameters[] = { reinterpret_cast<ULONG_PTR>( i_address ), static_cast<ULONG_PTR>( i_size ) };
		RaiseException( EXC_DEALLOCATION, 0, 2, parameters );
	}

	// CorruptionDetectorAllocator::protect
	void CorruptionDetectorAllocator::protect()
	{
		RaiseException( EXC_PROTECT, 0, 0, NULL );
	}

	// CorruptionDetectorAllocator::unprotect
	void CorruptionDetectorAllocator::unprotect()
	{
		RaiseException( EXC_UNPROTECT, 0, 0, NULL );
	}
	
	// CorruptionDetectorAllocator::alloc
	void * CorruptionDetectorAllocator::alloc( size_t i_size, size_t i_alignment, size_t i_alignment_offset )
	{
		if( m_tlsf == nullptr )
			return nullptr; // VirtualAlloc in the constructor may fail

		m_mutex.lock();

		unprotect();

		void * block = tlsf_aligned_alloc( m_tlsf, i_size + m_config.m_heading_nomansland + m_config.m_tailing_nomansland + sizeof( Header ), 
			i_alignment, i_alignment_offset + m_config.m_heading_nomansland + sizeof( Header ) );

		void * user_block = nullptr;

		if( block != nullptr )
		{
			Header * header = static_cast<Header *>( block );
			header->m_size = i_size;

			user_block = address_add( block, m_config.m_heading_nomansland + sizeof( Header ) );

			notify_allocation( user_block, i_size );
		}
		else
			protect();
		
		m_mutex.unlock();

		return user_block;
	}

	// CorruptionDetectorAllocator::realloc
	void * CorruptionDetectorAllocator::realloc( void * i_address, size_t i_new_size, size_t i_alignment, size_t i_alignment_offset )
	{
		m_mutex.lock();

		unprotect();

		Header * header = static_cast<Header *>( address_sub( i_address, m_config.m_heading_nomansland + sizeof( Header ) ) );
		const size_t prev_size = header->m_size;

		void * new_block = tlsf_aligned_realloc( m_tlsf, header, i_new_size + m_config.m_heading_nomansland + m_config.m_tailing_nomansland, 
			i_alignment, i_alignment_offset + m_config.m_heading_nomansland + sizeof( Header ) );

		void * new_user_block = nullptr;

		if( new_block != nullptr )
		{
			header = static_cast<Header *>( new_block );
			header->m_size = i_new_size;

			new_user_block = address_add( new_block, m_config.m_heading_nomansland + sizeof( Header ) );

			notify_deallocation( i_address, prev_size );
			notify_allocation( new_user_block, i_new_size );
		}
		else
			protect();

		m_mutex.unlock();

		return new_user_block;
	}

	// CorruptionDetectorAllocator::free
	void CorruptionDetectorAllocator::free( void * i_address )
	{
		m_mutex.lock();

		unprotect();

		Header * header = static_cast<Header *>( address_sub( i_address, m_config.m_heading_nomansland + sizeof( Header ) ) );
		const size_t prev_size = header->m_size;

		tlsf_aligned_free( m_tlsf, header );

		notify_deallocation( i_address, prev_size );
		
		m_mutex.unlock();
	}

	// CorruptionDetectorAllocator::dbg_check
	void CorruptionDetectorAllocator::dbg_check( void * i_address )
	{
		MEMO_ASSERT( i_address != nullptr );
		MEMO_UNUSED( i_address );
	}

	// CorruptionDetectorAllocator::unaligned_alloc
	void * CorruptionDetectorAllocator::unaligned_alloc( size_t i_size )
	{
		if( m_tlsf == nullptr )
			return nullptr; // VirtualAlloc in the constructor may fail

		m_mutex.lock();

		unprotect();

		void * block = tlsf_malloc( m_tlsf, i_size + m_config.m_heading_nomansland + m_config.m_tailing_nomansland + sizeof( Header ) );

		void * user_block = nullptr;

		if( block != nullptr )
		{
			Header * header = static_cast<Header *>( block );
			header->m_size = i_size;

			user_block = address_add( block, m_config.m_heading_nomansland + sizeof( Header ) );

			notify_allocation( user_block, i_size );
		}
		else
			protect();

		m_mutex.unlock();

		return user_block;		
	}

	// CorruptionDetectorAllocator::unaligned_realloc
	void * CorruptionDetectorAllocator::unaligned_realloc( void * i_address, size_t i_new_size )
	{
		m_mutex.lock();
		unprotect();

		Header * header = static_cast<Header *>( address_sub( i_address, m_config.m_heading_nomansland + sizeof( Header ) ) );
		const size_t prev_size = header->m_size;

		void * new_block = tlsf_realloc( m_tlsf, header, i_new_size + m_config.m_heading_nomansland + m_config.m_tailing_nomansland );
		
		void * new_user_block = nullptr;
		
		if( new_block != nullptr )
		{
			header = static_cast<Header *>( new_block );
			header->m_size = i_new_size;

			new_user_block = address_add( new_block, m_config.m_heading_nomansland + sizeof( Header ) );

			notify_deallocation( i_address, prev_size );
			notify_allocation( new_user_block, i_new_size );
		}
		else
			protect();

		m_mutex.unlock();

		return new_user_block;		
	}

	// CorruptionDetectorAllocator::unaligned_free
	void CorruptionDetectorAllocator::unaligned_free( void * i_address )
	{
		m_mutex.lock();
		unprotect();

		Header * header = static_cast<Header *>( address_sub( i_address, m_config.m_heading_nomansland + sizeof( Header ) ) );
		const size_t prev_size = header->m_size;

		tlsf_free( m_tlsf, header );

		notify_deallocation( i_address, prev_size );
		
		m_mutex.unlock();
	}

	// CorruptionDetectorAllocator::unaligned_dbg_check
	void CorruptionDetectorAllocator::unaligned_dbg_check( void * i_address )
	{
		MEMO_ASSERT( i_address != nullptr );
		MEMO_UNUSED( i_address );
	}
	
	// CorruptionDetectorAllocator::Config::try_recognize_property
	bool CorruptionDetectorAllocator::Config::try_recognize_property( serialization::IConfigReader & i_config_reader )
	{
		if( i_config_reader.try_recognize_property( "size" ) )
		{
			if( !i_config_reader.curr_property_vakue_as_uint( &m_region_size ) )
				i_config_reader.output_message( serialization::eWrongContent );

			return true;
		}

		/*if( DecoratorAllocator::Config::try_recognize_property( i_config_reader ) )
		{
			return true;
		}
		else if( i_config_reader.try_recognize_property( "new_memory_fill" ) )
		{
			const char * fill_mode_string = i_config_reader.curr_property_vakue_as_string();

			if( !string_to_fill_mode( fill_mode_string, &m_new_memory_fill_mode ) )
				i_config_reader.output_message( serialization::eWrongContent );

			return true;
		}
		else if( i_config_reader.try_recognize_property( "deleted_memory_fill" ) )
		{
			const char * fill_mode_string = i_config_reader.curr_property_vakue_as_string();

			if( !string_to_fill_mode( fill_mode_string, &m_deleted_memory_fill_mode ) )
				i_config_reader.output_message( serialization::eWrongContent );

			return true;
		}
		else if( i_config_reader.try_recognize_property( "nomansland_words" ) )
		{
			if( !i_config_reader.curr_property_vakue_as_uint( &m_nomansland_words ) )
				i_config_reader.output_message( serialization::eWrongContent );

			return true;
		}*/

		return true;
	}

	// CorruptionDetectorAllocator::dump_state
	void CorruptionDetectorAllocator::dump_state( StateWriter & i_state_writer )
	{
		i_state_writer.write( "type", "corruption_detector" );
		i_state_writer.write_mem_size( "region_size", m_config.m_region_size );
		i_state_writer.write_mem_size( "heading_nomansland_size", m_config.m_heading_nomansland );
		i_state_writer.write_mem_size( "tailing_nomansland_size", m_config.m_tailing_nomansland );
	}

} // namespace memo

#endif // #if defined( _MSC_VER ) && defined( MEMO_ENABLE_TLSF )
