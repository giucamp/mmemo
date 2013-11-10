
/*! \file memo_externals.h

    This headers contains:
		- definitions which requires non-standard or customizable features (mutex, thread local storage).
		- definitions to configure memo at compile-time (to enable asserts, tests, inlines).

	memo_externals.h is supposed to be customized by the user. An implementation file (for example memo_externals.cpp)
	may be added to implement the required functions.
*/

/** \def MEMO_ENABLE_ASSERT
	If this macro evaluates to non-zero, MEMO_ASSERT will check the parameter and break execution if it is false.
	Otherwise MEMO_ASSERT will expand to a null statement. */
#ifdef _DEBUG
	#define MEMO_ENABLE_ASSERT		(1==1)
#else
	#define MEMO_ENABLE_ASSERT		(1==0)
#endif

/** \def MEMO_ENABLE_ASSERT( bool_expr )
	Breaks the execution if the argument evaluates to false */
#if MEMO_ENABLE_ASSERT
	#define MEMO_ASSERT( expr )		if( !(expr) ) memo_externals::assert_failure( #expr ); else (void)0u
#else
	#define MEMO_ASSERT( expr )		(void)0u
#endif

/** \def MEMO_LIFO_ALLOC_DEBUG
	If this macro evaluates to non-zero, some extra debug check is enabled on memo::LifoAllocator and memo::DataStack */
#define MEMO_LIFO_ALLOC_DEBUG		MEMO_ENABLE_ASSERT

/** \def MEMO_ALIGNMENT_OF( Type )
	This macro should evaluate to an integer equal to the required alignment of the specified type */
#if defined( _MSC_VER )
	namespace memo_externals
	{
		template <bool IsAbstract, typename CLASS > struct _MSC_AlignmentOfImpl;
		template <typename CLASS> struct _MSC_AlignmentOfImpl<true, CLASS> { static const size_t alignment = 1; };
		template <typename CLASS> struct _MSC_AlignmentOfImpl<false, CLASS> { static const size_t alignment = __alignof( CLASS ); };
	}
	#define MEMO_ALIGNMENT_OF( ... )		( memo_externals::_MSC_AlignmentOfImpl<__is_abstract( __VA_ARGS__ ), __VA_ARGS__>::alignment )
#else
	#define MEMO_ALIGNMENT_OF( ... )		std::alignment_of< __VA_ARGS__ >::value
#endif

/** \def MEMO_MIN_ALIGNMENT 
	This macro should evaluate to an integer equal to the minimum required alignment for any allocated memory block.
	Any ::malloc implementation has a minimum alignment, suitable to store in the memory block any primitive type.
	When memo allocates an object of a known type (for example MEMO_NEW, or memo::StdAllocator), if the alignment
	required by the type is equal or less than MEMO_MIN_ALIGNMENT, the unaligned allocation functions are used. */
#define MEMO_MIN_ALIGNMENT			MEMO_ALIGNMENT_OF( void* )


/** \def MEMO_ENABLE_TEST 
	If this macro evaluates to non-zero, some classes with testing purpose are defined */
#define MEMO_ENABLE_TEST			(1)

/** \def MEMO_ENABLE_INLINE
	If this macro evaluates to non-zero, functions suitable for inline expansion are declared inline and defined in every 
	compilation unit that includes memo.h. If this macro evaluates to false, inline expansion is disabled, and many
	.inl sources are not included in memo.h (this should speedup the compilation). 
	In any case classes and function templates are defined in memo.h. */
#define MEMO_ENABLE_INLINE			(!MEMO_ENABLE_ASSERT)

/** \def MEMO_ENABLE_RVALUE_REFERENCE
	If this macro evaluates to non-zero, memo uses r-value references, that are are available from C++11 on.ù
	Otherwise classic references are used in place of r-value references. 
	Currently only the macro MEMO_NEW_ARRAY_SRC exploits r-value references. */
#define MEMO_ENABLE_RVALUE_REFERENCES	1

/** \def MEMO_UNUSED( var )
	This macro should avoid the compiler warning for the variable\parameter var to be not used */
#define MEMO_UNUSED( var )			&var

/** \def MEMO_ONLY_DEFAULT_ALLOCATOR */
#define MEMO_ONLY_DEFAULT_ALLOCATOR		(0)

/** \def MEMO_ENABLE_TLSF
	If this macro evaluates to non-zero, tlsf.c is include in the build, and TlsfAllocator is defined and implemented. */
#define MEMO_ENABLE_TLSF			(1)

namespace memo_externals
{
	/** Name of the memory configuration file. 
		The memory manager reads the configuration file during the initialization */
	const char g_config_file_name[] = "mem_config";

	static const size_t g_max_config_line_length = 4098;

	/** Floating point not-a-number */
	typedef int32_t FloatNanIntegerRepresentationType;
	static const FloatNanIntegerRepresentationType g_float_nan_integer_representation = 0x7FFFFFFF;

	/** This function is supposed to print a message (for example with printf, cout, OutputDebugString) */
	void output_message( const char * i_message );

	/** This function is called when the condition of a MEMO_ASSERT evaluates to false */
	void assert_failure( const char * i_condition );

	/**  This function is called when a debug error occurs. */
	void debug_break();

	// Mutex - non re-entrant mutex
	class Mutex
	{
	public:

		Mutex() : m_lock(0) {}

		~Mutex() { }

		void lock();

		void unlock();

	private:
		volatile uint32_t m_lock;
	};

	memo::IAllocator * get_current_thread_allocator();

	void set_current_thread_allocator( memo::IAllocator * i_allocator );

	memo::ThreadRoot * get_thread_root();

	void set_thread_root( memo::ThreadRoot * i_thread_context );

	void register_custom_allocators( memo::AllocatorConfigFactory & i_factory );

} // namespace memo_externals

