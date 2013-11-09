	
namespace memo
{
	// DebugAllocator::_fill_memory
	void DebugAllocator::_fill_memory( FillMode i_mode, void * i_start_address, size_t i_size )
	{
		switch( i_mode )
		{
		case eNone:			
			break;

		case eNaNs:
		{
			typedef memo_externals::FloatNanIntegerRepresentationType FloatIntType;
			void * start = upper_align( i_start_address, MEMO_ALIGNMENT_OF(uint32_t) );
			void * const end = address_add( i_start_address, i_size );
			void * const aligned_end = lower_align( end, MEMO_ALIGNMENT_OF(uint32_t) );
			uint32_t * dest = static_cast<uint32_t *>( start );
			for( ; dest < aligned_end; dest++ )
			{
				*dest = memo_externals::g_float_nan_integer_representation;
			}
			char * c_dest = reinterpret_cast<char *>( dest );
			for( ; c_dest < end; c_dest++ )
			{
				*c_dest = 'N'; 
			}
			break;
		}

		case ePseudoRandom:
		{
			void * start = upper_align( i_start_address, MEMO_ALIGNMENT_OF(uint32_t) );
			void * const end = address_add( i_start_address, i_size );
			void * const aligned_end = lower_align( end, MEMO_ALIGNMENT_OF(uint32_t) );
			uint32_t * dest = static_cast<uint32_t *>( start );
			uint32_t random = static_cast<uint32_t>( reinterpret_cast<uintptr_t>( dest ) &0xFFFFFFFF );
			for( ; dest < aligned_end; dest++ )
			{
				generate_rand_32( random );
				*dest = random;
			}
			char * c_dest = reinterpret_cast<char *>( dest );
			for( ; c_dest < end; c_dest++ )
			{
				*c_dest = 'D'; 
			}
			break;
		}

		case eBadF00d:
		{
			void * start = upper_align( i_start_address, MEMO_ALIGNMENT_OF(uint32_t) );
			void * const end = address_add( i_start_address, i_size );
			void * const aligned_end = lower_align( end, MEMO_ALIGNMENT_OF(uint32_t) );
			uint32_t * dest = static_cast<uint32_t *>( start );
			for( ; dest < aligned_end; dest++ )
			{
				*dest = 0xBADF00D;
			}
			char * c_dest = reinterpret_cast<char *>( dest );
			for( ; c_dest < end; c_dest++ )
			{
				*c_dest = 'B'; 
			}
			break;
		}

		default:
			MEMO_ASSERT( false ); // unknown value
		}
	}

	// DebugAllocator::_check_memory
	void DebugAllocator::_check_memory( FillMode i_mode, const void * i_start_address, size_t i_size )
	{
		switch( i_mode )
		{
		case eNone:			
			break;

		case eNaNs:
			{
				typedef memo_externals::FloatNanIntegerRepresentationType FloatIntType;
				const void * const end = address_add( i_start_address, i_size );
				const FloatIntType * dest = static_cast<const FloatIntType *>( i_start_address );
				for( ; dest < end; dest++ )
				{
					if( *dest != memo_externals::g_float_nan_integer_representation )
					{
						memo_externals::output_message( "memory corruption\n" );
						memo_externals::debug_break();
					}
				}
				const char * c_dest = reinterpret_cast<const char *>( dest );
				for( ; c_dest < end; c_dest++ )
				{
					if( *c_dest != 'N' )
					{
						memo_externals::output_message( "memory corruption\n" );
						memo_externals::debug_break();
					}
				}
				break;
			}

		case ePseudoRandom:
			{
				const void * const end = address_add( i_start_address, i_size );
				const uint32_t * dest = static_cast<const uint32_t *>( i_start_address );
				uint32_t random = static_cast<const uint32_t>( reinterpret_cast<uintptr_t>( dest ) &0xFFFFFFFF );
				for( ; dest < end; dest++ )
				{
					generate_rand_32( random );
					if( *dest != random )
					{
						memo_externals::output_message( "memory corruption\n" );
						memo_externals::debug_break();
					}
				}
				const char * c_dest = reinterpret_cast<const char *>( dest );
				for( ; c_dest < end; c_dest++ )
				{
					if( *c_dest != 'D' )
					{
						memo_externals::output_message( "memory corruption\n" );
						memo_externals::debug_break();
					}
				}
				break;
			}

		case eBadF00d:
			{
				const void * const end = address_add( i_start_address, i_size );
				const uint32_t * dest = static_cast<const uint32_t *>( i_start_address );
				for( ; dest < end; dest++ )
				{
					if( *dest != 0xBADF00D )
					{
						memo_externals::output_message( "memory corruption\n" );
						memo_externals::debug_break();
					}
				}
				const char * c_dest = reinterpret_cast<const char *>( dest );
				for( ; c_dest < end; c_dest++ )
				{
					if( *c_dest != 'B' )
					{
						memo_externals::output_message( "memory corruption\n" );
						memo_externals::debug_break();
					}
				}
				break;
			}

		default:
			MEMO_ASSERT( false ); // unknown value
		}
	}

	// DebugAllocator::_set_nomansland
	void DebugAllocator::_set_nomansland( void * i_start_address, size_t i_size, void * i_parameter )
	{
		MEMO_ASSERT( ( i_size % sizeof(uintptr_t) ) == 0 );
		unsigned char * dest = static_cast< unsigned char * >( i_start_address );
		unsigned char * end = static_cast< unsigned char * >( address_add( i_start_address, i_size ) );
		unsigned char value = ~static_cast<unsigned char>( ( reinterpret_cast< uintptr_t >( i_parameter ) & std::numeric_limits<unsigned char>::max() ) );

		while( dest < end )
		{
			*dest = value;
			value = ( value + 17 ) & std::numeric_limits<unsigned char>::max();
			dest++;
		}
	}
	
	// DebugAllocator::_check_nomansland
	void DebugAllocator::_check_nomansland( const void * i_start_address, size_t i_size, void * i_parameter )
	{
		MEMO_ASSERT( ( i_size % sizeof(uintptr_t) ) == 0 );
		const unsigned char * dest = static_cast< const unsigned char * >( i_start_address );
		const unsigned char * end = static_cast< const unsigned char * >( address_add( i_start_address, i_size ) );
		unsigned char value = ~static_cast<unsigned char>( ( reinterpret_cast< uintptr_t >( i_parameter ) & std::numeric_limits<unsigned char>::max() ) );

		while( dest < end )
		{
			if( *dest != value )
			{
				memo_externals::output_message( "no mans land memory corrupted\n" );
				memo_externals::debug_break();
			}
			value = ( value + 17 ) & std::numeric_limits<unsigned char>::max();
			dest++;
		}
	}

	// DebugAllocator::_invert_address
	void * DebugAllocator::_invert_address( void * i_address )
	{
		return reinterpret_cast< void * >( ~reinterpret_cast< uintptr_t >( i_address ) );
	}

	// DebugAllocator::Config::configure_allocator
	IAllocator * DebugAllocator::Config::configure_allocator( IAllocator * i_new_allocator ) const
	{
		DebugAllocator * allocator;
		if( i_new_allocator != nullptr )
			allocator = static_cast< DebugAllocator * >( i_new_allocator );
		else
			allocator = MEMO_NEW( DebugAllocator );

		DecoratorAllocator::Config::configure_allocator( allocator );

		allocator->set_config( *this );

		return allocator;
	}

	// DebugAllocator::constructor
	DebugAllocator::DebugAllocator()
		: m_new_memory_fill_mode( eNone ), m_deleted_memory_fill_mode( eNone ),
		  m_heading_nomansland_size( 0 ), m_tailing_nomansland_size( 0 )
	{
	}

	// DebugAllocator::set_config
	void DebugAllocator::set_config( const DebugAllocator::Config & i_config )
	{
		m_new_memory_fill_mode = i_config.m_new_memory_fill_mode;
		m_deleted_memory_fill_mode = i_config.m_deleted_memory_fill_mode;

		m_tailing_nomansland_size = m_heading_nomansland_size = i_config.m_nomansland_words / 2;
		m_heading_nomansland_size += i_config.m_nomansland_words & 1;

		m_heading_nomansland_size *= sizeof( void * );
		m_tailing_nomansland_size *= sizeof( void * );
	}

	// DebugAllocator::alloc
	void * DebugAllocator::alloc( size_t i_size, size_t i_alignment, size_t i_alignment_offset )
	{
		void * const block = dest_allocator().alloc( i_size + m_heading_nomansland_size + m_tailing_nomansland_size, 
			i_alignment, i_alignment_offset + m_heading_nomansland_size );

		if( block == nullptr )
			return nullptr;

		void * const user_block = address_add( block, m_heading_nomansland_size );

		void * const user_block_end = address_add( user_block, i_size );

		_set_nomansland( block, m_heading_nomansland_size, user_block );

		if( m_heading_nomansland_size >= sizeof( void * ) )
			*reinterpret_cast< void * * >( block ) = _invert_address( user_block_end );

		_fill_memory( m_new_memory_fill_mode, user_block, i_size );

		_set_nomansland( user_block_end, m_tailing_nomansland_size, user_block );

		return user_block;
	}

	// DebugAllocator::realloc
	void * DebugAllocator::realloc( void * i_address, size_t i_new_size, size_t i_alignment, size_t i_alignment_offset )
	{
		const size_t old_size = _do_check( i_address ); // the return value is valid only if m_heading_nomansland_size >= sizeof( void * )

		void * const old_block = address_sub( i_address, m_heading_nomansland_size );

		void * const new_block = dest_allocator().realloc( old_block, i_new_size + m_heading_nomansland_size + m_tailing_nomansland_size, 
			i_alignment, i_alignment_offset + m_heading_nomansland_size );

		if( new_block == nullptr )
			return nullptr;

		void * const user_block = address_add( new_block, m_heading_nomansland_size );

		void * const user_block_end = address_add( user_block, i_new_size );

		if( old_block != new_block )
		{
			_set_nomansland( new_block, m_heading_nomansland_size, user_block );
		}

		_set_nomansland( user_block_end, m_tailing_nomansland_size, user_block );

		if( m_heading_nomansland_size >= sizeof( void * ) )
		{
			*reinterpret_cast< void * * >( new_block ) = _invert_address( user_block_end );

			if( i_new_size > old_size )
				_fill_memory( m_new_memory_fill_mode, address_add( user_block, old_size ), i_new_size - old_size );
		}

		return user_block;
	}

	// DebugAllocator::free
	void DebugAllocator::free( void * i_address )
	{
		const size_t old_size = _do_check( i_address ); // the return value is valid only if m_heading_nomansland_size >= sizeof( void * )

		void * const block = address_sub( i_address, m_heading_nomansland_size );

		if( m_heading_nomansland_size >= sizeof( void * ) )
			_fill_memory( m_deleted_memory_fill_mode, block, old_size + m_heading_nomansland_size + m_tailing_nomansland_size );
		else
			_fill_memory( m_deleted_memory_fill_mode, block, m_heading_nomansland_size + m_tailing_nomansland_size );

		dest_allocator().free( block );
	}

	// DebugAllocator::_do_check
	size_t DebugAllocator::_do_check( void * i_address ) const
	{
		MEMO_ASSERT( i_address != nullptr ); // i_address can't be null

		void * const block = address_sub( i_address, m_heading_nomansland_size );

		size_t size = 0;

		if( m_heading_nomansland_size >= sizeof( void * ) )
		{
			_check_nomansland( block, m_heading_nomansland_size - sizeof( void * ), 
				address_add( i_address, sizeof( void * ) ) );

			void * user_block_end = _invert_address( *reinterpret_cast< void * * >( block ) );
			size = address_diff( user_block_end, i_address );

			_check_nomansland( user_block_end, m_tailing_nomansland_size, i_address );
		}
		else
		{
			_check_nomansland( block, m_heading_nomansland_size, i_address );
		}

		return size;
	}

	// DebugAllocator::dbg_check
	void DebugAllocator::dbg_check( void * i_address )
	{
		_do_check( i_address );
	}

	// DebugAllocator::unaligned_alloc
	void * DebugAllocator::unaligned_alloc( size_t i_size )
	{
		void * const block = dest_allocator().unaligned_alloc( i_size + m_heading_nomansland_size + m_tailing_nomansland_size );

		if( block == nullptr )
			return nullptr;

		void * const user_block = address_add( block, m_heading_nomansland_size );

		void * const user_block_end = address_add( user_block, i_size );

		_set_nomansland( block, m_heading_nomansland_size, user_block );

		if( m_heading_nomansland_size >= sizeof( void * ) )
			*reinterpret_cast< void * * >( block ) = _invert_address( user_block_end );

		_fill_memory( m_new_memory_fill_mode, user_block, i_size );

		_set_nomansland( user_block_end, m_tailing_nomansland_size, user_block );

		return user_block;
	}

	// DebugAllocator::unaligned_realloc
	void * DebugAllocator::unaligned_realloc( void * i_address, size_t i_new_size )
	{
		const size_t old_size = _do_check( i_address ); // the return value is valid only if m_heading_nomansland_size >= sizeof( void * )

		void * const old_block = address_sub( i_address, m_heading_nomansland_size );

		void * const new_block = dest_allocator().unaligned_realloc( old_block, i_new_size + m_heading_nomansland_size + m_tailing_nomansland_size );

		if( new_block == nullptr )
			return nullptr;

		void * const user_block = address_add( new_block, m_heading_nomansland_size );

		void * const user_block_end = address_add( user_block, i_new_size );

		if( old_block != new_block )
		{
			_set_nomansland( new_block, m_heading_nomansland_size, user_block );
		}

		_set_nomansland( user_block_end, m_tailing_nomansland_size, user_block );

		if( m_heading_nomansland_size >= sizeof( void * ) )
		{
			*reinterpret_cast< void * * >( new_block ) = _invert_address( user_block_end );

			if( i_new_size > old_size )
				_fill_memory( m_new_memory_fill_mode, address_add( user_block, old_size ), i_new_size - old_size );
		}

		return user_block;
	}

	// DebugAllocator::unaligned_free
	void DebugAllocator::unaligned_free( void * i_address )
	{
		const size_t old_size = _do_check( i_address ); // the return value is valid only if m_heading_nomansland_size >= sizeof( void * )

		void * const block = address_sub( i_address, m_heading_nomansland_size );

		if( m_heading_nomansland_size >= sizeof( void * ) )
			_fill_memory( m_deleted_memory_fill_mode, block, old_size + m_heading_nomansland_size + m_tailing_nomansland_size );
		else
			_fill_memory( m_deleted_memory_fill_mode, block, m_heading_nomansland_size + m_tailing_nomansland_size );

		dest_allocator().unaligned_free( block );
	}

	// DebugAllocator::unaligned_dbg_check
	void DebugAllocator::unaligned_dbg_check( void * i_address )
	{
		_do_check( i_address );
	}

	// DebugAllocator::get_block_size
	bool DebugAllocator::get_block_size( void * i_address, size_t * o_size ) const
	{
		*o_size = _do_check( i_address ); // the return value is valid only if m_heading_nomansland_size >= sizeof( void * )

		return m_heading_nomansland_size >= sizeof( void * );
	}

	// DebugAllocator::string_to_fill_mode
	bool DebugAllocator::string_to_fill_mode( const char * i_string, FillMode * o_fill_mode )
	{
		if( strcmp( i_string, "none" ) == 0 )
			*o_fill_mode = eNone;
		else if( strcmp( i_string, "NaNs" ) == 0 )
			*o_fill_mode = eNaNs;
		else if( strcmp( i_string, "pseudo_random" ) == 0 )
			*o_fill_mode = ePseudoRandom;
		else if( strcmp( i_string, "badf00d" ) == 0 )
			*o_fill_mode = eBadF00d;
		else
		{
			// unrecognized
			memo_externals::output_message( "invalid fill mode, should be one of: none, nans, pseudo_random, badf00d: " );
			memo_externals::output_message( i_string );
			memo_externals::output_message( "\n" );
			memo_externals::debug_break();
			return false;
		}

		return true;
	}

	// DebugAllocator::Config::try_recognize_property
	bool DebugAllocator::Config::try_recognize_property( serialization::IConfigReader & i_config_reader )
	{
		if( DecoratorAllocator::Config::try_recognize_property( i_config_reader ) )
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
		}

		return true;
	}

	// DebugAllocator::dump_state
	void DebugAllocator::dump_state( StateWriter & i_state_writer )
	{
		i_state_writer.write( "type", "debug" );
		i_state_writer.write_mem_size( "heading_nomansland_size", m_heading_nomansland_size );
		i_state_writer.write_mem_size( "tailing_nomansland_size", m_tailing_nomansland_size );

		DecoratorAllocator::dump_state( i_state_writer );
	}

} // namespace memo
