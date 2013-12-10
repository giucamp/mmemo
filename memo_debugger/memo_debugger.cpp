

#include "memo_debugger.h"
#include <vector>

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
DebugHelp g_debug_help;

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
		printf_s( "ERROR: could not read the id of the target process from the commandline\n" );
		return;
	}

	if( !DebugActiveProcess( g_target_process_id ) )
	{
		printf_s( "ERROR: could not attach the process %d for debug\n", g_target_process_id );
		return;
	}

	g_target_process_handle = OpenProcess( PROCESS_ALL_ACCESS, FALSE, g_target_process_id );
	if( g_target_process_handle == NULL )
	{
		printf_s( "ERROR: could not open the process %d\n", g_target_process_id );
		return;
	}

	g_thread_ids.reserve( 64 );
	g_thread_handles.reserve( 64 );

	if( !g_debug_help.init(g_target_process_handle) )
	{
		printf_s( "ERROR: probably DbgHelp.dll is not available in your system, cannot access symbol database\n" );
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
	size_t start_index = reinterpret_cast<size_t>( address ) - reinterpret_cast<size_t>( g_buffer_address );
	size_t end_index = start_index + size;
	for( size_t index = start_index; index < end_index; index++ )
	{
		g_can_write[index] = true;
		g_can_read[index] = false;
	}
}

void NotifyDeallocation( void * address, size_t size )
{
	size_t start_index = reinterpret_cast<size_t>( address ) - reinterpret_cast<size_t>( g_buffer_address );
	size_t end_index = start_index + size;
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
				size_t index = reinterpret_cast<size_t>(address) - reinterpret_cast<size_t>(g_buffer_address);
				const bool write_access = exception_debug_info.ExceptionRecord.ExceptionInformation[0] != 0;
				if( write_access )
				{
					if( g_can_write[index] )
						g_can_read[index] = true;
					else
					{
						result = BadAccess( debug_info, write_access, address );
					}
				}
				else
				{
					if( !g_can_read[index] )
					{
						result = BadAccess( debug_info, write_access, address );						
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
			if( exception_debug_info.ExceptionRecord.NumberParameters == 2 )
			{
				g_buffer_address = (void*)exception_debug_info.ExceptionRecord.ExceptionInformation[0];
				g_buffer_size = exception_debug_info.ExceptionRecord.ExceptionInformation[1];
				g_buffer_end = (void*)(exception_debug_info.ExceptionRecord.ExceptionInformation[0] + exception_debug_info.ExceptionRecord.ExceptionInformation[1] );
				g_can_read.resize( g_buffer_size, false );
				g_can_write.resize( g_buffer_size, false );
				OpenThread( debug_info.dwThreadId );
				return DBG_CONTINUE;
			}
			else
			{
				printf_s( "ERROR: exception %x expects 2 parameters\n", EXC_SET_BUFFER );
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
				printf_s( "ERROR: exception %x expects 2 parameters\n", EXC_SET_BUFFER );
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
				printf_s( "ERROR: exception %x expects 2 parameters\n", EXC_SET_BUFFER );
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
			if( exception_debug_info.dwFirstChance )
				printf_s( "first chance exception: %x\n", exception_debug_info.ExceptionRecord.ExceptionCode );
			else
				printf_s( "second chance exception: %x\n", exception_debug_info.ExceptionRecord.ExceptionCode );
			PrintStackTrace( debug_info.dwThreadId );
			return DBG_EXCEPTION_NOT_HANDLED;
	}
}


void PrintStackTrace( DWORD thread_id )
{
	printf_s( "\n*** STACK TRACE ***\n" );

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
		printf_s( "%s0x%p %s (%d)\n", index == 0 ? "->" : "  ",
			stack_trace[index], address_info.function_name.c_str(), address_info.line );
	}
}

void DumpMemoryContent( void * i_address, uintptr_t i_unit )
{
	if( i_unit != 1 && i_unit != 2 && i_unit != 4 && i_unit != 8 )
	{
		printf_s( "DumpMemoryContent called with wrong i_unit: %d\n", i_unit );
		return;
	}

	printf_s( "\n*** MEMORY CONTENT AROUND 0x%p grouped by %d byte(s) ***\n", i_address, i_unit );

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
		
		can_read = valid && g_can_read[ curr_address - (uintptr_t)g_buffer_address ];
		can_write = valid && g_can_write[ curr_address - (uintptr_t)g_buffer_address ];

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
				printf_s( "%s0x%p: 0x%02x - %s\n", indicator, curr_address, buffer[0], access );
			else if( i_unit == 2 )
				printf_s( "%s0x%p: 0x%04x - %s\n", indicator, curr_address, *(__int16*)buffer, access );
			else if( i_unit == 4 )
				printf_s( "%s0x%p: 0x%08x - %s\n", indicator, curr_address, *(__int32*)buffer, access );
			else if( i_unit == 8 )
			{
				char str_num[256];
				_i64toa_s( *(__int64*)buffer, str_num, sizeof(str_num) / sizeof(char), 16 );
				printf_s( "%s0x%p: 0x%s - %s\n", indicator, curr_address, str_num, access );
			}
		}
		else
			printf_s( "%s0x%p - invalid\n", indicator, curr_address );
	}
}

void DumpCodeAround( void * i_address )
{
	printf_s( "\n*** SOURCE CODE AROUND 0x%p ***\n", i_address );


	std::string module_name;
	if( !g_debug_help.get_containing_module( (uintptr_t)i_address, module_name ) )
		printf_s( "could not find a module containing the address 0x%p\n", i_address );
	else
		printf_s( "module: %s\n", module_name.c_str() );

	DebugHelp::CodeAddressInfo info;
	g_debug_help.get_code_address_info( (uintptr_t)i_address, info );

	FILE * source_code_file = NULL;
	fopen_s( &source_code_file, info.source_file_name.c_str(), "r" );
	if( source_code_file == NULL )
	{
		printf_s( "could not open the source file: %s\n", info.source_file_name.c_str() );
	}
	else
	{
		printf_s( "source file: %s\n", info.source_file_name.c_str() );

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
					printf_s( "->%4d: %s\n", line_index, line );
				else
					printf_s( "  %4d: %s\n", line_index, line );
			}
			
			line_index++;
		}
		
		fclose( source_code_file );
	}
}

void SaveDump( DEBUG_EVENT & debug_info, MINIDUMP_TYPE i_type )
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
	bool result = g_debug_help.write_mini_dump( "memo_dump.dmp", MiniDumpWithFullMemory, info );
	if( result )
		printf_s( "dump saved\n" );
	else
		printf_s( "failed to rite a dump\n" );
}


DWORD BadAccess( DEBUG_EVENT & debug_info, bool write, void * address )
{
	printf_s( "\n\n\n *********** ERROR: " );
	if( write )
		printf_s( "writing unwritable address: 0x%p", address );
	else
		printf_s( "reading unreadable address: 0x%p", address );
	printf_s( " ***********\n" );

	// stack trace
	PrintStackTrace( debug_info.dwThreadId );

	// code
	HANDLE thread_handle = GetThreadHandle( debug_info.dwThreadId );
	CONTEXT context;
	ZeroMemory( &context, sizeof(context) );
	context.ContextFlags = CONTEXT_ALL;
	GetThreadContext( thread_handle, &context );
	DumpCodeAround( (void*)context.Eip );

	// memory content
	DumpMemoryContent( address, 4 );

	bool debugging = true;

	while( !g_detach )
	{
		printf_s( "\nType a command:\n" );
		printf_s( "  'd': detach the target process, to allow another debugger to attach to it\n" );
		printf_s( "  'r': resume the target process, ignoring the error\n" );
		printf_s( "  'q': resume the target process, and quit\n" );
		printf_s( "  'm': save a minidump\n" );
		printf_s( "  'c': save a complete dump\n" );
		printf_s( "  '1', '2', '4', '8': dump memory content around 0x%p\n", address );
		printf_s( ">> " );
		char command[1024] = "";
		gets_s( command );
		switch( command[0] )
		{
		case 'd':
			if( debugging )
			{
				printf_s( "detaching the process\n" );
				DebugActiveProcessStop( g_target_process_id );
				debugging = false;
			}
			else
				printf_s( "process not attached\n" );
			break;

		case 'r':
			if( !debugging )
			{
				printf_s( "attaching the process\n" );
				DebugActiveProcess( g_target_process_id );
				debugging = true;
			}
			printf_s( "resuming execution\n" );
			return DBG_CONTINUE;

		case 'q':
			printf_s( "resuming execution and quitting\n" );
			g_detach = true;			
			break;

		case 'm':
		{
			SaveDump( debug_info, MiniDumpNormal );
			continue;
		}

		case 'c':
		{
			SaveDump( debug_info, MiniDumpWithFullMemory );
			continue;
		}

		case '1':
		{
			DumpMemoryContent( address, 1 );
			continue;
		}

		case '2':
		{
			DumpMemoryContent( address, 2 );
			continue;
		}

		case '4':
		{
			DumpMemoryContent( address, 4);
			continue;
		}

		case '8':
		{
			DumpMemoryContent( address, 8 );
			continue;
		}
		
		default:
			printf_s( "unrecognized command: %s\n", command );
		}
	}	

	return DBG_CONTINUE;
}
