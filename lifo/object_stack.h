
namespace memo
{
	/**	\class ObjectStack
		\brief Class implementing LIFO-ordered allocation services.
		A LIFO allocator provides allocation\deallocation services with the constrain that only the memory block of top
		(BOT) can be reallocated or freed. The BOT is the last allocated or resized memory block. After it is freed,
		the previously allocated block is the new BOT.
		ObjectStack is initialized with an allocator, which is used to allocate pages of memory.
		This class is not thread safe.
	*/
	class ObjectStack
	{
	public:

		/** Config structure for ObjectStack */
		struct Config : public IAllocator::Config
		{
		public:
			
			Config(); /**< Set the default values. m_external_allocator is set to nullptr */
			~Config(); /**< Deletes the config pointed by m_external_allocator, if not null */

			IAllocator::Config * m_external_allocator; /**< Pointer to the config of the target allocator. It has the ownership of the pointed object. */
		};

		ObjectStack();

		bool init( IAllocator & i_target_allocator, size_t i_first_page_size, size_t i_other_page_size );
		
		bool is_initialized() const;

		void uninit();

		/** destroys the allocator. All the allocations are freed. */
		~ObjectStack();

		/** allocates a new memory block, respecting the requested alignment with an offset from the beginning of the block.
			If the allocation fails nullptr is returned. If the requested size is zero the return value is a non-null address.
			The content of the newly allocated block is undefined. 
		  @param i_size size of the block in bytes
		  @param i_alignment alignment requested for the block. It must be an integer power of 2
		  @param i_alignment_offset offset from beginning of the block of the address that respects the alignment
		  @param i_deallocation_callback function to call when the allocation is freed. It may be used to call the destructor of the object being allocated. It can be nullptr.
		  @return the address of the first byte in the block, or nullptr if the allocation fails
		*/
		void * alloc( size_t i_size, size_t i_alignment, size_t i_alignment_offset, DeallocationCallback i_deallocation_callback );
		
		/** deallocates a memory block allocated by alloc
		  @param i_address address of the memory block to free. It must be the block on top. It cannot be nullptr.
		  */
		void free( void * i_address ); 

		/** resets the allocator, freeing all the allocated memory blocks. */
		void free_all();



					/// getters ///

		struct StateInfo
		{
			#if MEMO_LIFO_ALLOC_DEBUG
				size_t m_dbg_block_count;
			#endif
			size_t m_total_used_space;
			size_t m_page_count;
			size_t m_pages_total_space;

			StateInfo();

			void reset();
		};

		void get_state_info( StateInfo & o_info ) const;



					/// tester ///

		#if 0 && MEMO_ENABLE_TEST
			
			/** encapsulates a test session to discover bugs in LifoAllocatorTemp */
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
				ObjectStack * m_stack;
			};

		#endif

	private: // not implemented
		ObjectStack( const ObjectStack & );
		ObjectStack & operator = ( const ObjectStack & );

	private: // internal services

		struct PageHeader
		{
			ObjectLifoAllocator m_lifo_allocator;
			PageHeader * m_prev_page;
			size_t m_size;
		};

		bool new_page( size_t i_min_size );

		void destroy_page( PageHeader * ); 

	private: // data members		
		PageHeader * m_last_page;
		IAllocator * m_target_allocator;
		size_t m_page_size;
	};

} // namespace memo
