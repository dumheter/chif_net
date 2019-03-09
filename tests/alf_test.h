// MIT License
//
// Copyright (c) 2018-2019 Filip Björklund
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef ALF_TEST_H
#define ALF_TEST_H

#if defined(__cplusplus)
extern "C" {
#endif

// ========================================================================== //
// Usage
// ========================================================================== //

/* INTRODUCTION:
 * This library is made to be easy to use, this means that there is no linking
 * involed.
 *
 * FEATURES:
 * The AlfTest supports an array of features that are useful primarily for unit
 * testing but also in the future for other types of testing.
 *
 * - Unit testing
 * - Suite based testing
 * - Wide array of checking (assert) functions
 * - Timing data for suites, tests and overall
 * - Colorized output with multiple predefined or custom themes
 * - No dependency on standard library if configurated
 *
 * USAGE:
 * To use it simply drop the "alf_test.h" file into your project. Then in ONLY
 * ONE source file define the macro "ALF_TEST_IMPL". This will create the
 * implementation in that file. Doing this in multiple files will result in
 * multiply defined symbols. To use the header from other files simply include
 * the header without defining the macro.
 *
 * CONFIG:
 * There are also some optional macros that can be defined to alter the
 * behaviour of the implementation.
 * - ALF_TEST_CUSTOM_ALLOC: Defining this macro signals the implementation that
 *		the user wants to supply its own allocation functions. The user then
 *		needs to define the macro ALF_TEST_ALLOC and ALF_TEST_FREE. The alloc
 *		function takes the size of the allocation being made while the free
 *		function takes the memory being freed.
 *		If this macro is not defined then malloc and free will be used for the
 *		implementation.
 * - ALF_TEST_ASSERT: Defining this macro signals the implementation that the
 *		user wants to supply its own assertion function for internal assertions.
 *		Note that this is not the assertions used for the actual testing. The
 *		ALF_TEST_ASSERT macro takes two arguments, the first being the condition
 *		and the second a message.
 *		If this macro is not defined then the assert function from the standard
 *		library will be used. The message is separated from the condition with a
 *		logical and (&&).
 *	- ALF_TEST_PRINTF: Defining this macro signals the implementation that the
 *		user wants to supply its own printf-like function that is used for
 *		printing the output of the tests. This macro expects a format string and
 *		a list of arguments to the format.
 *	- ALF_TEST_NO_TRUECOLOR: Defining this macro signals the implementation that
 *		it should not use truecolor ansi escape sequences for colorization of
 *		the printed information. This will then fallback to using 8-bit
 *		escape-sequence colors instead. The macro ALF_THEME_NONE can be used to
 *		disable colorization altogether.
 * - ALF_THEME_NONE: Defining this macro signals the implementation to not use
 *		any colorization at all for the printed output. It will instead be
 *		printed to the standard output without any formatting.
 * - ALF_THEME_*: This is a family of macro that can be defined to alter the
 *		color theme of the implementation. The themes might look different when
 *		truecolor support has been disabled as all colors are not available
 *		then.
 *		The star in the macro is supposed to be one of the following themes:
 *		- PALE: Pale colors.
 *		- AUTUMN_FRUIT: Purple and orange colors.
 *		If no theme macro is defined then the default color theme is used.
 * - ALF_THEME_PRINT_ABOUT: Defining this macro will have the implementation
 *		print information about the library before running any tests. This is
 *		off by default as to not clutter the information being printed.
 *
 * The user can also create custom themes. This is done by modifying the
 * implementation slightly to include a custom theme. The user should use the
 * template that is found in the theme declaration.
 *
 * THEORY:
 * The first thing to do when setting up tests is to define the function that
 * should be run as part of the test. These function has to have the prototype:
 * "void name(AlfTestState* state)", where the name can be anything.
 * These test are then collected in an array and specified as the argument,
 * together with the count, during creation of the suite.
 *
 * Suites are created with "alfCreateTestSuite". This function takes the name of
 * the suite, the array of tests and a count for the number of tests in the
 * array. When the user is done with the suite it should call
 * "alfDestroyTestSuite" to free all memory that it's using.
 *
 * The user may optionally call some functions for setting user data or callback
 * for the suite before running any tests. The user data is retrievable from any
 * test and can therefore be used to store data that a test must be able to use.
 * The callbacks that can be set is a setup and a teardown function that is
 * called before and after the suite is run.
 *
 * When the user has created a suite it's time to one of the multiple run
 * functions available. For running a single suite "alfRunSuite" should be used.
 * For instead running multiple suites at the same time "alfRunSuites" should be
 * used. Of course "alfRunSuites" can also be used to run a single suite,
 * however the information presented in the summary will slightly differ.
 *
 * During the tests the user calls one of the many "check" functions (in other
 * implementations called 'assert') to check/assert that some condition is true.
 * The possible alternatives are:
 * - alfCheckTrue: For checking that a condition is true.
 * - alfCheckFalse: For checking that a condition is false.
 * - alfCheckNotNull: For checking that a pointer is not NULL.
 * - alfCheckNull: For checking that a pointer is NULL.
 * - alfCheckMemEq: For checking that two regions of memory contains the same
 *		data.
 * - alfCheckStrEq: For checking that two nul-terminated c-strings are equal.
 * - alfCheckFloatEq: For checking that two single-precision floating-point
 *		numbers are equal. This takes into account the machine epsilon.
 * - alfCheckDoubleEq: For checking that two double-precision floating-point
 *		numbers are equal. This takes into account the machine epsilon.
 *
 * Each of the check functions also has a corresponding version with 'M'
 * appended to the name. These alternative functions also accepts a final
 * argument that is supposed to represent the explantion for or the reason
 * behind the test. This can be used to describe WHY the test is done and/or
 * WHAT it accomplishes.
 *
 * EXAMPLE:
 * Here follows a simple example of how to setup a suite to run 3 different
 * tests. The number of fails from running the test is then returned. The test
 * also sets the theme to "pale".
 *
 * !Note that the test in the example will fail because 3.14 != 3.15.
 *
 * -------------------------------------------------------------------------- *
 *
 * #define ALF_TEST_IMPL
 * #define ALF_TEST_THEME_PALE
 * #include "alf_test.h"
 *
 * void test1(AlfTestState* state)
 * {
 *     alfCheckFalse(state, 2 == 3);
 * 	   alfCheckTrueM(state, 3 == 3, "3 must equal 3");
 * }
 *
 * void test2(AlfTestState* state)
 * {
 *     alfCheckStrEq(state, "a string", "a string");
 *     alfCheckNotNull(state, (void*)1245);
 * }
 *
 * void test3(AlfTestState* state)
 * {
 * 	   alfCheckNull(state, NULL);
 * 	   alfCheckFloatEq(state, 3.14f, 3.15f);
 * }
 *
 * int main()
 * {
 *     AlfTest tests[3];
 *     tests[0] = (AlfTest) { .name = "Test1", .TestFunction = test1 };
 *     tests[1] = (AlfTest) { .name = "Test2", .TestFunction = test2 };
 *     tests[2] = (AlfTest) { .name = "Test3", .TestFunction = test3 };
 *     AlfTestSuite* suite = alfCreateTestSuite("Demo Suite", tests, 3);
 *
 *     const uint32_t fails = alfRunSuite(suite);
 *     alfDestroyTestSuite(suite);
 * 	   return fails;
 * }
 *
 * -------------------------------------------------------------------------- *
 *
 * END:
 * For more information visit the github repository: TODO(Filip Björklund)
 */

// ====================================================================== //
// Forward Declarations
// ====================================================================== //

typedef struct tag_AlfTestSuite AlfTestSuite;
typedef struct tag_AlfTestState AlfTestState;

// ========================================================================== //
// Type definitions
// ========================================================================== //

/** Test function **/
typedef void(*PFN_Test)(AlfTestState* state);

// -------------------------------------------------------------------------- //

/** Suite setup function **/
typedef void(*PFN_SuiteSetup)(AlfTestSuite* suite);

// -------------------------------------------------------------------------- //

/** Suite teardown function **/
typedef PFN_SuiteSetup PFN_SuiteTeardown;

// ========================================================================== //
// Macro declarations
// ========================================================================== //

// Filename macro
#if defined(_WIN32)
	/** Macro for name of current file **/
#	define __FILENAME__ (alfLastIndexOf(__FILE__, '\\') ? alfLastIndexOf(__FILE__, '\\') + 1 : __FILE__)
#else
	/** Macro for name of current file **/
#	define __FILENAME__ (alfLastIndexOf(__FILE__, '/') ? alfLastIndexOf(__FILE__, '/') + 1 : __FILE__)
#endif

/** Float epsilon **/
#define ALF_FLOAT_EPSILON (1.19e-07)
/** Double epsilon **/
#define ALF_DOUBLE_EPSILON (2.22e-16)

// ========================================================================== //
// Macros for checking
// ========================================================================== //

/** Check that condition is true **/
#define ALF_CHECK_TRUE(state, condition) \
	alfCheckTrue(state, condition, #condition, __FILENAME__, __LINE__, NULL)
/** Check that condition is true. With a reason **/
#define ALF_CHECK_TRUE_R(state, condition, reason) \
	alfCheckTrue(state, condition, #condition, __FILENAME__, __LINE__, reason)

/** Check that condition is false **/
#define ALF_CHECK_FALSE(state, condition) \
	alfCheckFalse(state, !(condition), #condition, __FILENAME__, __LINE__, NULL)
/** Check that condition is false. With a reason **/
#define ALF_CHECK_FALSE_R(state, condition, reason) \
	alfCheckFalse(state, !(condition), #condition, __FILENAME__, __LINE__, reason)

/** Check that a pointer is not NULL **/
#define ALF_CHECK_NOT_NULL(state, pointer)	\
	alfCheckNotNull(state, pointer, #pointer, __FILENAME__, __LINE__, NULL)
/** Check that a pointer is not NULL. With a reason **/
#define ALF_CHECK_NOT_NULL_R(state, pointer, reason)	\
	alfCheckNotNull(state, pointer, #pointer, __FILENAME__, __LINE__, reason)

/** Check that a pointer is NULL **/
#define ALF_CHECK_NULL(state, pointer)	\
	alfCheckNull(state, pointer, #pointer, __FILENAME__, __LINE__, NULL)
/** Check that a pointer is NULL. With a reason **/
#define ALF_CHECK_NULL_R(state, pointer, reason)	\
	alfCheckNull(state, pointer, #pointer, __FILENAME__, __LINE__, reason)

/** Check that two memory regions contains the same data **/
#define ALF_CHECK_MEMEQ(state, m0, m1, size)	\
	alfCheckMemEq(state, m0, m1, #m0, #m1, size, __FILENAME__, __LINE__, NULL)
/** Check that two memory regions contains the same data. With a reason **/
#define ALF_CHECK_MEMEQ_R(state, m0, m1, size, reason)	\
	alfCheckMemEq(state, m0, m1, #m0, #m1, size, __FILENAME__, __LINE__, reason)

/** Check that two nul-terminated c-strings are equal **/
#define ALF_CHECK_STREQ(state, str0, str1)								\
	alfCheckStrEq(														\
		state, str0, str1, #str0, #str1, __FILENAME__, __LINE__, NULL)
/** Check that two nul-terminated c-strings are equal. With a reason **/
#define ALF_CHECK_STREQ_R(state, str0, str1, reason)					\
	alfCheckStrEq(														\
		state, str0, str1, #str0, #str1, __FILENAME__, __LINE__, reason)

// ========================================================================== //
// Type definitions
// ========================================================================== //

/** Boolean type used **/
typedef unsigned long BOOL_TYPE;

// -------------------------------------------------------------------------- //

/** Standard integer type used **/
typedef unsigned long INT_TYPE;

// -------------------------------------------------------------------------- //

/** Size type used **/
typedef unsigned long long SIZE_TYPE;

// -------------------------------------------------------------------------- //

/** Integer type used for time **/
typedef unsigned long long TIME_TYPE;

// ========================================================================== //
// Structures
// ========================================================================== //

/** \struct AlfTestSuite
 * \author Filip Björklund
 * \date 29 september 2018 - 13:35
 * \brief Test suite.
 * \details
 * Represents a test suite which is a collection of tests.
 */
typedef struct tag_AlfTestSuite AlfTestSuite;

// -------------------------------------------------------------------------- //

/** \struct AlfTestState
 * \author Filip Björklund
 * \date 29 september 2018 - 13:35
 * \brief Test state.
 * \details
 * Represents the state during testing. An object of this type is the argument
 * of each test function.
 */
typedef struct tag_AlfTestState AlfTestState;

// -------------------------------------------------------------------------- //

/** \struct AlfTest
 * \author Filip Björklund
 * \date 29 september 2018 - 13:44
 * \brief Test.
 * \details
 * Represents a single test to be run as part of a test suite.
 */
typedef struct AlfTest
{
	/** Name of test **/
	char* name;
	/** Test function **/
	PFN_Test TestFunction;
} AlfTest;

// ========================================================================== //
// Functions
// ========================================================================== //

/** Create a test suite with a specified set of tests.
 * \brief Create test suite.
 * \note No transfer of ownership of dynamic resources is done. It's therefore
 * up to the user to free any data that it has allocated.
 * \param name Name of the test suite.
 * \param tests Tests that belong to suite.
 * \param count Number of tests.
 * \return Created test suite.
 */
AlfTestSuite* alfCreateTestSuite(char* name, AlfTest* tests, INT_TYPE count);

// -------------------------------------------------------------------------- //

/** Delete a test suite that was previously created with alfCreateTestSuite.
 * \brief Delete test suite.
 * \param suite Test suite to delete.
 */
void alfDestroyTestSuite(AlfTestSuite* suite);

// -------------------------------------------------------------------------- //

/** Run all the tests of a single test suite.
 * \brief Run test suite.
 * \param suite Suite to run.
 * \return Number of failed tests.
 */
INT_TYPE alfRunSuite(AlfTestSuite* suite);

// -------------------------------------------------------------------------- //

/** Run all the tests for a set of suites. The test are run in the order
 * specified and a summary is displayed to the user.
 * \brief Run a set of test suites.
 * \param suites Suites to run.
 * \param suiteCount Number of suites.
 * \return Number of failed checks in total in all tests.
 */
INT_TYPE alfRunSuites(AlfTestSuite** suites, INT_TYPE suiteCount);

// -------------------------------------------------------------------------- //

/** Set the user data of a test suite. This can contain any type of data that
 * the user might want to access during a test.
 * \brief Set suite user data.
 * \param suite Suite to set user data for.
 * \param data User data to set.
 */
void alfSetSuiteUserData(AlfTestSuite* suite, void* data);

// -------------------------------------------------------------------------- //

/** Returns the user data of a test suite.
 * \brief Returns test suite user data.
 * \param suite Suite to return user data of.
 * \return User data. NULL if no data has been set.
 */
void* alfGetSuiteUserData(AlfTestSuite* suite);

// -------------------------------------------------------------------------- //

/** Returns the user data of a test suite from a test state.
 * \brief Returns test suite user data from state.
 * \param state State to retrieve suite user data from.
 * \return User data. NULL if no data has been set.
 */
void* alfGetSuiteUserDataFromState(AlfTestState* state);

// -------------------------------------------------------------------------- //

/** Set the callback that will be called for a suite to setup. This is called
 * before the suite is run.
 * \note By default a blank function is called for setup.
 * \brief Set suite setup callback.
 * \param suite Suite to set callback for.
 * \param callback Callback to set.
 */
void alfSetSuiteSetupCallback(AlfTestSuite* suite, PFN_SuiteSetup callback);

// -------------------------------------------------------------------------- //

/** Set the callback that will be called for a suite to teardown. This is called
 * after all the tests of the suite has run.
 * \brief Set suite teardowns callback.
 * \param suite Suite to set callback for.
 * \param callback Callback to set.
 */
void alfSetSuiteTeardownCallback(AlfTestSuite* suite, PFN_SuiteTeardown callback);

// -------------------------------------------------------------------------- //

/** Clear the setup callback for a suite.
 * \brief Clear suite setup callback.
 * \param suite Suite to clear callback for.
 */
void alfClearSuiteSetupCallback(AlfTestSuite* suite);

// -------------------------------------------------------------------------- //

/** Clear the teardown callback for a suite.
 * \brief Clear suite teardown callback.
 * \param suite Suite to clear callback for.
 */
void alfClearSuiteTeardownCallback(AlfTestSuite* suite);

// -------------------------------------------------------------------------- //

/** Function to do a check (assertion) during a test. This should however not be
 * used directly. Instead use the check macros.
 * \brief Function for checking during test.
 * \param state Test state.
 * \param predicate Predicate to check.
 * \param predicateString Predicate in string form.
 * \param file File in which check is done.
 * \param line Line in file.
 * \param reason Reason for the check.
 */
void alfCheckTrue(
	AlfTestState* state,
	BOOL_TYPE predicate,
	const char* predicateString,
	const char* file,
	INT_TYPE line,
	const char* reason);

// -------------------------------------------------------------------------- //

void alfCheckFalse(
	AlfTestState* state,
	BOOL_TYPE predicate,
	const char* predicateString,
	const char* file,
	INT_TYPE line,
	const char* reason);

// -------------------------------------------------------------------------- //

void alfCheckNotNull(
	AlfTestState* state,
	void* pointer,
	const char* pointerText,
	const char* file,
	INT_TYPE line,
	const char* reason);

// -------------------------------------------------------------------------- //

void alfCheckNull(
	AlfTestState* state,
	void* pointer,
	const char* pointerText,
	const char* file,
	INT_TYPE line,
	const char* reason);

// -------------------------------------------------------------------------- //

void alfCheckMemEq(
	AlfTestState* state,
	const void* m0,
	const void* m1,
	const char* var0,
	const char* var1,
	SIZE_TYPE size,
	const char* file,
	INT_TYPE line,
	const char* reason);

// -------------------------------------------------------------------------- //

void alfCheckStrEq(
	AlfTestState* state,
	const char* str0,
	const char* str1,
	const char* var0,
	const char* var1,
	const char* file,
	INT_TYPE line,
	const char* reason);

// -------------------------------------------------------------------------- //

char* alfLastIndexOf(char* string, char character);

// ========================================================================== //
// End of header
// ========================================================================== //

#if defined(__cplusplus)
}
#endif

#endif // ALF_TEST_H
