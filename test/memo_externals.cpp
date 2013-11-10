
#include "..\memo.h"

#include <windows.h>

namespace memo_externals
{
	void output_message( const char * i_message )
	{
		printf( "%s", i_message );
	}

	void assert_failure( const char * i_condition )
	{
		printf( "assert failed: %s\n", i_condition );
		DebugBreak();
	}
	
	void debug_break()
	{
		DebugBreak();
	}


	void Mutex::lock()
	{
		while( InterlockedExchange( &m_lock, 1 ) != 0 )
		{
			SwitchToThread();
		}
	}

	void Mutex::unlock()
	{
		MEMO_ASSERT( m_lock == 1 );
		m_lock = 0;
	}

	__declspec( thread ) memo::IAllocator * g_allocator;
	__declspec( thread ) memo::ThreadRoot * g_thread_context; 
		
	memo::IAllocator * get_current_thread_allocator()
	{
		return g_allocator;
	}

	void set_current_thread_allocator( memo::IAllocator * i_allocator )
	{
		g_allocator = i_allocator;
	}

	memo::ThreadRoot * get_thread_root()
	{
		return g_thread_context;
	}

	void set_thread_root( memo::ThreadRoot * i_thread_context )
	{
		g_thread_context = i_thread_context;
	}

	void register_custom_allocators( memo::AllocatorConfigFactory & i_factory )
	{
		MEMO_UNUSED( i_factory );
	}

} // namespace memo_externals
