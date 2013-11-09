
namespace memo
{
	typedef uint32_t ContextHash;
	extern ContextHash empty_hash;
	ContextHash compute_hash( const char * i_name, size_t i_length );

	/** \class StaticName
		Stores a name as a string, and computes its hash. The storage of the 
		string must be handled by the user. */
	class StaticName
	{
	public:

		/** Initializes the name to an empty string */
		StaticName()
			: m_hash( empty_hash ), m_name( "" ) {  }

		/** Initializes the name to a null-terminated string */
		StaticName( const char * i_name )		
			: m_hash( compute_hash( i_name, strlen( i_name ) ) ), m_name( i_name ) { }

		/** Initializes the name to a string of a given length  */
		StaticName( const char * i_name, size_t i_length )		
			: m_hash( compute_hash( i_name, i_length ) ), m_name( i_name ) { }

		/** Initializes the name to a string literal */
		template < size_t NAME_LENGTH > StaticName( const char (&i_name)[ NAME_LENGTH ] )		
			: m_hash( compute_hash( i_name, LENGTH ) ), m_name( i_name ) { }

		/** Returns the hash of the string */
		ContextHash hash() const			{ return m_hash; }
		
		/** Returns the string */
		const char * name() const			{ return m_name; }

		// operators == and !=
		bool operator == ( const StaticName & i_source ) const 
			{ return m_hash == i_source.m_hash; }
		bool operator != ( const StaticName & i_source ) const 
			{ return m_hash != i_source.m_hash; }

		/** This struct can be used as hash evaluator for std containers */
		struct Hash
		{
			typedef StaticName argument_type;
			typedef ContextHash result_type;

			ContextHash operator() ( const StaticName & i_name ) const { return i_name.m_hash; }
		};

	private: // data members
		ContextHash m_hash;
		const char * m_name;
	};

	/** This class holds a path made of StaticName tokens.
		Actually this class just keep the hash of the path, and no string is maintained. */
	class NamePath
	{
	public:

		/** Initializes an empty path */
		NamePath() : m_hash( empty_hash ) { }

		/** Initializes a path with a string. Every token in the path must be separated by '/'. */
		explicit NamePath( const char * i_path )
			: m_hash( empty_hash ) 
		{ 
			append_path( i_path );
		}

		/** Appends to the path a new token */
		void append_context( const StaticName & i_name )
		{
			m_hash ^= i_name.hash();
			m_hash = (m_hash >> 3) | ( (m_hash & 8) << ( sizeof(m_hash)*8 - 3) );
		}

		/** Appends a string path. Every token in the path must be separated by '/'. */
		void append_path( const char * i_path )
		{
			const char * curr_token = i_path;
			do {
				const char * next_token = strchr( curr_token, '/' );
				const size_t token_length = next_token != nullptr ? (next_token - curr_token) : strlen( curr_token );

				const StaticName token( curr_token, token_length );

				append_context( token );

				curr_token = next_token;

			} while( curr_token != nullptr );
		}

		/** Returns the hash of the path */
		ContextHash hash() const			{ return m_hash; }

		// operators == and !=
		bool operator == ( const NamePath & i_source ) const 
			{ return m_hash == i_source.m_hash; }
		bool operator != ( const NamePath & i_source ) const 
			{ return m_hash != i_source.m_hash; }

		/** This struct can be used as hash evaluator for std containers */
		struct Hash
		{
			typedef NamePath argument_type;
			typedef ContextHash result_type;

			ContextHash operator() ( const NamePath & i_name ) const { return i_name.m_hash; }
		};

	private: // data members
		ContextHash m_hash;
	};

} // namespace memo
