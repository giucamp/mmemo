
namespace memo
{
	#if MEMO_ENABLE_TLSF

		/**	\class TlsfAllocator
			This class wraps the tlsf allocator implemented by Matthew Conte (http://tlsf.baisoku.org) */
		class TlsfAllocator : public RegionAllocator
		{
		public:

			/** Static function returning the name of the allocator class, used to register the type.
			   This name can be used to instantiate this allocator in the configuration file. */
			static const char * type_name() { return "tlsf_allocator"; }


									///// configuration /////
		
			/** Config structure for TlsfAllocator */
			struct Config : public RegionAllocator::Config
			{
			protected:

				/** configures and returns a new allocator, eventually creating it using MEMO_NEW if i_new_allocator is nullptr */
				virtual IAllocator * configure_allocator( IAllocator * i_new_allocator ) const;
			};




									///// aligned allocations /////

			/** allocates an aligned memory block. Implements IAllocator::alloc.
				tlsf is used to perform the allocation.
			  @param i_size size of the block in bytes
			  @param i_alignment alignment requested for the block. It must be an integer power of 2
			  @param i_alignment_offset offset from beginning of the block of the address that respects the alignment
			  @return the address of the first byte in the block, or nullptr if the allocation fails
			*/
			void * alloc( size_t i_size, size_t i_alignment, size_t i_alignment_offset );


			/** changes the size of the memory block allocated by alloc, possibly moving it in a new location. Implements IAllocator::realloc.
				tlsf is used to perform the allocation.
			  @param i_address address of the memory block to reallocate. This parameter cannot be nullptr.
			  @param i_new_size new size of the block in bytes
			  @param i_alignment alignment requested for the block. It must be an integer power of 2
			  @param i_alignment_offset offset from the address that respects the alignment
			  @return the new address of the first byte in the block, or nullptr if the reallocation fails */
			void * realloc( void * i_address, size_t i_new_size, size_t i_alignment, size_t i_alignment_offset );

			/** deallocates a memory block allocated by alloc or realloc. Implements IAllocator::free.
				tlsf is used to perform the allocation.
			  @param i_address address of the memory block to free. It cannot be nullptr. */
			void free( void * i_address );

			/** Implements IAllocator::dbg_check.
			  @param i_address address of the memory block to check */
			void dbg_check( void * i_address );




								///// unaligned allocations /////

			/** allocates a new memory block. Implements IAllocator::unaligned_alloc.
			  tlsf is used to perform the allocation.
			  @param i_size size of the block in bytes
			  @return the address of the first byte in the block, or nullptr if the allocation fails */
			void * unaligned_alloc( size_t i_size );

			/** changes the size of the memory block allocated by unaligned_alloc. Implements IAllocator::unaligned_realloc.
			  tlsf is used to perform the allocation.
			  @param i_address address of the memory block to reallocate. This parameter cannot be nullptr.
			  @param i_new_size new size of the block in bytes
			  @return the new address of the first byte in the block, or nullptr if the reallocation fails */
			void * unaligned_realloc( void * i_address, size_t i_new_size );

			/** deallocates a memory block allocated by unaligned_alloc or unaligned_realloc. Implements IAllocator::unaligned_free.
			  tlsf is used to perform the allocation.
			  @param i_address address of the memory block to free. It cannot be nullptr. */
			void unaligned_free( void * i_address );

			/** This function performs some integrity check on the block. Implements IAllocator::unaligned_dbg_check.
			  tlsf is used to perform the allocation.
			  @param i_address address of the memory block to check */
			void unaligned_dbg_check( void * i_address );

			/** Constructs the flsf pool */
			TlsfAllocator( const Config & i_config );

			/** Destroys the tlsf pool */
			~TlsfAllocator();

			/** Writes out in a human readable way the state of the allocator */
			void dump_state( StateWriter & i_state_writer );

		private:
			TlsfAllocator( const TlsfAllocator & ); // not implemented
			TlsfAllocator & operator = ( const TlsfAllocator & ); // not implemented

			struct AlignmentHeader;

		private:
			void * m_tlsf;
		};

	#endif // #if MEMO_ENABLE_TLSF

} // namespace memo
