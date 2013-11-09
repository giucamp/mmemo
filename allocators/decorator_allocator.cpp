

namespace memo
{
	// DecoratorAllocator::Config::constructor
	DecoratorAllocator::Config::Config()
		: m_target( nullptr )
	{

	}

	// DecoratorAllocator::Config::destructor
	DecoratorAllocator::Config::~Config()
	{
		if( m_target != nullptr )
		{
			MEMO_DELETE( m_target );
		}
	}

	// DecoratorAllocator::Config::configure_allocator
	IAllocator * DecoratorAllocator::Config::configure_allocator( IAllocator * i_new_allocator ) const
	{
		MEMO_ASSERT( i_new_allocator != nullptr ); /* DecoratorAllocator is an abstract class,
			so the allocator must be created by the derived class */

		IAllocator::Config::configure_allocator( i_new_allocator );

		DecoratorAllocator * this_allocator = static_cast< DecoratorAllocator *>( i_new_allocator );

		// create the target allocator
		MEMO_ASSERT( this_allocator->m_dest_allocator == nullptr );
		this_allocator->m_dest_allocator = m_target->create_allocator();

		return i_new_allocator;
	}

	// DecoratorAllocator::Config::try_recognize_property
	bool DecoratorAllocator::Config::try_recognize_property( serialization::IConfigReader & i_config_reader )
	{
		if( i_config_reader.try_recognize_property( "target" ) )
		{
			if( m_target != nullptr )
			{
				MEMO_DELETE( m_target );
				m_target = nullptr;
			}

			const char * target_allocator = i_config_reader.curr_property_vakue_as_string();
			m_target = MemoryManager::get_instance().allocator_config_factor().create_allocator_config( target_allocator );
			if( m_target == nullptr )
				i_config_reader.output_message( serialization::eWrongContent );
			else
			{
				i_config_reader.tab();

				m_target->load( i_config_reader );

				i_config_reader.untab();
			}

			return true;
		}	

		return false;
	}

	// DecoratorAllocator::dump_state
	void DecoratorAllocator::dump_state( StateWriter & i_state_writer )
	{
		if( m_dest_allocator != nullptr )
		{
			i_state_writer.tab( "allocator" );
			m_dest_allocator->dump_state( i_state_writer );
			i_state_writer.untab();
		}
		else
		{
			i_state_writer.write( "target", "null" );
		}
	}

} // namespace memo

