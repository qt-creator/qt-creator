%{Cpp:LicenseTemplate}\
#include <QTest>
@if "%{RequireApplication}" == "true"
%{JS: QtSupport.qtIncludes([ 'QtCore/QCoreApplication' ],
                           [ 'QtCore/QCoreApplication' ]) }\
@endif

// add necessary includes here

class %{TestCaseName} : public QObject
{
 Q_OBJECT

public:
    %{TestCaseName}();
    ~%{TestCaseName}() override;

private slots:
@if "%{GenerateInitAndCleanup}" == "true"
    void initTestCase();
    void init();
    void cleanupTestCase();
    void cleanup();
@endif
    void test_case1();

};

%{TestCaseName}::%{TestCaseName}()
{

}

%{TestCaseName}::~%{TestCaseName}() = default;

@if "%{GenerateInitAndCleanup}" == "true"
void %{TestCaseName}::initTestCase()
{
    // code to be executed before the first test function
}

void %{TestCaseName}::init()
{
    // code to be executed before each test function
}

void %{TestCaseName}::cleanupTestCase()
{
    // code to be executed after the last test function
}

void %{TestCaseName}::cleanup()
{
    // code to be executed after each test function
}

@endif
void %{TestCaseName}::test_case1()
{

}

@if "%{RequireApplication}" == "true"
QTEST_MAIN(%{TestCaseName})
@else
QTEST_APPLESS_MAIN(%{TestCaseName})
@endif

#include "%{JS: 'tst_%{TestCaseName}.moc'.toLowerCase() }"
