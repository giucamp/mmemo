
namespace memo
{
	ContextStack::ContextStack()
	{
		IAllocator * allocator = memo_externals::get_current_thread_allocator();
		
		NamePath empty_path;
		StaticName empty_name;
		m_context_stack.push_back( ContextEntry( empty_name, allocator, empty_path ) );

		m_context_stack.reserve( 8 );
	}

	// ask to the memory manager an allocator associated to curr_complete_path XOR i_name 
	void ContextStack::push_context( const StaticName & i_name )
	{
		NamePath path( m_context_stack.back().m_complete_path_hash );
		path.append_context( i_name );

		IAllocator * allocator = memo::MemoryManager::get_instance().get_allocator( path );
		
		if( allocator != nullptr )
			memo_externals::set_current_thread_allocator( allocator );

		m_context_stack.push_back( ContextEntry( i_name, allocator, path ) );
	}

	// sets the previous allocator
	void ContextStack::pop_context()
	{
		m_context_stack.pop_back();

		IAllocator * allocator = m_context_stack.back().m_allocator;

		if( allocator != nullptr )
			memo_externals::set_current_thread_allocator( allocator );
	}

	ThreadRoot::DefaultAllocSetter::DefaultAllocSetter()
	{
		DefaultAllocator & default_allocator = safe_get_default_allocator();
		memo_externals::set_current_thread_allocator( &default_allocator );
	}

	// ThreadRoot::constructor
	ThreadRoot::ThreadRoot( const char * i_thread_name )
	{
		MemoryManager::get_instance();

		DefaultAllocator & default_allocator = safe_get_default_allocator();
		m_lifo_allocator.init( default_allocator, 512, 512 );

		#if MEMO_ENABLE_ASSERT
			// every thread can have at most one thread context
			ThreadRoot * thread_context = memo_externals::get_thread_root();
			MEMO_ASSERT( thread_context == nullptr );
		#endif

		memo_externals::set_thread_root( this );
	}

	// ThreadRoot::destructor
	ThreadRoot::~ThreadRoot()
	{
		#if MEMO_ENABLE_ASSERT
			// every thread can have at most one thread context
			ThreadRoot * thread_context = memo_externals::get_thread_root();
			MEMO_ASSERT( thread_context == this );
		#endif

		memo_externals::set_thread_root( nullptr );
	}

} // namespace memo