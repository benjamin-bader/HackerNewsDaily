#include "pch.h"

void Log(Platform::String ^statement)
{
	LogDebug(statement->Data());
}

void LogDebug(const char *statement)
{
	OutputDebugStringA(statement);
	OutputDebugStringA("\n");
}

void LogDebug(const wchar_t *statement)
{
	OutputDebugStringW(statement);
	OutputDebugStringW(L"\n");
}