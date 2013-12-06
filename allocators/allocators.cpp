
#if MEMO_ENABLE_TLSF

	#include "..\external_sources\tlsf\tlsf.h"
	#include "..\external_sources\tlsf\tlsf.c"

	namespace memo
	{
		struct TlsfAlignmentHeader
		{
			void * m_block;
		};

		// tlsf_aligned_alloc
		void * tlsf_aligned_alloc( void * i_tlsf, size_t i_size, size_t i_alignment, size_t i_alignment_offset )
		{
			// check the parameters
			MEMO_ASSERT( i_alignment > 0 && is_integer_power_of_2( i_alignment ) );
			MEMO_ASSERT( i_alignment_offset <= i_size );

			// make the allocation
			const size_t extra_size = ( i_alignment >= sizeof( TlsfAlignmentHeader ) ? i_alignment : sizeof( TlsfAlignmentHeader ) );
			const size_t actual_size = i_size + extra_size;
			void * block = tlsf_malloc( i_tlsf, actual_size );
			if( block == nullptr )
			{
				return nullptr;
			}

			// setup the header
			void * aligned_address = lower_align( address_add( block, extra_size ), i_alignment, i_alignment_offset );
			TlsfAlignmentHeader & header = *( static_cast<TlsfAlignmentHeader*>( aligned_address ) - 1 );
			MEMO_ASSERT( &header >= block );
			header.m_block = block;

			// done
			return aligned_address;
		}

		// tlsf_aligned_realloc
		void * tlsf_aligned_realloc( void * i_tlsf, void * i_address, size_t i_new_size, size_t i_alignment, size_t i_alignment_offset )
		{
			// check the parameters
			MEMO_ASSERT( i_address != nullptr );
			MEMO_ASSERT( i_alignment > 0 && is_integer_power_of_2( i_alignment ) );
			MEMO_ASSERT( i_alignment_offset <= i_new_size );

			// realloc
			TlsfAlignmentHeader & old_header = *( static_cast<TlsfAlignmentHeader*>( i_address ) - 1 );
			const size_t extra_size = ( i_alignment >= sizeof( TlsfAlignmentHeader ) ? i_alignment : sizeof( TlsfAlignmentHeader ) );
			const size_t new_actual_size = i_new_size + extra_size;
			void * new_block = tlsf_realloc( i_tlsf, old_header.m_block, new_actual_size );
			if( new_block == nullptr )
			{
				return nullptr;
			}

			// setup the header
			void * aligned_address = lower_align( address_add( new_block, extra_size ), i_alignment, i_alignment_offset );
			TlsfAlignmentHeader & new_header = *( static_cast<TlsfAlignmentHeader*>( aligned_address ) - 1 );
			new_header.m_block = new_block;

			// done
			return aligned_address;
		}

		// tlsf_aligned_free
		void tlsf_aligned_free( void * i_tlsf, void * i_address )
		{
			MEMO_ASSERT( i_address != nullptr );

			TlsfAlignmentHeader & header = *( static_cast<TlsfAlignmentHeader*>( i_address ) - 1 );
			tlsf_free( i_tlsf, header.m_block );
		}
	}

#endif

#include "iallocator_config.cpp"
#include "default_allocator.cpp"
#include "decorator_allocator.cpp"
#include "debug_allocator.cpp"
#include "statistics_allocator.cpp"
#include "region_allocator.cpp"
#include "tlsf_allocator.cpp"
#include "allocator_tester.cpp"
#include "corruption_detector_allocator.cpp"