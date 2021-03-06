/*
  © 2009–2016, Jon Stewart
  Released under the terms of the Boost license (http://www.boost.org/LICENSE_1_0.txt). See License.txt for details.
*/

#pragma once

#include <cassert>
#include <csignal>
#include <iostream>
#include <memory>
#include <map>

#include "tclap/CmdLine.h"


#include "test.h"

namespace scope {

  void RunFunction(scope::TestFunction test, const char* testname, bool shouldFail, MessageList& messages) {
    try {
      test();
      if (shouldFail) {
        std::string msg(testname);
        msg.append(": marked for failure but did not throw scope::test_failure.");
        messages.push_back(msg);
      }
    }
    catch (const test_failure& fail) {
      if (!shouldFail) {
        std::ostringstream buf;
        buf << fail.File << ":" << fail.Line << ": " << testname << ": " << fail.what();
        messages.push_back(buf.str());
      }
    }
    catch (const std::exception& except) {
      messages.push_back(std::string(testname) + ": " + except.what());
    }
    catch (...) {
      CaughtBadExceptionType(testname, "test threw unrecognized type");
      throw;
    }
  }

  void CaughtBadExceptionType(const std::string& name, const std::string& msg) {
    std::cerr << name << ": " << msg << "; please at least inherit from std::exception" << std::endl;
  }

  namespace {
    class TestRunnerImpl : public TestRunner {
    public:
      TestRunnerImpl():
        NumTests(0), NumRun(0), Debug(false) {}

      virtual void runTest(const TestCase& test, const std::string& nameFilter, MessageList& messages) {
        if (nameFilter.empty() || nameFilter == test.Name) {
          lastTest() = test.Name;
          if (Debug) {
            std::cerr << "Running " << test.Name << std::endl;
          }
          ++NumRun;
          test.Run(messages);
          if (Debug) {
            std::cerr << "Done with " << test.Name << std::endl;
          }
        }
      }

      virtual void run(const std::string& nameFilter, MessageList& messages) {
        auto& r(root());
        for (auto curlist(r.FirstChild); curlist; curlist = curlist->Next) {
          for (auto cur(curlist->FirstChild); cur; cur = cur->Next) {
            ++NumTests;
          }
        }
        for (auto curlist(r.FirstChild); curlist; curlist = curlist->Next) {
          for (auto cur(curlist->FirstChild); cur; cur = cur->Next) {
            std::unique_ptr<TestCase> test(cur->Construct());
            runTest(*test, nameFilter, messages);
          }
        }
      }

      virtual unsigned int numTests() const {
        return NumTests;
      }

      virtual unsigned int numRun() const {
        return NumRun;
      }

      virtual void setDebug(bool val) {
        Debug = val;
      }

    private:
      unsigned int  NumTests,
                    NumRun;
      bool          Debug;
    };
  }

  Node<AutoRegister>& TestRunner::root(void) {
    static Node<AutoRegister> root;
    return root;
  }

  std::string& TestRunner::lastTest(void) {
    static std::string last;
    return last;
  }

  void handleTerminate() {
    std::cerr << "std::terminate called, last test was " << TestRunner::lastTest() << ". Aborting." << std::endl;
    std::abort();
  }

  std::map<int, std::string> signalMap() {
    return std::map<int, std::string> {
      {SIGFPE, "floating point exception (SIGFPE)"},
      {SIGSEGV, "segmentation fault (SIGSEGV)"},
      {SIGTERM, "termination request (SIGTERM)"},
      {SIGINT, "interrupt request (SIGINT)"}
    };
  }

  void handleSignal(int signum) {
    auto friendlySig = signalMap()[signum];
    std::cerr << "Received signal " << signum << ", " << friendlySig
      << ". Last test was " << TestRunner::lastTest() << ". Aborting." << std::endl;
    std::abort();
  }

  template<typename HandlerT>
  void setHandlers(HandlerT handler) {
    auto sigmap = signalMap();
    for (auto sig: sigmap) {
      std::signal(sig.first, handler);
    }
  }

  bool DefaultRun(std::ostream& out, int argc, char** argv) {
    TCLAP::CmdLine parser("Scope test", ' ', "version number?");

    TCLAP::ValueArg<std::string> filter("f", "filter", "Test to run", false, "", "string", parser);

    TCLAP::SwitchArg verbose("v", "verbose", "Print debugging info", parser);

    try {
      parser.parse(argc, argv);
    }
    catch (TCLAP::ArgException& e) {
      std::cerr << "Error: " << e.error() << " for argument " << e.argId() << std::endl;
      return false;
    }
    MessageList msgs;
    TestRunnerImpl runner;
    if (verbose.getValue()) {
      runner.setDebug(true);
      out << "Running in debug mode" << std::endl;
    }
    setHandlers(handleSignal);
    std::set_terminate(&handleTerminate);
    runner.run(filter.getValue(), msgs);
    std::set_terminate(0);
    setHandlers(SIG_DFL);

    for(const std::string& m : msgs) {
      out << m << '\n';
    }

    if (msgs.empty()) {
      out << "OK (" << runner.numRun() << " tests)" << std::endl;
      return true;
    }
    else {
      out << "Failures!" << std::endl;
      out << "Tests run: " << runner.numRun() << ", Failures: " << msgs.size() << std::endl;
      return false;
    }
  }
}
