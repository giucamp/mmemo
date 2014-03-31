
namespace memo
{
	/**	\class Queue
		Class implementing FIFO-ordered allocation services. The FIFO constraint requires that only the oldest allocated block 
		can be freed. Queue allows to get the address of the oldest allocated block, so that it can be consumed before 
		being freed.
		This class is not thread safe.
	*/
	class Queue
	{
	public:

		/** default constructor. The memory buffer must be assigned before using the queue (see set_buffer) */
		Queue();

		~Queue();

		bool init( IAllocator & i_target_allocator, size_t i_first_page_size, size_t i_other_page_size );
		
		bool is_initialized() const;

		void uninit();

		/** allocates a new memory block, respecting the requested alignment with an offset from the beginning of the block.
			If the allocation fails nullptr is returned. If the requested size is zero the return value is a non-null address.
			The content of the newly allocated block is undefined.
		  @param i_size size of the block in bytes
		  @param i_alignment alignment requested for the block. It must be an integer power of 2
		  @param i_alignment_offset offset from beginning of the block of the address that respects the alignment
		  @return the address of the first byte in the block, or nullptr if the allocation fails
		*/
		void * alloc( size_t i_size, size_t i_alignment, size_t i_alignment_offset );

		/** allocates a new memory block for an instance of the type TYPE. This method just allocates: no constructor is called. 
			If the allocation fails nullptr is returned. If the requested size is zero the return value is a non-null address.
			The content of the newly allocated block is undefined.
			You can use this method with  the in-place new:
			\code{.cpp}
				MyType * my_object = new ( queue.typed_alloc<MyType>() ) MyType( arg1, arg2 ... );
			\endcode
		  @return the address of the first byte in the block, or nullptr if the allocation fails
		*/
		template < typename TYPE > void * typed_alloc() 
				{ return alloc( sizeof(TYPE), MEMO_ALIGNMENT_OF(TYPE), 0 ); }

		/** allocates a new memory block for an array of the type TYPE. This method just allocates: no constructor is called. 
			If the allocation fails nullptr is returned. If the requested size is zero the return value is a non-null address.
			The content of the newly allocated block is undefined.
			@param i_count length of the allocated array
		  @return the address of the first byte in the block, or nullptr if the allocation fails
		*/
		template < typename TYPE > TYPE * typed_array_alloc( size_t i_count ) 
				{ return static_cast< TYPE * >( alloc( i_count * sizeof(TYPE), MEMO_ALIGNMENT_OF(TYPE), 0 ) ); }
		
		/** returns the oldest block in the buffer (the front of the queue).
		  @return the address of the first byte in the oldest block, or nullptr if the queue is empty
		*/
		void * get_first_block();

		/** deallocates a memory block allocated by alloc or realloc.
		  @param i_address address of the memory block to free. It must be the block on top. It cannot be nullptr.
		  */
		void free_first( void * i_first_block );
				
		/** frees all the blocks in the queue */
		void clear();


					/// tester ///

		#if MEMO_ENABLE_TEST
			
			/** encapsulates a test session to discover bugs in FifoAllocator */
			class TestSession
			{
			public:

				TestSession();

				~TestSession();

				void fill_and_empty_test( size_t i_iterations );
				
			private: // internal services

				void allocate();

				/** frees the BOT if it exists */
				bool free();

				static void check_val( const void * i_address, size_t i_size, uint8_t i_value );

			private: // data members
				Queue * m_fifo_allocator;
				std_deque< std_vector< int >::type >::type m_test_queue;
			};

		#endif // #if MEMO_ENABLE_TEST

	private: // not implemented
		Queue( const Queue & );
		Queue & operator = ( const Queue & );

	private: // internal services

		struct PageHeader
		{
			FifoAllocator m_fifo_allocator;
			PageHeader * m_next_page;
		};

		PageHeader * create_page( size_t i_min_size );

		void remove_page( PageHeader * );

		void destroy_page( PageHeader * ); 

		static const size_t s_min_page_size;

	private: // data members		
		PageHeader * m_first_page, * m_last_page; // linked list of all pages
		PageHeader * m_put_page, * m_peek_page; // head and tail of the queue
		IAllocator * m_target_allocator;
		size_t m_page_size;
	};

} // namespace memo
