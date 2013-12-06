
#if defined( _MSC_VER ) && defined( MEMO_ENABLE_TLSF )

namespace memo
{
	/**	This is a special allocator that is able to detect:
			- read accesses to memory that was allocated but never written (i.e. uninitialized memory usage)
			- read or write access to memory outside allocated blocks
		When the program tries to access a wrong memory access, a SEH exception is thrown (a crash), so that 
		the program can be debugged (do not confuse SEH with C++ exception mechanism).
		CorruptionDetectorAllocator exploits the virtual memory guard mechanism, and traps every single access 
		to the memory blocks: every time the program read or writes memory inside a block, the system invokes a 
		trapping routine that checks the validity of the access. For these reason, accessing the memory allocated
		by CorruptionDetectorAllocator is extremely slow.
		Currently this allocator is defined only for windows, as it uses windows specific APIs. Internally
		CorruptionDetectorAllocator uses the tlsf allocator implemented by Matthew Conte (http://tlsf.baisoku.org).
	*/
	class CorruptionDetectorAllocator : public IAllocator
	{
	public:

		/** Static function returning the name of the allocator class, used to register the type.
		   This name can be used to instantiate this allocator in the configuration file. */
		static const char * type_name() { return "corruption_detector_allocator"; }


								///// configuration /////
		
		/** Config structure for CorruptionDetectorAllocator */
		struct Config : public IAllocator::Config
		{
		public:

			size_t m_region_size; /**< size (in bytes) of the memory that can be used by the allocator */
			size_t m_heading_nomansland; /**< specifies the size of the heading no man's lands for every block */
			size_t m_tailing_nomansland; /**< specifies the size of the tailing no man's lands for every block */
			uint32_t m_bad_read_access_exception; /**< exception to throw when the program reads memory inside the 
										  region of the allocator, but outside a block */
			uint32_t m_bad_write_access_exception; /**< exception to throw when the program reads memory inside the 
										  region of the allocator, but outside a block */

			Config()
				{ set_defaults(); }

			void set_defaults()
			{
				m_region_size = 1024 * 1024 * 1;
				m_heading_nomansland = sizeof(void*);
				m_tailing_nomansland = sizeof(void*);
				m_bad_read_access_exception = 0x0BADBAD0;
				m_bad_write_access_exception = 0x0BADBAD1;
			}

		protected:

			/** configures and returns a new allocator, eventually creating it using MEMO_NEW if i_new_allocator is nullptr */
			virtual IAllocator * configure_allocator( IAllocator * i_new_allocator ) const;

			/** Tries to recognize the current property from the stream, and eventually reads its value.
			  @param i_config_reader the source stream
			  @return true if the property has been recognized, false otherwise */
			bool try_recognize_property( serialization::IConfigReader & i_config_reader );
		};


		bool can_read( void * i_address ) const;

		bool can_write( void * i_address ) const;

		size_t count_writable_bytes() const;

		size_t count_readable_bytes() const;


								///// aligned allocations /////

		/** allocates an aligned memory block. Implements IAllocator::alloc.
		  @param i_size size of the block in bytes
		  @param i_alignment alignment requested for the block. It must be an integer power of 2
		  @param i_alignment_offset offset from beginning of the block of the address that respects the alignment
		  @return the address of the first byte in the block, or nullptr if the allocation fails
		*/
		void * alloc( size_t i_size, size_t i_alignment, size_t i_alignment_offset );


		/** changes the size of the memory block allocated by alloc, possibly moving it in a new location. Implements IAllocator::realloc.
		  @param i_address address of the memory block to reallocate. This parameter cannot be nullptr.
		  @param i_new_size new size of the block in bytes
		  @param i_alignment alignment requested for the block. It must be an integer power of 2
		  @param i_alignment_offset offset from the address that respects the alignment
		  @return the new address of the first byte in the block, or nullptr if the reallocation fails */
		void * realloc( void * i_address, size_t i_new_size, size_t i_alignment, size_t i_alignment_offset );

		/** deallocates a memory block allocated by alloc or realloc. Implements IAllocator::free.
		  @param i_address address of the memory block to free. It cannot be nullptr. */
		void free( void * i_address );

		/** Implements IAllocator::dbg_check. All the consistency checks are performed on the memory block.
		  @param i_address address of the memory block to check */
		void dbg_check( void * i_address );




							///// unaligned allocations /////

		/** allocates a new memory block. Implements IAllocator::unaligned_alloc.
		  @param i_size size of the block in bytes
		  @return the address of the first byte in the block, or nullptr if the allocation fails */
		void * unaligned_alloc( size_t i_size );

		/** changes the size of the memory block allocated by unaligned_alloc. Implements IAllocator::unaligned_realloc.
		  @param i_address address of the memory block to reallocate. This parameter cannot be nullptr.
		  @param i_new_size new size of the block in bytes
		  @return the new address of the first byte in the block, or nullptr if the reallocation fails */
		void * unaligned_realloc( void * i_address, size_t i_new_size );

		/** deallocates a memory block allocated by unaligned_alloc or unaligned_realloc. Implements IAllocator::unaligned_free.
		  @param i_address address of the memory block to free. It cannot be nullptr. */
		void unaligned_free( void * i_address );

		/** This function performs some integrity check on the block. Implements IAllocator::unaligned_dbg_check.
		  @param i_address address of the memory block to check */
		void unaligned_dbg_check( void * i_address );
				
		/** Writes out in a human readable way the state of the allocator */
		void dump_state( StateWriter & i_state_writer );

	protected:
		CorruptionDetectorAllocator( const Config & i_config );
		~CorruptionDetectorAllocator();

	private:
		CorruptionDetectorAllocator( const CorruptionDetectorAllocator & ); // not implemented
		CorruptionDetectorAllocator & operator = ( const CorruptionDetectorAllocator & ); // not implemented

		struct Header
		{
			size_t m_size;
		};

		struct ExceptionHandler;

		void notify_allocation( void * i_address, size_t i_size );
		void notify_deallocation( void * i_address, size_t i_size );

		void protect();
		void unprotect();

	private: // data members
		void * m_buffer, * m_end_of_buffer;
		size_t m_size;
		void * m_tlsf;
		std::vector< bool > m_can_read, m_can_write;
		Config m_config;
		void * m_exception_handler;
		memo_externals::Mutex m_mutex;
		static CorruptionDetectorAllocator * s_first_allocator;
		CorruptionDetectorAllocator * m_next_allocator;
	};

} // namespace memo

#endif // #if defined( _MSC_VER ) && defined( MEMO_ENABLE_TLSF )
