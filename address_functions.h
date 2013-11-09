
namespace memo
{
	/** returns true if the parameter can be expressed as (1 << n), with n integer
		@param i_number integer number to test
		@return true if the parameter is an integer power of 2, false otherwise
	*/
	inline bool is_integer_power_of_2( size_t i_number )
	{
		return ( i_number & (i_number>>1) ) == 0; 
	}

	/** adds an offset to a pointer
		@param i_address source address
		@param i_offset number to add to the address
		@return i_address plus i_offset
	*/
	inline void * address_add( void * i_address, size_t i_offset )
	{
		const uintptr_t uint_pointer = reinterpret_cast<uintptr_t>( i_address );
		return reinterpret_cast< void * >( uint_pointer + i_offset );
	}
	inline const void * address_add( const void * i_address, size_t i_offset )
	{
		const uintptr_t uint_pointer = reinterpret_cast<uintptr_t>( i_address );
		return reinterpret_cast< void * >( uint_pointer + i_offset );
	}

	/** subtracts an offset from a pointer
		@param i_address source address
		@param i_offset number to subtract from the address
		@return i_address minus i_offset
	*/
	inline void * address_sub( void * i_address, size_t i_offset )
	{
		const uintptr_t uint_pointer = reinterpret_cast<uintptr_t>( i_address );
		MEMO_ASSERT( uint_pointer >= i_offset );
		return reinterpret_cast< void * >( uint_pointer - i_offset );
	}
	inline const void * address_sub( const void * i_address, size_t i_offset )
	{
		const uintptr_t uint_pointer = reinterpret_cast<uintptr_t>( i_address );
		MEMO_ASSERT( uint_pointer >= i_offset );
		return reinterpret_cast< void * >( uint_pointer - i_offset );
	}

	/** computes the unsigned difference between two pointers. The first must be above or equal to the second.
		@param i_end_address first address
		@param i_start_address second address
		@return i_end_address minus i_start_address
	*/
	inline uintptr_t address_diff( const void * i_end_address, const void * i_start_address )
	{
		MEMO_ASSERT( i_end_address >= i_start_address );

		const uintptr_t end_uint_pointer = reinterpret_cast<uintptr_t>( i_end_address );
		const uintptr_t start_uint_pointer = reinterpret_cast<uintptr_t>( i_start_address );
		
		return end_uint_pointer - start_uint_pointer;
	}

	/** tests whether a pointer is aligned
		@param i_address pointer to test
		@param i_alignment alignment required from the pointer. It must be an integer power of 2.
		@return true if the pointer is aligned, false otherwise
	*/
	inline bool is_aligned( const void * i_address, size_t i_alignment )
	{
		MEMO_ASSERT( i_alignment > 0 && is_integer_power_of_2( i_alignment ) );

		const uintptr_t uint_pointer = reinterpret_cast<uintptr_t>( i_address );
		return ( uint_pointer & ( i_alignment - 1 ) ) == 0;
	}

	/** returns the biggest aligned address lesser than or equal to a given address
		@param i_address address to be aligned
		@param i_alignment alignment required from the pointer. It must be an integer power of 2.
		@return the aligned address
	*/
	inline void * lower_align( void * i_address, size_t i_alignment )
	{
		MEMO_ASSERT( i_alignment > 0 && is_integer_power_of_2( i_alignment ) );

		const uintptr_t uint_pointer = reinterpret_cast<uintptr_t>( i_address );

		const size_t mask = i_alignment - 1;

		return reinterpret_cast< void * >( uint_pointer & ~mask );
	}
	inline const void * lower_align( const void * i_address, size_t i_alignment )
	{
		MEMO_ASSERT( i_alignment > 0 && is_integer_power_of_2( i_alignment ) );

		const uintptr_t uint_pointer = reinterpret_cast<uintptr_t>( i_address );

		const size_t mask = i_alignment - 1;

		return reinterpret_cast< void * >( uint_pointer & ~mask );
	}

	/** returns	the biggest address lesser than te first parameter, such that i_address + i_alignment_offset is aligned
		@param i_address address to be aligned
		@param i_alignment alignment required from the pointer. It must be an integer power of 2
		@param i_alignment_offset alignment offset
		@return the result address
	*/
	inline void * lower_align( void * i_address, size_t i_alignment, size_t i_alignment_offset )
	{
		void * address = address_add( i_address, i_alignment_offset );

		address = lower_align( address, i_alignment );

		address = address_sub( address, i_alignment_offset );
		
		return address;
	}
	inline const void * lower_align( const void * i_address, size_t i_alignment, size_t i_alignment_offset )
	{
		const void * address = address_add( i_address, i_alignment_offset );

		address = lower_align( address, i_alignment );

		address = address_sub( address, i_alignment_offset );
		
		return address;
	}

	/** returns the smallest aligned address greater than or equal to a given address
		@param i_address address to be aligned
		@param i_alignment alignment required from the pointer. It must be an integer power of 2.
		@return the aligned address
	*/
	inline void * upper_align( void * i_address, size_t i_alignment )
	{
		MEMO_ASSERT( i_alignment > 0 && is_integer_power_of_2( i_alignment ) );

		const uintptr_t uint_pointer = reinterpret_cast<uintptr_t>( i_address );

		const size_t mask = i_alignment - 1;

		return reinterpret_cast< void * >( ( uint_pointer + mask ) & ~mask );
	}
	inline const void * upper_align( const void * i_address, size_t i_alignment )
	{
		MEMO_ASSERT( i_alignment > 0 && is_integer_power_of_2( i_alignment ) );

		const uintptr_t uint_pointer = reinterpret_cast<uintptr_t>( i_address );

		const size_t mask = i_alignment - 1;

		return reinterpret_cast< void * >( ( uint_pointer + mask ) & ~mask );
	}

	/** returns	the smallest address greater than the first parameter, such that i_address + i_alignment_offset is aligned
		@param i_address address to be aligned
		@param i_alignment alignment required from the pointer. It must be an integer power of 2
		@param i_alignment_offset alignment offset
		@return the result address
	*/
	inline void * upper_align( void * i_address, size_t i_alignment, size_t i_alignment_offset )
	{
		void * address = address_add( i_address, i_alignment_offset );

		address = upper_align( address, i_alignment );

		address = address_sub( address, i_alignment_offset );
		
		return address;
	}
	inline const void * upper_align( const void * i_address, size_t i_alignment, size_t i_alignment_offset )
	{
		const void * address = address_add( i_address, i_alignment_offset );

		address = upper_align( address, i_alignment );

		address = address_sub( address, i_alignment_offset );
		
		return address;
	}
}
