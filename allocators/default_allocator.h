
namespace memo
{
	/**	Retrieves the global instance of the default allocator. This function does not check whether the allocator 
		has already been created, so calling it during the initialization of global variables is unsafe. If
		the function safe_get_default_allocator has been called at least once, safe_get_default_allocator
		can be used safely. */
	DefaultAllocator & get_default_allocator();

	/**	Retrieves the global instance of the default allocator. If the global instance has not been created, this 
		function creates it, so calling it during the initialization of global variables is safe. */
	DefaultAllocator & safe_get_default_allocator();

	/**	\class DefaultAllocator
		\brief Implements IAllocator using the C dynamic memory allocation (malloc, realloc and free)
		This implementation wraps malloc, realloc and free. These functions don't not handle custom alignment
		natively, so extra space equal to the alignment is requested to malloc. A native support for alignment 
		would avoid this waste of space. */
	class DefaultAllocator : public IAllocator
	{
	public:

		/** Static function returning the name of the allocator class, used to register the type.
		   This name can be used to instantiate this allocator in the configuration file. */
		static const char * type_name() { return "default_allocator"; }

		/** Config structure for DefaultAllocator. No configuration parameters are defined, so this
			struct serves only to create the allocator. */
		struct Config : public IAllocator::Config
		{
		protected:

			/** This function should try to recognize the current property of the input stream, and eventually read
				the value. Currently DefaultAllocator does not support any parameter */
			virtual bool try_recognize_property( serialization::IConfigReader & /*i_config_reader*/ ) 
				{ return false; }

			/** configures and returns a new allocator, eventually creating it using MEMO_NEW if i_new_allocator is nullptr */
			virtual IAllocator * configure_allocator( IAllocator * i_new_allocator ) const;
		};

							///// aligned allocations /////

		/** \brief allocates an aligned memory block. Implements IAllocator::alloc.
			::malloc is used to allocate the memory block. In order to respect the alignment, the actual memory 
			block is bigger than i_size.
		  @param i_size size of the block in bytes
		  @param i_alignment alignment requested for the block. It must be an integer power of 2
		  @param i_alignment_offset offset from beginning of the block of the address that respects the alignment
		  @return the address of the first byte in the block, or nullptr if the allocation fails
		*/
		void * alloc( size_t i_size, size_t i_alignment, size_t i_alignment_offset )
			{ return s_alloc( i_size, i_alignment, i_alignment_offset ); }


		/** \brief changes the size of the memory block allocated by alloc, possibly moving it in a new location.
			Implements IAllocator::realloc. ::realloc is used to resize the memory block.
		  @param i_address address of the memory block to reallocate. This parameter cannot be nullptr.
		  @param i_new_size new size of the block in bytes
		  @param i_alignment alignment requested for the block. It must be an integer power of 2
		  @param i_alignment_offset offset from the address that respects the alignment
		  @return the new address of the first byte in the block, or nullptr if the reallocation fails
		*/
		void * realloc( void * i_address, size_t i_new_size, size_t i_alignment, size_t i_alignment_offset )
			{ return s_realloc( i_address, i_new_size, i_alignment, i_alignment_offset ); }

		/** \brief deallocates a memory block allocated by alloc or realloc. Implements IAllocator::free.
			::free is used to deallocate the memory block.
		  @param i_address address of the memory block to free. It cannot be nullptr.
		  */
		void free( void * i_address )
			{ s_free( i_address ); }

		/** \brief Implements IAllocator::dbg_check. Some basic checks is performed on the memory block.
		  @param i_address address of the memory block to check
		*/
		void dbg_check( void * i_address )
			{ s_dbg_check( i_address ); }




							///// unaligned allocations /////

		/** \brief allocates a new memory block. Implements IAllocator::unaligned_alloc.
		  ::malloc is used to allocate the memory block
		  @param i_size size of the block in bytes
		  @return the address of the first byte in the block, or nullptr if the allocation fails
		*/
		void * unaligned_alloc( size_t i_size )
			{ return s_unaligned_alloc( i_size ); }

		/** \brief changes the size of the memory block allocated by unaligned_alloc. Implements IAllocator::unaligned_realloc.
		  ::realloc is used to resize the memory block.
		  @param i_address address of the memory block to reallocate. This parameter cannot be nullptr.
		  @param i_new_size new size of the block in bytes
		  @return the new address of the first byte in the block, or nullptr if the reallocation fails
		*/
		void * unaligned_realloc( void * i_address, size_t i_new_size )
			{ return s_unaligned_realloc( i_address, i_new_size ); }

		/** \brief deallocates a memory block allocated by unaligned_alloc or unaligned_realloc. Implements IAllocator::unaligned_free.
		  ::free is used to deallocate the memory block.
		  @param i_address address of the memory block to free. It cannot be nullptr.
		  */
		void unaligned_free( void * i_address )
			{ s_unaligned_free( i_address ); }

		/** \brief This function performs no action. Implements IAllocator::unaligned_dbg_check.
		  @param i_address address of the memory block to check
		*/
		void unaligned_dbg_check( void * i_address )
			{ s_unaligned_dbg_check( i_address ); }

		
		/** Writes out in a human readable way the state of the allocator */
		void dump_state( StateWriter & i_state_writer );


					///// static allocation functions /////

		static void * s_alloc( size_t i_size, size_t i_alignment, size_t i_alignment_offset );
		static void * s_realloc( void * i_address, size_t i_new_size, size_t i_alignment, size_t i_alignment_offset );
		static void s_free( void * i_address );
		static void s_dbg_check( void * i_address );

		static void * s_unaligned_alloc( size_t i_size );
		static void * s_unaligned_realloc( void * i_address, size_t i_new_size );
		static void s_unaligned_free( void * i_address );
		static void s_unaligned_dbg_check( void * i_address );

	private:
		struct AlignmentHeader;
	};

} // namespace memo
