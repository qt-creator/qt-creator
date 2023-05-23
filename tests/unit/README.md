# Contribution Guideline

This document summarizes;

* Best practices for writing tests
* How to add a new test
* How to build only specific test

All tests here depend on the [GoogleTest][1] framework.

## Best Practices

We're following those patterns/approaches;

* The Arrange, Act, and Assert (AAA) Pattern
* Given When Then (GWT) Pattern

## Adding a New Unit Test

* Please add your tests under  `unit/unittest`. No subfolders are needed.
* Name your class as `foo-test.cpp`

* Always include `googletest.h` header. Without that you may get the printer function can be broken because the are not anymore ODR (because of weak linking to printers for example). It is also necessary for nice printers, also adds Qt known matchers.

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
