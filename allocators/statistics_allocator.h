
namespace memo
{
	/**	Decorator allocator that collects statistics. Statistics like allocation count or total allocated memory are collected 
		(see struct Statistics). This allocator adds to IAllocator interface the capability to retrieve the size of a 
		memory block (see StatAllocator::get_size). To support this a size_t is added as overhead to everey memory block.

		The following parameters are supported in the configuration file:
		- target: inherited from DecoratorAllocator, is the name of the type of target allocator (for example "default_allocator", 
			"debug_allocator", "stat_allocator").

		\note This class uses a mutex to ensure thread-safeness.
	*/
	class StatAllocator : public DecoratorAllocator
	{
	public:

		static const char * type_name() { return "stat_allocator"; }

		/** default constructor. The dest allocator must be assigned before using the allocator (see DecoratorAllocator::set_dest_allocator) */
		StatAllocator();
		

							//// statistics ////

		struct Statistics
		{
			size_t m_allocation_count; /**< number of allocated memory blocks */
			size_t m_allocation_count_peak; /**< maximum number of allocated blocks in the whole lifetime of this StatAllocator */
			size_t m_total_allocated; /**< total number of bytes allocated in memory blocks. This count does not take count 
									  of any internal or external fragmentation, or overhead added to respect the alignment.
									  It's just the memory requested to the allocator. */
			size_t m_total_allocated_peak; /**< maximum number of bytes allocated in the whole lifetime of this StatAllocator */
			void * m_min_address; /**< smallest address ever allocated */
			void * m_max_address; /**< biggest address ever allocated */
			
			/** default constructor. All the counters are initialized with zero */
			Statistics();
		};

		/** retrieves the current statistics of this allocator
		  @param o_dest struct instance to be written
		*/
		void get_stats( Statistics & o_dest );


		/** retrieves the size of a memory block allocated by alloc, realloc unaligned_alloc or unaligned_realloc.
		  @param i_address address of the memory block. It can't be nullptr.
		  @return the size in bytes of the memory block. It is exactly the number of bytes requested to the allocator.
		*/
		size_t get_size( void * i_address );


							//// statistics ////

		struct Config : public DecoratorAllocator::Config
		{
		protected:

			/** configures and returns a new allocator, eventually creating it using MEMO_NEW if i_new_allocator is nullptr */
			virtual IAllocator * configure_allocator( IAllocator * i_new_allocator ) const;
		};


							///// aligned allocations /////

		/** \brief allocates an aligned memory block. Implements IAllocator::alloc.
			The dest allocator is used to perform the operation. See DecoratorAllocator.
		  @param i_size size of the block in bytes
		  @param i_alignment alignment requested for the block. It must be an integer power of 2
		  @param i_alignment_offset offset from beginning of the block of the address that respects the alignment
		  @return the address of the first byte in the block, or nullptr if the allocation fails
		*/
		void * alloc( size_t i_size, size_t i_alignment, size_t i_alignment_offset );


		/** \brief changes the size of the memory block allocated by alloc, possibly moving it in a new location. Implements IAllocator::realloc.
			The dest allocator is used to perform the operation. See DecoratorAllocator.
		  @param i_address address of the memory block to reallocate. This parameter cannot be nullptr.
		  @param i_new_size new size of the block in bytes
		  @param i_alignment alignment requested for the block. It must be an integer power of 2
		  @param i_alignment_offset offset from the address that respects the alignment
		  @return the new address of the first byte in the block, or nullptr if the reallocation fails
		*/
		void * realloc( void * i_address, size_t i_new_size, size_t i_alignment, size_t i_alignment_offset );

		/** \brief deallocates a memory block allocated by alloc or realloc. Implements IAllocator::free.
			The dest allocator is used to perform the operation. See DecoratorAllocator.
		  @param i_address address of the memory block to free. It cannot be nullptr.
		  */
		void free( void * i_address );

		/** \brief Implements IAllocator::dbg_check. Some basic checks is performed on the memory block.
		  @param i_address address of the memory block to check
		*/
		void dbg_check( void * i_address );




							///// unaligned allocations /////

		/** \brief allocates a new memory block. Implements IAllocator::unaligned_alloc.
		  The dest allocator is used to perform the operation. See DecoratorAllocator.
		  @param i_size size of the block in bytes
		  @return the address of the first byte in the block, or nullptr if the allocation fails
		*/
		void * unaligned_alloc( size_t i_size );

		/** \brief changes the size of the memory block allocated by unaligned_alloc. Implements IAllocator::unaligned_realloc.
		  The dest allocator is used to perform the operation. See DecoratorAllocator.
		  @param i_address address of the memory block to reallocate. This parameter cannot be nullptr.
		  @param i_new_size new size of the block in bytes
		  @return the new address of the first byte in the block, or nullptr if the reallocation fails
		*/
		void * unaligned_realloc( void * i_address, size_t i_new_size );

		/** \brief deallocates a memory block allocated by unaligned_alloc or unaligned_realloc. Implements IAllocator::unaligned_free.
		  The dest allocator is used to perform the operation. See DecoratorAllocator.
		  @param i_address address of the memory block to free. It cannot be nullptr.
		  */
		void unaligned_free( void * i_address );

		/** \brief This function performs no action. Implements IAllocator::unaligned_dbg_check.
		  The dest allocator is used to perform the operation. See DecoratorAllocator.
		  @param i_address address of the memory block to check
		*/
		void unaligned_dbg_check( void * i_address );

		/** Writes out in a human readable way the state of the allocator */
		void dump_state( StateWriter & i_state_writer );

	private: // data members
		Statistics m_stats;
		memo_externals::Mutex m_mutex;

	private: // internal services
		struct Header;
	};

} // namespace memo

