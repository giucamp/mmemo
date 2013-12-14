

namespace memo
{
	namespace serialization
	{
		void strcpy_safe( char * o_dest, size_t i_dest_buffer_length, const char * i_source )
		{
			if( i_dest_buffer_length > 0 )
			{
				char * curr_pos = o_dest;
				for( size_t index = 0; index < i_dest_buffer_length - 1; index++ )
				{
					o_dest[index] = i_source[index];
					if( o_dest[index] == 0 )
						return;
				}
				o_dest[i_dest_buffer_length-1] = 0;
			}
		}

		void strcat_safe( char * o_dest, size_t i_dest_buffer_length, const char * i_source )
		{
			const size_t existing_len = strlen( o_dest );
			MEMO_ASSERT( i_dest_buffer_length >= existing_len );
			if( i_dest_buffer_length > existing_len )
				strcpy_safe( o_dest + existing_len, i_dest_buffer_length - existing_len, i_source );
		}

		class ConfigReader : public serialization::IConfigReader
		{
		public:

			ConfigReader( const char * i_file_name ) 
				: m_exprected_depth( 0 ), m_current_depth(0), m_result( eSuccessful ), m_line_recognized( true )
			{
				m_file.open( i_file_name );

				if( m_file.fail() )
				{
					m_result = eCantOpenStream;
					strcpy_safe( m_property_name, memo_externals::g_max_config_line_length, "could not open the file" );
					strcpy_safe( m_property_value, memo_externals::g_max_config_line_length, i_file_name );
				}
			}

			~ConfigReader()
			{
				MEMO_ASSERT( m_file.eof() && m_line_recognized );
			}

			bool read_next_property()
			{
				if( m_result != eSuccessful )
					return false;

				if( !m_line_recognized )
					return m_exprected_depth == m_current_depth;

				char line[ memo_externals::g_max_config_line_length ];
				for(;;)
				{
					if( m_file.eof() )
						return false;
					m_file.getline( line, memo_externals::g_max_config_line_length );
					if( m_file.fail() )
					{
						m_result = eBadFormed;
						strcpy_safe( m_property_name, memo_externals::g_max_config_line_length, "could not read a line (was too long?)" );
						strcpy_safe( m_property_value, memo_externals::g_max_config_line_length, line );
						return false;
					}

					// fill m_property_name
					{
						// tabs
						const char * curr_pos = line;
						const char * const end = curr_pos + memo_externals::g_max_config_line_length;
						while( curr_pos < end && *curr_pos == '\t' )
							curr_pos++;
						m_current_depth = static_cast<int>( curr_pos - line );

						// white spaces
						while( curr_pos < end && isspace( *curr_pos ) )
							curr_pos++;

						// skip comments and blank lines
						if( curr_pos < end && ( *curr_pos == ';' || *curr_pos == 0 ) )
							continue;

						strcpy_safe( m_property_name, memo_externals::g_max_config_line_length, curr_pos );
					}

					// fill m_property_value
					{
						char * curr_pos = m_property_name;
						const char * const end = curr_pos + memo_externals::g_max_config_line_length;
						while( curr_pos < end && (isalnum(*curr_pos) || *curr_pos == '_' || *curr_pos == ' ') )
							curr_pos++;

						// white spaces
						while( curr_pos < end && isspace( *curr_pos ) )
						{
							*curr_pos = 0;
							curr_pos++;
						}

						// ':'
						if( curr_pos < end && *curr_pos == ':' )
						{
							*curr_pos = 0;
							curr_pos++;
						}
						else
						{
							m_result = eBadFormed;
							strcpy_safe( m_property_name, memo_externals::g_max_config_line_length, "expected ':'" );
							strcpy_safe( m_property_value, memo_externals::g_max_config_line_length, line );
							return false;
						}

						// white spaces
						while( curr_pos < end && isspace( *curr_pos ) )
							curr_pos++;

						// property value
						m_property_value[0] = 0;
						if( curr_pos < end )
							strcpy_safe( m_property_value, memo_externals::g_max_config_line_length, curr_pos );
						size_t length = strlen( m_property_value );
						while( length > 1 && isspace( m_property_value[length - 1] ) )
						{
							length--;
							m_property_value[length] = 0;
						}

						m_line_recognized = false;
						return m_exprected_depth == m_current_depth;
					}

				}
			}

			Result get_result( char * o_message, size_t i_message_buff_length )
			{
				if( m_result != eSuccessful )
				{
					strcpy_safe( o_message, i_message_buff_length, m_property_value );
					strcat_safe( o_message, i_message_buff_length, ": " );
					strcat_safe( o_message, i_message_buff_length, m_property_name );
				}
				else
				{
					if( i_message_buff_length > 0 )
						*o_message = 0;
				}
				return m_result;
			}

			void tab()
			{
				m_exprected_depth++;
			}

			void untab()
			{
				MEMO_ASSERT( m_exprected_depth > 0 );
				m_exprected_depth--;
			}

			bool try_recognize_property( const char * i_property_name )
			{
				if( m_result == eSuccessful && m_exprected_depth == m_current_depth &&
					strcmp( m_property_name, i_property_name ) == 0 )
				{
					m_line_recognized = true;
					return true;
				}
				else
					return false;
			}

			const char * curr_property_vakue_as_string() const
			{
				return m_property_value;
			}

			bool curr_property_vakue_as_uint( size_t * o_value ) const
			{
				double value = 0.;
				const char * curr_char = m_property_value;
				
				while( isspace(*curr_char) )
					curr_char++;

				while( *curr_char >= '0' && *curr_char <= '9' )
				{
					value *= 10.;
					value += static_cast< double >( *curr_char - '0' );
					curr_char++;
				}

				if( *curr_char == '.' )
				{
					curr_char++;
					double div_by = 1.;
					while( *curr_char >= '0' && *curr_char <= '9' )
					{
						div_by *= 10.;
						value *= 10.;
						value += static_cast< double >( *curr_char - '0' );
						curr_char++;
					}

					value /= div_by;
				}

				while( isspace(*curr_char) )
					curr_char++;

				switch( *curr_char )
				{
					case 'k':
					case 'K':
						curr_char++;
						value *= 1024.;
						break;

					case 'm':
					case 'M':
						curr_char++;
						value *= (1024. * 1024.);
						break;

					case 'g':
					case 'G':
						curr_char++;
						value *= (1024. * 1024. * 1024.);
						break;

					case 't':
					case 'T':
						curr_char++;
						value *= (1024. * 1024. * 1024. * 1024.);
						break;
				}

				while( isspace(*curr_char) )
					curr_char++;

				*o_value = static_cast< size_t >( value );
				return true;
			}

			void output_message( Result i_result )
			{
				if( i_result == eUnrecognizedProperty )
				{
					memo_externals::output_message( "unrecognized property in the config file: " );
					memo_externals::output_message( m_property_name );
					memo_externals::output_message( "\n" );
					memo_externals::debug_break();
				}

				if( m_result == eSuccessful ) // only the first error is stored
				{
					m_result = i_result;

					if( i_result == eUnrecognizedProperty )
					{
						strcpy_safe( m_property_value, memo_externals::g_max_config_line_length, m_property_name );
						strcpy_safe( m_property_name, memo_externals::g_max_config_line_length, "unrecognized property in the config file" );
					}
				}
			}

		private:
			std::ifstream m_file;
			int m_exprected_depth, m_current_depth;
			Result m_result;
			bool m_line_recognized;
			char m_property_name[ memo_externals::g_max_config_line_length ];
			char m_property_value[ memo_externals::g_max_config_line_length ];
		};

		// load_config_file
		Result load_config_file( MemoryManager::Config & io_config, const char * i_file_name )
		{
			const size_t error_msg_len = 256;
			char error_msg[ error_msg_len ];

			ConfigReader reader( i_file_name );
			
			if( reader.get_result( error_msg, error_msg_len ) != eSuccessful )
			{
				// the memory configuration file is missing
				memo_externals::output_message( error_msg );
				memo_externals::output_message( "\n" );
				memo_externals::debug_break();
				return serialization::eCantOpenStream;
			}

			io_config.load( reader );

			if( reader.get_result( error_msg, error_msg_len ) != eSuccessful )
			{
				memo_externals::output_message( error_msg );
				memo_externals::output_message( "\n" );
				memo_externals::debug_break();
				return serialization::eBadFormed;
			}

			return serialization::eSuccessful;
		}

	} // namespace serialization

} // namespace memo
