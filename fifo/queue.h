
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
	private:
		struct PageHeader
		{
			FifoAllocator m_fifo_allocator;
			PageHeader * m_next_page;
		};

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
				void * alloc = queue.typed_alloc<MyType>();
				MyType * my_object = alloc != nullptr ? new( alloc ) MyType( arg1, arg2 ... ) : nullptr;
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


		/** This class enumerates, from the oldest to the newest, all the living allocation in a FifoAllocator. 
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

			/** Construct an iterator and assigns to it a Queue. */
			Iterator( const Queue & i_queue )		{ start_iteration( i_queue ); }

			/** Starts iterating a Queue, moving to its oldest allocation */
			void start_iteration( const Queue & i_queue );

			/** Returns whether the iteration of the LifoALlocator is finished. When this method returns true,
					the iteration cannot longer be used, unless start_iteration is called. */
			bool is_over() const;

			/** Moves to the next allocation. This method cannot be called if is_over() returns true. */
			void operator ++ ( int );

			/** Moves to the next allocation. This method cannot be called if is_over() returns true. */
			Iterator & operator ++ ()					{ (*this)++; return *this; }

			/** Returns the adders of the current memory block. This method cannot be called if is_over() returns true. */
			void * curr_block() const;

		private:
			FifoAllocator::Iterator m_inner_iterator;
			PageHeader * m_curr_page;
			const Queue * m_queue;
		};



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
