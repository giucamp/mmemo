
#if MEMO_ENABLE_TLSF

	#include "..\external_sources\tlsf\tlsf.h"
	#include "..\external_sources\tlsf\tlsf.c"

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
			// check the parameters
			MEMO_ASSERT( i_alignment > 0 && is_integer_power_of_2( i_alignment ) );
			MEMO_ASSERT( i_alignment_offset <= i_size );

			// make the allocation
			const size_t extra_size = ( i_alignment >= sizeof( AlignmentHeader ) ? i_alignment : sizeof( AlignmentHeader ) );
			const size_t actual_size = i_size + extra_size;
			void * block = tlsf_malloc( m_tlsf, actual_size );
			if( block == nullptr )
			{
				return nullptr;
			}

			// setup the header
			void * aligned_address = lower_align( address_add( block, extra_size ), i_alignment, i_alignment_offset );
			AlignmentHeader & header = *( static_cast<AlignmentHeader*>( aligned_address ) - 1 );
			MEMO_ASSERT( &header >= block );
			header.m_block = block;

			// done
			return aligned_address;
		}

		// TlsfAllocator::realloc
		void * TlsfAllocator::realloc( void * i_address, size_t i_new_size, size_t i_alignment, size_t i_alignment_offset )
		{
			// check the parameters
			MEMO_ASSERT( i_address != nullptr );
			MEMO_ASSERT( i_alignment > 0 && is_integer_power_of_2( i_alignment ) );
			MEMO_ASSERT( i_alignment_offset <= i_new_size );

			// realloc
			AlignmentHeader & old_header = *( static_cast<AlignmentHeader*>( i_address ) - 1 );
			const size_t extra_size = ( i_alignment >= sizeof( AlignmentHeader ) ? i_alignment : sizeof( AlignmentHeader ) );
			const size_t new_actual_size = i_new_size + extra_size;
			void * new_block = tlsf_realloc( m_tlsf, old_header.m_block, new_actual_size );
			if( new_block == nullptr )
				return nullptr;

			// setup the header
			void * aligned_address = lower_align( address_add( new_block, extra_size ), i_alignment, i_alignment_offset );
			AlignmentHeader & new_header = *( static_cast<AlignmentHeader*>( aligned_address ) - 1 );
			new_header.m_block = new_block;

			// done
			return aligned_address;
		}

		// TlsfAllocator::free
		void TlsfAllocator::free( void * i_address )
		{
			MEMO_ASSERT( i_address != nullptr );

			AlignmentHeader & header = *( static_cast<AlignmentHeader*>( i_address ) - 1 );
			tlsf_free( m_tlsf, header.m_block );
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
			return tlsf_malloc( m_tlsf, i_size );
		}
	
		// TlsfAllocator::unaligned_realloc
		void * TlsfAllocator::unaligned_realloc( void * i_address, size_t i_new_size )	
		{
			MEMO_ASSERT( i_address != nullptr );

			return tlsf_realloc( m_tlsf, i_address, std::max<size_t>( i_new_size, 1 ) );
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
