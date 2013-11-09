
namespace memo
{
	/**	\interface IAllocator include.h memo.h
		Interface for classes that provide memory allocation services.
		Classes implementing this interface provide function to allocate, reallocate and free dynamic memory blocks.
		Two separate sets of functions are defined, for aligned and unaligned allocations. The two sets are mutually 
		incompatible, and mixing them causes memory corruption. For example, memory allocated with alloc can be 
		deallocated by free, but not by unaligned_free.
		\include memo.h */
	class IAllocator
	{
	public:

		/** Static function returning the name of the type, used for example in the configuration file, where a
			type of allocated must be specified. AllocatorConfigFactory uses this function to associate a name to 
			the allocator.
			Every class implementing IAllocator should define a static function with this name and signature. The 
			returned string should lie in the static storage (a string literal is the best choice). */
		static const char * type_name() { return "allocator"; }

		/** This is the base for the config structure of the allocator. The config structure has two purposes:
				- provides parameters to configure the allocator. These parameter are read from the configuration file,
					and are read-only for the allocator
				- creates and configures the allocator, acting like an allocator template
			Every class implementing IAllocator should define an inner struct with this name.
			The config structure is mandatory for any allocator, as it is the standard way to instantiate it */
		struct Config
		{
		public:

			/** Loads the properties of the configurations from the given stream. This functions loops the properties 
				in the input stream trying to recognize them with try_recognize_property. If a property is not recognized,
				or if any other issue occurs, an eroor is reported with serialization::IConfigReader::output_message.
			  @param i_config_reader the source stream. */
			void load( serialization::IConfigReader & i_config_reader );

			/** Creates a new instance of the allocator related to this config, and configures it. */
			IAllocator * create_allocator() const;

		protected:

			/** This function should try to recognize the current property of the input stream, and eventually read
				the value. Implementations of IAllocator and derived classes can implement or override it to read their 
				own properties from the configuration file.
				If the property is recognized but the value is not valid, the function should report the error to the 
				stream (see serialization::IConfigReader::output_message), but the return value should be true.
			  @param i_config_reader the source stream
			  @return true if the property has been recognized, false otherwise */
			virtual bool try_recognize_property( serialization::IConfigReader & i_config_reader ) = 0;

			/** This function receives or creates a new allocator compatible with this config, and configures it with
				the content of this config. The point of the parameter i_new_allocator is to allow a derived classes to
				create and configure an allocator, calling the base class' version just configure it.
				Any allocator config struct should call the base class' configure_allocator.
				@param i_new_allocator Allocator instance created by a derived class' override of create_allocator.
				@return The return must be i_new_allocator if it is not null, otherwise a new instance created with MEMO_NEW. */
			virtual IAllocator * configure_allocator( IAllocator * i_new_allocator ) const;
		};
		
					///// aligned allocations /////
		
		/** Allocates a new memory block, respecting the requested alignment with an offset from the beginning of the block.
			Anyway the first byte of the block is aligned such that it can be used to store any integral or pointer variable.
			If the allocation fails nullptr is returned. If the request size is zero the return value is a non-null address 
			that points to a zero-sized memory block.
			The content of the newly allocated block is undefined. 
		  @param i_size size of the block in bytes
		  @param i_alignment alignment requested for the block. It must be an integer power of 2
		  @param i_alignment_offset offset from beginning of the block of the address that respects the alignment
		  @return the address of the first byte in the block, or nullptr if the allocation fails */
		virtual void * alloc( size_t i_size, size_t i_alignment, size_t i_alignment_offset ) = 0;

		/** Changes the size of the memory block allocated by alloc, possibly moving it in a new location.
			If the memory block can be resized, the same address is returned. Otherwise the block is 
			allocated in a new location, and the content is copied to the new location. Then the old 
			block is freed.
			The content of the memory block is preserved up to the lesser of the new size and the old 
			size, while the content of the newly allocated portion is undefined. 
			If the request size is zero the return value is a non-null address that points to a 
			zero-sized memory block.
			The alignment and the offset of the alignment of an existing memory block cannot be changed by realloc.
			If the reallocation fails nullptr is returned, and the memory block is left unchanged. 
		  @param i_address address of the memory block to reallocate. This parameter cannot be nullptr.
		  @param i_new_size new size of the block in bytes
		  @param i_alignment alignment requested for the block. It must be an integer power of 2
		  @param i_alignment_offset offset from the address that respects the alignment
		  @return the new address of the first byte in the block, or nullptr if the reallocation fails */
		virtual void * realloc( void * i_address, size_t i_new_size, size_t i_alignment, size_t i_alignment_offset ) = 0;

		/** Deallocates a memory block allocated by alloc or realloc.
		  @param i_address address of the memory block to free. It cannot be nullptr. */
		virtual void free( void * i_address ) = 0;

		/** Performs some checks on a new memory block allocated by alloc or realloc. The actual checks depend on the
			allocator and the configuration being compiled.
		  @param i_address address of the memory block to check */
		virtual void dbg_check( void * i_address ) = 0;




					///// unaligned allocations /////

		/** Allocates a new memory block. If the allocation fails nullptr is returned. If the request size is 
			zero the return value is a non-null address that points to a zero-sized memory block.
			The alignment of the newly allocated block allows to use it to store any integral or pointer variable.
			The content of the newly allocated block is undefined.  
		  @param i_size size of the block in bytes
		  @return the address of the first byte in the block, or nullptr if the allocation fails */
		virtual void * unaligned_alloc( size_t i_size ) = 0;

		/** Changes the size of the memory block allocated by unaligned_alloc, possibly moving it in a new location.
			If the memory block can be resized, the same address is returned. Otherwise the block is 
			allocated in a new location, and the content is copied to the new location. Then the old 
			block is freed.
			The content of the memory block is preserved up to the lesser of the new size and the old 
			size, while the content of the newly allocated portion is undefined. 
			If the request size is zero the return value is a non-null address that points to a 
			zero-sized memory block.
			If the reallocation fails nullptr is returned, and the memory block is left unchanged. 
		  @param i_address address of the memory block to reallocate. This parameter cannot be nullptr.
		  @param i_new_size new size of the block in bytes
		  @return the new address of the first byte in the block, or nullptr if the reallocation fails */
		virtual void * unaligned_realloc( void * i_address, size_t i_new_size ) = 0;

		/** Deallocates a memory block allocated by unaligned_alloc or unaligned_realloc.
		  @param i_address address of the memory block to free. It cannot be nullptr. */
		virtual void unaligned_free( void * i_address ) = 0;

		/** Performs some checks on a new memory block allocated by unaligned_alloc or unaligned_realloc. The actual checks
			depend on the allocator and the configuration being compiled.
		  @param i_address address of the memory block to check */
		virtual void unaligned_dbg_check( void * i_address ) = 0;

		/** virtual destructor */
		virtual ~IAllocator() { }


			//// Dump State ////

		/** Signature of the function to pass to dump_state. */
		class StateWriter
		{
		public:

			virtual void tab( const char * i_section_name ) = 0;

			virtual void untab() = 0;

			/** Writes a property */
			virtual void write( const char * i_property_name, const char * i_property_value ) = 0;

			/** writes an unsigned integer property */
			void write_uint( const char * i_property_name, size_t i_mem_size );

			/** writes a memory size property */
			void write_mem_size( const char * i_property_name, size_t i_mem_size );
		};

		/** Writes out in a human readable way the state of the allocator. This method is not for serialization, but 
			should instead print statistics meaningful for the user as a list of property's name-value pairs.
				@param i_write_function Function to call for every property
				@param i_user_param user parameter to be passed to i_write_function */
		virtual void dump_state( StateWriter & i_state_writer ) = 0;
	};

} // namespace memo
