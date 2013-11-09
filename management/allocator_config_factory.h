

namespace memo
{
	/**	\class AllocatorConfigFactory
		Use this class to create the config structure of an allocator, given the name of its type ( for example "default_allocator", 
		"debug_allocator", "stat_allocator" ). The config structure provides the create_allocator() function to create the actual 
		allocator. Both the config structure and the allocator are always created with MEMO_NEW.
		The type of allocators must be registered (see register_allocator). memo provides a set of built-in allocators, but user 
		defined allocators can be registered (see memo_externals.h and class MemoryManager). */
	class AllocatorConfigFactory
	{
	public:

		/** Registers a new type of allocator using a default creator function. 
			This function assumes that:
				- the config structure is an inner struct of ALLOCATOR named Config.  
				- the name of the allocator is returned by a static function of ALLOCATOR named type_name.
			If the name is already registered, an error is reported by memo_externals::output_message. */
		template < typename ALLOCATOR >
			void register_allocator()
				{ register_config_creater( ALLOCATOR::type_name(), &_default_allocator_config_creater<ALLOCATOR> ); }

		/** Creates a config structure, given the name of the type.
			If the name is not registered, an error is reported by memo_externals::output_message, and
			nullptr is returned.
			To destroy the config structure, MEMO_DELETE. */
		IAllocator::Config * create_allocator_config( const char * i_name );
		
	private: // internal services

		template < typename ALLOCATOR >
			static IAllocator::Config * _default_allocator_config_creater()
				{ return MEMO_NEW( ALLOCATOR::Config ); }

		/** Prototype of the function that creates the config structure */
		typedef IAllocator::Config * (*CreaterConfigFunction)();

		/** Registers a new type of allocator. The name must be unique. The specified function
			is used to create a new instance of the config structure.
			If the name is already registered, an error is reported by memo_externals::output_message. */
		void register_config_creater( const char * i_name, CreaterConfigFunction i_creater_function );

	private: // data members
		typedef std_unordered_map< StaticName, CreaterConfigFunction, StaticName::Hash >::type Map;
		Map m_registry;
	};


} // namespace memo

