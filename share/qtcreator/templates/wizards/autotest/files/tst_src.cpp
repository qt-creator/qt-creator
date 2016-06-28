%{Cpp:LicenseTemplate}\
#include <QtTest>
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
    ~%{TestCaseName}();

private slots:
@if "%{GenerateInitAndCleanup}" == "true"
    void initTestCase();
    void cleanupTestCase();
@endif
    void test_case1();

};

%{TestCaseName}::%{TestCaseName}()
{

}

%{TestCaseName}::~%{TestCaseName}()
{

}

@if "%{GenerateInitAndCleanup}" == "true"
void %{TestCaseName}::initTestCase()
{

}

void %{TestCaseName}::cleanupTestCase()
{

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
