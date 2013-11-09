
namespace memo
{
	#if MEMO_ENABLE_TEST

		/** \class AllocatorTester
			This class does a test session on an allocator, doing aligned and unaligned allocations and deallocations,
				writing magic numbers on the allocated blocks and verifying their integrity */
		class AllocatorTester
		{
		public:

			AllocatorTester( IAllocator & i_allocator );
		
			void do_test_session( size_t i_allocation_count );
			
			bool do_alloc();

			bool do_realloc();

			bool do_free();

		private:

			IAllocator & m_allocator;
			size_t m_max_allocation_size;
		
			struct Allocation
			{
				void * m_block;
				size_t m_size, m_alignment, m_offset;

				void _fill_mem( void * i_start_address, size_t i_size ) const;
				void _check_mem( const void * i_start_address, size_t i_size ) const;
			};

			std_vector< Allocation >::type m_allocations;

			size_t m_allocated_memory;
			size_t m_max_allocated_memory;
			size_t m_max_allocation_count;
			size_t m_failed_allocations;
			size_t m_failed_reallocations;
			size_t m_failed_frees;
		
		private:
			AllocatorTester( const AllocatorTester & ); // not implemented
			AllocatorTester & operator = ( const AllocatorTester & ); // not implemented
		};

	#endif
		
}; // namespace memo

