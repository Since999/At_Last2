#define _WINSOCKAPI_
#include <Windows.h>
#include <DbgHelp.h>
#include "server.h"

#pragma comment(lib, "Dbghelp.lib")

LONG WINAPI ExceptionCallBack(EXCEPTION_POINTERS* exceptionInfo) {

	MINIDUMP_EXCEPTION_INFORMATION info = { 0 };
	info.ThreadId = ::GetCurrentThreadId(); // Threae ID ����
	info.ExceptionPointers = exceptionInfo; // Exception ���� ����
	info.ClientPointers = FALSE;

	std::wstring stemp(L"test.dmp");

	HANDLE hFile = CreateFile(stemp.c_str(), GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	// ������ ���� ������� ���� ���� ������ �����.
	MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpNormal, &info, NULL, NULL);

	cout << "Exception!" << endl;

	return 0L;
}

int main()
{
	SetUnhandledExceptionFilter(ExceptionCallBack);

	try {
		Server server;
		server.Update();
	}
	catch (Exception& ex)
	{
		cout << ex.what() << "\n";
	}
}