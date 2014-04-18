

namespace memo
{
	/** RAII-style class that wraps a single lifo allocation (see memo::lifo_alloc) */
	class AutoLifo
	{
	public:

		/** construct an object with no buffer allocated */
		AutoLifo();
		
		/** allocates memory with memo::lifo_alloc. If a block was already allocated, 
			it is freed before the new allocation. */
		void * alloc( size_t i_size, size_t i_alignment );

		/** frees the allocated block, if any */
		void free();

		/** frees the allocated block, if any */
		~AutoLifo();

		/** returns whether a buffer has been allocated */
		bool is_allocated() const;

		/** returns the allocated buffer, or nullptr if no buffer has been allocated */
		void * buffer() const;

		/** returns the size of the allocated buffer. If no buffer has been allocated, the return value is undefined */
		size_t size() const;

		/** returns the alignment of the allocated buffer. If no buffer has been allocated, the return value is undefined */
		size_t alignment() const;

	private:
		AutoLifo( const AutoLifo & ); // not implemented
		AutoLifo & operator = ( const AutoLifo & ); // not implemented

	private:
		ObjectStack & m_thread_stack;
		void * m_buffer;
		size_t m_size, m_alignment;
	};

} // namespace memo
