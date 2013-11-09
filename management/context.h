

namespace memo
{
	/** Configuration for a context. The members of this structure are read from the
		configuration file */
	class ContextConfig
	{
	public:
		ContextConfig();
		ContextConfig( const char * i_name );
		~ContextConfig();

		typedef std_vector< ContextConfig * >::type ContextVector;

		/** Retrieves the name of the context */
		const StaticName & name() const						{ return m_name; }

		/** Sets the name of the context */
		void set_name( const char * i_name );

		/** Retrieves the configuration of the associated allocator */
		IAllocator::Config * allocator_config() const						{ return m_allocator_config; }

		/** Sets the configuration of the associated allocator */
		void set_allocator_config( IAllocator::Config *  i_config )			{ m_allocator_config = i_config; }

		/** Retrieves the child contexts */
		const ContextVector & inner_contexts() const			{ return m_inner_contexts; } 

		/** Adds a child context */
		void add_inner_context( ContextConfig * i_context )		{ m_inner_contexts.push_back( i_context ); }

		/** Loads the configuration */
		void load( serialization::IConfigReader & i_config_reader );

	private: // data members
		StaticName m_name;
		char * m_name_string;
		IAllocator::Config * m_allocator_config;
		std_vector< ContextConfig * >::type m_inner_contexts; 

	private:
		ContextConfig( const ContextConfig & ); // not implemented
		ContextConfig & operator = ( const ContextConfig & ); // not implemented
	};


	class Context
	{
	public:

		Context( const StaticName & i_name )
		{
			memo_externals::get_thread_root()->context_stack().push_context( i_name );
		}

		~Context()
		{
			memo_externals::get_thread_root()->context_stack().pop_context();
		}

	private:
		Context( const Context & ); // not implemented
		Context & operator = ( const Context & ); // not implemented
	};

} // namespace memo


