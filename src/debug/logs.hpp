#pragma once
#include <iostream>

/** Logging System
 * 
 * Global logging via Log::warn, Log::error, and Log::info streams
 * Scope-based logging system via Log::Scope
 */

enum Logging { None, Errors, Warnings, Info, All };

namespace Log {
  class Level {
    private:
      static inline Logging level = Warnings; // C++17 inline static = single global copy
    public:
      static void set(Logging log_level) {
          level = log_level;
      }
  
      static Logging get() {
          return level;
      }
  };

  namespace LogStream {
    static inline std::ostream cnull(nullptr);
  
    class Warning {
      public:
      const char* scope = nullptr;
      void setScope(const char* s) {scope = s;}
      std::ostream& operator<<(const char* warn) {
        if ( Level::get() >= Logging::Warnings ) {
          return scope ? std::cerr << "(!warn) <" << scope << ">: " << warn:
            std::cerr << "(!warn): " << warn;
        }
        return cnull;
      }
    };

    class Error {
      const char* scope = nullptr;
      public:
      void setScope(const char* s) {scope = s;}
      std::ostream& operator<<(const char* warn) {
        if ( Level::get() >= Logging::Errors ) {
          return scope ? std::cerr << "[!Error] <" << scope << ">: " << warn:
            std::cerr << "[!Error]: " << warn;
        }
        return cnull;
      }
    };

    class Info {
      const char* scope = nullptr;
      public:
      void setScope(const char* s) {scope = s;}
      std::ostream& operator<<(const char* warn) {
        if ( Level::get() >= Logging::Info ) {
          return scope ? std::cerr << "(info) <" << scope << ">: " << warn:
            std::cerr << "(info): " << warn;
        }
        return cnull;
      }
    };
  }

  class Scope {
    public:
    LogStream::Warning warn;
    LogStream::Error error;
    LogStream::Info info;
    Scope(const char* scope) {
      warn.setScope(scope);
      error.setScope(scope);
      info.setScope(scope);
    }
  };

  static inline LogStream::Warning warn;
  static inline LogStream::Error error;
  static inline LogStream::Info info;
}