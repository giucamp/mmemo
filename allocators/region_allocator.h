
namespace memo
{
	/**	\class RegionAllocator
		Abstract base class for classes providing allocation services consuming memory inside a monolithic buffer 
		allocated with an extern allocator, which usually is the default allocator. 
		Derived classes may use their own algorithm to manage the buffer and perform allocations. If the allocation
		is not possible (i.e. the space in the monolithic buffer is over), the extern allocator is used to perform the 
		allocation. */
	class RegionAllocator : public IAllocator
	{
	public:

		/** Static function returning the name of the allocator class */
		static const char * type_name() { return "region_allocator"; }

		/** Config structure for RegionAllocator */
		struct Config : public IAllocator::Config
		{
		public:

			Config()	: m_buffer_size( 0 ) { }

			size_t m_buffer_size;

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

		/** Returns the size of the buffer used for the region. This size is the maximum between an implementation 
			dependant minimum, and the member m_buffer_size in the configuration struct */
		size_t buffer_size() const						{ return m_buffer_size; }

		/** retrieves the extern allocator. */
		IAllocator & extern_allocator() const
			{ return *m_extern_allocator; }

		/** Destroys a RegionAllocator, freeing the buffer */
		~RegionAllocator();

		/** Writes out in a human readable way the state of the allocator */
		void dump_state( StateWriter & i_state_writer );

	protected:

		/** Constructs a RegionAllocator, allocating the buffer to use for the region 
			@param i_minimum_buffer_size The actual buffer size is the maximum between this parameter 
				and the member m_buffer_size in the configuration struct. */
		RegionAllocator( const Config & i_config, size_t i_minimum_buffer_size );

		/** Returns a pointer to the start of the buffer */
		void * buffer() const { return m_buffer; }

	private: // data members
		IAllocator * m_extern_allocator;
		size_t m_buffer_size;
		void * m_buffer;
	};

} // namespace memo
