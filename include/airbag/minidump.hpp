/* This file is part of airbag library
 * Copyright 2020 Andrei Ilin <ortfero@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once


#include <filesystem>
#include <system_error>
#include <cstdio>
#include "system_failure.hpp"


#if defined(_WIN32)

#if !defined(_X86_) && !defined(_AMD64_) && !defined(_ARM_) && !defined(_ARM64_)
#if defined(_M_IX86)
#define _X86_
#elif defined(_M_AMD64)
#define _AMD64_
#elif defined(_M_ARM)
#define _ARM_
#elif defined(_M_ARM64)
#define _ARM64_
#endif
#endif

#include <minwindef.h>
#include <libloaderapi.h>
#include <errhandlingapi.h>
#include <sysinfoapi.h>
#include <fileapi.h>
#include <handleapi.h>
#include <processthreadsapi.h>
#include <timezoneapi.h>
#include <DbgHelp.h>

#pragma comment(lib, "Dbghelp.lib")

#else

#error Unsupported system

#endif


namespace airbag {
  
  
  struct minidump {
    
    using path_type = std::filesystem::path;


    static std::error_code last_error() {
      return {int(GetLastError()), std::system_category()};
    }

        
    minidump() {
      char path_buffer[MAX_PATH];
      GetModuleFileNameA(nullptr, path_buffer, MAX_PATH);
      dump_dir_ = path_buffer;
      executable_name_ = dump_dir_.stem().string();
      dump_dir_= dump_dir_.parent_path();
      dump_dir_ /= "crash";
    }
    
    
    explicit minidump(path_type const& dir): dump_dir_{dir} {
      char path_buffer[MAX_PATH];
      GetModuleFileNameA(nullptr, path_buffer, MAX_PATH);
      path_type path{path_buffer};
      executable_name_ = path.stem().string();
    }
    
    
    minidump(minidump const&) = default;
    minidump& operator = (minidump const&) = default;
    minidump(minidump&&) = default;
    minidump& operator = (minidump&&) = default;
    void directory(path_type const& dir) { dump_dir_ = dir; }
    path_type const& directory() const noexcept { return dump_dir_; }    
    
    
    bool generate(system_failure const& failure) {

      namespace fs = std::filesystem;
      if(!fs::exists(dump_dir_)) {
        std::error_code failed;
        fs::create_directories(dump_dir_, failed);
        if(!!failed)
          return false;
      }
      
      SYSTEMTIME time;
      GetSystemTime(&time);
      char dump_name[MAX_PATH];

      std::snprintf(dump_name, sizeof(dump_name), "%s-%u_%02u_%02u-%02u_%02u_%02u.dmp", executable_name_.data(),
               time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond);
      
      path_type path = dump_dir_; path /= dump_name;
      
      auto constexpr generic_read = 0x80000000;
      auto constexpr generic_write = 0x40000000;
      auto constexpr file_attribute_normal = 0x00000080;

      HANDLE file = CreateFileA(path.string().data(), generic_read | generic_write, 0,
        nullptr, CREATE_ALWAYS, file_attribute_normal, nullptr);
      if(file == HANDLE(-1))
        return false;
      
      MINIDUMP_EXCEPTION_INFORMATION mdei;
      MINIDUMP_EXCEPTION_INFORMATION* pmdei;
      
      if(failure.info() != nullptr) {
        mdei.ThreadId = GetCurrentThreadId();
        mdei.ExceptionPointers = failure.info();
        mdei.ClientPointers = FALSE;
        pmdei = &mdei;
      } else {
        pmdei = nullptr;
      }
      
      int const mdt = MiniDumpWithPrivateReadWriteMemory
                                | MiniDumpWithDataSegs
                                | MiniDumpWithHandleData
                                | MiniDumpWithFullMemoryInfo
                                | MiniDumpWithThreadInfo;
      
      BOOL const written = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(),
        file, MINIDUMP_TYPE(mdt), pmdei, nullptr, nullptr);
        
      CloseHandle(file);
      return !!written;
    }
    
  private:
    
    path_type dump_dir_;
    std::string executable_name_;
  }; // minidump
  
  
} // airbag
