
namespace memo
{
	/**	\class DebugAllocator
		This is a decorator allocator that can adds consistency check to another allocator.
		DebugAllocator adds heading and tailing no man's land, that is some extra memory before and after the allocated memory block, filled with 
		pseudo-random values. An access to this memory indicates a buffer overflow or an index out of bounds, as this memory is outside the allocated 
		block. DebugAllocator is able to detect whether this memory has been written. Any function taking the address of a memory block as parameter 
		do this check. The size of the no man's lands are specified in the config structure.
		The newly allocated memory and the memory being freed can filled with special values (depending on the parameters of the config structure). This 
		can help to discover uninitialized memory usage and block used after deallocation.

		The following parameters are supported in the configuration file:
			- new_memory_fill: "none", "NaNs", "pseudo_random", "badf00d". See DebugAllocator::Config::m_new_memory_fill_mode
			- deleted_memory_fill: "none", "NaNs", "pseudo_random", "badf00d". See DebugAllocator::Config::m_deleted_memory_fill_mode
			- nomansland_words: integer number
			- target: inherited from DecoratorAllocator, is the name of the type of target allocator (for example "default_allocator", 
			  "debug_allocator", "stat_allocator").
	*/
	class DebugAllocator : public DecoratorAllocator
	{
	public:

		/** Static function returning the name of the allocator class, used to register the type.
		   This name can be used to instantiate this allocator in the configuration file. */
		static const char * type_name() { return "debug_allocator"; }


								///// configuration /////
		
		/** Specifies whether and how to fill the memory */
		enum FillMode
		{
			eNone, /**< do not fill the memory */
			eNaNs, /**< fill the memory with quiet not-a-number float values. If floating point exception are enabled, 
						this can help the discovery of uninitialized float variables. */
			ePseudoRandom, /**< fill the memory with pseudo-random values */
			eBadF00d, /**< fill the memory with the 32-bit integer 0xBADF00D */
		};

		/** Config structure for DebugAllocator */
		struct Config : public DecoratorAllocator::Config
		{
		public:

			FillMode m_new_memory_fill_mode; /**< specifies whether and how the newly allocated memory is filled. */
			FillMode m_deleted_memory_fill_mode; /**< specifies whether and how the memory being deallocated is filled. */
			size_t m_nomansland_words; /**< specifies the size of the no man's lands, in words. A word is as wide as a pointer.
				The no man's land words are distributed between the heading and the tailing lands, with the heading having
				the extra word in case of odd m_nomansland_words. So, if m_nomansland_words = 5, you have 3 words in the 
				heading, and 2 in the tailing. 
				The heading no man land is used to store the size of the allocation. The value zero is not recommended for 
				m_nomansland_words, as it causes the parameter m_deleted_memory_fill_mode to not work properly. */

			Config()
				{ set_defaults(); }

			void set_defaults()
			{
				m_new_memory_fill_mode = eNaNs;
				m_deleted_memory_fill_mode = eBadF00d;
				m_nomansland_words = 2;
			}

		protected:

			/** configures and returns a new allocator, eventually creating it using MEMO_NEW if i_new_allocator is nullptr */
			virtual IAllocator * configure_allocator( IAllocator * i_new_allocator ) const;

			/** Tries to recognize the current property from the stream, and eventually reads its value.
			  @param i_config_reader the source stream
			  @return true if the property has been recognized, false otherwise */
			bool try_recognize_property( serialization::IConfigReader & i_config_reader );
		};




								///// aligned allocations /////

		/** allocates an aligned memory block. Implements IAllocator::alloc.
			The dest allocator is used to perform the operation. See DecoratorAllocator.
		  @param i_size size of the block in bytes
		  @param i_alignment alignment requested for the block. It must be an integer power of 2
		  @param i_alignment_offset offset from beginning of the block of the address that respects the alignment
		  @return the address of the first byte in the block, or nullptr if the allocation fails
		*/
		void * alloc( size_t i_size, size_t i_alignment, size_t i_alignment_offset );


		/** changes the size of the memory block allocated by alloc, possibly moving it in a new location. Implements IAllocator::realloc.
			The dest allocator is used to perform the operation. See DecoratorAllocator.
		  @param i_address address of the memory block to reallocate. This parameter cannot be nullptr.
		  @param i_new_size new size of the block in bytes
		  @param i_alignment alignment requested for the block. It must be an integer power of 2
		  @param i_alignment_offset offset from the address that respects the alignment
		  @return the new address of the first byte in the block, or nullptr if the reallocation fails */
		void * realloc( void * i_address, size_t i_new_size, size_t i_alignment, size_t i_alignment_offset );

		/** deallocates a memory block allocated by alloc or realloc. Implements IAllocator::free.
			The dest allocator is used to perform the operation. See DecoratorAllocator.
		  @param i_address address of the memory block to free. It cannot be nullptr. */
		void free( void * i_address );

		/** Implements IAllocator::dbg_check. All the consistency checks are performed on the memory block.
		  @param i_address address of the memory block to check */
		void dbg_check( void * i_address );




							///// unaligned allocations /////

		/** allocates a new memory block. Implements IAllocator::unaligned_alloc.
		  The dest allocator is used to perform the operation. See DecoratorAllocator.
		  @param i_size size of the block in bytes
		  @return the address of the first byte in the block, or nullptr if the allocation fails */
		void * unaligned_alloc( size_t i_size );

		/** changes the size of the memory block allocated by unaligned_alloc. Implements IAllocator::unaligned_realloc.
		  The dest allocator is used to perform the operation. See DecoratorAllocator.
		  @param i_address address of the memory block to reallocate. This parameter cannot be nullptr.
		  @param i_new_size new size of the block in bytes
		  @return the new address of the first byte in the block, or nullptr if the reallocation fails */
		void * unaligned_realloc( void * i_address, size_t i_new_size );

		/** deallocates a memory block allocated by unaligned_alloc or unaligned_realloc. Implements IAllocator::unaligned_free.
		  The dest allocator is used to perform the operation. See DecoratorAllocator.
		  @param i_address address of the memory block to free. It cannot be nullptr. */
		void unaligned_free( void * i_address );

		/** This function performs some integrity check on the block. Implements IAllocator::unaligned_dbg_check.
		  The dest allocator is used to perform the operation. See DecoratorAllocator.
		  @param i_address address of the memory block to check */
		void unaligned_dbg_check( void * i_address );

		/** Tries to retrieve the size of a memory block. The block may be allocated or reallocated with any function of DebugAllocator.
		  The retrieved size, if any, is the exact size requested when the block was allocated
		  @param i_address address of the memory block to check
		  @param o_size pointer to a size_t that can assigned with the size
		  @return true if the size has been assigned to o_size */
		bool get_block_size( void * i_address, size_t * o_size ) const;

		/** tries to recognize the specified string as a member of FillMode 
		  @return true if a member of FillMode is recognized, false otherwise */
		static bool string_to_fill_mode( const char * i_string, FillMode * o_fill_mode );

		/** Writes out in a human readable way the state of the allocator */
		void dump_state( StateWriter & i_state_writer );

	protected:

		DebugAllocator();

	private: // data members
		FillMode m_new_memory_fill_mode, m_deleted_memory_fill_mode;
		size_t m_heading_nomansland_size, m_tailing_nomansland_size;
		
	private: // internal services

		/** Assigns the configuration struct */
		void set_config( const Config & i_config );
		
		static void * _invert_address( void * i_address );

		size_t _do_check( void * i_address ) const; // returns the size of the block
		
		static void _fill_memory( FillMode i_mode, void * i_start_address, size_t i_size );
		static void _check_memory( FillMode i_mode, const void * i_start_address, size_t i_size );

		static void _set_nomansland( void * i_start_address, size_t i_size, void * i_parameter );
		static void _check_nomansland( const void * i_start_address, size_t i_size, void * i_parameter );
	};

} // namespace memo
