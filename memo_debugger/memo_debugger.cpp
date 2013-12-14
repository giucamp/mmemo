

#include "memo_debugger.h"
#include <vector>
#include <iostream>
#include <sstream>

const DWORD EXC_SET_BUFFER = 0xCC9B7983;
const DWORD EXC_ALLOCATION = 0xCC9B7984;
const DWORD EXC_DEALLOCATION = 0xCC9B7985;
const DWORD EXC_STOP_DEBUGGING = 0xCC9B7986;
const DWORD EXC_PROTECT = 0xCC9B7987;
const DWORD EXC_UNPROTECT = 0xCC9B7988;

DWORD g_target_process_id = 0;
HANDLE g_target_process_handle = INVALID_HANDLE_VALUE;
void * g_buffer_address = NULL;
void * g_buffer_end = NULL;
size_t g_buffer_size = NULL;
bool g_detach = false;
std::vector<bool> g_can_read;
std::vector<bool> g_can_write;
std::vector<DWORD> g_thread_ids;
std::vector<HANDLE> g_thread_handles;
DWORD g_granularity = 2;
DebugHelp g_debug_help;
std::ostringstream g_output;
			
void Print( const char * i_format, ... )
{
	const size_t buffer_size = 2048;
	char buffer[ buffer_size ];

	va_list args;
	va_start( args, i_format );
	vprintf( i_format, args );
	vsprintf_s( buffer, i_format, args );
	va_end( args );

	g_output << buffer;
}

HANDLE OpenThread( DWORD thread_id )
{
	HANDLE thread_handle = OpenThread( THREAD_ALL_ACCESS, FALSE, thread_id );
	g_thread_ids.push_back( thread_id );
	g_thread_handles.push_back( thread_handle );
	return thread_handle;
}

HANDLE GetThreadHandle( DWORD thread_id )
{
	size_t index = 0;
	for(;;)
	{
		if( index >= g_thread_ids.size() )
		{
			return OpenThread( thread_id );
		}

		if( g_thread_ids[index] == thread_id )
		{
			return g_thread_handles[index];
		}

		index++;
	}
}


void main()
{	
	const char * command_line = GetCommandLine();

	if( sscanf_s( command_line, "%d", &g_target_process_id ) != 1 )
	{
		Print( "ERROR: could not read the id of the target process from the commandline\n" );
		return;
	}

	if( !DebugActiveProcess( g_target_process_id ) )
	{
		Print( "ERROR: could not attach the process %d for debug\n", g_target_process_id );
		return;
	}

	g_target_process_handle = OpenProcess( PROCESS_ALL_ACCESS, FALSE, g_target_process_id );
	if( g_target_process_handle == NULL )
	{
		Print( "ERROR: could not open the process %d\n", g_target_process_id );
		return;
	}

	g_thread_ids.reserve( 64 );
	g_thread_handles.reserve( 64 );

	if( !g_debug_help.init(g_target_process_handle) )
	{
		Print( "ERROR: probably DbgHelp.dll is not available in your system, cannot access symbol database\n" );
	}

	Protect();

	DebuggerLoop();

	Unprotect();

	DebugActiveProcessStop( g_target_process_id );

	CloseHandle( g_target_process_handle );
}

void Protect()
{
	DWORD old_protect;
	VirtualProtectEx( g_target_process_handle, g_buffer_address, g_buffer_size, PAGE_READWRITE | PAGE_GUARD, &old_protect );
}

void Unprotect()
{
	DWORD old_protect;
	VirtualProtectEx( g_target_process_handle, g_buffer_address, g_buffer_size, PAGE_READWRITE, &old_protect );
}

void NotifyAllocation( void * address, size_t size )
{
	size_t start_index = (reinterpret_cast<size_t>( address ) - reinterpret_cast<size_t>( g_buffer_address )) >> g_granularity;
	size_t end_index = start_index + (size >> g_granularity);
	for( size_t index = start_index; index < end_index; index++ )
	{
		g_can_write[index] = true;
		g_can_read[index] = false;
	}
}

void NotifyDeallocation( void * address, size_t size )
{
	size_t start_index = (reinterpret_cast<size_t>( address ) - reinterpret_cast<size_t>( g_buffer_address )) >> g_granularity;
	size_t end_index = start_index + (size >> g_granularity);
	for( size_t index = start_index; index < end_index; index++ )
	{
		g_can_write[index] = false;
		g_can_read[index] = false;
	}
}

void DebuggerLoop()
{
	DEBUG_EVENT debug_event;
	ZeroMemory( &debug_event, sizeof(debug_event) );

	while( !g_detach )
	{
		DWORD dwContinueStatus = DBG_CONTINUE;

		WaitForDebugEvent( &debug_event, INFINITE );
		switch( debug_event.dwDebugEventCode )
		{
		case EXCEPTION_DEBUG_EVENT:
			dwContinueStatus = HandleException( debug_event );
			break;

		default:
			break;
		}

		ContinueDebugEvent( debug_event.dwProcessId, 
			debug_event.dwThreadId, 
			dwContinueStatus);
	}
}


DWORD HandleException( DEBUG_EVENT & debug_info )
{
	EXCEPTION_DEBUG_INFO & exception_debug_info = debug_info.u.Exception;
	
	switch( exception_debug_info.ExceptionRecord.ExceptionCode )
	{
		case EXCEPTION_SINGLE_STEP:
		{
			Protect();
			return DBG_CONTINUE;
		}
		
		case EXCEPTION_GUARD_PAGE:
		{
			Unprotect();
			
			DWORD result = DBG_CONTINUE;

			void * address = (void*)exception_debug_info.ExceptionRecord.ExceptionInformation[1];
			if( address >= g_buffer_address && address <= g_buffer_end )
			{
				size_t index = (reinterpret_cast<size_t>(address) - reinterpret_cast<size_t>(g_buffer_address)) >> g_granularity;
				const bool write_access = exception_debug_info.ExceptionRecord.ExceptionInformation[0] != 0;
				if( write_access )
				{
					if( g_can_write[index] )
						g_can_read[index] = true;
					else
					{
						result = BadAccess( debug_info, true, address );
					}
				}
				else
				{
					if( !g_can_read[index] )
					{
						result = BadAccess( debug_info, false, address );						
					}
				}
				if( !g_detach )
				{
					HANDLE thread_handle = GetThreadHandle( debug_info.dwThreadId );
					CONTEXT context;
					context.ContextFlags = CONTEXT_CONTROL;
					GetThreadContext( thread_handle, &context );
					context.EFlags |= 1 << 8;
					SetThreadContext( thread_handle, &context );
				}

			}
			return result;
		}

		case EXC_SET_BUFFER:
			if( exception_debug_info.ExceptionRecord.NumberParameters == 3 )
			{
				g_buffer_address = (void*)exception_debug_info.ExceptionRecord.ExceptionInformation[0];
				g_buffer_size = exception_debug_info.ExceptionRecord.ExceptionInformation[1];
				g_buffer_end = (void*)(exception_debug_info.ExceptionRecord.ExceptionInformation[0] + exception_debug_info.ExceptionRecord.ExceptionInformation[1] );
				g_granularity = (DWORD)exception_debug_info.ExceptionRecord.ExceptionInformation[2];
				g_can_read.resize( g_buffer_size >> g_granularity, false );
				g_can_write.resize( g_buffer_size >> g_granularity, false );
				OpenThread( debug_info.dwThreadId );
				return DBG_CONTINUE;
			}
			else
			{
				Print( "ERROR: exception %x expects 2 parameters\n", EXC_SET_BUFFER );
				return DBG_EXCEPTION_NOT_HANDLED;
			}

		case EXC_ALLOCATION:
			if( exception_debug_info.ExceptionRecord.NumberParameters == 2 )
			{
				void * address = (void*)exception_debug_info.ExceptionRecord.ExceptionInformation[0];
				size_t size = exception_debug_info.ExceptionRecord.ExceptionInformation[1];
				NotifyAllocation( address, size );
				Protect();
				return DBG_CONTINUE;
			}
			else
			{
				Print( "ERROR: exception %x expects 2 parameters\n", EXC_SET_BUFFER );
				return DBG_EXCEPTION_NOT_HANDLED;
			}			

		case EXC_DEALLOCATION:
			if( exception_debug_info.ExceptionRecord.NumberParameters == 2 )
			{
				void * address = (void*)exception_debug_info.ExceptionRecord.ExceptionInformation[0];
				size_t size = exception_debug_info.ExceptionRecord.ExceptionInformation[1];
				NotifyDeallocation( address, size );
				Protect();
				return DBG_CONTINUE;
			}
			else
			{
				Print( "ERROR: exception %x expects 2 parameters\n", EXC_SET_BUFFER );
				return DBG_EXCEPTION_NOT_HANDLED;
			}			

		case EXC_STOP_DEBUGGING:
			g_detach = true;
			return DBG_CONTINUE;

		case EXC_PROTECT:
			Protect();
			return DBG_CONTINUE;

		case EXC_UNPROTECT:
			Unprotect();
			return DBG_CONTINUE;

		default:
			return HandleGenericException( debug_info );
	}
}

DWORD HandleGenericException( DEBUG_EVENT & debug_info )
{
	EXCEPTION_DEBUG_INFO & exception_debug_info = debug_info.u.Exception;
	const bool continuable = (exception_debug_info.ExceptionRecord.ExceptionFlags & EXCEPTION_NONCONTINUABLE) == 0;
	Print( "\n\n\n *********** %s %s chance exception: %x ***********\n\n",
		continuable ? "continuable" : "noncontinuable",
		exception_debug_info.dwFirstChance ? "first" : "second",
		exception_debug_info.ExceptionRecord.ExceptionCode );

	HANDLE thread_handle = GetThreadHandle( debug_info.dwThreadId );
	CONTEXT context;
	ZeroMemory( &context, sizeof(context) );
	context.ContextFlags = CONTEXT_ALL;
	GetThreadContext( thread_handle, &context );
	void * code_address = GetCodeAddress( context );

	DumpCodeAround( code_address );

	PrintStackTrace( debug_info.dwThreadId );

	if( !exception_debug_info.dwFirstChance && continuable )
	{				
		Print( "\nType a command:\n" );
		Print( "  continue: ignore and continue execution\n" );
		Print( "  quit: quit and let the system handle the exception\n" );

		for(;;)
		{
			std::string command, parameters;
			AskCommand( command, parameters );
			if( command == "continue" )
			{
				if( parameters.length() > 0 )
				{
					Print( "no parameters expected\n" );
					continue;
				}
				return DBG_CONTINUE;
			}
			else if( command == "quit" )
			{
				if( parameters.length() > 0 )
				{
					Print( "no parameters expected\n" );
					continue;
				}
				g_detach = true;
				return DBG_EXCEPTION_NOT_HANDLED;
			}
			else
			{
				Print( "unrecognized command: %s\n", command.c_str() );
			}
		}
	}
	return DBG_EXCEPTION_NOT_HANDLED;
}

void PrintStackTrace( DWORD thread_id )
{
	Print( "\n*** STACK TRACE ***\n" );

	HANDLE thread_handle = GetThreadHandle( thread_id );
	CONTEXT context;
	ZeroMemory( &context, sizeof(context) );
	context.ContextFlags = CONTEXT_ALL;
	GetThreadContext( thread_handle, &context );

	const size_t max_stack_size = 1024;
	uintptr_t stack_trace[ max_stack_size ];
	size_t stack_size = g_debug_help.capture_call_stack_trace( g_target_process_handle, thread_handle,
		context, stack_trace, max_stack_size );
	for( size_t index = 0; index < stack_size; index++ )
	{
		DebugHelp::CodeAddressInfo address_info;
		g_debug_help.get_code_address_info( stack_trace[index], address_info );
		Print( "%s0x%p %s (%d)\n", index == 0 ? "->" : "  ",
			stack_trace[index], address_info.function_name.c_str(), address_info.line );
	}
}

void DumpMemoryContent( void * i_address, uintptr_t i_unit )
{
	if( i_unit != 1 && i_unit != 2 && i_unit != 4 && i_unit != 8 )
	{
		Print( "DumpMemoryContent called with wrong i_unit: %d\n", i_unit );
		return;
	}

	Print( "\n*** MEMORY CONTENT AROUND 0x%p grouped by %d byte(s) ***\n", i_address, i_unit );

	uintptr_t range = 8 * i_unit;
	uintptr_t from = (uintptr_t)i_address;
	uintptr_t to = (uintptr_t)i_address;
	
	from -= range;
	to += range;

	unsigned char buffer[8];
	SIZE_T bytes_read = 0;

	for( uintptr_t curr_address = from; curr_address <= to; curr_address += i_unit )
	{
		bool valid = false, can_read = false, can_write = false;
		valid = curr_address >= (uintptr_t)g_buffer_address;
		valid = valid && curr_address < (uintptr_t)g_buffer_address + g_buffer_size;
		valid = valid && ReadProcessMemory( g_target_process_handle, (LPCVOID)curr_address, buffer, i_unit, &bytes_read );
		valid = valid && bytes_read >= i_unit;
		
		can_read = valid && g_can_read[ (curr_address - (uintptr_t)g_buffer_address) >> g_granularity ];
		can_write = valid && g_can_write[ (curr_address - (uintptr_t)g_buffer_address) >> g_granularity ];

		const char * indicator = "  ";
		if( curr_address == (uintptr_t)i_address )
			indicator = "->";

		if( valid )
		{
			const char * access = "not allocated";
			if( can_read && can_write )
				access = "readable/writable";
			else if( can_read )
				access = "readable";
			else if( can_write )
				access = "writable";
			if( i_unit == 1 )
				Print( "%s0x%p: 0x%02x - %s\n", indicator, curr_address, buffer[0], access );
			else if( i_unit == 2 )
				Print( "%s0x%p: 0x%04x - %s\n", indicator, curr_address, *(__int16*)buffer, access );
			else if( i_unit == 4 )
				Print( "%s0x%p: 0x%08x - %s\n", indicator, curr_address, *(__int32*)buffer, access );
			else if( i_unit == 8 )
			{
				char str_num[256];
				_i64toa_s( *(__int64*)buffer, str_num, sizeof(str_num) / sizeof(char), 16 );
				Print( "%s0x%p: 0x%s - %s\n", indicator, curr_address, str_num, access );
			}
		}
		else
			Print( "%s0x%p - invalid\n", indicator, curr_address );
	}
}

void DumpCodeAround( void * i_address )
{
	Print( "\n*** SOURCE CODE AROUND 0x%p ***\n", i_address );


	std::string module_name;
	if( !g_debug_help.get_containing_module( (uintptr_t)i_address, module_name ) )
		Print( "could not find a module containing the address 0x%p\n", i_address );
	else
		Print( "module: %s\n", module_name.c_str() );

	DebugHelp::CodeAddressInfo info;
	g_debug_help.get_code_address_info( (uintptr_t)i_address, info );

	FILE * source_code_file = NULL;
	fopen_s( &source_code_file, info.source_file_name.c_str(), "r" );
	if( source_code_file == NULL )
	{
		Print( "could not open the source file: %s\n", info.source_file_name.c_str() );
	}
	else
	{
		Print( "source file: %s\n", info.source_file_name.c_str() );

		int line_index = 1;
		const size_t max_line_length = 2048;
		char line[ max_line_length ];
		
		const int line_spread = 2;

		while( !feof(source_code_file) )
		{
			fgets( line, max_line_length, source_code_file );
			
			// remove tailing '\n' and '\r'
			size_t curr_pos = strlen(line);
			while( curr_pos > 0 && (line[curr_pos-1] == '\n' || line[curr_pos-1] == '\r') )
			{
				line[curr_pos-1] = 0;
				curr_pos--;
			}

			if( abs(line_index - info.line) <= line_spread )
			{
				if( line_index == info.line )
					Print( "->%4d: %s\n", line_index, line );
				else
					Print( "  %4d: %s\n", line_index, line );
			}
			
			line_index++;
		}
		
		fclose( source_code_file );
	}
}

void SaveDump( DEBUG_EVENT & debug_info, MINIDUMP_TYPE i_type, const char * i_file_name )
{
	HANDLE thread_handle = GetThreadHandle( debug_info.dwThreadId );
	CONTEXT context;
	ZeroMemory( &context, sizeof(context) );
	context.ContextFlags = CONTEXT_ALL;
	GetThreadContext( thread_handle, &context );

	MINIDUMP_EXCEPTION_INFORMATION info;
	ZeroMemory( &info, sizeof(info) );

	_EXCEPTION_POINTERS except_info;
	except_info.ContextRecord = &context;
	except_info.ExceptionRecord = &debug_info.u.Exception.ExceptionRecord;

	info.ExceptionPointers = &except_info;
	info.ClientPointers = FALSE;
	info.ThreadId = debug_info.dwThreadId;
	bool result = g_debug_help.write_mini_dump( i_file_name, MiniDumpWithFullMemory, info );
	if( result )
		Print( "dump saved\n" );
	else
		Print( "failed to rite a dump\n" );
}

void * GetCodeAddress( const CONTEXT & i_context )
{
	#if defined( _M_IX86 )
		return (void*)i_context.Eip;
	#elif defined( _M_X64 )
		return (void*)i_context.Rip;
	#else
		#error assign instruction pointer
	#endif
}

DWORD BadAccess( DEBUG_EVENT & debug_info, bool write, void * address )
{
	Print( "\n\n\n *********** ERROR: attempt to %s address 0x%p ***********\n", 
		write ? "write unwritable" : "read unreadable", address );

	// code
	HANDLE thread_handle = GetThreadHandle( debug_info.dwThreadId );
	CONTEXT context;
	ZeroMemory( &context, sizeof(context) );
	context.ContextFlags = CONTEXT_ALL;
	GetThreadContext( thread_handle, &context );	
	void * code_address = GetCodeAddress( context );

	// stack trace
	PrintStackTrace( debug_info.dwThreadId );

	DumpCodeAround( code_address );

	// memory content
	DumpMemoryContent( address, 4 );

	bool debugging = true;

	while( !g_detach )
	{
		std::string mini_dump_file_name, dump_file_name, output_file_name;
		if( !g_debug_help.get_containing_module( (uintptr_t)code_address, dump_file_name ) )
		{
			dump_file_name = "memo_dump.dmp";
			mini_dump_file_name = "memo_mini_dump.dmp";
			output_file_name = "memo_output.txt";
		}
		else
		{
			size_t char_pos;
			char_pos = dump_file_name.rfind('.');
			if( char_pos != std::string::npos )
				dump_file_name = dump_file_name.substr( 0, char_pos );
			char_pos = dump_file_name.find_last_of( "\\/" );
			if( char_pos != std::string::npos && char_pos + 2 < dump_file_name.length() )
				dump_file_name = dump_file_name.substr( char_pos + 1 );
			mini_dump_file_name = dump_file_name + "_memo_mini.dmp";
			output_file_name = dump_file_name + "_memo_output.txt";
			dump_file_name += "_memo.dmp";
		}

		Print( "\nType a command:\n" );
		Print( "  detach: detach the target process, to allow another debugger to attach to it\n" );
		Print( "  ignore: resume the target process, ignoring the error\n" );
		Print( "  quit: resume the target process, and quit\n" );
		Print( "  minidump [file=%s]: save a minidump\n", mini_dump_file_name.c_str() );
		Print( "  dump [file=%s]: save a complete dump\n", dump_file_name.c_str() );
		Print( "  mem [1|2|4|8]: dump memory content around 0x%p\n", address );
		Print( "  save [file=%s]: save all the output of this program\n", output_file_name.c_str() );
		Print( "  copy: copy all the output of this program into the clipboard\n" );

		std::string command, parameters;
		AskCommand( command, parameters );
		
		if( command == "detach" )
		{
			// detach
			if( parameters.length() > 0 )
			{
				Print( "no parameters expected\n" );
				continue;
			}
			if( debugging )
			{
				Print( "detaching the process\n" );
				DebugActiveProcessStop( g_target_process_id );
				debugging = false;
			}
			else
				Print( "process not attached\n" );
		}
		else if( command == "ignore" )
		{
			// ignore
			if( parameters.length() > 0 )
			{
				Print( "no parameters expected\n" );
				continue;
			}
			if( !debugging )
			{
				Print( "attaching the process\n" );
				DebugActiveProcess( g_target_process_id );
				debugging = true;
			}
			Print( "resuming execution\n" );
			return DBG_CONTINUE;
		}
		else if( command == "quit" )
		{
			// quit
			if( parameters.length() > 0 )
			{
				Print( "no parameters expected\n" );
				continue;
			}
			if( parameters.length() > 0 )
				Print( "no parameters expected\n" );
			else 
				Print( "resuming execution and quitting\n" );
				g_detach = true;	
		}
		else if( command == "minidump" )
		{
			// minidump
			if( parameters.length() > 0 )
				mini_dump_file_name = parameters;
			SaveDump( debug_info, MiniDumpNormal, mini_dump_file_name.c_str() );
			continue;
		}
		else if( command == "dump" )
		{
			// dump
			if( parameters.length() > 0 )
				dump_file_name = parameters;
			SaveDump( debug_info, MiniDumpWithFullMemory, dump_file_name.c_str() );
			continue;			
		}
		else if( command == "mem" )
		{
			// mem
			uintptr_t unit = 4;
			if( parameters.length() > 0 )
			{
				if( parameters == "1" )
					unit = 1;
				else if( parameters == "2" )
					unit = 2;
				else if( parameters == "4" )
					unit = 4;
				else if( parameters == "8" )
					unit = 8;
				else
				{
					Print( "parameter not recognized, expected 1, 2, 4, or 8\n" );
					continue;
				}
			}
			DumpMemoryContent( address, unit );
		}
		else if( command == "save" )
		{
			// save
			if( parameters.length() > 0 )
				output_file_name = parameters;
			SaveOuput( output_file_name.c_str() );
		}
		else if( command == "copy" )
		{
			// save
			if( parameters.length() > 0 )
			{
				Print( "no parameters expected\n" );
				continue;
			}
			CopyOuput();
		}
		else
		{
			Print( "unrecognized command: %s\n", command.c_str() );
		}
	}	

	return DBG_CONTINUE;
}

void FixNewlines( std::string & io_string )
{
	size_t start_from = 0;
	for(;;)
	{
		size_t pos = io_string.find( '\n', start_from );
		if( pos == std::string::npos )
			break;

		io_string = io_string.substr( 0, pos ) + "\r\n" + io_string.substr( pos + 1 ); 
		start_from = pos + 2;
	}
}

bool SaveOuput( const char * i_file_name )
{
	std::string file_name( i_file_name );
	HANDLE file;
	int tries = 0;
	for(;;)
	{
		file = CreateFile( file_name.c_str(), GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL );
		if( file != INVALID_HANDLE_VALUE )
			break;

		if( tries == 0 )
			srand( GetTickCount() );
		else if( tries > 100 )
		{
			printf("failed to pick a valid filename for the ouput: %s\n", i_file_name );
			return false;
		}

		tries++;

		char buffer[512];
		int random = rand();
		sprintf_s( buffer, "%x", random );

		const char * dot = strrchr( i_file_name, '.' );
		if( dot )
		{
			file_name = std::string( i_file_name, dot - i_file_name );
			file_name += '_';
			file_name += buffer;
			file_name += dot;
		}
		else
		{			
			file_name = i_file_name;
			file_name += '_';
			file_name += buffer;
		}
	}

	std::string string = g_output.str();
	FixNewlines( string );
	DWORD written = 0;
	DWORD to_write = (DWORD)string.length();
	BOOL result = WriteFile( file, string.c_str(), (DWORD)string.length(), &written, NULL );
	result = result && written == to_write;
	CloseHandle( file );

	if( result != 0 )
	{
		printf("dump successfully saved: %s\n", file_name.c_str() );
		return true;
	}
	else
	{
		printf("failed to write the file: %s\n", file_name.c_str() );
		return false;
	}
}

bool CopyOuput()
{
	std::string string = g_output.str();
	FixNewlines( string );

	bool result = false;

	if( !OpenClipboard( NULL ) )
	{
		Print("failed to open the clipboard\n" );
		return false;
	}

	HGLOBAL hMem = GlobalAlloc( GMEM_MOVEABLE, string.length() + 1 );
	if( !hMem )
	{
		Print("failed to allocate memory to copy to the clipboard\n" );
	}
	else
	{
		char * str_dest = (char*)GlobalLock(hMem);

		if( str_dest )
			strcpy_s( str_dest, string.length() + 1, string.c_str() );

		GlobalUnlock( hMem );

		if( str_dest )
		{
			EmptyClipboard();
			HANDLE hobj = SetClipboardData(CF_TEXT, hMem);
			result = hobj != NULL;
		}
	}

	CloseClipboard();
	return result;
}

void AskCommand( std::string & io_command, std::string & io_parameters )
{
	Print( ">> " );

	io_command.reserve( 16 );
	io_parameters.reserve( 16 );
	io_command.clear();
	io_parameters.clear();

	char line[1024*2] = "";
	gets_s( line );

	const char * curr_char = line;
	while( isspace(*curr_char) )
		curr_char++;
	while( isalnum(*curr_char) )
	{
		io_command += *curr_char;
		curr_char++;
	}
	while( isspace(*curr_char) )
		curr_char++;
	while( *curr_char )
	{
		io_parameters += *curr_char;
		curr_char++;
	}

	while( io_parameters.length() > 0 && isspace( io_parameters.back() ) )
		io_parameters.pop_back();

	if( io_parameters.length() )
		Print( "command: \"%s\" \"%s\"\n", io_command.c_str(), io_parameters.c_str() );
	else
		Print( "command: \"%s\"\n", io_command.c_str() );
}