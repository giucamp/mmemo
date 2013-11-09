

namespace memo
{
	namespace serialization
	{
		class IConfigReader
		{
		public:

			virtual bool read_next_property() = 0;

			virtual void tab() = 0;

			virtual void untab() = 0;

			virtual bool try_recognize_property( const char * i_property_name ) = 0;

			virtual const char * curr_property_vakue_as_string() const = 0;

			virtual bool curr_property_vakue_as_uint( size_t * o_value ) const = 0;

			virtual Result get_result( char * o_message, size_t i_message_buff_length ) = 0;

			virtual void output_message( Result i_result ) = 0;

			/** virtual destructor */
			virtual ~IConfigReader() { }
		};

		Result load_config_file( MemoryManager::Config & io_config, const char * i_file_name );

	} // namespace serialization

} //namespace memo

