
#include <limits>
#include <malloc.h>
#include <algorithm>
#include <iostream>
#include <fstream>

#include "memo.h"

#if defined( _MSC_VER ) && defined( MEMO_ENABLE_TLSF )
	#define WIN32_LEAN_AND_MEAN
	#include "windows.h" // needed for CorruptionDetectorAllocator
	#undef min
	#undef max
#endif

namespace memo
{
	#if MEMO_ENABLE_TEST

		// generate_rand_32
		uint32_t generate_rand_32( uint32_t & io_rand_seed )
		{
			const uint32_t even = 0xAAAAAAAA, odd = 0x55555555;
			io_rand_seed ^= even;
			io_rand_seed += odd;
			return io_rand_seed;
		}

		uint32_t generate_rand_32()
		{
			MEMO_ASSERT( RAND_MAX >= 0xFF );
			uint32_t result = rand() & 0xFF;
			result <<= 8;
			result |= rand() & 0xFF;
			result <<= 8;
			result |= rand() & 0xFF;
			result <<= 8;
			result |= rand() & 0xFF;
			return result;
		}

	#endif

	char * double_to_string( char * i_dest_buffer, size_t i_dest_buffer_size, double i_value, const char i_suffix )
	{	
		char * dest = i_dest_buffer + i_dest_buffer_size;
		
		double curr_value = i_value * 100.f;
		
		--dest;
		*dest = 0;

		if( i_suffix != 0 && dest >= i_dest_buffer )
		{
			--dest;
			*dest = i_suffix;
		}

		int position = 0;
		bool some_nonzero_digit = false;
		while( (curr_value >= 1.0 || position <= 3) && dest >= i_dest_buffer )
		{
			++position;

			if( position == 3 )
			{
				if( dest[0] == '0' && dest[1] == '0' )
				{
					dest[0] = i_suffix;
					dest[1] = 0;
				}
				else
				{
					--dest;
					*dest = '.';
				}
			}
			else
			{
				double next_val = curr_value / 10.0;
				double fdigit = next_val - floor( next_val ) + 0.001;
				curr_value = next_val;

				int digit = static_cast<int>( fdigit * 10.0 );
				if( digit < 0 )
					digit = 0;
				else if( digit > 9 )
					digit = 9;
				
				--dest;
				*dest = static_cast< char >( '0' + digit );
			}
		}
		
		if( dest < i_dest_buffer )
		{ 
			return nullptr;
		}
		else
		{
			return dest;
		}
	}

	static const size_t g_suffix_array_len = 5;
	static const char g_suffix_array[] = { 0, 'K', 'M', 'G', 'T' };

	char * mem_size_to_string( char * i_dest_buffer, size_t i_dest_buffer_size, size_t i_mem_size )
	{
		double value = static_cast<double>( i_mem_size );
		for( size_t index = 0; index < g_suffix_array_len - 1; index++ )
		{
			if( value < 800. )
				return double_to_string( i_dest_buffer, i_dest_buffer_size, value, g_suffix_array[ index ] );

			value *= (1. / 1024.);
		}

		return double_to_string( i_dest_buffer, i_dest_buffer_size, value, g_suffix_array[ g_suffix_array_len - 1 ] );
	}

	void output_mem_size( size_t i_mem_size )
	{
		const size_t buff_len = 1024;
		char buffer[ buff_len ];
		char * str = mem_size_to_string( buffer, buff_len, i_mem_size );
		if( str != nullptr )
			memo_externals::output_message( str );
		else
			memo_externals::output_message( "[Overflow]" );
	}

	void output_integer( size_t i_mem_size )
	{
		const size_t buff_len = 32;
		char buffer[ buff_len ];
		char * str = double_to_string( buffer, buff_len, static_cast<double>( i_mem_size ), 0 );
		if( str != nullptr )
			memo_externals::output_message( str );
		else
			memo_externals::output_message( "[Overflow]" );
	}
}

#if !(MEMO_ENABLE_INLINE)

	#ifdef MEMO_INLINE
		#error MEMO_INLINE is already defined
	#endif
	#define MEMO_INLINE

	#include "memo.inl"

	#undef MEMO_INLINE

#endif

#include "allocation_functions.cpp"
#include "address_functions.cpp"
#include "allocators\allocators.cpp"
#include "lifo\lifo.cpp"
#include "fifo\fifo.cpp"
#include "pool\pool.cpp"
#include "management\management.cpp"
#if MEMO_ENABLE_TEST
	#include "memo_test.cpp"
#endif
