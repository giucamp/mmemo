
namespace memo
{
	ContextHash empty_hash = 5381;

	// hash( name, length )
	ContextHash compute_hash( const char * name, size_t length )
	{
		/*	djb2 - http://www.cse.yorku.ca/~oz/hash.html
		"this algorithm (k=33) was first reported by dan bernstein many years ago in comp.lang.c. another version of
		 this algorithm (now favored by bernstein) uses xor: hash(i) = hash(i - 1) * 33 ^ str[i]; the magic of number
		 33 (why it works better than many other constants, prime or not) has never been adequately explained." */

		/*Hash hash = 5381;
		int c;
		while (c = *str++)
			hash = ((hash << 5) + hash) + c; // hash * 33 + c
		REFLECTIVE_ASSERT( hash == result );*/

		uint32_t result = 5381;
		const char * str = name;
		if( length >= 4 ) do {
			result = (result << 5) + ( result + str[0] );
			result = (result << 5) + ( result + str[1] );
			result = (result << 5) + ( result + str[2] );
			result = (result << 5) + ( result + str[3] );
			str += 4;
			length -= 4;
		} while( length >= 4 );

		if( length ) do {
			result = (result << 5) + ( result + *str++ );
		} while( --length );

		return result;
	}

} // namespace memo