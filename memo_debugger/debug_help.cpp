
#include "debug_help.h"
#include "Strsafe.h"
#include "time.h"

// DebugHelp::constructor
DebugHelp::DebugHelp()
{
	m_module = NULL;
	m_curr_process_handle = NULL;
	m_p_sym_initialize = NULL;
	m_p_sym_cleanup = NULL;
	m_p_stack_walk64 = NULL;		
	m_p_function_table_access = NULL;
	m_p_sym_module_base = NULL;
	m_p_sym_from_address = NULL;
	m_p_get_line_from_address = NULL;
	m_enumerate_loaded_modulesW64 = NULL;
	m_load_module_ex = NULL;
	m_mini_dump_write_dump = NULL;

	m_initialized = false;

	ZeroMemory( &m_critical_section, sizeof( m_critical_section ) );
	InitializeCriticalSection( &m_critical_section );
}

// DebugHelp::destructor
DebugHelp::~DebugHelp()
{
	uninit();

	DeleteCriticalSection( &m_critical_section );
}

// DebugHelp::init
bool DebugHelp::init( HANDLE h_target_process )
{
	CriticalSectionLock lock( m_critical_section );

	if( !m_initialized )
	{
		m_initialized = true;

		m_module = LoadLibraryA( "DbgHelp.dll" );		
		if( m_module == NULL )
			return false;

		m_p_sym_initialize = (_SymInitialize)GetProcAddress( m_module, "SymInitialize" );
		m_p_sym_cleanup = (_SymCleanup)GetProcAddress( m_module, "SymCleanup" );
		m_p_stack_walk64 = (_StackWalk64)GetProcAddress( m_module, "StackWalk64" );
		m_p_function_table_access = (_SymFunctionTableAccess64)GetProcAddress( m_module, "SymFunctionTableAccess64" );
		m_p_sym_module_base = (_SymGetModuleBase64)GetProcAddress( m_module, "SymGetModuleBase64" );
		m_p_sym_from_address = (_SymFromAddr)GetProcAddress( m_module, "SymFromAddr" );
		m_p_get_line_from_address = (_SymGetLineFromAddr64)GetProcAddress( m_module, "SymGetLineFromAddr64" );	
		m_enumerate_loaded_modulesW64 = (_EnumerateLoadedModules64)GetProcAddress( m_module, "EnumerateLoadedModules64" );
		m_load_module_ex = (_SymLoadModuleEx)GetProcAddress( m_module, "SymLoadModuleEx" );
		m_mini_dump_write_dump = (_MiniDumpWriteDump)GetProcAddress( m_module, "MiniDumpWriteDump" );

		if( m_p_sym_initialize == NULL ||
			m_p_sym_cleanup == NULL ||
			m_p_stack_walk64 == NULL || 
			m_p_function_table_access == NULL ||
			m_p_sym_module_base == NULL ||
			m_p_sym_from_address == NULL ||
			m_p_get_line_from_address == NULL ||
			m_load_module_ex == NULL ||
			m_mini_dump_write_dump == NULL )
		{
			uninit();
			return false;
		}	
		m_curr_process_handle = h_target_process; // OpenProcess( PROCESS_ALL_ACCESS, FALSE, GetCurrentProcessId() );
		BOOL result = (*m_p_sym_initialize)( m_curr_process_handle, NULL, TRUE );
		if( !result )
		{
			uninit();
			return false;
		}

	}

	return true;
}

// DebugHelp::uninit
void DebugHelp::uninit()
{
	CriticalSectionLock lock( m_critical_section );

	if( m_initialized )
	{
		(*m_p_sym_cleanup)( m_curr_process_handle );

		if( m_module != NULL )
			FreeLibrary( m_module );
		m_module = NULL;

		CloseHandle( m_curr_process_handle );

		m_p_sym_initialize = NULL;
		m_p_sym_cleanup = NULL;
		m_p_stack_walk64 = NULL;		
		m_p_function_table_access = NULL;
		m_p_sym_module_base = NULL;
		m_p_sym_from_address = NULL;
		m_p_get_line_from_address = NULL;
		m_load_module_ex = NULL;
		m_mini_dump_write_dump = NULL;

		m_initialized = false;
	}
}

// DebugHelp::capture_call_stack_trace
size_t DebugHelp::capture_call_stack_trace( HANDLE i_process_handle, HANDLE i_thread_handle, const CONTEXT & i_thread_context, 
	uintptr_t * out_entries, size_t i_max_entry_count )
{
	// stack frame
	STACKFRAME64 stack_frame;
	ZeroMemory( &stack_frame, sizeof( stack_frame ) );
	DWORD MachineType = IMAGE_FILE_MACHINE_I386;

	CONTEXT thread_context( i_thread_context );

	MachineType                 = IMAGE_FILE_MACHINE_I386;
	stack_frame.AddrPC.Offset    = thread_context.Eip;
	stack_frame.AddrPC.Mode      = AddrModeFlat;
	stack_frame.AddrFrame.Offset = thread_context.Ebp;
	stack_frame.AddrFrame.Mode   = AddrModeFlat;
	stack_frame.AddrStack.Offset = thread_context.Esp;
	stack_frame.AddrStack.Mode   = AddrModeFlat;
	void * context = (MachineType == IMAGE_FILE_MACHINE_I386) ? NULL : &thread_context;
	

	size_t size = 0;
	while( size < i_max_entry_count )
	{
		BOOL result = (*m_p_stack_walk64)( MachineType, i_process_handle, i_thread_handle,
			&stack_frame, context, 
			NULL, m_p_function_table_access, m_p_sym_module_base, NULL );
		if( !result || !stack_frame.AddrPC.Offset )
			break;

		if( size < i_max_entry_count )
			out_entries[ size ] = (uintptr_t)stack_frame.AddrPC.Offset;
		size++;
	}

	return size;
}

// DebugHelp::EnumerateLoadedModulesProc64
BOOL CALLBACK DebugHelp::EnumerateLoadedModulesProc64(
	_In_      PCTSTR ModuleName, _In_      DWORD64 ModuleBase,
	_In_      ULONG ModuleSize, _In_opt_  PVOID UserContext )
{
	std::vector< ModuleInfo > & io_dest = *static_cast< std::vector< ModuleInfo >* >( UserContext );
	
	ModuleInfo info;
	info.m_module_name = ModuleName;
	info.m_module_base = ModuleBase;
	info.m_module_size = ModuleSize;
	io_dest.push_back( info );

	return TRUE;
}

// DebugHelp::get_module_infos
bool DebugHelp::get_module_infos( HANDLE i_process_handle, std::vector< ModuleInfo > & io_dest )
{
	return (*m_enumerate_loaded_modulesW64)( i_process_handle, &EnumerateLoadedModulesProc64, &io_dest ) != FALSE;
}

bool DebugHelp::get_containing_module( uintptr_t i_address, std::string & o_dest )
{
	std::vector< ModuleInfo > modules;
	if( !get_module_infos( m_curr_process_handle, modules ) )
		return false;

	for( size_t index = 0; index < modules.size(); index++ )
	{
		const ModuleInfo & module_info = modules[index];
		if( i_address >= module_info.m_module_base && i_address < module_info.m_module_base + module_info.m_module_size )
		{
			o_dest = module_info.m_module_name;
			return true;
		}
	}
	return false;
}


// DebugHelp::print_last_error
void DebugHelp::print_last_error() 
{ 
	// Retrieve the system error message for the last-error code

	LPVOID lpMsgBuf;
	DWORD dw = GetLastError(); 

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dw,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0, NULL );

	printf( "Last Error: %s\n", (LPCTSTR)lpMsgBuf ); 

	LocalFree(lpMsgBuf);
}

// DebugHelp::get_code_address_info
bool DebugHelp::get_code_address_info( uintptr_t i_address, DebugHelp::CodeAddressInfo & io_dest_info )
{		
	const size_t symbol_info_chars = (sizeof( SYMBOL_INFO ) + sizeof( char ) - 1 ) / sizeof( char );

	const size_t max_name_length = 1024;

	char buffer[ symbol_info_chars + max_name_length ];
	ZeroMemory( &buffer, sizeof( buffer ) );

	SYMBOL_INFO & symbol_info = *(SYMBOL_INFO*)buffer;

	symbol_info.SizeOfStruct = sizeof( symbol_info );
	symbol_info.MaxNameLen = max_name_length;

	DWORD64 displacement = 0;
	BOOL result;
	result = (*m_p_sym_from_address)( 
		m_curr_process_handle, DWORD64(i_address), &displacement, &symbol_info );
	if( !result )
	{
		DWORD error = GetLastError();
		printf("SymFromAddr returned error : %d\n", error);
		print_last_error();
		io_dest_info.function_name = "";
		io_dest_info.line = -1;
		io_dest_info.source_file_name = "";
		return false;
	}

	io_dest_info.function_name = symbol_info.Name;

	DWORD other_dispalacement = 0;
	IMAGEHLP_LINE64 line_info;
	ZeroMemory( &line_info, sizeof( line_info ) );
	line_info.SizeOfStruct = sizeof( line_info );
	result = (*m_p_get_line_from_address)(
		m_curr_process_handle, i_address, &other_dispalacement, &line_info );
	if( !result )
		return false;
		
	io_dest_info.line = line_info.LineNumber;
	io_dest_info.source_file_name = line_info.FileName;

	return true;
}

bool DebugHelp::write_mini_dump( const char * i_file_name, MINIDUMP_TYPE i_type, 
	const MINIDUMP_EXCEPTION_INFORMATION i_dump_info )
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
			printf("failed to pick a valid filename for the dump: %s\n", i_file_name );
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

	DWORD processId = GetProcessId( m_curr_process_handle );

	MINIDUMP_EXCEPTION_INFORMATION dump_info( i_dump_info );

	const BOOL result = (*m_mini_dump_write_dump)( m_curr_process_handle, processId, file, i_type, 
		&dump_info, NULL, NULL );

	CloseHandle( file );

	if( result != 0 )
	{
		printf("dump successfully saved: %s\n", file_name.c_str() );
		return true;
	}
	else
		return false;
}