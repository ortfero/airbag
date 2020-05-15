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

#include <stdint.h>
#include <string.h>
#include <minwindef.h>
//#include <winnt.h>
#include <processthreadsapi.h>
#include <libloaderapi.h>
#include <psapi.h>

#else

#error Unsupported system

#endif



namespace airbag {


  struct system_failure {

    using code_type =  DWORD;
    using info_type = _EXCEPTION_POINTERS;

    static constexpr auto module_name_capacity = 63;

    system_failure() noexcept {
      module_name_[0] = '\0';
    }


    system_failure(system_failure const&) noexcept = default;
    system_failure& operator = (system_failure const&) noexcept = default;

    code_type code() const noexcept { return code_; }
    info_type* info() const noexcept { return info_; }
    char const* module_name() const noexcept { return module_name_; }


    explicit system_failure(code_type code) noexcept:
      code_{code}
    { }


    explicit system_failure(_EXCEPTION_POINTERS *info) noexcept:
      code_{info->ExceptionRecord->ExceptionCode}, info_{info} {

      module_name_[0] = '\0';

      auto const process = GetCurrentProcess();
      size_t const exception_address = size_t(info->ExceptionRecord->ExceptionAddress);

      HMODULE modules[512]; DWORD modules_count;
      if(!K32EnumProcessModules(process, modules, sizeof(modules), &modules_count))
         return;
      modules_count /= sizeof(HMODULE);
      for(DWORD i = 0; i != modules_count; ++i) {
        MODULEINFO module_info;
        if(!K32GetModuleInformation(process, modules[i], &module_info, sizeof(MODULEINFO)))
          continue;
        size_t const module_address = size_t(module_info.lpBaseOfDll);
        if(module_address > exception_address || exception_address > module_address + module_info.SizeOfImage)
          continue;
        char module_name[MAX_PATH];
        GetModuleFileNameA(modules[i], module_name, MAX_PATH);
        char const* last_back_slash = std::strrchr(module_name, '\\');
        if(last_back_slash == nullptr)
          strncpy_s(module_name_, module_name_capacity + 1, module_name, module_name_capacity);
        else
          strncpy_s(module_name_, module_name_capacity + 1, last_back_slash + 1, module_name_capacity);
        break;
      }
    }


    char const* title() const noexcept {
      switch(code_) {
        case 0:
          return "None";
        case STATUS_INVALID_PARAMETER:
          return "Invalid parameter";
        case STATUS_ACCESS_VIOLATION:
          return "Access violation";
        case STATUS_DATATYPE_MISALIGNMENT:
          return "Datatype misalignment";
        case STATUS_BREAKPOINT:
          return "Breakpoint";
        case STATUS_SINGLE_STEP:
          return "Single step";
        case STATUS_ARRAY_BOUNDS_EXCEEDED:
          return "Array bounds exceeded";
        case STATUS_FLOAT_DENORMAL_OPERAND:
          return "Float denormal operand";
        case STATUS_FLOAT_DIVIDE_BY_ZERO:
          return "Float divide by zero";
        case STATUS_FLOAT_INEXACT_RESULT:
          return "Float inexact result";
        case STATUS_FLOAT_INVALID_OPERATION:
          return "Float invalid operation";
        case STATUS_FLOAT_OVERFLOW:
          return "Float overflow";
        case STATUS_FLOAT_STACK_CHECK:
          return "Float stack check";
        case STATUS_FLOAT_UNDERFLOW:
          return "Float underflow";
        case STATUS_INTEGER_DIVIDE_BY_ZERO:
          return "Integer divide by zero";
        case STATUS_INTEGER_OVERFLOW:
          return "Integer overflow";
        case STATUS_PRIVILEGED_INSTRUCTION:
          return "Privileged instruction";
        case STATUS_IN_PAGE_ERROR:
          return "In page error";
        case STATUS_ILLEGAL_INSTRUCTION:
          return "Status illegal instruction";
        case STATUS_NONCONTINUABLE_EXCEPTION:
          return "Noncontinuable exception";
        case STATUS_STACK_OVERFLOW:
          return "Stack overflow";
        case STATUS_INVALID_DISPOSITION:
          return "Invalid disposition";
        case STATUS_GUARD_PAGE_VIOLATION:
          return "Guard page violation";
        case STATUS_INVALID_HANDLE:
          return "Invalid handle";
        default:
          return "Unknwon";
      }
    }

  private:

    code_type code_{0};
    info_type* info_{nullptr};
    char module_name_[module_name_capacity + 1];

  }; // system_failure


} // airbag
