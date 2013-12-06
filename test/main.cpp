

#include "main.h"

class Dog
{
public:
	Dog( const char * ) {}
};

struct INFO
{

};


const memo::StaticName g_graphics( "graphics" );

void load_archive( const char * i_file_name )
{
	memo::Context context( g_graphics );
	// ...
	//void * buffer = memo::alloc( buffer_length, buffer_alignment, 0/*offset*/ );
	// ...

}

const memo::StaticName g_zoo( "zoo" );
const memo::StaticName g_robots( "robots" );

void load_animals( const char * i_file_name )
{
	memo::Context context( g_zoo );

	load_archive( i_file_name );
}

void load_robots( const char * i_file_name )
{
	memo::Context context( g_robots );

	load_archive( i_file_name );
}

class Window
{
public:
	Window( const char * i_name, int i_width, int i_height )
	{

	}
};

size_t get_required_size()
{
	return 16;
}

void test_lifo()
{

	char str1[] = "abc";
	char str2[] = "123";

	size_t required_size = get_required_size();

	char * buffer = static_cast< char * >( memo::lifo_alloc( sizeof(char) * required_size, MEMO_ALIGNMENT_OF(char), 0, nullptr ) );

	strcpy( buffer, str1 );
	strcat( buffer, str2 );

	memo::lifo_free( buffer );

	
}

int main()
{
	memo::ThreadRoot root( "main" ); 
	memo::test();

	size_t buffer_length = 16, buffer_alignment = 4;
	void * buffer = memo::alloc( buffer_length, buffer_alignment, 0/*offset*/ );
	memo::free( buffer );

	Dog * bell = MEMO_NEW( Dog, "Bell" );
	MEMO_DELETE( bell );

	return 0;
}