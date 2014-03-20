

/*#define MEMO_ENABLE_POOL( TYPE, OBJECT_COUNT ) - WIP
	namespace memo
	{
		template <> inline void * _typed_alloc()
		{
			static TypedPool<TYPE> s_pool( OBJECT_COUNT );
		}
	}*/

#include "untyped_pool.h"
#include "typed_pool.h"
