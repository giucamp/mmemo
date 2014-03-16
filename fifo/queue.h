
namespace memo
{
	/**	\class Queue
		\brief Class implementing LIFO-ordered allocation services.
	*/
	class Queue
	{
	public:

		/** default constructor. The memory buffer must be assigned before using the queue (see set_buffer) */
		Queue();

		/** constructor that assigns soon the memory buffer */
		Queue( void * i_buffer_start_address, size_t i_buffer_length );

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

		/** returns the oldest block in the buffer (the front of the queue).
		  @return the address of the first byte in the oldest block, or nullptr if the queue is empty
		*/
		void * get_first_block();

		/** deallocates a memory block allocated by alloc or realloc.
		  @param i_address address of the memory block to free. It must be the first block, returned by Queue::get_first_block. It cannot be nullptr.
		  */
		void free_first( void * i_first_block );

		/** frees all the blocks in the queue */
		void clear();

		/** checks whether the queue is empty
		 @return true if the queue is empty, false otherwise
		*/
		bool is_empty() const;

		class Iterator
		{
		public:

			Iterator();

			Iterator( const Queue & i_queue )		{ start_iteration( i_queue ); }

			void start_iteration( const Queue & i_queue );

			bool is_over() const;

			void operator ++ ( int );

			Iterator operator ++ ()					{ (*this)++;  return *this; }

			void * curr_block() const;

		private:
			const Queue * m_queue;
			void * m_curr_header;
		};


							/// tester ///

		#if MEMO_ENABLE_TEST
			
			/** encapsulates a test session to discover bugs in DataStack */
			class TestSession
			{
			public:

				/** construct a test session with a fifo allocator */
				TestSession( size_t i_buffer_size );

				void fill_and_empty_test();

				~TestSession();

			private: // internal services

				bool allocate();

				bool free();

				static void check_val( const void * i_address, size_t i_size, uint8_t i_value );

				void check_consistency();

			private: // data members

				struct Allocation
				{
					void * m_block;
					size_t m_block_size;
				};

				std_deque< Allocation >::type m_allocations;
				Queue * m_fifo_allocator;
				void * m_buffer;
			};

		#endif // #if MEMO_ENABLE_TEST

	private:

		struct _Header
		{
			size_t m_next_header_offset;
			size_t m_user_block_offset;
		};

	private: // data members
		void * m_buffer_start, * m_buffer_end;
		void * m_start; // oldest allocated block
		void * m_end; // starting position for the next block to allocate
		size_t m_allocation_count;
	};

} // namespace memo
