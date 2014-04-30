

namespace std
{
	// forwards of std strings
	template < typename charT, typename TRAITS, typename ALLOCATOR > class basic_string;

	// forwards of std containers
	template < typename TYPE, class ALLOCATOR > class vector;
	template < typename TYPE, class ALLOCATOR > class list;
	template < typename TYPE, class ALLOCATOR > class queue;
	template < typename TYPE, class ALLOCATOR > class deque;
	template < typename KEY, typename VALUE, typename COMPARE, typename ALLOCATOR > class map;
	template < typename KEY, typename VALUE, typename COMPARE, typename ALLOCATOR > class multimap;
	template < typename VALUE, typename COMPARE, typename ALLOCATOR > class set;
	template < typename VALUE, typename COMPARE, typename ALLOCATOR > class multiset;
	template < typename KEY, typename VALUE, typename HASH, typename PRED, typename ALLOCATOR > class unordered_map;
	template < typename KEY, typename VALUE, typename HASH, typename PRED, typename ALLOCATOR > class unordered_multimap;
	template < typename KEY, typename HASH, typename PRED, typename ALLOCATOR > class unordered_set;
	template < typename KEY, typename HASH, typename PRED, typename ALLOCATOR > class unordered_multiset;

} // namespace std


namespace memo
{
	// std_string
	typedef std::basic_string< char, std::char_traits<char>, StdAllocator< char > > std_string;

	// std_wstring
	typedef std::basic_string< wchar_t, std::char_traits<wchar_t>, StdAllocator< wchar_t > > std_wstring;	
	
	// std_vector< TYPE >::type
	template <typename TYPE> struct std_vector
		{ typedef std::vector< TYPE, StdAllocator<TYPE> > type; };

	// std_list< TYPE >::type
	template <typename TYPE> struct std_list
		{ typedef std::list< TYPE, StdAllocator<TYPE> > type; };

	// std_queue< TYPE >::type
	template <typename TYPE> struct std_queue
		{ typedef std::queue< TYPE, StdAllocator<TYPE> > type; };

	// std_deque< TYPE >::type
	template <typename TYPE> struct std_deque
		{ typedef std::deque< TYPE, StdAllocator<TYPE> > type; };

	// std_map< KEY, VALUE >::type
	template <typename KEY, typename VALUE> struct std_map
		{ typedef std::map< KEY, VALUE, typename std::less< KEY >, typename StdAllocator< std::pair<KEY, VALUE> > > type; };

	// std_multimap< KEY, VALUE >::type
	template <typename KEY, typename VALUE> struct std_multimap
		{ typedef std::multimap< KEY, VALUE, std::less< KEY >, StdAllocator< std::pair<KEY, VALUE> > > type; };

	// std_set< VALUE >::type
	template <typename VALUE> struct std_set
		{ typedef std::set< VALUE, std::less< VALUE >, StdAllocator< VALUE > > type; };

	// std_multiset< VALUE >::type
	template <typename VALUE> struct std_multiset
		{ typedef std::multiset< VALUE, std::less< VALUE >, StdAllocator< VALUE > > type; };

	// std_unordered_map< KEY, VALUE, HASH = std::hash<KEY>, PREDICATE = std::equal_to<KEY> >::type
	template <typename KEY, typename VALUE, typename HASH = std::hash<KEY>, typename PREDICATE = std::equal_to<KEY> > struct std_unordered_map
		{ typedef std::unordered_map< KEY, VALUE, HASH, PREDICATE, StdAllocator< std::pair<KEY, VALUE> > > type; };

	// std_unordered_multimap< KEY, VALUE, HASH = std::hash<KEY>, PREDICATE = std::equal_to<KEY> >::type
	template <typename KEY, typename VALUE, typename HASH = std::hash<KEY>, typename PREDICATE = std::equal_to<KEY> > struct std_unordered_multimap
		{ typedef std::unordered_multimap< KEY, VALUE, HASH, PREDICATE, StdAllocator< std::pair<KEY, VALUE> > > type; };

	// std_unordered_set< VALUE, HASH = std::hash<KEY>, PREDICATE = std::equal_to<KEY> >::type
	template < typename VALUE, typename HASH = std::hash<KEY>, typename PREDICATE = std::equal_to<KEY> > struct std_unordered_set
		{ typedef std::unordered_set< VALUE, HASH, PREDICATE, StdAllocator< VALUE > > type; };

	// std_unordered_multiset< VALUE, HASH = std::hash<KEY>, PREDICATE = std::equal_to<KEY> >::type
	template < typename VALUE, typename HASH = std::hash<KEY>, typename PREDICATE = std::equal_to<KEY> > struct std_unordered_multiset
		{ typedef std::unordered_multiset< VALUE, HASH, PREDICATE, StdAllocator< VALUE > > type; };

	// std_priority_queue< VALUE, CONTAINER = std::vector<KEY>, PREDICATE = std::less<KEY> >::type
	template < typename VALUE, typename CONTAINER = memo::std_vector<VALUE>::type, typename PREDICATE = std::less< typename CONTAINER::value_type > > struct std_priority_queue
		{ typedef std::priority_queue< VALUE, CONTAINER, PREDICATE > type; };

} // namespace memo
