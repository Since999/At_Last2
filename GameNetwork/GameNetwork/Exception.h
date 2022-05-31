#pragma once
#include <exception>
#include <string>

class Exception : std::exception
{
private:
	std::string errString;
public:
	Exception(const char* errMsg);
	virtual ~Exception() {}
	virtual const char* what() const override;
};