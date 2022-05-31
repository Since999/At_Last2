#define _WINSOCKAPI_
#include <Windows.h>
#include <DbgHelp.h>
#include "server.h"

#pragma comment(lib, "Dbghelp.lib")

LONG WINAPI ExceptionCallBack(EXCEPTION_POINTERS* exceptionInfo) {

	MINIDUMP_EXCEPTION_INFORMATION info = { 0 };
	info.ThreadId = ::GetCurrentThreadId(); // Threae ID 설정
	info.ExceptionPointers = exceptionInfo; // Exception 정보 설정
	info.ClientPointers = FALSE;

	std::wstring stemp(L"test.dmp");

	HANDLE hFile = CreateFile(stemp.c_str(), GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	// 위에서 받은 내용들을 토대로 덤프 파일을 만든다.
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