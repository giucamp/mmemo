
#if defined( _MSC_VER ) && defined( MEMO_ENABLE_TLSF )

namespace memo
{
	/**	This is a special allocator that is able to detect:
			- read accesses to memory that was allocated but never written (i.e. uninitialized memory usage)
			- read or write access to memory outside allocated blocks
		
		CorruptionDetectorAllocator runs an external debugger, "memo_debugger.exe", and communicates with it.
		memo_debugger tracks every read or write access inside the memory managed by CorruptionDetectorAllocator,
		so the access to this memory is extremely slow.
		Currently this allocator is defined only for windows, as it uses windows specific APIs. Internally
		CorruptionDetectorAllocator uses the tlsf allocator implemented by Matthew Conte (http://tlsf.baisoku.org).
		
		When memo_debugger detects an invalid memory read or write operation, it breaks the execution of the program,
		writes about the error on its console window, and allows the user to choose one if these actions: 
			- detaching the target process, to allow another debugger to attach to it
			- resuming the target process, ignoring the error
			- quitting, after resuming the target process
			- saving a dump that can be opened in a compatible debugger, such visual studio or windbg.
			- dumping the memory content around 
			- saving or copying all the output of the debugger
		
		Here is an example of the output of the debugger:

		\verbatim

		 *********** ERROR: attempt to write unwritable address 0x00790C9C ***********

		*** STACK TRACE ***
		->0x0096047A memo::ContextTest::test_CorruptionDetectorAllocator (231)
		  0x0095FA95 memo::ContextTest::test (247)
		  0x0095F974 memo::test (262)
		  0x0094B5A6 main (82)
		  0x0098E7FF __tmainCRTStartup (555)
		  0x0098E62F mainCRTStartup (371)
		  0x75D5ED5C BaseThreadInitThunk (371)
		  0x778B37EB RtlInitializeExceptionChain (371)
		  0x778B37BE RtlInitializeExceptionChain (371)

		*** SOURCE CODE AROUND 0x0096047A ***
		module: D:\GitHub\mmemo\test\Debug\test.exe
		source file: d:\github\mmemo\memo_test.cpp
		   229: 			int * array = MEMO_NEW_ARRAY( int, 5 );
		   230: 			for( int i = 1; i <= 5; i++ )
		-> 231: 				array[i] = i;
		   232: 			MEMO_DELETE_ARRAY( array );
		   233: 

		*** MEMORY CONTENT AROUND 0x00790C9C grouped by 4 byte(s) ***
		  0x00790C7C: 0x00790000 - not allocated
		  0x00790C80: 0x0045842c - readable/writable
		  0x00790C84: 0x00000005 - readable/writable
		  0x00790C88: 0x40400000 - writable
		  0x00790C8C: 0x00000001 - readable/writable
		  0x00790C90: 0x00000002 - readable/writable
		  0x00790C94: 0x00000003 - readable/writable
		  0x00790C98: 0x00000004 - readable/writable
		->0x00790C9C: 0x00790c70 - not allocated
		  0x00790CA0: 0x000ff359 - not allocated
		  0x00790CA4: 0x00790000 - not allocated
		  0x00790CA8: 0x00790000 - not allocated
		  0x00790CAC: 0x00000000 - not allocated
		  0x00790CB0: 0x00000000 - not allocated
		  0x00790CB4: 0x00000000 - not allocated
		  0x00790CB8: 0x00000000 - not allocated
		  0x00790CBC: 0x00000000 - not allocated

		Type a command:
		  detach: detach the target process, to allow another debugger to attach to it
		  ignore: resume the target process, ignoring the error
		  quit: resume the target process, and quit
		  minidump [file=test_memo_mini.dmp]: save a minidump
		  dump [file=test_memo.dmp]: save a complete dump
		  mem [1|2|4|8]: dump memory content around 0x00790C9C
		  save [file=test_memo_output.txt]: save all the output of this program
		  copy: copy all the output of this program into the clipboard
		>> 


		\endverbatim

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
			uint32_t m_check_granularity;
			std_string m_memo_debugger_name;
			Config()
				{ set_defaults(); }

			void set_defaults()
			{
				m_region_size = 1024 * 1024 * 1;
				m_heading_nomansland = sizeof(void*);
				m_tailing_nomansland = sizeof(void*);
				m_check_granularity = 2;
				#if defined( _M_IX86 )
					#ifdef _DEBUG
						m_memo_debugger_name = "memo_debugger_Win32_Debug.exe";
					#else
						m_memo_debugger_name = "memo_debugger_Win32_Release.exe";
					#endif
				#elif defined( _M_X64 )
					#ifdef _DEBUG
						m_memo_debugger_name = "memo_debugger_x64_Debug.exe";
					#else
						m_memo_debugger_name = "memo_debugger_x64_Release.exe";
					#endif
				#else
					#error assign instruction pointer
				#endif				
			}

		protected:

			/** configures and returns a new allocator, eventually creating it using MEMO_NEW if i_new_allocator is nullptr */
			virtual IAllocator * configure_allocator( IAllocator * i_new_allocator ) const;

			/** Tries to recognize the current property from the stream, and eventually reads its value.
			  @param i_config_reader the source stream
			  @return true if the property has been recognized, false otherwise */
			bool try_recognize_property( serialization::IConfigReader & i_config_reader );
		};
		

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

		
		void notify_allocation( void * i_address, size_t i_size );
		void notify_deallocation( void * i_address, size_t i_size );
		
		void protect();
		void unprotect();

	private: // exception codes
		static const uint32_t EXC_SET_BUFFER = 0xCC9B7983;
		static const uint32_t EXC_ALLOCATION = 0xCC9B7984;
		static const uint32_t EXC_DEALLOCATION = 0xCC9B7985;
		static const uint32_t EXC_STOP_DEBUGGING = 0xCC9B7986;
		static const uint32_t EXC_PROTECT = 0xCC9B7987;
		static const uint32_t EXC_UNPROTECT = 0xCC9B7988;

	private: // data members
		void * m_buffer;
		size_t m_size;
		void * m_tlsf;
		Config m_config;
		memo_externals::Mutex m_mutex;
	};

} // namespace memo

#endif // #if defined( _MSC_VER ) && defined( MEMO_ENABLE_TLSF )
