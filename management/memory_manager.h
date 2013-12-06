

namespace memo
{
	/** MemoryManager */
	class MemoryManager
	{
	public:

		/** Returns the instance of the memory manager. The memory manager is a singleton, and is created
			the first time this function is called. */
		static MemoryManager & get_instance();

		/** This struct contains the configuration of the memory manager, which is read from the memory 
				configuration file. */
		struct Config
		{
			Config() { }

			ContextConfig m_root_context; /**< Root of the tree of contexts */

			void load( serialization::IConfigReader & i_config_reader );

		private:
			Config( const Config & );
			Config & operator = ( const Config & );
		};

		/** Retrieves the configuration of the memory manager. The configuration can be used to explore 
			the context tree. */
		const Config & get_configuration() const						{ return m_config; }

		/** Gets the allocator associated to the input context path.
			Currently this method accesses an unordered map (so the complexity is constant). No mutex lock is necessary,
			as the whole memory manager is immutable.
			@param i_context_path Path of the target context
			@return Allocator associated to the context, or null if no allocator is associated to the context */
		IAllocator * get_allocator( NamePath i_context_path ) const;

		/** Gets the allocator and the configuration of a context. The configuration can be used to iterate the child contexts.
			@param i_context_path Path of the target context
			@return Allocator associated to the context, or null if no allocator is associated to the context */
		bool get_allocator_and_config( NamePath i_context_path, IAllocator * * o_allocator, const ContextConfig * * o_context_config ) const;

		/** Retrieves the allocator config factor, that can be used to create config struct. The config struct can be then
			used to create instances of the allocator. */
		AllocatorConfigFactory & allocator_config_factor()					{ return m_allocator_config_factory; }
		const AllocatorConfigFactory & allocator_config_factor() const		{ return m_allocator_config_factory; }

		/** Dumps the state of the allocators associated to a context and all its sub-contexts.
				@param i_path path of the context to dump
				@param i_state_writer state-writer object to receive the state of the allocators.
			@return true if the context was found, false	otherwise */
		bool dump_contexts( const NamePath & i_path, IAllocator::StateWriter & i_state_writer ) const;

	private:

		MemoryManager();

		MemoryManager( const MemoryManager & ); // not implemented
		MemoryManager & operator = ( const MemoryManager & ); // not implemented

		void _build_context_maps( const ContextConfig & i_context, const NamePath & i_context_path );
		
	private: // data members

		// configuration
		Config m_config;

		// context map
		struct ContextEntry
		{
			IAllocator * m_allocator;
			const ContextConfig * m_config;
		};
		typedef std_unordered_map< NamePath, ContextEntry, NamePath::Hash >::type ContextMap;
		ContextMap m_contexts_map;

		// config factory
		AllocatorConfigFactory m_allocator_config_factory;
	};


}
