	
#if defined( _MSC_VER ) && defined( MEMO_ENABLE_TLSF )

namespace memo
{
	CorruptionDetectorAllocator * CorruptionDetectorAllocator::s_first_allocator = nullptr;

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

	// exception handler
	struct CorruptionDetectorAllocator::ExceptionHandler
	{
		static LONG CALLBACK Handler( EXCEPTION_POINTERS * ExceptionInfo )
		{
			if( ExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_SINGLE_STEP )
			{
				CorruptionDetectorAllocator * allocator = s_first_allocator;
				allocator->protect();
				//ExceptionInfo->ContextRecord->EFlags &= ~(1 << 8);
				return EXCEPTION_CONTINUE_EXECUTION;
			}
			else if( ExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_GUARD_PAGE )
			{
				const bool write_access = ExceptionInfo->ExceptionRecord->ExceptionInformation[0] != 0;
				void * address = (void*)ExceptionInfo->ExceptionRecord->ExceptionInformation[1];

				if( write_access )
				{
					CorruptionDetectorAllocator * allocator = s_first_allocator;
					if( address >= allocator->m_buffer && address < allocator->m_end_of_buffer )
					{
						allocator->unprotect();

						const size_t index = address_diff( address, allocator->m_buffer );

						if( write_access )
						{
							if( !allocator->m_can_write[ index ] )
							{
								// bad write
								ExceptionInfo->ExceptionRecord->ExceptionCode = allocator->m_config.m_bad_write_access_exception;
								return EXCEPTION_CONTINUE_SEARCH;
							}

							// now this address can be read
							allocator->m_can_read[index] = true;
							ExceptionInfo->ContextRecord->EFlags |= (1 << 8);
							return EXCEPTION_CONTINUE_EXECUTION;
						}
						else
						{
							if( !allocator->m_can_read[ index ] )
							{
								// bad read
								ExceptionInfo->ExceptionRecord->ExceptionCode = allocator->m_config.m_bad_read_access_exception;
								return EXCEPTION_CONTINUE_SEARCH;
							}
							ExceptionInfo->ContextRecord->EFlags |= (1 << 8);
							return EXCEPTION_CONTINUE_EXECUTION;
						}						
					}
				}
			}

			return EXCEPTION_CONTINUE_SEARCH;
		}
	};

	// CorruptionDetectorAllocator::can_read
	bool CorruptionDetectorAllocator::can_read( void * i_address ) const
	{
		if( i_address >= m_buffer && i_address < m_end_of_buffer )
		{
			const size_t index = address_diff( i_address, m_buffer );
			return m_can_read[ index ];
		}
		return true;
	}

	// CorruptionDetectorAllocator::can_write
	bool CorruptionDetectorAllocator::can_write( void * i_address ) const
	{
		if( i_address >= m_buffer && i_address < m_end_of_buffer )
		{
			const size_t index = address_diff( i_address, m_buffer );
			return m_can_write[ index ];
		}
		return true;
	}


	// CorruptionDetectorAllocator::constructor
	CorruptionDetectorAllocator::CorruptionDetectorAllocator( const CorruptionDetectorAllocator::Config & i_config )
		: m_config( i_config )
	{
		m_next_allocator = s_first_allocator;
		s_first_allocator = this;

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
			m_exception_handler = nullptr;
			m_end_of_buffer = nullptr;
			return;
		}

		m_end_of_buffer = address_add( m_buffer, m_size );

		m_tlsf = tlsf_create( m_buffer, m_size );

		m_can_read.resize( m_size, false );
		m_can_write.resize( m_size, false );

		m_exception_handler = ::AddVectoredExceptionHandler( 0, ExceptionHandler::Handler );

		protect();
	}

	CorruptionDetectorAllocator::~CorruptionDetectorAllocator()
	{
		unprotect();

		::RemoveVectoredExceptionHandler( m_exception_handler );

		::VirtualFree( m_buffer, m_size, MEM_RELEASE );
	}

	// CorruptionDetectorAllocator::notify_allocation
	void CorruptionDetectorAllocator::notify_allocation( void * i_address, size_t i_size )
	{
		size_t start_index = address_diff( i_address, m_buffer );
		size_t end_index = start_index + i_size;
		for( size_t index = start_index; index < end_index; index++ )
			m_can_write[index] = true;
	}

	// CorruptionDetectorAllocator::notify_deallocation
	void CorruptionDetectorAllocator::notify_deallocation( void * i_address, size_t i_size )
	{
		size_t start_index = address_diff( i_address, m_buffer );
		size_t end_index = start_index + i_size;
		for( size_t index = start_index; index < end_index; index++ )
		{
			m_can_write[index] = false;
			m_can_read[index] = false;
		}
	}
	
	// CorruptionDetectorAllocator::protect
	void CorruptionDetectorAllocator::protect()
	{
		DWORD old_protect;
		::VirtualProtect( m_buffer, m_size, PAGE_READWRITE | PAGE_GUARD,  &old_protect );
	}
	
	// CorruptionDetectorAllocator::unprotect
	void CorruptionDetectorAllocator::unprotect()
	{
		DWORD old_protect;
		::VirtualProtect( m_buffer, m_size, PAGE_READWRITE,  &old_protect );
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

		protect();

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

		protect();

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
