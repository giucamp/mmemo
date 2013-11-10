

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