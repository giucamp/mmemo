
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

	private:

		struct _Header
		{
			size_t m_next_header_offset;
			size_t m_user_block_offset;
		};

	private: // data members
		void * m_buffer_start, * m_buffer_end;
		void * m_first_block; // oldest allocated block
		void * m_next_block; // starting position for the next block to allocate
	};

} // namespace memo
