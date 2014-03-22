
#include <map>

namespace memo
{
	struct _TestClass
	{
		double m_a, m_b;
		_TestClass() { m_a = get_hash(); m_b = get_hash() + 1.; }
		_TestClass( double i_number )  { m_a = i_number; m_a = get_hash(); m_b = get_hash() + 1.; }
		
		double get_hash() const { return static_cast<double>( reinterpret_cast<uintptr_t>(this) ); }

		~_TestClass()
		{
			double hash = get_hash();
			MEMO_ASSERT( m_a == hash && m_b == hash + 1. );
			m_a = hash * 44.f;
			m_b = 41.f;
		}
	};

	template <>	class AllocationDispatcher<_TestClass > : public memo::PoolDispatcher< _TestClass, 20 >	{ };

	void test_allocators()
	{
		/*output_integer( 2425 ); memo_externals::output_message( "\n" );
		output_mem_size( 5 );	memo_externals::output_message( "\n" );
		output_mem_size( 1024 * 6 ); memo_externals::output_message( "\n" );
		output_mem_size( 1024 * 900 ); memo_externals::output_message( "\n" );
		output_mem_size( 1024 * 1024 * 66 ); memo_externals::output_message( "\n" );
		output_mem_size( 1024 * 1024 * (1024u * 2u + 500u) ); memo_externals::output_message( "\n" );*/

		memo_externals::output_message( "testing allocators...\n" );
		const size_t iterations = 4700;

		// pool
		{
			memo_externals::output_message( "testing pool..." );
			{ _TestClass test; }
			for( size_t i = 0; i < iterations; i++ )
			{
				memo::std_vector<_TestClass*>::type objects;
				for( size_t j = 0; j < 337; j++ ) 
				{
					const uint32_t rand = generate_rand_32();
					if( (rand & 7) == 0 && objects.size() > 0 )
					{
						size_t index_to_remove = generate_rand_32() % objects.size();
						MEMO_DELETE( objects[index_to_remove] );
						objects.erase( objects.begin() + index_to_remove );
					}
					else if( rand & 1 )
						objects.push_back( MEMO_NEW( _TestClass ) );
					else
						objects.push_back( MEMO_NEW( _TestClass, 1. ) );
				}

				while( objects.size() > 0 )
				{
					size_t index_to_remove = generate_rand_32() % objects.size();
					MEMO_DELETE( objects[index_to_remove] );
					objects.erase( objects.begin() + index_to_remove );
				}
			}
			memo_externals::output_message( "done\n" );
		}	
		

		// queue
		{
			memo_externals::output_message( "testing FifoAllocator..." );
			memo::FifoAllocator::TestSession queue_stack_tester( 256 );
			for( int i = 0; i < iterations; i++ )
			{
				queue_stack_tester.fill_and_empty_test();
			}
		}
		{
			memo::FifoAllocator::TestSession queue_stack_tester( 5557 );
			for( int i = 0; i < iterations; i++ )
			{
				queue_stack_tester.fill_and_empty_test();
			}
			memo_externals::output_message( "done\n" );
		}

		// fifo allocator
		{
			memo_externals::output_message( "testing Queue..." );
			memo::Queue::TestSession fifo_tester;
			for( int i = 0; i < 13; i++ )
			{
				fifo_tester.fill_and_empty_test( iterations );
			}
			memo_externals::output_message( "done\n" );
		}	

		// corruption detector allocator
		{
			/*memo_externals::output_message( "testing CorruptionDetectorAllocator...\n\t" );
			CorruptionDetectorAllocator::Config corruption_detector_allocator_config;
			IAllocator * corruption_detector_allocator = corruption_detector_allocator_config.create_allocator();
			int * p = (int*)corruption_detector_allocator->unaligned_alloc( sizeof(int) );
			*p = 5;
			int g = p[-1];
			p[1] = 3;
			memo::AllocatorTester debug_allocator_tester( *corruption_detector_allocator );
			debug_allocator_tester.do_test_session( iterations );
			MEMO_DELETE( corruption_detector_allocator );*/
		}

		
		// default allocator
		{
			memo_externals::output_message( "testing DefaultAllocator...\n\t" );
			memo::AllocatorTester default_allocator_tester( memo::safe_get_default_allocator() );
			default_allocator_tester.do_test_session( iterations );
		}


		// debug allocator
		{
			memo_externals::output_message( "testing DebugAllocator->DefaultAllocator...\n\t" );
			DebugAllocator::Config debug_allocator_config;
			debug_allocator_config.m_target = MEMO_NEW( DefaultAllocator::Config );
			IAllocator * debug_allocator = debug_allocator_config.create_allocator();
			memo::AllocatorTester debug_allocator_tester( *debug_allocator );
			debug_allocator_tester.do_test_session( iterations );
			MEMO_DELETE( debug_allocator );
		}

		// statistics allocator
		{
			memo_externals::output_message( "testing StatAllocator->DefaultAllocator...\n\t" );
			StatAllocator::Config stat_allocator_config;
			stat_allocator_config.m_target = MEMO_NEW( DefaultAllocator::Config );
			IAllocator * stat_allocator = stat_allocator_config.create_allocator();
			memo::AllocatorTester stat_allocator_tester( *stat_allocator );
			stat_allocator_tester.do_test_session( iterations );
			MEMO_DELETE( stat_allocator );
		}

		// debug + statistics allocator
		{
			memo_externals::output_message( "testing DebugAllocator->StatAllocator->DefaultAllocator...\n\t" );			
			StatAllocator::Config * stat_allocator_config = MEMO_NEW( StatAllocator::Config );
			stat_allocator_config->m_target = MEMO_NEW( DefaultAllocator::Config );
			DebugAllocator::Config debug_allocator_config;
			debug_allocator_config.m_target = stat_allocator_config;
			IAllocator * stat_allocator = stat_allocator_config->create_allocator();
			memo::AllocatorTester stat_allocator_tester( *stat_allocator );
			stat_allocator_tester.do_test_session( iterations );
			MEMO_DELETE( stat_allocator );
		}

		// tlsf
		#if MEMO_ENABLE_TLSF
		{
			memo_externals::output_message( "testing TlsfAllocator...\n\t" );
			TlsfAllocator::Config tls_config;
			tls_config.m_buffer_size = 1024 * 1024;
			TlsfAllocator tls_allocator( tls_config );
			memo::AllocatorTester tls_tester( tls_allocator );
			tls_tester.do_test_session( 15000 );
		}

		// debug + statistics allocator
		{
			memo_externals::output_message( "testing DebugAllocator->StatAllocator->TlsfAllocator...\n\t" );
			TlsfAllocator::Config * tls_config = MEMO_NEW( TlsfAllocator::Config );
			tls_config->m_buffer_size = 1024 * 1024 * 4;
			StatAllocator::Config * stat_allocator_config = MEMO_NEW( StatAllocator::Config );
			stat_allocator_config->m_target = tls_config;
			DebugAllocator::Config debug_allocator_config;
			debug_allocator_config.m_target = stat_allocator_config;
			IAllocator * stat_allocator = stat_allocator_config->create_allocator();
			memo::AllocatorTester stat_allocator_tester( *stat_allocator );
			stat_allocator_tester.do_test_session( iterations );
			MEMO_DELETE( stat_allocator );
		}
		#endif

		// data stack
		{
			memo_externals::output_message( "testing LifoAllocator..." );
			memo::LifoAllocator::TestSession data_stack_tester( 5555 );
			for( int i = 0; i < 13; i++ )
			{
				data_stack_tester.fill_and_empty_test();
			}
			memo_externals::output_message( "done\n" );
		}

		// object stack
		{
			memo_externals::output_message( "testing ObjectLifoAllocator..." );
			memo::ObjectLifoAllocator::TestSession object_stack_tester( 5555 );
			for( int i = 0; i < 13; i++ )
			{
				object_stack_tester.fill_and_empty_test();
			}
			memo_externals::output_message( "done\n" );
		}	


	}

	
	class ContextTest
	{
	public:

		void walls()
		{
			static StaticName context_name( "walls" );
			memo::Context memory_context( context_name );

			for( int i = 0; i < 5000; i++ )
				m_strings[ rand() ] = "walls";
		}

		void decor()
		{
			static StaticName context_name( "decor" );
			memo::Context memory_context( context_name );

			for( int i = 0; i < 5000; i++ )
				m_strings[ rand() ] = "decor";
		}

		void kitchen()
		{
			static StaticName context_name( "kitchen" );
			memo::Context memory_context( context_name );

			for( int i = 0; i < 5000; i++ )
				m_strings[ rand() ] = "kit";
			
			//walls();
			decor();
		}

		void restroom()
		{
			static StaticName context_name( "restroom" );
			memo::Context memory_context( context_name );

			for( int i = 0; i < 5000; i++ )
				m_strings[ rand() ] = "rest";

			walls();
			//decor();
		}

		void livingroom()
		{
			static StaticName context_name( "livingroom" );
			memo::Context memory_context( context_name );

			for( int i = 0; i < 10000; i++ )
				m_strings[ rand() ] = "live";

			walls();
			decor();
		}

		class StateWriter : public IAllocator::StateWriter
		{
		public:

			virtual void tab( const char * i_section_name )
			{
				
				memo_externals::output_message( m_tabs.c_str() );
				memo_externals::output_message( "+" );
				memo_externals::output_message( i_section_name );
				memo_externals::output_message( "\n" );
				m_tabs += '\t';
			}

			virtual void untab()
			{
				m_tabs.pop_back();
			}

			/** Writes a property */
			virtual void write( const char * i_property_name, const char * i_property_value )
			{
				memo_externals::output_message( m_tabs.c_str() );
				memo_externals::output_message( i_property_name );
				memo_externals::output_message( ": " );
				memo_externals::output_message( i_property_value );
				memo_externals::output_message( "\n" );
			}

		private:
			std_string m_tabs;
		};

		void test_CorruptionDetectorAllocator()
		{
			static StaticName context_name( "corruption_test" );
			memo::Context memory_context( context_name );

			float * f = MEMO_NEW_ARRAY( float, 5 );
			for( int i = 0; i < 5; i++ )
				f[i] = 3.f;
			MEMO_DELETE_ARRAY( f );

			int * array = MEMO_NEW_ARRAY( int, 5 );
			for( int i = 1; i <= 5; i++ )
				array[i] = i;
			MEMO_DELETE_ARRAY( array );

			double * array_d = MEMO_NEW_ARRAY( double, 5 );
			for( int i = 1; i < 5; i++ )
				array_d[i] = i;
			MEMO_DELETE_ARRAY( array_d );
		}

		void test()
		{
			restroom();
			kitchen();
			livingroom();
			//test_CorruptionDetectorAllocator();

			StateWriter writer;
			NamePath path;
			MemoryManager::get_instance().dump_contexts( path, writer );
		}

	private:
		memo::std_map< int, memo::std_string >::type m_strings; 
	};
	

	void test()
	{
		test_allocators();

		/*ContextTest context_test;
		context_test.test();*/
		
		system( "PAUSE" );
	}



} // namespace memo
