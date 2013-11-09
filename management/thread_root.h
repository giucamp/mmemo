
namespace memo
{

	class ContextStack
	{
	public:

		ContextStack();

		// ask to the memory manager an allocator associated to curr_complete_path XOR i_name 
		void push_context( const StaticName & i_name );

		// sets the previous allocator
		void pop_context();

	private:

		struct ContextEntry
		{
			ContextEntry() { }

			ContextEntry( const StaticName & i_name, IAllocator * i_allocator, NamePath i_complete_path_hash )
				: m_name( i_name ), m_allocator( i_allocator ), m_complete_path_hash( i_complete_path_hash ) { }

			StaticName m_name;
			IAllocator * m_allocator;
			NamePath m_complete_path_hash;
		};

	private: // data members
		std_vector< ContextEntry >::type m_context_stack;
	};

	class ThreadRoot
	{
	public:

		ThreadRoot( const char * i_thread_name );

		~ThreadRoot();

		ContextStack & context_stack()				{ return m_context_stack; }

		LifoAllocator & lifo_allocator()			{ return m_lifo_allocator; }

	private: // internal services
		struct DefaultAllocSetter
			{ DefaultAllocSetter(); };

	private: // data members
		DefaultAllocSetter m_default_alloc_setter;
		ContextStack m_context_stack;
		LifoAllocator m_lifo_allocator;
	};

} // namespace memo
