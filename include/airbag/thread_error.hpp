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


#include <string>
#include <stdexcept>
#include <functional>
#include <typeinfo>

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
#include <crtdbg.h>
#include <consoleapi.h>

#else

#error Unsupported system

#endif



namespace airbag {


  class thread_error : public std::runtime_error  {
  public:

    using base = std::runtime_error;
    using terminate_handler = std::function<void(char const*)>;

    thread_error() noexcept: base{""} {
      _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
      _CrtSetReportFile(_CRT_ASSERT, 0);
      _set_invalid_parameter_handler(&thread_error::invalid_parameter_dispatcher);
      _set_se_translator(&thread_error::system_failure_dispatcher);
      set_terminate(&thread_error::terminate_dispatcher);
    }


    thread_error(std::string const& message, system_failure const& f) noexcept:
      base{message}, failure_{f}
    { }


    thread_error(char const* message, system_failure const& f) noexcept:
      base{message}, failure_{f}
    { }


    system_failure const& failure() const noexcept {
      return failure_;
    }

    void on_terminate(terminate_handler h) noexcept {
      terminate_handler_ = std::move(h);
    }

  private:

    static inline thread_local terminate_handler terminate_handler_;

    system_failure failure_;


    static void append(std::string& s, wchar_t const* cc) {
      if(cc == nullptr)
        return;
      while(*cc)
        s.push_back(char(*cc++));
    }


    static void invalid_parameter_dispatcher(wchar_t const* expression, wchar_t const* function,
                                          wchar_t const*, unsigned, uintptr_t) {
      std::string message; message.reserve(512);
      message = "Invalid parameter for '";
      append(message, function);
      message += '\''; message += ','; message += ' ';
      append(message, expression);
      throw thread_error{message, system_failure{STATUS_INVALID_PARAMETER}};
    }


    static void system_failure_dispatcher(unsigned, EXCEPTION_POINTERS* info)
    {
      system_failure const failure{info};
      throw thread_error{failure.title(), failure};
    }


    static void terminate_dispatcher() {
      try {
          throw;
      } catch(std::exception const& e) {
        std::string message;
        message = "Uncaught ";
        message += typeid(e).name();
        message += ' '; message += '(';
        message.append(e.what());
        message += ')';
        terminate_handler_(message.data());
      } catch(...) {
        terminate_handler_("Uncaught exception");
      }
      exit(1);
    }

  }; // thread_error


} // airbag
