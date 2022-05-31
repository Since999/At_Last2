#include "Exception.h"
#include "Protocol.h"

Exception::Exception(const char* errMsg)
{
	LPSTR msgBuffer;
	size_t size = FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPSTR)&msgBuffer, 0, NULL);

	errString = errMsg;
	errString += ": ";
	errString += msgBuffer;
}

const char* Exception::what() const
{
	return errString.c_str();
}