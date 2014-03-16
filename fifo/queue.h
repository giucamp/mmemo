
namespace memo
{
	/**	\class FifoAllocator
		Class implementing LIFO-ordered allocation services. The user assign a memory buffer to FifoAllocator, and 
		it manages it as a circular buffer, allowing allocation of variable size and alignment. The FIFO constraint
		requires that only the oldest allocated block can be freed. FifoAllocator allows to get the address of the
		oldest allocated block, so that it can be consumed before being freed.
		If an allocation can be committed because there is not enough remaining space in the buffer, FifoAllocator
		returns nullptr. If you need a FIFO memory manager that can grow, you can use memo::Queue.
		FifoAllocator provide an iterator inner-class, that can enumerate all the living blocks in a FifoAllocator.
		This class is not thread safe.
	*/
	class FifoAllocator
	{
	public:

		/** default constructor. The memory buffer must be assigned before using the queue (see set_buffer) */
		FifoAllocator();

		/** constructor that assigns soon the memory buffer */
		FifoAllocator( void * i_buffer_start_address, size_t i_buffer_length );

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
		  @param i_address address of the memory block to free. It must be the first block, returned by FifoAllocator::get_first_block. It cannot be nullptr.
		  */
		void free_first( void * i_first_block );

		/** frees all the blocks in the queue */
		void clear();

		/** checks whether the queue is empty
		 @return true if the queue is empty, false otherwise
		*/
		bool is_empty() const;

		/** This class enumerates, from the oldest to the newest, all the living allocation in a LifoAllocator. 
			This is an example of how this class may be used:
			\code{.cpp}
			for( Iterator it( fifo_allocator ); !it.is_over(); it++ )
			{
				// ...
			}
			\endcode
			*/
		class Iterator
		{
		public:

			/** Construct an uninitialized iterator. Call start_iteration before using it. */
			Iterator();

			/** Construct an iterator and assigns to it a LifoAllocator. */
			Iterator( const FifoAllocator & i_lifo_allocator )		{ start_iteration( i_lifo_allocator ); }

			/** Starts iterating a FifoAllocator, moving to its oldest allocation */
			void start_iteration( const FifoAllocator & i_lifo_allocator );

			/** Returns whether the iteration of the LifoALlocator is finished. When this method returns true,
					the iteration cannot longer be used, unless start_iteration is called. */
			bool is_over() const;

			/** Moves to the next allocation. This method cannot be called if is_over() returns true. */
			void operator ++ ( int );

			/** Moves to the next allocation. This method cannot be called if is_over() returns true. */
			Iterator & operator ++ ()					{ (*this)++;  return *this; }

			/** Returns the adders of the current memory block. This method cannot be called if is_over() returns true. */
			void * curr_block() const;

		private:
			const FifoAllocator * m_queue;
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
				FifoAllocator * m_fifo_allocator;
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
	};

} // namespace memo
