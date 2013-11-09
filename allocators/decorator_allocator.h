

namespace memo
{
	/**	Abstract base class for classes using the decorator pattern to add functionalities to an allocator (dest allocator)
		The dest allocator must be assigned before using any of the allocation functions. */
	class DecoratorAllocator : public IAllocator
	{
	public:

		/** Static function returning the name of the allocator class */
		static const char * type_name() { return "decorator_allocator"; }

		/** Config structure for DecoratorAllocator */
		struct Config : public IAllocator::Config
		{
		public:

			/** Set the default values. m_target is set to nullptr */
			Config();

			/** Deletes the config pointed by m_target, is not null */
			~Config();

			/** Pointer to the config of the target allocator. It has the ownership of the pointed object. */
			IAllocator::Config * m_target;

		protected:

			/** configures and returns a new allocator, eventually creating it using MEMO_NEW if i_new_allocator is nullptr */
			virtual IAllocator * configure_allocator( IAllocator * i_new_allocator ) const;

			/** Tries to recognize the current property from the stream, and eventually reads its value.
				Derived classes that override try_recognize_property should call this this function (the 
				base class' version). 
			  @param i_config_reader the source stream
			  @return true if the property has been recognized, false otherwise */
			virtual bool try_recognize_property( serialization::IConfigReader & i_config_reader );
		};

		/** Retrieves the dest allocator */
		IAllocator & dest_allocator()
			{ return *m_dest_allocator; }

		/** retrieves the dest allocator	*/
		const IAllocator & dest_allocator() const
			{ return *m_dest_allocator; }

		/** Writes out in a human readable way the state of the allocator */
		void dump_state( StateWriter & i_state_writer );

	protected:

		/** Default constructor */
		DecoratorAllocator() 
			: m_dest_allocator( nullptr ) { }

	private: // data members
		IAllocator * m_dest_allocator;
	};

} // namespace memo
