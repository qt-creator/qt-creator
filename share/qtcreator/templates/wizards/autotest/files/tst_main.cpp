@if  "%{TestFrameWork}" == "QtQuickTest"
#include <QtQuickTest/quicktest.h>

QUICK_TEST_MAIN(example)
@else
%{Cpp:LicenseTemplate}\
#include "%{TestCaseFileWithHeaderSuffix}"

#include <gtest/gtest.h>

int main(int argc, char *argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
@endif
