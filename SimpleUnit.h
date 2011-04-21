// This code is licensed under New BSD Licence. For details see project page at
// http://code.google.com/p/craplib/source/checkout

#pragma once
#ifndef __SIMPLE_UNIT_H__
#define __SIMPLE_UNIT_H__

#include <vector>
#include <sstream>
#include <string>
#include <exception>
#include <iostream>
#include <algorithm>

#include <math.h>

#include "LogUtil.h"

// Define our assert macros
#define rassert(cond) unittest::TestMan::test_assert(#cond, __FILE__, __LINE__, cond)

#define rassert_throws(cond) try { cond; \
	unittest::TestMan::test_assert_throw(__FILE__, __LINE__); \
} catch(unittest::TestMan::assert_exception& ex) { throw ex; } \
catch(...){ /* ok */ }

#define rassert_close(a,b,epsilon) unittest::TestMan::test_assert_close(__FILE__, __LINE__, a, b, epsilon)

// Define our own syntax
#define RUN_ALL() unittest::TestMan::runall()

#define RUN_SUITE(name) unittest::TestMan::run(#name)

#define SUITE(name) FIX_SUITE(name, unittest::NullFixture)

#define FIX_SUITE(name, fix) \
static struct SUITE_##name : public unittest::TestSuite<fix> {\
	SUITE_##name() : unittest::TestSuite<fix>(#name) {};\
} SUITE_##name##_object;

#define TEST(name, suite) FIX_TEST(name, suite, unittest::NullFixture)

#define FIX_TEST(name, suite, fix) \
struct TEST_##suite##_##name : public unittest::Test<SUITE_##suite, SUITE_##suite::fixture_type>, virtual public fix \
{ \
	virtual void run(); \
}; \
struct TEST_##suite##_##name##_HOLDER : public unittest::TestHolder< TEST_##suite##_##name > \
{\
	TEST_##suite##_##name##_HOLDER() \
	: unittest::TestHolder<TEST_##suite##_##name>( &SUITE_##suite##_object ) \
	{}; \
} TEST_##suite##_##name##_object; \
void TEST_##suite##_##name::run()

namespace unittest {
	
	struct BaseTestSuite;
	struct BaseTest;
	
	// BaseTest* is safe here. all the test objects are on the stack anyway so they will be automatically cleared
	typedef std::vector<BaseTest*>		TestsVec; 
	
	typedef std::pair<std::string, BaseTestSuite*> SuitePair;
	typedef std::vector<SuitePair> SuitesVec;
		
	////////////////////////////////////////////////////////////////////////////	
	/// A null fixture (default fixture, does nothing)
	struct NullFixture {
		NullFixture() {};
		~NullFixture() {};
	};
	
	////////////////////////////////////////////////////////////////////////////
	class TestMan {
	private:
		static SuitesVec	suites_;
				
		template<class _Suite>
		static TestsVec& getSuiteContainer() {
			static TestsVec suite_tests;
			return suite_tests;
		}
		
	public:
		class assert_exception : public std::exception {
		private:
			const char* codeSnippet_;
			const char* file_;
			const int	line_;
			
		public:
			assert_exception(const char* cs, const char* file, const int line) throw ()
			: codeSnippet_(cs)
			, file_(file)
			, line_(line)
			{};		
			virtual ~assert_exception() throw() { };
			
			virtual const char* what() const throw() { 
				std::stringstream ss;
				ss << "Assertion failure [" << file_ << ":" << line_ << "] " << codeSnippet_;
				return ss.str().c_str();
			};
		};
		
		
		template<class _Suite>
		static void reg(BaseTest* p) {
			
			// Register the test into the corresponding vector
			TestMan::getSuiteContainer<_Suite>().push_back( p );
			
		}

		static void reg(const char* name, BaseTestSuite* p) {
			
			// Just add a pointer to the suite
			TestMan::suites_.push_back( SuitePair( name, p) );
			
		}		
		
		static void runall();
		static void run(const char* name);
		
		static void test_assert(const char* code, 
								const char* file, 
								const int line, 
								bool condition) throw (assert_exception) 
		{
			if(!condition) {
				throw assert_exception( code, file, line );
			}
		}
		
		static void test_assert_throw(const char* file,
									   const int line) throw (assert_exception)
		{
			throw assert_exception("code didn't throw exception", file, line);
		};
		
		template<class T>
		static void test_assert_close(const char* file, 
									  const int line,
									  const T a,
									  const T b,
									  const T epsilon) throw (assert_exception);
	};
	
	SuitesVec TestMan::suites_;
	
	// Specializations of asserts for common types
	template<>
	void TestMan::test_assert_close<float>(const char* file, 
										   const int line,
										   const float a,
										   const float b,
										   const float epsilon) throw (assert_exception)
	{
		if( fabsf( a - b) >= epsilon ) 
			throw assert_exception("float values are not close enough", file, line);
	}
	
	
	////////////////////////////////////////////////////////////////////////////	
	/// A base class for suites. Just to store into a vector
	class BaseTestSuite {		
	public:
		BaseTestSuite(const char* name) {
			TestMan::reg( name, this );			
		};
				
		virtual void reg( BaseTest* p ) = 0;
		
		virtual void run() = 0;
	};
		
	////////////////////////////////////////////////////////////////////////////	
	/// A base class for tests. Just to store into a vector
	struct BaseTest {		
		virtual void run() = 0;
	};
	
	///////////////////////////////////////////////////////////////////////////	
	/// A struct which holds type information for a test and is capable
	/// of creating an instance of that test.
	template <class _Test>
	struct TestHolder
	: public BaseTest 
	{
		TestHolder(BaseTestSuite* suite) {
			suite->reg( this );
		};	
		
		virtual void run() { 
			_Test test;	// Create the test (setup)
			test.run(); // Run and teardown when we leave this method
		};		
	};	
	

	////////////////////////////////////////////////////////////////////////////	
	/// The test template
	template<class _Suite, class _FIX=NullFixture>
	struct Test 
	: public BaseTest
	, virtual public _FIX
	{		
	public:	
		// Override this
		virtual void run() = 0;
	};

	
	////////////////////////////////////////////////////////////////////////////	
	/// Test suite base template
	template<class _FIX=NullFixture>
	class TestSuite 
	: public BaseTestSuite
	{
	private:
		TestsVec tests_;

		class Runner {
		private:
			size_t totalTests_;
			size_t curTest_;
			size_t fail_;
		
		public:			
			Runner(size_t totalTests)
			: totalTests_(totalTests)
			, curTest_(1)
			, fail_(0)
			{ };
			
			void operator()(BaseTest* pair) {
				wLog("[+] Running test %d out of %d", curTest_, totalTests_);
				
				try {
					pair->run();
				} catch(std::exception& ex) {
					
					wLog("[!] FAILED: %s", ex.what());
					++fail_;
					
				} catch(...) {
					
					wLog("[!] FAILED: Unknown reason");
					++fail_;
					
				}
				
				++curTest_;				
			}
			
			operator size_t () { return fail_; };
		};

	public:
		typedef _FIX fixture_type;
		
		TestSuite(const char* name) 
		: BaseTestSuite(name)
		{};

		virtual void reg( BaseTest* p ) {
			tests_.push_back( p );
		}

		virtual void run() {
			size_t totalTests = tests_.size();
			size_t fail = 0;
			
			wLog("[+] Run all tests [%d]...", totalTests);
			fail = for_each(tests_.begin(), tests_.end(), Runner( totalTests ) );			
				
			if(fail) {
				wLog("[!] %u of %u failed!", fail, totalTests);
			} else {
				wLog("[+] All tests finished succesfully.");
			}
		}
	};

	
	void TestMan::runall() {
		size_t totalSuites = suites_.size();
		
		wLog("[+] Run all suites [%d]...", totalSuites);		
		for(SuitesVec::iterator it=suites_.begin(); it!=suites_.end(); ++it) {

			wLog("-----------------------------");
			wLog("Running suite %s", it->first.c_str());
			wLog("-----------------------------");
			it->second->run();
			wLog("-----------------------------");
			
		}
	}	

	class SuiteLookup {
	private:
		const std::string query_;
		
	public:
		SuiteLookup(const std::string& name) 
		: query_(name)
		{};
		
		bool operator()(const SuitePair& p) {
			if(query_ == p.first) 
				return true;
			return false;
		};
	};

	void TestMan::run(const char* name) {
		SuitesVec::iterator it = std::find_if( suites_.begin(), suites_.end(), SuiteLookup( name ) );
		if(it != suites_.end()) {
			wLog("-----------------------------");
			wLog("Running suite %s", it->first.c_str());
			wLog("-----------------------------");
			it->second->run();
			wLog("-----------------------------");
		}
	}
}

#endif // __SIMPLE_UNIT_H__
