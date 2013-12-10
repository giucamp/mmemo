
#include "windows.h"
#include "Dbghelp.h"
#include <string>
#include <vector>

class DebugHelp
{
public:

	DebugHelp();

	bool init( HANDLE h_target_process );

	void uninit();

	~DebugHelp();

	/* capture_call_stack_trace - writes the callstack trace to out_entries - the return 
		value is the length of the calling callstack (which can be greater than max_entry_count). */
	size_t capture_call_stack_trace( 
		HANDLE i_process_handle, HANDLE i_thread_handle, const CONTEXT & i_thread_context, 
		uintptr_t * out_entries, size_t i_max_entry_count );

	struct ModuleInfo
	{
		std::string m_module_name;
		DWORD64 m_module_base;
		ULONG m_module_size;
	};
	bool get_module_infos( HANDLE i_process_handle, std::vector< ModuleInfo > & io_dest );
	bool get_containing_module( uintptr_t i_address, std::string & o_dest );

	struct CodeAddressInfo
	{
		std::string function_name;
		std::string source_file_name;
		int line;
	};
	bool get_code_address_info( uintptr_t i_address, CodeAddressInfo & io_dest_info );

	bool write_mini_dump( const char * i_file_name, MINIDUMP_TYPE i_type, 
		const MINIDUMP_EXCEPTION_INFORMATION i_dump_info );

private: // data members

	typedef BOOL ( WINAPI * _StackWalk64 )(
		__in      DWORD MachineType,
		__in      HANDLE hProcess,
		__in      HANDLE hThread,
		__inout   LPSTACKFRAME64 StackFrame,
		__inout   PVOID ContextRecord,
		__in_opt  PREAD_PROCESS_MEMORY_ROUTINE64 ReadMemoryRoutine,
		__in_opt  PFUNCTION_TABLE_ACCESS_ROUTINE64 FunctionTableAccessRoutine,
		__in_opt  PGET_MODULE_BASE_ROUTINE64 GetModuleBaseRoutine,
		__in_opt  PTRANSLATE_ADDRESS_ROUTINE64 TranslateAddress );

	typedef PVOID ( WINAPI * _SymFunctionTableAccess64 )(
		__in  HANDLE hProcess,
		__in  DWORD64 AddrBase );

	typedef DWORD64 ( WINAPI * _SymGetModuleBase64 )(
		__in  HANDLE hProcess,
		__in  DWORD64 dwAddr );

	typedef BOOL ( WINAPI * _SymInitialize )(
		__in      HANDLE hProcess,
		__in_opt  PCTSTR UserSearchPath,
		__in      BOOL fInvadeProcess );

	typedef BOOL ( WINAPI * _SymCleanup )(
		__in  HANDLE hProcess );

	typedef BOOL ( WINAPI * _SymFromAddr )(
		__in       HANDLE hProcess,
		__in       DWORD64 uintptr_t,
		__out_opt  PDWORD64 Displacement,
		__inout    PSYMBOL_INFO Symbol );

	typedef BOOL ( WINAPI * _SymGetLineFromAddr64 )(
		__in   HANDLE hProcess,
		__in   DWORD64 dwAddr,
		__out  PDWORD pdwDisplacement,
		__out  PIMAGEHLP_LINE64 Line );

	typedef BOOL ( WINAPI * _EnumerateLoadedModules64)(
		__in HANDLE hProcess,
		__in PENUMLOADED_MODULES_CALLBACK64 EnumLoadedModulesCallback,
		__in_opt PVOID UserContext );

	typedef DWORD64 ( WINAPI * _SymLoadModuleEx )(
		__in HANDLE hProcess, __in_opt HANDLE hFile, __in_opt PCSTR ImageName,
		__in_opt PCSTR ModuleName, __in DWORD64 BaseOfDll, __in DWORD DllSize,
		__in_opt PMODLOAD_DATA Data, __in_opt DWORD Flags );

	typedef BOOL ( WINAPI * _MiniDumpWriteDump)(
		__in HANDLE hProcess,
		__in DWORD ProcessId,
		__in HANDLE hFile,
		__in MINIDUMP_TYPE DumpType,
		__in_opt PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
		__in_opt PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
		__in_opt PMINIDUMP_CALLBACK_INFORMATION CallbackParam );

	HANDLE m_curr_process_handle;
	HMODULE m_module;
	_SymInitialize m_p_sym_initialize;
	_StackWalk64 m_p_stack_walk64;		
	_SymFunctionTableAccess64 m_p_function_table_access;
	_SymGetModuleBase64 m_p_sym_module_base;
	_SymCleanup m_p_sym_cleanup;
	_SymFromAddr m_p_sym_from_address;
	_SymGetLineFromAddr64 m_p_get_line_from_address;
	_EnumerateLoadedModules64 m_enumerate_loaded_modulesW64; 
	_SymLoadModuleEx m_load_module_ex; 
	_MiniDumpWriteDump m_mini_dump_write_dump;

	static BOOL CALLBACK EnumerateLoadedModulesProc64(
		_In_      PCTSTR ModuleName, _In_      DWORD64 ModuleBase,
		_In_      ULONG ModuleSize, _In_opt_  PVOID UserContext );

	void print_last_error();
		
	bool m_initialized;
	
	struct CriticalSectionLock
	{
	public:
		CriticalSectionLock( CRITICAL_SECTION & i_critical_section ) : m_critical_section( &i_critical_section )
			{ EnterCriticalSection( &i_critical_section ); }
		~CriticalSectionLock()
			{ LeaveCriticalSection( m_critical_section ); }
	private:
		CRITICAL_SECTION * m_critical_section;
	};

	CRITICAL_SECTION m_critical_section;
};
