

namespace memo
{
	#ifndef MEMO_ONLY_DEFAULT_ALLOCATOR
		#error MEMO_ONLY_DEFAULT_ALLOCATOR must be defined in memo_externals.h
	#endif

	#if MEMO_ONLY_DEFAULT_ALLOCATOR

		void * alloc( size_t i_size, size_t i_alignment, size_t i_alignment_offset )
		{
			return DefaultAllocator::s_alloc( i_size, i_alignment, i_alignment_offset );
		}

		void * realloc( void * i_address, size_t i_new_size, size_t i_alignment, size_t i_alignment_offset )
		{
			return DefaultAllocator::s_realloc( i_address, i_new_size, i_alignment, i_alignment_offset );
		}

		void free( void * i_address )
		{
			DefaultAllocator::s_free( i_address );
		}

		void dbg_check( void * i_address )
		{
			DefaultAllocator::s_dbg_check( i_address );
		}
	
		void * unaligned_alloc( size_t i_size )
		{
			return DefaultAllocator::s_unaligned_alloc( i_size );
		}

		void * unaligned_realloc( void * i_address, size_t i_new_size )
		{
			return DefaultAllocator::s_unaligned_realloc( i_address, i_new_size );
		}

		void unaligned_free( void * i_address )
		{
			DefaultAllocator::s_unaligned_free( i_address );
		}

		void unaligned_dbg_check( void * i_address )
		{
			DefaultAllocator::s_unaligned_dbg_check( i_address );
		}

	#else
	
		struct _AllocationHeader
		{
			IAllocator * m_allocator;		
		};
		
		// alloc
		void * alloc( size_t i_size, size_t i_alignment, size_t i_alignment_offset )
		{
			IAllocator * current_allocator = memo_externals::get_current_thread_allocator();

			_AllocationHeader * header = static_cast<_AllocationHeader*>(
				current_allocator->alloc( i_size + sizeof(_AllocationHeader), i_alignment, i_alignment_offset + sizeof(_AllocationHeader) ) );
			
			if( header != nullptr )
			{
				header->m_allocator = current_allocator;

				return header + 1;
			}
			else
				return nullptr;
		}

		// realloc
		void * realloc( void * i_address, size_t i_new_size, size_t i_alignment, size_t i_alignment_offset )
		{
			MEMO_ASSERT( i_address != nullptr ); // realloc with null address is not allowed, use alloc instead
			
			_AllocationHeader * header = static_cast<_AllocationHeader*>( i_address ) - 1;

			_AllocationHeader * new_header = static_cast<_AllocationHeader*>(
				header->m_allocator->realloc( header, i_new_size + sizeof(_AllocationHeader), i_alignment, i_alignment_offset + sizeof(_AllocationHeader) ) );
			
			if( new_header != nullptr )
				return new_header + 1;
			else
				return nullptr;
		}

		// free
		void free( void * i_address )
		{
			MEMO_ASSERT( i_address != nullptr ); // free with null address is not allowed

			_AllocationHeader * header = static_cast<_AllocationHeader*>( i_address ) - 1;

			header->m_allocator->free( header );
		}

		// dbg_check
		void dbg_check( void * i_address )
		{
			MEMO_ASSERT( i_address != nullptr ); // dbg_check with null address is not allowed

			_AllocationHeader * header = static_cast<_AllocationHeader*>( i_address ) - 1;

			header->m_allocator->dbg_check( header );
		}

		// unaligned_alloc
		void * unaligned_alloc( size_t i_size )
		{
			IAllocator * current_allocator = memo_externals::get_current_thread_allocator();

			_AllocationHeader * header = static_cast<_AllocationHeader*>(current_allocator->unaligned_alloc( i_size + sizeof(_AllocationHeader) ) );

			if( header != nullptr )
			{
				header->m_allocator = current_allocator;

				return header + 1;
			}
			else
				return nullptr;
		}

		// unaligned_realloc
		void * unaligned_realloc( void * i_address, size_t i_new_size )
		{
			MEMO_ASSERT( i_address != nullptr ); // unaligned_realloc with null address is not allowed, use alloc instead

			_AllocationHeader * header = static_cast<_AllocationHeader*>( i_address ) - 1;

			_AllocationHeader * new_header = static_cast<_AllocationHeader*>(
				header->m_allocator->unaligned_realloc( header, i_new_size + sizeof(_AllocationHeader) ) );

			if( new_header != nullptr )
				return new_header + 1;
			else
				return nullptr;
		}

		// unaligned_free
		void unaligned_free( void * i_address )
		{
			MEMO_ASSERT( i_address != nullptr ); // unaligned_free with null address is not allowed

			_AllocationHeader * header = static_cast<_AllocationHeader*>( i_address ) - 1;

			header->m_allocator->unaligned_free( header );
		}

		// unaligned_dbg_check
		void unaligned_dbg_check( void * i_address )
		{
			MEMO_ASSERT( i_address != nullptr ); // unaligned_dbg_check with null address is not allowed

			_AllocationHeader * header = static_cast<_AllocationHeader*>( i_address ) - 1;

			header->m_allocator->unaligned_dbg_check( header );
		}	

	#endif

	IAllocator & get_current_allocator()
	{
		IAllocator * current_allocator = memo_externals::get_current_thread_allocator();

		return *current_allocator;
	}

	LifoAllocator & get_lifo_allocator()
	{
		ThreadRoot * thread_context = memo_externals::get_thread_root();
		MEMO_ASSERT( thread_context != nullptr );
		return thread_context->lifo_allocator();
	}
	
	// lifo_alloc
	void * lifo_alloc( size_t i_size, size_t i_alignment, size_t i_alignment_offset, DeallocationCallback i_deallocation_callback )
	{
		ThreadRoot * thread_context = memo_externals::get_thread_root();
		MEMO_ASSERT( thread_context != nullptr );

		return thread_context->lifo_allocator().alloc( i_size, i_alignment, i_alignment_offset, i_deallocation_callback );
	}

	// lifo_free
	void lifo_free( void * i_address )
	{
		ThreadRoot * thread_context = memo_externals::get_thread_root();

		return thread_context->lifo_allocator().free( i_address );
	}

} // namespace memo



