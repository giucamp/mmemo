
#ifndef MEMO_LIFO_ALLOC_DEBUG
	#error MEMO_LIFO_ALLOC_DEBUG must be defined in memo_externals.h
#endif

namespace memo
{
	/**	\class LifoAllocator
		Class implementing LIFO-ordered allocation services.
		A LIFO allocator provides allocation\deallocation services with the constrain that only the memory block of top
		(BOT) can be reallocated or freed. The BOT is the last allocated or resized memory block. After it is freed,
		the previously allocated block is the new BOT.
		LifoAllocator is initialized with a memory buffer, which is used to allocate the memory blocks fo the user. If 
		the space remaining in the buffer is not enough to accomplish the alloc or realloc, this class just returns nullptr.
		LifoAllocator does not provide a deallocation callback, so you have to destroy manually any non-POD object. See also memo::ObjectLifoAllocator.
		This class is not thread safe.
	*/
	class LifoAllocator
	{
	public:


							/// allocation services ///

		/** default constructor. The memory buffer must be assigned before using the LifoAllocator (see set_buffer) */
		LifoAllocator();

		/** constructor that assigns soon the memory buffer */
		LifoAllocator( void * i_buffer_start_address, size_t i_buffer_length );

		/** destroys the allocator. All the allocations are freed. */
		~LifoAllocator();

		/** assigns the memory buffer.
		  @param i_buffer_start_address pointer to the first byte in the buffer
		  @param i_buffer_length number of bytes in the buffer 
		*/
		void set_buffer( void * i_buffer_start_address, size_t i_buffer_length );

		/** allocates a new memory block, respecting the requested alignment with an offset from the beginning of the block.
			If the allocation fails nullptr is returned. If the requested size is zero the return value is a non-null address.
			The content of the newly allocated block is undefined. 
		  @param i_size size of the block in bytes
		  @param i_alignment alignment requested for the block. It must be an integer power of 2
		  @param i_alignment_offset offset from beginning of the block of the address that respects the alignment
		  @return the address of the first byte in the block, or nullptr if the allocation fails
		*/
		void * alloc( size_t i_size, size_t i_alignment, size_t i_alignment_offset );

		/** changes the size of the memory block on top of the stack.
			The content of the memory block is preserved up to the lesser of the new size and the old size, while the content 
			of the newly allocated portion is undefined. 
			The alignment and the offset of the alignment of an existing memory block cannot be changed by realloc.
			If the reallocation fails false is returned, and the memory block is left unchanged. 
		  @param i_address address of the memory block to reallocate. This parameter cannot be nullptr.
		  @param i_new_size new size of the block in bytes
		  @return true if the reallocations succeeds, false otherwise 
		*/
		bool realloc( void * i_address, size_t i_new_size );

		/** deallocates a memory block allocated by alloc or realloc.
		  @param i_address address of the memory block to free. It must be the block on top. It cannot be nullptr.
		  */
		void free( void * i_address );

		/** resets the allocator, freeing all the allocated memory blocks. */
		void free_all();



					/// getters ///

		/** retrieves the beginning of the buffer used to perform allocations. Writing this buffer causes memory corruption. 
		  @return pointer to the beginning if the buffer */
		const void * get_buffer_start() const;

		/** retrieves the size of the buffer.
		  @return size of the buffer in bytes  */
		size_t get_buffer_size() const;

		/** retrieves the size of the free portion of the buffer, that is the remaining space.
		  @return free space in bytes  */
		size_t get_free_space() const;

		/** retrieves the size of the used portion of the buffer, that is the allocated space.
		  @return used space in bytes */
		size_t get_used_space() const;

		#if MEMO_LIFO_ALLOC_DEBUG
			
			/** retrieves the numbler of blocks currently allocated.
				@return number of blocks. */
			size_t dbg_get_curr_block_count() const;

			/** retrieves the last non-freed allocated block, that is the BOT.
				@return address of the last allocated block. */
			void * dbg_get_block_on_top() const;

		#endif


					/// tester ///

		#if MEMO_ENABLE_TEST
			
			/** encapsulates a test session to discover bugs in LifoAllocator */
			class TestSession
			{
			public:

				/** construct a test session with a lifo allocator */
				TestSession( size_t i_buffer_size );

				void fill_and_empty_test();

				~TestSession();

			private: // internal services

				/** tries to make an allocation */
				bool allocate();

				/** tries to deallocate the BOT */
				bool reallocate();

				/** frees the BOT if it exists */
				bool free();

				static void check_val( const void * i_address, size_t i_size, uint8_t i_value );

			private: // data members

				struct Allocation
				{
					void * m_block;
					size_t m_block_size;
				};

				std_vector< Allocation >::type m_allocations;
				LifoAllocator * m_lifo_allocator;
				void * m_buffer;
			};

		#endif // #if MEMO_ENABLE_TEST

	private: // not implemented
		LifoAllocator( const LifoAllocator & );
		LifoAllocator & operator = ( const LifoAllocator & );

	private: // data members
		void * m_curr_address, * m_start_address, * m_end_address;
		#if MEMO_LIFO_ALLOC_DEBUG
			std_vector< void* >::type m_dbg_allocations; /** debug address stack used to check the LIFO consistency */
			static const uint8_t s_dbg_initialized_mem = 0x17;
			static const uint8_t s_dbg_allocated_mem = 0xAA;
			static const uint8_t s_dbg_freed_mem = 0xFD;
		#endif
	};

} // namespace memo
