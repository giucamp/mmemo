
#if MEMO_ENABLE_TLSF

	namespace memo
	{
	
		struct TlsfAllocator::AlignmentHeader
		{
			void * m_block;
		};

		// TlsfAllocator::Config::configure_allocator
		IAllocator * TlsfAllocator::Config::configure_allocator( IAllocator * i_new_allocator ) const
		{
			TlsfAllocator * allocator;
			if( i_new_allocator != nullptr )
				allocator = static_cast< TlsfAllocator * >( i_new_allocator );
			else
				allocator = MEMO_NEW( TlsfAllocator, *this );

			RegionAllocator::Config::configure_allocator( allocator );

			return allocator;
		}

		// TlsfAllocator::constructor
		TlsfAllocator::TlsfAllocator( const TlsfAllocator::Config & i_config )
			: RegionAllocator( i_config, tlsf_overhead() ), m_tlsf( nullptr )
		{
			void * region_buffer = buffer();
			if( region_buffer != nullptr )
				m_tlsf = tlsf_create( region_buffer, buffer_size() );
		}

		// TlsfAllocator::destructor
		TlsfAllocator::~TlsfAllocator()
		{
			if( m_tlsf != nullptr )
				tlsf_destroy( m_tlsf );
		}

		// TlsfAllocator::alloc
		void * TlsfAllocator::alloc( size_t i_size, size_t i_alignment, size_t i_alignment_offset )
		{
			void * result = tlsf_aligned_alloc( m_tlsf, i_size, i_alignment, i_alignment_offset );
			//if( result != nullptr )
				return result;

			//return extern_allocator().alloc( i_size, i_alignment, i_alignment_offset );
		}

		// TlsfAllocator::realloc
		void * TlsfAllocator::realloc( void * i_address, size_t i_new_size, size_t i_alignment, size_t i_alignment_offset )
		{
			void * result = tlsf_aligned_realloc( m_tlsf, i_address, i_new_size, i_alignment, i_alignment_offset );
			return result;
		}

		// TlsfAllocator::free
		void TlsfAllocator::free( void * i_address )
		{
			tlsf_aligned_free( m_tlsf, i_address );
		}

		// TlsfAllocator::dbg_check
		void TlsfAllocator::dbg_check( void * i_address )
		{
			MEMO_ASSERT( i_address != nullptr );
			MEMO_UNUSED( i_address );
		}

		// TlsfAllocator::unaligned_alloc
		void * TlsfAllocator::unaligned_alloc( size_t i_size )
		{
			void * result = tlsf_malloc( m_tlsf, i_size );
			//if( result != nullptr )
				return result;
			//return extern_allocator().unaligned_alloc( i_size );
		}
	
		// TlsfAllocator::unaligned_realloc
		void * TlsfAllocator::unaligned_realloc( void * i_address, size_t i_new_size )	
		{
			MEMO_ASSERT( i_address != nullptr );

			void * result = tlsf_realloc( m_tlsf, i_address, std::max<size_t>( i_new_size, 1 ) );
			return result;
		}

		// TlsfAllocator::unaligned_free
		void TlsfAllocator::unaligned_free( void * i_address )
		{
			MEMO_ASSERT( i_address != nullptr );

			tlsf_free( m_tlsf, i_address );
		}
	
		// TlsfAllocator::unaligned_dbg_check
		void TlsfAllocator::unaligned_dbg_check( void * i_address )
		{
			MEMO_ASSERT( i_address != nullptr );

			MEMO_UNUSED( i_address );
		}

		// TlsfAllocator::dump_state
		void TlsfAllocator::dump_state( StateWriter & i_state_writer )
		{
			i_state_writer.write( "type", "tlsf" );

			RegionAllocator::dump_state( i_state_writer );
		}

	
	} // namespace memo

#endif
