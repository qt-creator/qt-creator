# Contribution Guideline

This document summarizes;

* Best practices for writing tests
* How the test folder is organized
* How to add a new test
* How to build only specific test

All tests here depend on the [GoogleTest][1] framework.

## Best Practices

We're following those patterns/approaches;

* The Arrange, Act, and Assert (AAA) Pattern
* Given When Then (GWT) Pattern

## Test Organization

Here is the general folder structure;

```cpp
unit (main CMakeLists.txt)
|- README.md
|- 3rdparty // 3rd party dependencies
|  `- googletest
|- tools    // custom tools for testing
|   `- your-custom-folder
`- tests    // all tests are here, they all extend main CMake
    |- integrationtests // integration tests, executable
    |- matchers         // custom google-test matchers for testing, library
    |- mocks            // mocks for testing, library
    |- stubs            // stubs for testing, library or executable
    |- printers         // custom google-test matcher printers for testing, library
    |- unittests        // unit tests are here, executable
    `- utils            // common utilities which are mostly included by tests
```

Unit test and integration test folders are structured as the following;

```cpp
unittests (and integrationtests)
|- †est-folder-1        // folder for a specific test cluster (or test set)
|   |- CMakelists.txt   // cmake file for extending main CMake
|   |- data             // data folder for testing
|   `- foo-test.cpp     // necessary test files
`- test-folder-2
    |- CMakelists.txt
    |- data
    `- foo-test.cpp
```

## Adding a New Test

* Please add your tests under  `tests/unittest` or `tests/integrationtest` folder.
* Always add your tests to a specific test folder. Please check the test organization section for more information.
* If you need to add a new test folder;
    * Create a new folder
    * Create a new CMakeLists.txt file
    * Add your test files to the folder
    * Add your test data to the folder. Please use `data` folder for test data.
    * Add `unittest_copy_data_folder()` to your CMakeLists.txt file to copy your test data to the build folder.
    * You can access test data from your test code with `UNITTEST_DIR` macro followed by `<your-folder-name>/data` path.
* Name your test files as `foo-test.cpp`.
* Always include `googletest.h` header. Without that you may get the printer function can be broken because the are not anymore ODR (because of weak linking to printers for example). It is also necessary for nice printers, also adds Qt known matchers.
* Use snake_case for the test name to improve readability for long sentences

## Building Tests

> Note:
> When you're building the application from the terminal, you can set environment variables instead of settings CMake flags.
> The corresponding environment variable name is same with CMake variable name but with a 'QTC_' prefix.
> CMake Variable: WITH_TESTS
> Environment Variable: QTC_WITH_TESTS

You have to enable tests with the following CMake variable otherwise the default configuration skips them.

```bash
WITH_TESTS=ON
```

## Building Specific Tests

After enabling tests you can use test-specific CMake flags to customize which tests should be built instead of building all of them at once. Please check the relevant CMake file to see which variable is required to enable that specific test.

```bash
BUILD_TESTS_BY_DEFAULT=OFF
BUILD_TEST_UNITTEST=ON
BUILD_TEST_TST_QML_TESTCORE=ON
```

[1]: https://github.com/google/googletest
