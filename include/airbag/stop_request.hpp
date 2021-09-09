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


#include <atomic>

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
#include <consoleapi.h>
#include <synchapi.h>

#else

#error Unsupported system

#endif


namespace airbag {


  class stop_request {
  public:


    static void take_care() noexcept {
#if _WIN32

      SetConsoleCtrlHandler(&stop_request::handler, TRUE);

#else

      signal(SIGINT, &stop_request::handler);

#endif // _WIN32
    }


    static bool signaled() noexcept {
      return signaled_;
    }


    static void signal() noexcept {
      signaled_ = true;
    }


    static void processed() noexcept {
      processed_ = true;
    }


  private:

    static std::atomic_bool signaled_;
    static std::atomic_bool processed_;

#if _WIN32


    static BOOL WINAPI handler(DWORD) {
      signaled_ = true;
      while(!processed_)
        Sleep(100);
      return TRUE;
    }


#else


    static void handler(int) noexcept {
      signaled_ = true;
    }


#endif

  }; // stop_request

  inline std::atomic_bool stop_request::signaled_;
  inline std::atomic_bool stop_request::processed_;


} // airbag
