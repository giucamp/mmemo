
namespace memo
{
	/**	\class UntypedPool
		Class providing efficient fixed size allocation services. When initialized, the pool allocates a buffer with the default allocator.
		This buffer is large enough to contain the number of elements specified in the configuration (see UntypedPool::init).
		The pool handle its free slots as a linked list. So both allocations and deallocation are constant time and very fast.
		There is not space overhead, nor any fragmentation. The only drawback is that the pool must be initialized with a size.
		When the pool is full, this class can allocate transparently using the default allocator.
		This class is not thread safe.
	*/
	class UntypedPool
	{
	public:

		struct Config
		{
			Config()
				: m_element_size( 0 ), m_element_alignment( 0 ), m_element_count( 0 ) { }

			Config( size_t i_element_size, size_t i_element_alignment, size_t i_element_count ) 
				: m_element_size( i_element_size ), m_element_alignment( i_element_alignment ), m_element_count( i_element_count ) { }

			size_t m_element_size; /**< size of an allocable element */
			size_t m_element_alignment; /**< alignment of an allocable element */
			size_t m_element_count; /**< number of elements that  */
		};

		/** Constructs an unitialized pool. Call UntypedPool::init before using any other method. */
		UntypedPool();

		/** Destroys the pool, uninitializing it if necessary. The pool can be destroyed only when there are no living blocks 
			allocated by the it. */
		~UntypedPool()			{ uninit(); }

		/** Allocates a buffer with the default allocator, and formats it to be used for fixed size allocations.
			@params i_config configuration of the pool
			@return true if the buffer was successfully allocated and formatted, false otherwise */
		bool init( const Config & i_config );

		/** Deallocates the buffer used by the pool. This method should be called only when there are no living blocks 
			allocated by the pool. It's legal to call this method before init or twice. */
		void uninit();

		/** Allocates a block of memory. The size and the alignment of the block are those specified in the configuration of the pool.
			If there is not a free sot in the pool, this method allocates using the default allocator.
			@return address of the newly allocated block, or nullptr if both the pool and the default allocator could not allocate the block. */
		void * alloc();

		/** Allocates a block of memory. The size and the alignment of the block are those specified in the configuration of the pool.
			Unlike UntypedPool::alloc, this method allocates only in the pool.
			@return address of the newly allocated block, or nullptr if the pool could not allocate the block. */
		void * alloc_slot();

		/** Frees a block of memory, allocated with alloc or alloc_slot. If the block is outside the pool, this method uses the default allocator
			to free the block.
			@param i_element address of the block to free. */
		void free( void * i_element );

		/** Frees a block of memory, allocated with alloc_slot. Unlike UntypedPool::free, this method can free only blocks allocated in the pool.
			Use free_slot only if the block was allocated with alloc_slot.
			@param i_element address of the block to free. */
		void free_slot( void * i_element );

	private:

		struct FreeSlot
		{
			FreeSlot * m_next;
		};

		void format_free_space();

	private:
		Config m_config;
		FreeSlot * m_buffer_start, * m_buffer_end, * m_first_free;
	};

} // namespace memo
