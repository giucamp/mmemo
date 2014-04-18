

namespace memo
{
	// http://www.codeproject.com/Articles/4795/C-Standard-StdAllocator-An-Introduction-and-Implement
	template <typename TYPE>
		class StdAllocator
	{
	public: 
		
		// typedefs
		typedef TYPE value_type;
		typedef value_type* pointer;
		typedef const value_type* const_pointer;
		typedef value_type& reference;
		typedef const value_type& const_reference;
		typedef std::size_t size_type;
		typedef std::ptrdiff_t difference_type;

	public:

		// convert an allocator<TYPE> to allocator<OTHER_TYPE>
		template<typename OTHER_TYPE> struct rebind 
		{
			typedef StdAllocator< OTHER_TYPE > other;
		};

		StdAllocator()																		{ }
		~StdAllocator()																		{ }
		StdAllocator( const StdAllocator & )												{ }
		template <typename OTHER_TYPE> StdAllocator( const StdAllocator<OTHER_TYPE> & )		{ }

		// address
		pointer address( reference i_reference )					{ return &i_reference; }
		const_pointer address( const_reference i_reference )		{ return &i_reference; }

		// allocate
		pointer allocate( size_type i_count, typename std::allocator<void>::const_pointer = nullptr )
		{ 
			void * new_block; 
			if( MEMO_ALIGNMENT_OF( TYPE ) > MEMO_MIN_ALIGNMENT ) 
				new_block = memo::alloc( i_count * sizeof(TYPE), MEMO_ALIGNMENT_OF( TYPE ), 0 );
			else
				new_block = memo::unaligned_alloc( i_count * sizeof(TYPE) );

			return static_cast< TYPE * >( new_block ); 
		}

		// deallocate
		void deallocate( pointer i_pointer, size_type i_count )
		{ 
			MEMO_UNUSED( i_count );
			if( i_pointer != nullptr )
			{
				if( MEMO_ALIGNMENT_OF( TYPE ) > MEMO_MIN_ALIGNMENT ) 
					memo::free( i_pointer );
				else
					memo::unaligned_free( i_pointer );
			}			
		}

		// construction/destruction
		void construct( pointer i_pointer, const TYPE & i_source )		{ new( i_pointer ) TYPE( i_source ); }
		void destroy( pointer i_pointer )								{ i_pointer->~TYPE(); 
																		  MEMO_UNUSED( i_pointer ); } // workaround for msc bug (Incorrect "unreferenced formal parameter" warning for explicit destruction)

		// size
		size_type max_size() const										{ return std::numeric_limits<size_type>::max() / sizeof(TYPE); }

		bool operator == ( const StdAllocator & )						{ return true; }
		bool operator != ( const StdAllocator & )						{ return false; }
	};

} // namespace memo
