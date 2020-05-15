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


#include <functional>
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

#include <stdlib.h>
#include <minwindef.h>
#include <crtdbg.h>
#include <errhandlingapi.h>

#else

#error Unsupported system

#endif


namespace airbag {


  class process_error {
  public:

    enum kind {
      undefined, pure_call
    };


    static char const* title(kind k) {
      switch(k) {
        case undefined:
          return "Undefined";
        case pure_call:
          return "Pure virtual function call";
        default:
          return "Unknown";
      }
    }


    using pure_call_handler = std::function<void()>;
    using system_failure_handler = std::function<void(system_failure const&)>;


    process_error() noexcept { }


    ~process_error() noexcept {
      _CrtSetReportMode(_CRT_ERROR, previous_report_mode_);
      _set_purecall_handler(previous_purecall_handler_);
      RemoveVectoredExceptionHandler(&process_error::system_failure_dispatcher);
    }


    void on_pure_call(pure_call_handler h) noexcept {
      pure_call_handler_ = std::move(h);
      previous_report_mode_ = _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
      _CrtSetReportFile(_CRT_ERROR, 0);
      previous_purecall_handler_ = _set_purecall_handler(&process_error::pure_call_dispatcher);
    }


    static void pre_system_failure(system_failure_handler h) noexcept {
      system_failure_handler_ = std::move(h);
      constexpr ULONG call_me_first = 1;
      AddVectoredExceptionHandler(call_me_first, &process_error::system_failure_dispatcher);
    }


  private:

    static pure_call_handler pure_call_handler_;
    static system_failure_handler system_failure_handler_;

    int previous_report_mode_{0};
    _purecall_handler previous_purecall_handler_{nullptr};


    static void pure_call_dispatcher() noexcept {
      pure_call_handler_();
      exit(1);
    }


    static LONG WINAPI system_failure_dispatcher(_EXCEPTION_POINTERS *info) noexcept {
      constexpr DWORD microsoft_cxx_exception = 0xE06D7363;
      if (info->ExceptionRecord->ExceptionCode == microsoft_cxx_exception)
        return EXCEPTION_CONTINUE_SEARCH; // C++ exception -> search next catch
      system_failure_handler_(system_failure{info});
      return EXCEPTION_CONTINUE_SEARCH;
    }

  }; // fatal_error

  inline process_error::pure_call_handler process_error::pure_call_handler_;
  inline process_error::system_failure_handler process_error::system_failure_handler_;


} // airbag
