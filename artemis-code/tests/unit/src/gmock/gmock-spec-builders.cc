// Copyright 2007, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Author: wan@google.com (Zhanyong Wan)

// Google Mock - a framework for writing C++ mock classes.
//
// This file implements the spec builder syntax (ON_CALL and
// EXPECT_CALL).

#include "include/gmock/gmock-spec-builders.h"

#include <stdlib.h>
#include <iostream>  // NOLINT
#include <map>
#include <set>
#include <string>
#include "include/gmock/gmock.h"
#include "include/gtest/gtest.h"

#if GTEST_OS_CYGWIN || GTEST_OS_LINUX || GTEST_OS_MAC
# include <unistd.h>  // NOLINT
#endif

namespace testing {
namespace internal {

// Protects the mock object registry (in class Mock), all function
// mockers, and all expectations.
GTEST_DEFINE_STATIC_MUTEX_(g_gmock_mutex);

// Logs a message including file and line number information.
void LogWithLocation(testing::internal::LogSeverity severity,
                     const char* file, int line,
                     const string& message) {
  ::std::ostringstream s;
  s << file << ":" << line << ": " << message << ::std::endl;
  Log(severity, s.str(), 0);
}

// Constructs an ExpectationBase object.
ExpectationBase::ExpectationBase(const char* a_file,
                                 int a_line,
                                 const string& a_source_text)
    : file_(a_file),
      line_(a_line),
      source_text_(a_source_text),
      cardinality_specified_(false),
      cardinality_(Exactly(1)),
      call_count_(0),
      retired_(false),
      extra_matcher_specified_(false),
      repeated_action_specified_(false),
      retires_on_saturation_(false),
      last_clause_(kNone),
      action_count_checked_(false) {}

// Destructs an ExpectationBase object.
ExpectationBase::~ExpectationBase() {}

// Explicitly specifies the cardinality of this expectation.  Used by
// the subclasses to implement the .Times() clause.
void ExpectationBase::SpecifyCardinality(const Cardinality& a_cardinality) {
  cardinality_specified_ = true;
  cardinality_ = a_cardinality;
}

// Retires all pre-requisites of this expectation.
void ExpectationBase::RetireAllPreRequisites() {
  if (is_retired()) {
    // We can take this short-cut as we never retire an expectation
    // until we have retired all its pre-requisites.
    return;
  }

  for (ExpectationSet::const_iterator it = immediate_prerequisites_.begin();
       it != immediate_prerequisites_.end(); ++it) {
    ExpectationBase* const prerequisite = it->expectation_base().get();
    if (!prerequisite->is_retired()) {
      prerequisite->RetireAllPreRequisites();
      prerequisite->Retire();
    }
  }
}

// Returns true iff all pre-requisites of this expectation have been
// satisfied.
// L >= g_gmock_mutex
bool ExpectationBase::AllPrerequisitesAreSatisfied() const {
  g_gmock_mutex.AssertHeld();
  for (ExpectationSet::const_iterator it = immediate_prerequisites_.begin();
       it != immediate_prerequisites_.end(); ++it) {
    if (!(it->expectation_base()->IsSatisfied()) ||
        !(it->expectation_base()->AllPrerequisitesAreSatisfied()))
      return false;
  }
  return true;
}

// Adds unsatisfied pre-requisites of this expectation to 'result'.
// L >= g_gmock_mutex
void ExpectationBase::FindUnsatisfiedPrerequisites(
    ExpectationSet* result) const {
  g_gmock_mutex.AssertHeld();
  for (ExpectationSet::const_iterator it = immediate_prerequisites_.begin();
       it != immediate_prerequisites_.end(); ++it) {
    if (it->expectation_base()->IsSatisfied()) {
      // If *it is satisfied and has a call count of 0, some of its
      // pre-requisites may not be satisfied yet.
      if (it->expectation_base()->call_count_ == 0) {
        it->expectation_base()->FindUnsatisfiedPrerequisites(result);
      }
    } else {
      // Now that we know *it is unsatisfied, we are not so interested
      // in whether its pre-requisites are satisfied.  Therefore we
      // don't recursively call FindUnsatisfiedPrerequisites() here.
      *result += *it;
    }
  }
}

// Describes how many times a function call matching this
// expectation has occurred.
// L >= g_gmock_mutex
void ExpectationBase::DescribeCallCountTo(::std::ostream* os) const {
  g_gmock_mutex.AssertHeld();

  // Describes how many times the function is expected to be called.
  *os << "         Expected: to be ";
  cardinality().DescribeTo(os);
  *os << "\n           Actual: ";
  Cardinality::DescribeActualCallCountTo(call_count(), os);

  // Describes the state of the expectation (e.g. is it satisfied?
  // is it active?).
  *os << " - " << (IsOverSaturated() ? "over-saturated" :
                   IsSaturated() ? "saturated" :
                   IsSatisfied() ? "satisfied" : "unsatisfied")
      << " and "
      << (is_retired() ? "retired" : "active");
}

// Checks the action count (i.e. the number of WillOnce() and
// WillRepeatedly() clauses) against the cardinality if this hasn't
// been done before.  Prints a warning if there are too many or too
// few actions.
// L < mutex_
void ExpectationBase::CheckActionCountIfNotDone() const {
  bool should_check = false;
  {
    MutexLock l(&mutex_);
    if (!action_count_checked_) {
      action_count_checked_ = true;
      should_check = true;
    }
  }

  if (should_check) {
    if (!cardinality_specified_) {
      // The cardinality was inferred - no need to check the action
      // count against it.
      return;
    }

    // The cardinality was explicitly specified.
    const int action_count = static_cast<int>(untyped_actions_.size());
    const int upper_bound = cardinality().ConservativeUpperBound();
    const int lower_bound = cardinality().ConservativeLowerBound();
    bool too_many;  // True if there are too many actions, or false
    // if there are too few.
    if (action_count > upper_bound ||
        (action_count == upper_bound && repeated_action_specified_)) {
      too_many = true;
    } else if (0 < action_count && action_count < lower_bound &&
               !repeated_action_specified_) {
      too_many = false;
    } else {
      return;
    }

    ::std::stringstream ss;
    DescribeLocationTo(&ss);
    ss << "Too " << (too_many ? "many" : "few")
       << " actions specified in " << source_text() << "...\n"
       << "Expected to be ";
    cardinality().DescribeTo(&ss);
    ss << ", but has " << (too_many ? "" : "only ")
       << action_count << " WillOnce()"
       << (action_count == 1 ? "" : "s");
    if (repeated_action_specified_) {
      ss << " and a WillRepeatedly()";
    }
    ss << ".";
    Log(WARNING, ss.str(), -1);  // -1 means "don't print stack trace".
  }
}

// Implements the .Times() clause.
void ExpectationBase::UntypedTimes(const Cardinality& a_cardinality) {
  if (last_clause_ == kTimes) {
    ExpectSpecProperty(false,
                       ".Times() cannot appear "
                       "more than once in an EXPECT_CALL().");
  } else {
    ExpectSpecProperty(last_clause_ < kTimes,
                       ".Times() cannot appear after "
                       ".InSequence(), .WillOnce(), .WillRepeatedly(), "
                       "or .RetiresOnSaturation().");
  }
  last_clause_ = kTimes;

  SpecifyCardinality(a_cardinality);
}

// Points to the implicit sequence introduced by a living InSequence
// object (if any) in the current thread or NULL.
ThreadLocal<Sequence*> g_gmock_implicit_sequence;

// Reports an uninteresting call (whose description is in msg) in the
// manner specified by 'reaction'.
void ReportUninterestingCall(CallReaction reaction, const string& msg) {
  switch (reaction) {
    case ALLOW:
      Log(INFO, msg, 3);
      break;
    case WARN:
      Log(WARNING, msg, 3);
      break;
    default:  // FAIL
      Expect(false, NULL, -1, msg);
  }
}

UntypedFunctionMockerBase::UntypedFunctionMockerBase()
    : mock_obj_(NULL), name_("") {}

UntypedFunctionMockerBase::~UntypedFunctionMockerBase() {}

// Sets the mock object this mock method belongs to, and registers
// this information in the global mock registry.  Will be called
// whenever an EXPECT_CALL() or ON_CALL() is executed on this mock
// method.
// L < g_gmock_mutex
void UntypedFunctionMockerBase::RegisterOwner(const void* mock_obj) {
  {
    MutexLock l(&g_gmock_mutex);
    mock_obj_ = mock_obj;
  }
  Mock::Register(mock_obj, this);
}

// Sets the mock object this mock method belongs to, and sets the name
// of the mock function.  Will be called upon each invocation of this
// mock function.
// L < g_gmock_mutex
void UntypedFunctionMockerBase::SetOwnerAndName(
    const void* mock_obj, const char* name) {
  // We protect name_ under g_gmock_mutex in case this mock function
  // is called from two threads concurrently.
  MutexLock l(&g_gmock_mutex);
  mock_obj_ = mock_obj;
  name_ = name;
}

// Returns the name of the function being mocked.  Must be called
// after RegisterOwner() or SetOwnerAndName() has been called.
// L < g_gmock_mutex
const void* UntypedFunctionMockerBase::MockObject() const {
  const void* mock_obj;
  {
    // We protect mock_obj_ under g_gmock_mutex in case this mock
    // function is called from two threads concurrently.
    MutexLock l(&g_gmock_mutex);
    Assert(mock_obj_ != NULL, __FILE__, __LINE__,
           "MockObject() must not be called before RegisterOwner() or "
           "SetOwnerAndName() has been called.");
    mock_obj = mock_obj_;
  }
  return mock_obj;
}

// Returns the name of this mock method.  Must be called after
// SetOwnerAndName() has been called.
// L < g_gmock_mutex
const char* UntypedFunctionMockerBase::Name() const {
  const char* name;
  {
    // We protect name_ under g_gmock_mutex in case this mock
    // function is called from two threads concurrently.
    MutexLock l(&g_gmock_mutex);
    Assert(name_ != NULL, __FILE__, __LINE__,
           "Name() must not be called before SetOwnerAndName() has "
           "been called.");
    name = name_;
  }
  return name;
}

// Calculates the result of invoking this mock function with the given
// arguments, prints it, and returns it.  The caller is responsible
// for deleting the result.
// L < g_gmock_mutex
const UntypedActionResultHolderBase*
UntypedFunctionMockerBase::UntypedInvokeWith(const void* const untyped_args) {
  if (untyped_expectations_.size() == 0) {
    // No expectation is set on this mock method - we have an
    // uninteresting call.

    // We must get Google Mock's reaction on uninteresting calls
    // made on this mock object BEFORE performing the action,
    // because the action may DELETE the mock object and make the
    // following expression meaningless.
    const CallReaction reaction =
        Mock::GetReactionOnUninterestingCalls(MockObject());

    // True iff we need to print this call's arguments and return
    // value.  This definition must be kept in sync with
    // the behavior of ReportUninterestingCall().
    const bool need_to_report_uninteresting_call =
        // If the user allows this uninteresting call, we print it
        // only when he wants informational messages.
        reaction == ALLOW ? LogIsVisible(INFO) :
        // If the user wants this to be a warning, we print it only
        // when he wants to see warnings.
        reaction == WARN ? LogIsVisible(WARNING) :
        // Otherwise, the user wants this to be an error, and we
        // should always print detailed information in the error.
        true;

    if (!need_to_report_uninteresting_call) {
      // Perform the action without printing the call information.
      return this->UntypedPerformDefaultAction(untyped_args, "");
    }

    // Warns about the uninteresting call.
    ::std::stringstream ss;
    this->UntypedDescribeUninterestingCall(untyped_args, &ss);

    // Calculates the function result.
    const UntypedActionResultHolderBase* const result =
        this->UntypedPerformDefaultAction(untyped_args, ss.str());

    // Prints the function result.
    if (result != NULL)
      result->PrintAsActionResult(&ss);

    ReportUninterestingCall(reaction, ss.str());
    return result;
  }

  bool is_excessive = false;
  ::std::stringstream ss;
  ::std::stringstream why;
  ::std::stringstream loc;
  const void* untyped_action = NULL;

  // The UntypedFindMatchingExpectation() function acquires and
  // releases g_gmock_mutex.
  const ExpectationBase* const untyped_expectation =
      this->UntypedFindMatchingExpectation(
          untyped_args, &untyped_action, &is_excessive,
          &ss, &why);
  const bool found = untyped_expectation != NULL;

  // True iff we need to print the call's arguments and return value.
  // This definition must be kept in sync with the uses of Expect()
  // and Log() in this function.
  const bool need_to_report_call = !found || is_excessive || LogIsVisible(INFO);
  if (!need_to_report_call) {
    // Perform the action without printing the call information.
    return
        untyped_action == NULL ?
        this->UntypedPerformDefaultAction(untyped_args, "") :
        this->UntypedPerformAction(untyped_action, untyped_args);
  }

  ss << "    Function call: " << Name();
  this->UntypedPrintArgs(untyped_args, &ss);

  // In case the action deletes a piece of the expectation, we
  // generate the message beforehand.
  if (found && !is_excessive) {
    untyped_expectation->DescribeLocationTo(&loc);
  }

  const UntypedActionResultHolderBase* const result =
      untyped_action == NULL ?
      this->UntypedPerformDefaultAction(untyped_args, ss.str()) :
      this->UntypedPerformAction(untyped_action, untyped_args);
  if (result != NULL)
    result->PrintAsActionResult(&ss);
  ss << "\n" << why.str();

  if (!found) {
    // No expectation matches this call - reports a failure.
    Expect(false, NULL, -1, ss.str());
  } else if (is_excessive) {
    // We had an upper-bound violation and the failure message is in ss.
    Expect(false, untyped_expectation->file(),
           untyped_expectation->line(), ss.str());
  } else {
    // We had an expected call and the matching expectation is
    // described in ss.
    Log(INFO, loc.str() + ss.str(), 2);
  }

  return result;
}

// Returns an Expectation object that references and co-owns exp,
// which must be an expectation on this mock function.
Expectation UntypedFunctionMockerBase::GetHandleOf(ExpectationBase* exp) {
  for (UntypedExpectations::const_iterator it =
           untyped_expectations_.begin();
       it != untyped_expectations_.end(); ++it) {
    if (it->get() == exp) {
      return Expectation(*it);
    }
  }

  Assert(false, __FILE__, __LINE__, "Cannot find expectation.");
  return Expectation();
  // The above statement is just to make the code compile, and will
  // never be executed.
}

// Verifies that all expectations on this mock function have been
// satisfied.  Reports one or more Google Test non-fatal failures
// and returns false if not.
// L >= g_gmock_mutex
bool UntypedFunctionMockerBase::VerifyAndClearExpectationsLocked() {
  g_gmock_mutex.AssertHeld();
  bool expectations_met = true;
  for (UntypedExpectations::const_iterator it =
           untyped_expectations_.begin();
       it != untyped_expectations_.end(); ++it) {
    ExpectationBase* const untyped_expectation = it->get();
    if (untyped_expectation->IsOverSaturated()) {
      // There was an upper-bound violation.  Since the error was
      // already reported when it occurred, there is no need to do
      // anything here.
      expectations_met = false;
    } else if (!untyped_expectation->IsSatisfied()) {
      expectations_met = false;
      ::std::stringstream ss;
      ss  << "Actual function call count doesn't match "
          << untyped_expectation->source_text() << "...\n";
      // No need to show the source file location of the expectation
      // in the description, as the Expect() call that follows already
      // takes care of it.
      untyped_expectation->MaybeDescribeExtraMatcherTo(&ss);
      untyped_expectation->DescribeCallCountTo(&ss);
      Expect(false, untyped_expectation->file(),
             untyped_expectation->line(), ss.str());
    }
  }
  untyped_expectations_.clear();
  return expectations_met;
}

}  // namespace internal

// Class Mock.

namespace {

typedef std::set<internal::UntypedFunctionMockerBase*> FunctionMockers;

// The current state of a mock object.  Such information is needed for
// detecting leaked mock objects and explicitly verifying a mock's
// expectations.
struct MockObjectState {
  MockObjectState()
      : first_used_file(NULL), first_used_line(-1), leakable(false) {}

  // Where in the source file an ON_CALL or EXPECT_CALL is first
  // invoked on this mock object.
  const char* first_used_file;
  int first_used_line;
  ::std::string first_used_test_case;
  ::std::string first_used_test;
  bool leakable;  // true iff it's OK to leak the object.
  FunctionMockers function_mockers;  // All registered methods of the object.
};

// A global registry holding the state of all mock objects that are
// alive.  A mock object is added to this registry the first time
// Mock::AllowLeak(), ON_CALL(), or EXPECT_CALL() is called on it.  It
// is removed from the registry in the mock object's destructor.
class MockObjectRegistry {
 public:
  // Maps a mock object (identified by its address) to its state.
  typedef std::map<const void*, MockObjectState> StateMap;

  // This destructor will be called when a program exits, after all
  // tests in it have been run.  By then, there should be no mock
  // object alive.  Therefore we report any living object as test
  // failure, unless the user explicitly asked us to ignore it.
  ~MockObjectRegistry() {
    // "using ::std::cout;" doesn't work with Symbian's STLport, where cout is
    // a macro.

    if (!GMOCK_FLAG(catch_leaked_mocks))
      return;

    int leaked_count = 0;
    for (StateMap::const_iterator it = states_.begin(); it != states_.end();
         ++it) {
      if (it->second.leakable)  // The user said it's fine to leak this object.
        continue;

      // TODO(wan@google.com): Print the type of the leaked object.
      // This can help the user identify the leaked object.
      std::cout << "\n";
      const MockObjectState& state = it->second;
      std::cout << internal::FormatFileLocation(state.first_used_file,
                                                state.first_used_line);
      std::cout << " ERROR: this mock object";
      if (state.first_used_test != "") {
        std::cout << " (used in test " << state.first_used_test_case << "."
             << state.first_used_test << ")";
      }
      std::cout << " should be deleted but never is. Its address is @"
           << it->first << ".";
      leaked_count++;
    }
    if (leaked_count > 0) {
      std::cout << "\nERROR: " << leaked_count
           << " leaked mock " << (leaked_count == 1 ? "object" : "objects")
           << " found at program exit.\n";
      std::cout.flush();
      ::std::cerr.flush();
      // RUN_ALL_TESTS() has already returned when this destructor is
      // called.  Therefore we cannot use the normal Google Test
      // failure reporting mechanism.
      _exit(1);  // We cannot call exit() as it is not reentrant and
                 // may already have been called.
    }
  }

  StateMap& states() { return states_; }
 private:
  StateMap states_;
};

// Protected by g_gmock_mutex.
MockObjectRegistry g_mock_object_registry;

// Maps a mock object to the reaction Google Mock should have when an
// uninteresting method is called.  Protected by g_gmock_mutex.
std::map<const void*, internal::CallReaction> g_uninteresting_call_reaction;

// Sets the reaction Google Mock should have when an uninteresting
// method of the given mock object is called.
// L < g_gmock_mutex
void SetReactionOnUninterestingCalls(const void* mock_obj,
                                     internal::CallReaction reaction) {
  internal::MutexLock l(&internal::g_gmock_mutex);
  g_uninteresting_call_reaction[mock_obj] = reaction;
}

}  // namespace

// Tells Google Mock to allow uninteresting calls on the given mock
// object.
// L < g_gmock_mutex
void Mock::AllowUninterestingCalls(const void* mock_obj) {
  SetReactionOnUninterestingCalls(mock_obj, internal::ALLOW);
}

// Tells Google Mock to warn the user about uninteresting calls on the
// given mock object.
// L < g_gmock_mutex
void Mock::WarnUninterestingCalls(const void* mock_obj) {
  SetReactionOnUninterestingCalls(mock_obj, internal::WARN);
}

// Tells Google Mock to fail uninteresting calls on the given mock
// object.
// L < g_gmock_mutex
void Mock::FailUninterestingCalls(const void* mock_obj) {
  SetReactionOnUninterestingCalls(mock_obj, internal::FAIL);
}

// Tells Google Mock the given mock object is being destroyed and its
// entry in the call-reaction table should be removed.
// L < g_gmock_mutex
void Mock::UnregisterCallReaction(const void* mock_obj) {
  internal::MutexLock l(&internal::g_gmock_mutex);
  g_uninteresting_call_reaction.erase(mock_obj);
}

// Returns the reaction Google Mock will have on uninteresting calls
// made on the given mock object.
// L < g_gmock_mutex
internal::CallReaction Mock::GetReactionOnUninterestingCalls(
    const void* mock_obj) {
  internal::MutexLock l(&internal::g_gmock_mutex);
  return (g_uninteresting_call_reaction.count(mock_obj) == 0) ?
      internal::WARN : g_uninteresting_call_reaction[mock_obj];
}

// Tells Google Mock to ignore mock_obj when checking for leaked mock
// objects.
// L < g_gmock_mutex
void Mock::AllowLeak(const void* mock_obj) {
  internal::MutexLock l(&internal::g_gmock_mutex);
  g_mock_object_registry.states()[mock_obj].leakable = true;
}

// Verifies and clears all expectations on the given mock object.  If
// the expectations aren't satisfied, generates one or more Google
// Test non-fatal failures and returns false.
// L < g_gmock_mutex
bool Mock::VerifyAndClearExpectations(void* mock_obj) {
  internal::MutexLock l(&internal::g_gmock_mutex);
  return VerifyAndClearExpectationsLocked(mock_obj);
}

// Verifies all expectations on the given mock object and clears its
// default actions and expectations.  Returns true iff the
// verification was successful.
// L < g_gmock_mutex
bool Mock::VerifyAndClear(void* mock_obj) {
  internal::MutexLock l(&internal::g_gmock_mutex);
  ClearDefaultActionsLocked(mock_obj);
  return VerifyAndClearExpectationsLocked(mock_obj);
}

// Verifies and clears all expectations on the given mock object.  If
// the expectations aren't satisfied, generates one or more Google
// Test non-fatal failures and returns false.
// L >= g_gmock_mutex
bool Mock::VerifyAndClearExpectationsLocked(void* mock_obj) {
  internal::g_gmock_mutex.AssertHeld();
  if (g_mock_object_registry.states().count(mock_obj) == 0) {
    // No EXPECT_CALL() was set on the given mock object.
    return true;
  }

  // Verifies and clears the expectations on each mock method in the
  // given mock object.
  bool expectations_met = true;
  FunctionMockers& mockers =
      g_mock_object_registry.states()[mock_obj].function_mockers;
  for (FunctionMockers::const_iterator it = mockers.begin();
       it != mockers.end(); ++it) {
    if (!(*it)->VerifyAndClearExpectationsLocked()) {
      expectations_met = false;
    }
  }

  // We don't clear the content of mockers, as they may still be
  // needed by ClearDefaultActionsLocked().
  return expectations_met;
}

// Registers a mock object and a mock method it owns.
// L < g_gmock_mutex
void Mock::Register(const void* mock_obj,
                    internal::UntypedFunctionMockerBase* mocker) {
  internal::MutexLock l(&internal::g_gmock_mutex);
  g_mock_object_registry.states()[mock_obj].function_mockers.insert(mocker);
}

// Tells Google Mock where in the source code mock_obj is used in an
// ON_CALL or EXPECT_CALL.  In case mock_obj is leaked, this
// information helps the user identify which object it is.
// L < g_gmock_mutex
void Mock::RegisterUseByOnCallOrExpectCall(
    const void* mock_obj, const char* file, int line) {
  internal::MutexLock l(&internal::g_gmock_mutex);
  MockObjectState& state = g_mock_object_registry.states()[mock_obj];
  if (state.first_used_file == NULL) {
    state.first_used_file = file;
    state.first_used_line = line;
    const TestInfo* const test_info =
        UnitTest::GetInstance()->current_test_info();
    if (test_info != NULL) {
      // TODO(wan@google.com): record the test case name when the
      // ON_CALL or EXPECT_CALL is invoked from SetUpTestCase() or
      // TearDownTestCase().
      state.first_used_test_case = test_info->test_case_name();
      state.first_used_test = test_info->name();
    }
  }
}

// Unregisters a mock method; removes the owning mock object from the
// registry when the last mock method associated with it has been
// unregistered.  This is called only in the destructor of
// FunctionMockerBase.
// L >= g_gmock_mutex
void Mock::UnregisterLocked(internal::UntypedFunctionMockerBase* mocker) {
  internal::g_gmock_mutex.AssertHeld();
  for (MockObjectRegistry::StateMap::iterator it =
           g_mock_object_registry.states().begin();
       it != g_mock_object_registry.states().end(); ++it) {
    FunctionMockers& mockers = it->second.function_mockers;
    if (mockers.erase(mocker) > 0) {
      // mocker was in mockers and has been just removed.
      if (mockers.empty()) {
        g_mock_object_registry.states().erase(it);
      }
      return;
    }
  }
}

// Clears all ON_CALL()s set on the given mock object.
// L >= g_gmock_mutex
void Mock::ClearDefaultActionsLocked(void* mock_obj) {
  internal::g_gmock_mutex.AssertHeld();

  if (g_mock_object_registry.states().count(mock_obj) == 0) {
    // No ON_CALL() was set on the given mock object.
    return;
  }

  // Clears the default actions for each mock method in the given mock
  // object.
  FunctionMockers& mockers =
      g_mock_object_registry.states()[mock_obj].function_mockers;
  for (FunctionMockers::const_iterator it = mockers.begin();
       it != mockers.end(); ++it) {
    (*it)->ClearDefaultActionsLocked();
  }

  // We don't clear the content of mockers, as they may still be
  // needed by VerifyAndClearExpectationsLocked().
}

Expectation::Expectation() {}

Expectation::Expectation(
    const internal::linked_ptr<internal::ExpectationBase>& an_expectation_base)
    : expectation_base_(an_expectation_base) {}

Expectation::~Expectation() {}

// Adds an expectation to a sequence.
void Sequence::AddExpectation(const Expectation& expectation) const {
  if (*last_expectation_ != expectation) {
    if (last_expectation_->expectation_base() != NULL) {
      expectation.expectation_base()->immediate_prerequisites_
          += *last_expectation_;
    }
    *last_expectation_ = expectation;
  }
}

// Creates the implicit sequence if there isn't one.
InSequence::InSequence() {
  if (internal::g_gmock_implicit_sequence.get() == NULL) {
    internal::g_gmock_implicit_sequence.set(new Sequence);
    sequence_created_ = true;
  } else {
    sequence_created_ = false;
  }
}

// Deletes the implicit sequence if it was created by the constructor
// of this object.
InSequence::~InSequence() {
  if (sequence_created_) {
    delete internal::g_gmock_implicit_sequence.get();
    internal::g_gmock_implicit_sequence.set(NULL);
  }
}

}  // namespace testing
