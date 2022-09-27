//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Developed by Minigraph
//
// Author:  James Stanard 
//

#pragma once

#include "pch.h"

namespace Utility
{
#ifdef _CONSOLE
    inline void Print( const char* msg ) { printf("%s", msg); }
    inline void Print( const wchar_t* msg ) { wprintf(L"%ws", msg); }
#else
    inline void Print( const char* msg ) { OutputDebugStringA(msg); }
    inline void Print( const wchar_t* msg ) { OutputDebugString(msg); }
#endif

    inline void Printf( const char* format, ... )
    {
        char buffer[256];
        va_list ap;
        va_start(ap, format);
        vsprintf_s(buffer, 256, format, ap);
        va_end(ap);
        Print(buffer);
    }

    inline void Printf( const wchar_t* format, ... )
    {
        wchar_t buffer[256];
        va_list ap;
        va_start(ap, format);
        vswprintf(buffer, 256, format, ap);
        va_end(ap);
        Print(buffer);
    }

#ifndef RELEASE
    inline void PrintSubMessage( const char* format, ... )
    {
        Print("--> ");
        char buffer[256];
        va_list ap;
        va_start(ap, format);
        vsprintf_s(buffer, 256, format, ap);
        va_end(ap);
        Print(buffer);
        Print("\n");
    }
    inline void PrintSubMessage( const wchar_t* format, ... )
    {
        Print("--> ");
        wchar_t buffer[256];
        va_list ap;
        va_start(ap, format);
        vswprintf(buffer, 256, format, ap);
        va_end(ap);
        Print(buffer);
        Print("\n");
    }
    inline void PrintSubMessage( void )
    {
    }
#endif

    std::wstring AnsiToWideString(const std::string& str);
    std::string WideStringToAnsi(const std::wstring& wstr);
    std::wstring UTF8ToWideString( const std::string& str );
    std::string WideStringToUTF8( const std::wstring& wstr );
    std::string ToLower(const std::string& str);
    std::wstring ToLower(const std::wstring& str);
    std::string GetBasePath(const std::string& str);
    std::wstring GetBasePath(const std::wstring& str);
    std::string RemoveBasePath(const std::string& str);
    std::wstring RemoveBasePath(const std::wstring& str);
    std::string GetFileExtension(const std::string& str);
    std::wstring GetFileExtension(const std::wstring& str);
    std::string RemoveExtension(const std::string& str);
    std::wstring RemoveExtension(const std::wstring& str);
    std::wstring GetProgramDirectory();
    std::wstring GetWorkingDirectory();
    void SetWorkingDirectory(const std::wstring& dir);
    uint32_t GetThreadId();
} // namespace Utility

#ifdef ERROR
#undef ERROR
#endif
#ifdef ASSERT
#undef ASSERT
#endif
#ifdef HALT
#undef HALT
#endif

#define HALT( ... ) ERROR( __VA_ARGS__ ) __debugbreak();

#ifdef RELEASE

    #define ASSERT( isTrue, ... ) (void)(isTrue)
    #define WARN_ONCE_IF( isTrue, ... ) (void)(isTrue)
    #define WARN_ONCE_IF_NOT( isTrue, ... ) (void)(isTrue)
    #define ERROR( msg, ... )
    #define DEBUGPRINT( msg, ... ) do {} while(0)
    #define ASSERT_SUCCEEDED( hr, ... ) (void)(hr)

#else	// !RELEASE

    inline void LOG_ERROR_SUB(const char* format, ...)
    {
        char buffer[256];
        va_list ap;
        va_start(ap, format);
        vsprintf_s(buffer, 256, format, ap);
        va_end(ap);
        LOG_ERRORF("--> %s", buffer);
    }

    inline void LOG_ERROR_SUB()
    {
    }

    inline void LOG_WARN_SUB(const char* format, ...)
    {
        char buffer[256];
        va_list ap;
        va_start(ap, format);
        vsprintf_s(buffer, 256, format, ap);
        va_end(ap);
        LOG_WARNF("--> %s", buffer);
    }

    inline void LOG_WARN_SUB()
    {
    }

    #define STRINGIFY(x) #x
    #define STRINGIFY_BUILTIN(x) STRINGIFY(x)
    #define ASSERT( isFalse, ... ) \
        if (!(bool)(isFalse)) { \
            Utility::Print(__FILE__ "(" STRINGIFY_BUILTIN(__LINE__) ")\n"); \
            LOG_ERROR("Assertion failed in " STRINGIFY_BUILTIN(__FILE__) " @ " STRINGIFY_BUILTIN(__LINE__)); \
            LOG_ERROR("--> \'" #isFalse "\' is false"); \
            LOG_ERROR_SUB(__VA_ARGS__); \
            __debugbreak(); \
        }

    #define ASSERT_SUCCEEDED( hr, ... ) \
        if (FAILED(hr)) { \
            Utility::Print(__FILE__ "(" STRINGIFY_BUILTIN(__LINE__) ")\n"); \
            LOG_ERROR("HRESULT failed in " STRINGIFY_BUILTIN(__FILE__) " @ " STRINGIFY_BUILTIN(__LINE__)); \
            LOG_ERRORF("--> hr = 0x%08X", hr); \
            LOG_ERROR_SUB(__VA_ARGS__); \
            __debugbreak(); \
        }

    #define WARN_ONCE_IF( isTrue, ... ) \
    { \
        static bool s_TriggeredWarning = false; \
        if ((bool)(isTrue) && !s_TriggeredWarning) { \
            s_TriggeredWarning = true; \
            Utility::Print(__FILE__ "(" STRINGIFY_BUILTIN(__LINE__) ")\n"); \
            LOG_WARN("Warning issued in " STRINGIFY_BUILTIN(__FILE__) " @ " STRINGIFY_BUILTIN(__LINE__)); \
            LOG_WARN("\'" #isTrue "\' is true"); \
            LOG_WARN_SUB(__VA_ARGS__); \
        } \
    }

    #define WARN_ONCE_IF_NOT( isTrue, ... ) WARN_ONCE_IF(!(isTrue), __VA_ARGS__)

    #define ERROR( ... ) \
        Utility::Print(__FILE__ "(" STRINGIFY_BUILTIN(__LINE__) ")\n"); \
        LOG_ERROR("Error reported in " STRINGIFY_BUILTIN(__FILE__) " @ " STRINGIFY_BUILTIN(__LINE__)); \
        LOG_ERROR_SUB(__VA_ARGS__);

#endif

#define BreakIfFailed( hr ) if (FAILED(hr)) __debugbreak()

void SIMDMemCopy( void* __restrict Dest, const void* __restrict Source, size_t NumQuadwords );
void SIMDMemFill( void* __restrict Dest, __m128 FillVector, size_t NumQuadwords );
