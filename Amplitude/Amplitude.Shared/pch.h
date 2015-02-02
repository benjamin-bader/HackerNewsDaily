#pragma once

#define WIN32_LEAN_AND_MEAN
#include <collection.h>
#include <concrt.h>
#include <ppltasks.h>
#include <sqlite3.h>

void Log(Platform::String ^statement);
void LogDebug(const char *statement);
void LogDebug(const wchar_t *statement);