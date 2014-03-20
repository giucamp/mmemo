
namespace memo
{
	template <typename TYPE>
		class TypedPool
	{
	public:

		bool init( size_t i_object_count )
		{
			const UntypedPool::Config config( sizeof(TYPE), MEMO_ALIGNMENT_OF(TYPE), i_object_count );
			return m_pool.init( config );
		}
		
		void * alloc()								{ return m_pool.alloc(); }

		void * alloc_slot()							{ return m_pool.alloc_slot(); }

		void free( void * i_address )				{ m_pool.free( i_address ); }

		void free_slot( void * i_address )			{ m_pool.free_slot( i_address ); }

		TYPE * create()								{ return new( m_pool.alloc() ) TYPE; }

		TYPE * create( const TYPE & i_source )		{ return new( m_pool.alloc() ) TYPE( i_source ); }

		void destroy( TYPE * i_object )				{ i_object->~TYPE(); m_pool.free( i_object ); }

	private:
		UntypedPool m_pool;
 	};
}

