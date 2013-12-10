

#include "windows.h"
#include "stdio.h"
#include "debug_help.h"

void DebuggerLoop();

void Protect();

void Unprotect();

void NotifyAllocation( void * address, size_t size );

void NotifyDeallocation( void * address, size_t size );

DWORD HandleException( DEBUG_EVENT & debug_info );

DWORD BadAccess( DEBUG_EVENT & debug_info, bool write, void * address );

void PrintStackTrace( DWORD thread_id );

void DumpMemoryContent( void * i_address, uintptr_t i_unit );

void DumpCodeAround( void * i_address );

void SaveDump( DEBUG_EVENT & debug_info, MINIDUMP_TYPE i_type );


