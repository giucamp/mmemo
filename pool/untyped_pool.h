
namespace memo
{
	class UntypedPool
	{
	public:

		struct Config
		{
			size_t m_element_size;
			size_t m_element_alignment;
			size_t m_element_count;
		};

		UntypedPool();

		bool init( const Config & i_config );

		void * alloc();

		void * alloc_slot();

		void free( void * i_element );

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