/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "debuggerprotocol.h"
#include "simplifytype.h"
#include "watchdata.h"
#include "watchutils.h"

#ifdef Q_OS_WIN
#include <utils/environment.h>
#ifdef Q_CC_MSVC
#include <utils/qtcprocess.h>
#include <utils/fileutils.h>
#include <utils/synchronousprocess.h>
#endif // Q_CC_MSVC
#endif // Q_OS_WIN

#include <QtTest>
#include <math.h>

#define MSKIP_SINGLE(x) do { disarm(); QSKIP(x); } while (0)

using namespace Debugger;
using namespace Internal;

#ifdef Q_CC_MSVC

// Copied from abstractmsvctoolchain.cpp to avoid plugin dependency.
static bool generateEnvironmentSettings(Utils::Environment &env,
                                        const QString &batchFile,
                                        const QString &batchArgs,
                                        QMap<QString, QString> &envPairs)
{
    // Create a temporary file name for the output. Use a temporary file here
    // as I don't know another way to do this in Qt...
    // Note, can't just use a QTemporaryFile all the way through as it remains open
    // internally so it can't be streamed to later.
    QString tempOutFile;
    QTemporaryFile* pVarsTempFile = new QTemporaryFile(QDir::tempPath() + QLatin1String("/XXXXXX.txt"));
    pVarsTempFile->setAutoRemove(false);
    pVarsTempFile->open();
    pVarsTempFile->close();
    tempOutFile = pVarsTempFile->fileName();
    delete pVarsTempFile;

    // Create a batch file to create and save the env settings
    Utils::TempFileSaver saver(QDir::tempPath() + QLatin1String("/XXXXXX.bat"));

    QByteArray call = "call ";
    call += Utils::QtcProcess::quoteArg(batchFile).toLocal8Bit();
    if (!batchArgs.isEmpty()) {
        call += ' ';
        call += batchArgs.toLocal8Bit();
    }
    saver.write(call + "\r\n");

    const QByteArray redirect = "set > " + Utils::QtcProcess::quoteArg(
                                    QDir::toNativeSeparators(tempOutFile)).toLocal8Bit() + "\r\n";
    saver.write(redirect);
    if (!saver.finalize()) {
        qWarning("%s: %s", Q_FUNC_INFO, qPrintable(saver.errorString()));
        return false;
    }

    Utils::QtcProcess run;
    // As of WinSDK 7.1, there is logic preventing the path from being set
    // correctly if "ORIGINALPATH" is already set. That can cause problems
    // if Creator is launched within a session set up by setenv.cmd.
    env.unset(QLatin1String("ORIGINALPATH"));
    run.setEnvironment(env);
    const QString cmdPath = QString::fromLocal8Bit(qgetenv("COMSPEC"));
    // Windows SDK setup scripts require command line switches for environment expansion.
    QString cmdArguments = QLatin1String(" /E:ON /V:ON /c \"");
    cmdArguments += QDir::toNativeSeparators(saver.fileName());
    cmdArguments += QLatin1Char('"');
    run.setCommand(cmdPath, cmdArguments);
    run.start();

    if (!run.waitForStarted()) {
        qWarning("%s: Unable to run '%s': %s", Q_FUNC_INFO, qPrintable(batchFile),
                 qPrintable(run.errorString()));
        return false;
    }
    if (!run.waitForFinished()) {
        qWarning("%s: Timeout running '%s'", Q_FUNC_INFO, qPrintable(batchFile));
        Utils::SynchronousProcess::stopProcess(run);
        return false;
    }
    // The SDK/MSVC scripts do not return exit codes != 0. Check on stdout.
    const QByteArray stdOut = run.readAllStandardOutput();
    if (!stdOut.isEmpty() && (stdOut.contains("Unknown") || stdOut.contains("Error")))
        qWarning("%s: '%s' reports:\n%s", Q_FUNC_INFO, call.constData(), stdOut.constData());

    //
    // Now parse the file to get the environment settings
    QFile varsFile(tempOutFile);
    if (!varsFile.open(QIODevice::ReadOnly))
        return false;

    QRegExp regexp(QLatin1String("(\\w*)=(.*)"));
    while (!varsFile.atEnd()) {
        const QString line = QString::fromLocal8Bit(varsFile.readLine()).trimmed();
        if (regexp.exactMatch(line)) {
            const QString varName = regexp.cap(1);
            const QString varValue = regexp.cap(2);

            if (!varValue.isEmpty())
                envPairs.insert(varName, varValue);
        }
    }

    // Tidy up and remove the file
    varsFile.close();
    varsFile.remove();

    return true;
}


#ifndef CDBEXT_PATH
#define CDBEXT_PATH ""
#endif

#endif // Q_CC_MSVC

struct VersionBase
{
    // Minimum and maximum are inclusive.
    VersionBase(int minimum = 0, int maximum = INT_MAX)
    {
        isRestricted = minimum != 0 || maximum != INT_MAX;
        max = maximum;
        min = minimum;
    }

    bool covers(int what) const
    {
        return !isRestricted || (min <= what && what <= max);
    }

    bool isRestricted;
    int min;
    int max;
};

struct QtVersion : VersionBase
{
    explicit QtVersion(int minimum = 0, int maximum = INT_MAX)
        : VersionBase(minimum, maximum)
    {}
};

struct DwarfVersion : VersionBase
{
    explicit DwarfVersion(int minimum = 0, int maximum = INT_MAX)
        : VersionBase(minimum, maximum)
    {}
};

struct GccVersion : VersionBase
{
    explicit GccVersion(int minimum = 0, int maximum = INT_MAX)
        : VersionBase(minimum, maximum)
    {}
};

struct ClangVersion : VersionBase
{
    explicit ClangVersion(int minimum = 0, int maximum = INT_MAX)
        : VersionBase(minimum, maximum)
    {}
};

struct MsvcVersion : VersionBase
{
    explicit MsvcVersion(int minimum = 0, int maximum = INT_MAX)
        : VersionBase(minimum, maximum)
    {}
};

struct GdbVersion : VersionBase
{
    explicit GdbVersion(int minimum = 0, int maximum = INT_MAX)
        : VersionBase(minimum, maximum)
    {}
};

struct LldbVersion : VersionBase
{
    explicit LldbVersion(int minimum = 0, int maximum = INT_MAX)
        : VersionBase(minimum, maximum)
    {}
};

struct BoostVersion : VersionBase
{
    explicit BoostVersion(int minimum = 0, int maximum = INT_MAX)
        : VersionBase(minimum, maximum)
    {}
};

static QString noValue = "\001";

enum DebuggerEngine
{
    NoEngine = 0x00,

    GdbEngine = 0x01,
    CdbEngine = 0x02,
    LldbEngine = 0x04,

    AllEngines = GdbEngine | CdbEngine | LldbEngine,

    NoCdbEngine = AllEngines & (~CdbEngine),
    NoLldbEngine = AllEngines & (~LldbEngine),
    NoGdbEngine = AllEngines & (~GdbEngine)
};

struct Context
{
    Context(DebuggerEngine engine) : engine(engine) {}
    QString nameSpace;
    int qtVersion = 0;
    int gccVersion = 0;
    int clangVersion = 0;
    int msvcVersion = 0;
    int boostVersion = 0;
    DebuggerEngine engine;
};

struct Name
{
    Name() : name(noValue) {}
    Name(const char *str) : name(QString::fromUtf8(str)) {}
    Name(const QString &ba) : name(ba) {}

    bool matches(const QString &actualName0, const Context &context) const
    {
        QString actualName = actualName0;
        QString expectedName = name;
        expectedName.replace("@Q", context.nameSpace + 'Q');
        return actualName == expectedName;
    }

    QString name;
};

static Name nameFromIName(const QString &iname)
{
    int pos = iname.lastIndexOf('.');
    return Name(pos == -1 ? iname : iname.mid(pos + 1));
}

static QString parentIName(const QString &iname)
{
    int pos = iname.lastIndexOf('.');
    return pos == -1 ? QString() : iname.left(pos);
}

struct Value
{
    Value() : value(noValue) {}
    Value(const char *str) : value(QLatin1String(str)) {}
    Value(const QString &str) : value(str) {}

    bool matches(const QString &actualValue0, const Context &context) const
    {
        if (value == noValue)
            return true;

        if (context.qtVersion) {
            if (qtVersion == 4) {
                if (context.qtVersion < 0x40000 || context.qtVersion >= 0x50000) {
                    //QWARN("Qt 4 specific case skipped");
                    return true;
                }
            } else if (qtVersion == 5) {
                if (context.qtVersion < 0x50000 || context.qtVersion >= 0x60000) {
                    //QWARN("Qt 5 specific case skipped");
                    return true;
                }
            }
        }
        QString actualValue = actualValue0;
        if (actualValue == " ")
            actualValue.clear(); // FIXME: Remove later.
        QString expectedValue = value;
        if (substituteNamespace)
            expectedValue.replace('@', context.nameSpace);

        if (isPattern) {
            expectedValue.replace("(", "!");
            expectedValue.replace(")", "!");
            actualValue.replace("(", "!");
            actualValue.replace(")", "!");
            //QWARN(qPrintable("MATCH EXP: " + expectedValue + "   ACT: " + actualValue));
            //QWARN(QRegExp(expectedValue).exactMatch(actualValue) ? "OK" : "NOT OK");
            return QRegExp(expectedValue).exactMatch(actualValue);
        }

        if (hasPtrSuffix)
            return actualValue.startsWith(expectedValue + " @0x")
                || actualValue.startsWith(expectedValue + "@0x");

        if (isFloatValue) {
            double f1 = fabs(expectedValue.toDouble());
            double f2 = fabs(actualValue.toDouble());
            //qDebug() << "expected float: " << qPrintable(expectedValue) << f1;
            //qDebug() << "actual   float: " << qPrintable(actualValue) << f2;
            if (f1 < f2)
                std::swap(f1, f2);
            return f1 - f2 <= 0.01 * f2;
        }


        return actualValue == expectedValue;
    }

    QString value;
    bool hasPtrSuffix = false;
    bool isFloatValue = false;
    bool substituteNamespace = true;
    bool isPattern = false;
    int qtVersion = 0;
};

struct ValuePattern : Value
{
    ValuePattern(const QString &ba) : Value(ba) { isPattern = true; }
};

struct Pointer : Value
{
    Pointer() { hasPtrSuffix = true; }
    Pointer(const QString &value) : Value(value) { hasPtrSuffix = true; }
};

struct FloatValue : Value
{
    FloatValue() { isFloatValue = true; }
    FloatValue(const QString &value) : Value(value) { isFloatValue = true; }
};

struct Value4 : Value
{
    Value4(const QString &value) : Value(value) { qtVersion = 4; }
};

struct Value5 : Value
{
    Value5(const QString &value) : Value(value) { qtVersion = 5; }
};

struct UnsubstitutedValue : Value
{
    UnsubstitutedValue(const QString &value) : Value(value) { substituteNamespace = false; }
};

struct Optional {};

struct Type
{
    Type() {}
    Type(const char *str) : type(QString::fromUtf8(str)) {}
    Type(const QString &ba) : type(ba) {}

    bool matches(const QString &actualType0, const Context &context,
                 bool fullNamespaceMatch = true) const
    {
        if (context.qtVersion) {
            if (qtVersion == 4) {
                if (context.qtVersion < 0x40000 || context.qtVersion >= 0x50000) {
                    //QWARN("Qt 4 specific case skipped");
                    return true;
                }
            } else if (qtVersion == 5) {
                if (context.qtVersion < 0x50000 || context.qtVersion >= 0x60000) {
                    //QWARN("Qt 5 specific case skipped");
                    return true;
                }
            }
        }
        QString actualType = simplifyType(actualType0);
        actualType.replace(' ', "");
        actualType.replace("const", "");
        QString expectedType;
        if (aliasName.isEmpty() || context.engine == CdbEngine)
            expectedType = type;
        else
            expectedType = aliasName;
        expectedType.replace(' ', "");
        expectedType.replace("const", "");
        expectedType.replace('@', context.nameSpace);

        if (isPattern)
            return QRegExp(expectedType).exactMatch(actualType);

        if (fullNamespaceMatch)
            expectedType.replace('?', context.nameSpace);
        else
            expectedType.replace('?', "");

        if (actualType == expectedType)
            return true;

        // LLDB 3.7 on Linux doesn't get the namespace right in QMapNode:
        // t = lldb.target.FindFirstType('Myns::QMapNode<int, CustomStruct>')
        // t.GetName() -> QMapNode<int, CustomStruct> (no Myns::)
        // So try again without namespace.
        if (fullNamespaceMatch)
            return matches(actualType0, context, false);

        return false;
    }

    QString type;
    QString aliasName;
    int qtVersion = 0;
    bool isPattern = false;
};

struct Type4 : Type
{
    Type4(const QString &ba) : Type(ba) { qtVersion = 4; }
};

struct Type5 : Type
{
    Type5(const QString &ba) : Type(ba) { qtVersion = 5; }
};

struct TypePattern : Type
{
    TypePattern(const QString &ba) : Type(ba) { isPattern = true; }
};

struct TypeDef : Type
{
    TypeDef(const QString &typeName, const QString &aliasName) : Type(typeName)
    {
        this->aliasName = aliasName;
    }
};

struct RequiredMessage
{
    RequiredMessage() {}

    RequiredMessage(const QString &message) : message(message) {}

    QString message;
};

struct Check
{
    Check() {}

    Check(const QString &iname, const Value &value, const Type &type)
        : iname(iname), expectedName(nameFromIName(iname)),
          expectedValue(value), expectedType(type)
    {}

    Check(const QString &iname, const Name &name,
         const Value &value, const Type &type)
        : iname(iname), expectedName(name),
          expectedValue(value), expectedType(type)
    {}

    bool matches(DebuggerEngine engine, int debuggerVersion, const Context &context) const
    {
        return (engine & enginesForCheck)
            && debuggerVersionForCheck.covers(debuggerVersion)
            && gccVersionForCheck.covers(context.gccVersion)
            && clangVersionForCheck.covers(context.clangVersion)
            && msvcVersionForCheck.covers(context.msvcVersion)
            && qtVersionForCheck.covers(context.qtVersion);
    }

    const Check &operator%(Optional) const
    {
        optionallyPresent = true;
        return *this;
    }

    const Check &operator%(DebuggerEngine engine) const
    {
        enginesForCheck = engine;
        return *this;
    }

    const Check &operator%(GdbVersion version) const
    {
        enginesForCheck = GdbEngine;
        debuggerVersionForCheck = version;
        return *this;
    }

    const Check &operator%(LldbVersion version) const
    {
        enginesForCheck = LldbEngine;
        debuggerVersionForCheck = version;
        return *this;
    }

    const Check &operator%(GccVersion version) const
    {
        enginesForCheck = NoCdbEngine;
        gccVersionForCheck = version;
        return *this;
    }

    const Check &operator%(ClangVersion version) const
    {
        enginesForCheck = GdbEngine;
        clangVersionForCheck = version;
        return *this;
    }

    const Check &operator%(MsvcVersion version) const
    {
        enginesForCheck = CdbEngine;
        msvcVersionForCheck = version;
        return *this;
    }

    const Check &operator%(BoostVersion version) const
    {
        boostVersionForCheck = version;
        return *this;
    }

    const Check &operator%(QtVersion version) const
    {
        qtVersionForCheck = version;
        return *this;
    }

    QString iname;
    Name expectedName;
    Value expectedValue;
    Type expectedType;

    mutable int enginesForCheck = AllEngines;
    mutable VersionBase debuggerVersionForCheck;
    mutable VersionBase gccVersionForCheck;
    mutable VersionBase clangVersionForCheck;
    mutable VersionBase msvcVersionForCheck;
    mutable QtVersion qtVersionForCheck;
    mutable BoostVersion boostVersionForCheck;
    mutable bool optionallyPresent = false;
};

struct CheckType : public Check
{
    CheckType(const QByteArray &iname, const Name &name,
         const Type &type)
        : Check(iname, name, noValue, type)
    {}

    CheckType(const QByteArray &iname, const Type &type)
        : Check(iname, noValue, type)
    {}
};

const QtVersion Qt4 = QtVersion(0, 0x4ffff);
const QtVersion Qt5 = QtVersion(0x50000);

struct Check4 : Check
{
    Check4(const QByteArray &iname, const Value &value, const Type &type)
        : Check(iname, value, type) { qtVersionForCheck = Qt4; }

    Check4(const QByteArray &iname, const Name &name, const Value &value, const Type &type)
        : Check(iname, name, value, type) { qtVersionForCheck = Qt4; }
};

struct Check5 : Check
{
    Check5(const QByteArray &iname, const Value &value, const Type &type)
        : Check(iname, value, type) { qtVersionForCheck = Qt5; }

    Check5(const QByteArray &iname, const Name &name, const Value &value, const Type &type)
        : Check(iname, name, value, type) { qtVersionForCheck = Qt5; }

};

struct Profile
{
    Profile(const QByteArray &contents) : contents(contents) {}

    QByteArray includes;
    QByteArray contents;
};


struct Cxx11Profile : public Profile
{
    Cxx11Profile()
      : Profile("greaterThan(QT_MAJOR_VERSION,4): CONFIG += c++11\n"
                "else: QMAKE_CXXFLAGS += -std=c++0x\n")
    {}
};

struct BoostProfile : public Profile
{
    BoostProfile()
      : Profile(QByteArray())
    {
        const QByteArray &boostIncPath = qgetenv("QTC_BOOST_INCLUDE_PATH_FOR_TEST");
        if (!boostIncPath.isEmpty())
            contents = QByteArray("INCLUDEPATH += ") + boostIncPath.constData();
        else
            contents = "macx:INCLUDEPATH += /usr/local/include";
        const QByteArray &boostLibPath = qgetenv("QTC_BOOST_LIBRARY_PATH_FOR_TEST");
        if (!boostLibPath.isEmpty())
            contents += QByteArray("\nLIBS += \"-L") + boostLibPath.constData() + QByteArray("\"");
        includes = "#include <boost/version.hpp>\n";
    }
};

struct MacLibCppProfile : public Profile
{
    MacLibCppProfile()
      : Profile("macx {\n"
                "QMAKE_CXXFLAGS += -stdlib=libc++\n"
                "LIBS += -stdlib=libc++\n"
                "QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.7\n"
                "QMAKE_IOS_DEPLOYMENT_TARGET = 10.7\n"
                "QMAKE_CFLAGS -= -mmacosx-version-min=10.6\n"
                "QMAKE_CFLAGS += -mmacosx-version-min=10.7\n"
                "QMAKE_CXXFLAGS -= -mmacosx-version-min=10.6\n"
                "QMAKE_CXXFLAGS += -mmacosx-version-min=10.7\n"
                "QMAKE_OBJECTIVE_CFLAGS -= -mmacosx-version-min=10.6\n"
                "QMAKE_OBJECTIVE_CFLAGS += -mmacosx-version-min=10.7\n"
                "QMAKE_LFLAGS -= -mmacosx-version-min=10.6\n"
                "QMAKE_LFLAGS += -mmacosx-version-min=10.7\n"
                "}")
    {}
};

struct ForceC {};
struct EigenProfile {};
struct UseDebugImage {};
struct DwarfProfile { explicit DwarfProfile(int v) : version(v) {} int version; };

struct CoreProfile {};
struct CorePrivateProfile {};
struct GuiProfile {};
struct GuiPrivateProfile {};
struct NetworkProfile {};
struct QmlProfile {};
struct QmlPrivateProfile {};
struct SqlProfile {};

struct NimProfile {};

struct BigArrayProfile {};

class Data
{
public:
    Data() {}

    Data(const QString &includes, const QString &code)
        : includes(includes), code(code)
    {}

    const Data &operator+(const Check &check) const
    {
        checks.append(check);
        return *this;
    }

    const Data &operator+(const RequiredMessage &check) const
    {
        requiredMessages.append(check);
        return *this;
    }

    const Data &operator+(const Profile &profile) const
    {
        profileExtra += profile.contents;
        includes += profile.includes;
        return *this;
    }

    const Data &operator+(const QtVersion &qtVersion) const
    {
        neededQtVersion = qtVersion;
        return *this;
    }

    const Data &operator+(const GccVersion &gccVersion) const
    {
        neededGccVersion = gccVersion;
        return *this;
    }

    const Data &operator+(const ClangVersion &clangVersion) const
    {
        neededClangVersion = clangVersion;
        return *this;
    }

    const Data &operator+(const MsvcVersion &msvcVersion) const
    {
        neededMsvcVersion = msvcVersion;
        return *this;
    }

    const Data &operator+(const GdbVersion &gdbVersion) const
    {
        neededGdbVersion = gdbVersion;
        return *this;
    }

    const Data &operator+(const LldbVersion &lldbVersion) const
    {
        neededLldbVersion = lldbVersion;
        return *this;
    }

    const Data &operator+(const BoostVersion &boostVersion) const
    {
        neededBoostVersion = boostVersion;
        return *this;
    }

    const Data &operator+(const DwarfVersion &dwarfVersion) const
    {
        neededDwarfVersion = dwarfVersion;
        return *this;
    }

    const Data &operator+(const DebuggerEngine &enginesForTest) const
    {
        engines = enginesForTest;
        return *this;
    }

    const Data &operator+(const EigenProfile &) const
    {
        profileExtra +=
            "exists(/usr/include/eigen3/Eigen/Core) {\n"
            "    DEFINES += HAS_EIGEN\n"
            "    INCLUDEPATH += /usr/include/eigen3\n"
            "}\n"
            "exists(/usr/local/include/eigen3/Eigen/Core) {\n"
            "    DEFINES += HAS_EIGEN\n"
            "    INCLUDEPATH += /usr/local/include/eigen3\n"
            "}\n"
            "\n"
            "exists(/usr/include/eigen2/Eigen/Core) {\n"
            "    DEFINES += HAS_EIGEN\n"
            "    INCLUDEPATH += /usr/include/eigen2\n"
            "}\n"
            "exists(/usr/local/include/eigen2/Eigen/Core) {\n"
            "    DEFINES += HAS_EIGEN\n"
            "    INCLUDEPATH += /usr/local/include/eigen2\n"
            "}\n";

        return *this;
    }

    const Data &operator+(const DwarfProfile &p)
    {
        profileExtra +=
            "QMAKE_CXXFLAGS -= -g\n"
            "QMAKE_CXXFLAGS += -gdwarf-" + QString::number(p.version) + "\n";
        return *this;
    }

    const Data &operator+(const UseDebugImage &) const
    {
        useDebugImage = true;
        return *this;
    }

    const Data &operator+(const CoreProfile &) const
    {
        profileExtra += "QT += core\n";

        useQt = true;
        useQHash = true;

        return *this;
    }

    const Data &operator+(const NetworkProfile &) const
    {
        profileExtra += "QT += core network\n";

        useQt = true;
        useQHash = true;

        return *this;
    }

    const Data &operator+(const SqlProfile &) const
    {
        profileExtra += "QT += core sql\n";

        useQt = true;

        return *this;
    }

    const Data &operator+(const BigArrayProfile &) const
    {
        this->bigArray = true;
        return *this;
    }

    const Data &operator+(const GuiProfile &) const
    {
        this->operator+(CoreProfile());
        profileExtra +=
            "QT += gui\n"
            "greaterThan(QT_MAJOR_VERSION, 4):QT *= widgets\n";

        return *this;
    }

    const Data &operator+(const CorePrivateProfile &) const
    {
        this->operator+(CoreProfile());
        profileExtra +=
            "greaterThan(QT_MAJOR_VERSION, 4) {\n"
            "  QT += core-private\n"
            "  CONFIG += no_private_qt_headers_warning\n"
            "}";

        return *this;
    }

    const Data &operator+(const GuiPrivateProfile &) const
    {
        this->operator+(GuiProfile());
        profileExtra +=
            "greaterThan(QT_MAJOR_VERSION, 4) {\n"
            "  QT += gui-private\n"
            "  CONFIG += no_private_qt_headers_warning\n"
            "}";

        return *this;
    }

    const Data &operator+(const QmlProfile &) const
    {
        this->operator+(CoreProfile());
        profileExtra +=
            "greaterThan(QT_MAJOR_VERSION, 4) {\n"
            "  QT += qml\n"
            "}";

        return *this;
    }

    const Data &operator+(const QmlPrivateProfile &) const
    {
        this->operator+(QmlProfile());
        profileExtra +=
            "greaterThan(QT_MAJOR_VERSION, 4) {\n"
            "  QT += qml-private\n"
            "  CONFIG += no_private_qt_headers_warning\n"
            "}";

        return *this;
    }

    const Data &operator+(const ForceC &) const
    {
        mainFile = "main.c";
        return *this;
    }

public:
    mutable bool useQt = false;
    mutable bool useQHash = false;
    mutable int engines = AllEngines;
    mutable int skipLevels = 0;              // Levels to go 'up' before dumping variables.
    mutable bool glibcxxDebug = false;
    mutable bool useDebugImage = false;
    mutable bool bigArray = false;
    mutable GdbVersion neededGdbVersion;     // DEC. 70600
    mutable LldbVersion neededLldbVersion;
    mutable QtVersion neededQtVersion;       // HEX! 0x50300
    mutable GccVersion neededGccVersion;     // DEC. 40702  for 4.7.2
    mutable ClangVersion neededClangVersion; // DEC.
    mutable MsvcVersion neededMsvcVersion;   // DEC.
    mutable BoostVersion neededBoostVersion; // DEC. 105400 for 1.54.0
    mutable DwarfVersion neededDwarfVersion; // DEC. 105400 for 1.54.0

    mutable QString configTest;

    mutable QString allProfile;      // Overrides anything below if not empty.
    mutable QString allCode;         // Overrides anything below if not empty.

    mutable QString mainFile = "main.cpp";
    mutable QString projectFile = "doit.pro";

    mutable QString profileExtra;
    mutable QString includes;
    mutable QString code;

    mutable QList<Check> checks;
    mutable QList<RequiredMessage> requiredMessages;
};

struct TempStuff
{
    TempStuff(const char *tag) : buildTemp(QLatin1String("qt_tst_dumpers_")
                                           + QLatin1String(tag)
                                           + QLatin1Char('_'))
    {
        buildPath = QDir::currentPath() + QLatin1Char('/')  + buildTemp.path();
        buildTemp.setAutoRemove(false);
        QVERIFY(!buildPath.isEmpty());
    }

    QString input;
    QTemporaryDir buildTemp;
    QString buildPath;
};

Q_DECLARE_METATYPE(Data)

class tst_Dumpers : public QObject
{
    Q_OBJECT

public:
    tst_Dumpers() {}

private slots:
    void initTestCase();
    void dumper();
    void dumper_data();
    void init();
    void cleanup();

private:
    void disarm() { t->buildTemp.setAutoRemove(!keepTemp()); }
    bool keepTemp() const { return m_keepTemp || m_forceKeepTemp; }
    TempStuff *t = 0;
    QString m_debuggerBinary;
    QString m_qmakeBinary;
    QProcessEnvironment m_env;
    DebuggerEngine m_debuggerEngine;
    QString m_makeBinary;
    bool m_keepTemp = true;
    bool m_forceKeepTemp = false;
    int m_debuggerVersion = 0; // GDB: 7.5.1 -> 70501
    int m_gdbBuildVersion = 0;
    int m_qtVersion = 0; // 5.2.0 -> 50200
    int m_gccVersion = 0;
    int m_msvcVersion = 0;
    bool m_isMacGdb = false;
    bool m_isQnxGdb = false;
    bool m_useGLibCxxDebug = false;
};

void tst_Dumpers::initTestCase()
{
    m_debuggerBinary = QString::fromLocal8Bit(qgetenv("QTC_DEBUGGER_PATH_FOR_TEST"));
    if (m_debuggerBinary.isEmpty()) {
#ifdef Q_OS_MAC
        m_debuggerBinary = "/Applications/Xcode.app/Contents/Developer/usr/bin/lldb";
#else
        m_debuggerBinary = "gdb";
#endif
    }
    qDebug() << "Debugger           : " << m_debuggerBinary;

    m_debuggerEngine = GdbEngine;
    if (m_debuggerBinary.endsWith("cdb.exe"))
        m_debuggerEngine = CdbEngine;

    QString base = QFileInfo(m_debuggerBinary).baseName();
    if (base.startsWith("lldb"))
        m_debuggerEngine = LldbEngine;

    m_qmakeBinary = QString::fromLocal8Bit(qgetenv("QTC_QMAKE_PATH_FOR_TEST"));
    if (m_qmakeBinary.isEmpty())
        m_qmakeBinary = "qmake";
    qDebug() << "QMake              : " << m_qmakeBinary;

    m_useGLibCxxDebug = qgetenv("QTC_USE_GLIBCXXDEBUG_FOR_TEST").toInt();
    qDebug() << "Use _GLIBCXX_DEBUG : " << m_useGLibCxxDebug;

    m_forceKeepTemp = qgetenv("QTC_KEEP_TEMP_FOR_TEST").toInt();
    qDebug() << "Force keep temp    : " << m_forceKeepTemp;

    if (m_debuggerEngine == GdbEngine) {
        QProcess debugger;
        debugger.start(m_debuggerBinary + " -i mi -quiet -nx");
        bool ok = debugger.waitForStarted();
        debugger.write("set confirm off\npython print 43\nshow version\nquit\n");
        ok = debugger.waitForFinished();
        QVERIFY(ok);
        QByteArray output = debugger.readAllStandardOutput();
        //qDebug() << "stdout: " << output;
        bool usePython = !output.contains("Python scripting is not supported in this copy of GDB");
        qDebug() << "Python             : " << (usePython ? "ok" : "*** not ok ***");
        qDebug() << "Dumper dir         : " << DUMPERDIR;
        QVERIFY(usePython);

        QString version = QString::fromLocal8Bit(output);
        int pos1 = version.indexOf("&\"show version\\n");
        QVERIFY(pos1 != -1);
        pos1 += 20;
        int pos2 = version.indexOf("~\"Copyright (C) ", pos1);
        QVERIFY(pos2 != -1);
        pos2 -= 4;
        version = version.mid(pos1, pos2 - pos1);
        extractGdbVersion(version, &m_debuggerVersion,
            &m_gdbBuildVersion, &m_isMacGdb, &m_isQnxGdb);
        m_env = QProcessEnvironment::systemEnvironment();
        m_makeBinary = QString::fromLocal8Bit(qgetenv("QTC_MAKE_PATH_FOR_TEST"));
#ifdef Q_OS_WIN
        if (m_makeBinary.isEmpty())
            m_makeBinary = "mingw32-make";
        // if qmake is not in PATH make sure the correct libs for inferior are prepended to PATH
        if (m_qmakeBinary != "qmake") {
            Utils::Environment env = Utils::Environment::systemEnvironment();
            env.prependOrSetPath(QDir::toNativeSeparators(QFileInfo(m_qmakeBinary).absolutePath()));
            m_env = env.toProcessEnvironment();
        }
#else
        if (m_makeBinary.isEmpty())
            m_makeBinary = "make";
#endif
        qDebug() << "Make path          : " << m_makeBinary;
        qDebug() << "Gdb version        : " << m_debuggerVersion;
    } else if (m_debuggerEngine == CdbEngine) {
#ifdef Q_CC_MSVC
        QByteArray envBat = qgetenv("QTC_MSVC_ENV_BAT");
        QMap <QString, QString> envPairs;
        Utils::Environment env = Utils::Environment::systemEnvironment();
        QVERIFY(generateEnvironmentSettings(env, QString::fromLatin1(envBat), QString(), envPairs));
        for (auto envIt = envPairs.begin(); envIt != envPairs.end(); ++envIt)
            env.set(envIt.key(), envIt.value());
        QByteArray cdbextPath = qgetenv("QTC_CDBEXT_PATH");
        if (cdbextPath.isEmpty())
            cdbextPath = CDBEXT_PATH "\\qtcreatorcdbext64";
        QVERIFY(QFile::exists(QString::fromLatin1(cdbextPath + QByteArray("\\qtcreatorcdbext.dll"))));
        env.set(QLatin1String("_NT_DEBUGGER_EXTENSION_PATH"), QString::fromLatin1(cdbextPath));
        env.prependOrSetPath(QDir::toNativeSeparators(QFileInfo(m_qmakeBinary).absolutePath()));
        m_makeBinary = env.searchInPath(QLatin1String("nmake.exe")).toString();
        m_env = env.toProcessEnvironment();

        QProcess cl;
        cl.start(env.searchInPath(QLatin1String("cl.exe")).toString(), QStringList());
        QVERIFY(cl.waitForFinished());
        QString output = cl.readAllStandardError();
        int pos = output.indexOf('\n');
        if (pos != -1)
            output = output.left(pos);
        qDebug() << "Extracting MSVC version from: " << output;
        QRegularExpression reg(" (\\d\\d)\\.(\\d\\d)\\.");
        QRegularExpressionMatch match = reg.match(output);
        if (match.matchType() != QRegularExpression::NoMatch)
            m_msvcVersion = QString(match.captured(1) + match.captured(2)).toInt();
#endif //Q_CC_MSVC
    } else if (m_debuggerEngine == LldbEngine) {
        qDebug() << "Dumper dir         : " << DUMPERDIR;
        QProcess debugger;
        QString cmd = m_debuggerBinary + " -v";
        debugger.start(cmd);
        bool ok = debugger.waitForFinished(2000);
        QVERIFY(ok);
        QByteArray output = debugger.readAllStandardOutput();
        output += debugger.readAllStandardError();
        output = output.trimmed();
        // Should be something like "LLDB-178" (Mac OS X 10.8)
        // or "lldb version 3.5 ( revision )" (Ubuntu 13.10)
        QByteArray ba = output.mid(output.indexOf('-') + 1);
        int pos = ba.indexOf('.');
        if (pos >= 0)
            ba = ba.left(pos);
        m_debuggerVersion = ba.toInt();
        if (!m_debuggerVersion) {
            if (output.startsWith("lldb version")) {
                int pos1 = output.indexOf('.', 13);
                int major = output.mid(13, pos1++ - 13).toInt();
                int pos2 = output.indexOf(' ', pos1);
                int minor = output.mid(pos1, pos2++ - pos1).toInt();
                m_debuggerVersion = 100 * major + 10 * minor;
            }
        }

        qDebug() << "Lldb version       :" << output << ba << m_debuggerVersion;
        QVERIFY(m_debuggerVersion);

        m_env = QProcessEnvironment::systemEnvironment();
        m_makeBinary = "make";
    }
}

void tst_Dumpers::init()
{
    t = new TempStuff(QTest::currentDataTag());
}

void tst_Dumpers::cleanup()
{
    if (!t->buildTemp.autoRemove()) {
        QFile logger(t->buildPath + QLatin1String("/input.txt"));
        logger.open(QIODevice::ReadWrite);
        logger.write(t->input.toUtf8());
    }
    delete t;
}

void tst_Dumpers::dumper()
{
    QFETCH(Data, data);

    if (!(data.engines & m_debuggerEngine))
        MSKIP_SINGLE("The test is excluded for this debugger engine.");

    if (data.neededGdbVersion.isRestricted && m_debuggerEngine == GdbEngine) {
        if (data.neededGdbVersion.min > m_debuggerVersion)
            MSKIP_SINGLE("Need minimum GDB version "
                + QByteArray::number(data.neededGdbVersion.min));
        if (data.neededGdbVersion.max < m_debuggerVersion)
            MSKIP_SINGLE("Need maximum GDB version "
                + QByteArray::number(data.neededGdbVersion.max));
    }

    if (data.neededLldbVersion.isRestricted && m_debuggerEngine == LldbEngine) {
        if (data.neededLldbVersion.min > m_debuggerVersion)
            MSKIP_SINGLE("Need minimum LLDB version "
                + QByteArray::number(data.neededLldbVersion.min));
        if (data.neededLldbVersion.max < m_debuggerVersion)
            MSKIP_SINGLE("Need maximum LLDB version "
                + QByteArray::number(data.neededLldbVersion.max));
    }

    QString cmd;
    QByteArray output;
    QByteArray error;

    if (data.neededQtVersion.isRestricted) {
        QProcess qmake;
        qmake.setWorkingDirectory(t->buildPath);
        cmd = m_qmakeBinary;
        qmake.start(cmd, QStringList(QLatin1String("--version")));
        QVERIFY(qmake.waitForFinished());
        output = qmake.readAllStandardOutput();
        error = qmake.readAllStandardError();
        int pos0 = output.indexOf("Qt version");
        if (pos0 == -1) {
            qDebug() << "Output: " << output;
            qDebug() << "Error: " << error;
            QVERIFY(false);
        }
        pos0 += 11;
        int pos1 = output.indexOf('.', pos0 + 1);
        int major = output.mid(pos0, pos1++ - pos0).toInt();
        int pos2 = output.indexOf('.', pos1 + 1);
        int minor = output.mid(pos1, pos2++ - pos1).toInt();
        int pos3 = output.indexOf(' ', pos2 + 1);
        int patch = output.mid(pos2, pos3++ - pos2).toInt();
        m_qtVersion = 0x10000 * major + 0x100 * minor + patch;

        if (data.neededQtVersion.min > m_qtVersion)
            MSKIP_SINGLE("Need minimum Qt version "
                + QByteArray::number(data.neededQtVersion.min, 16));
        if (data.neededQtVersion.max < m_qtVersion)
            MSKIP_SINGLE("Need maximum Qt version "
                + QByteArray::number(data.neededQtVersion.max, 16));
    }

    if (data.neededGccVersion.isRestricted) {
        QProcess gcc;
        gcc.setWorkingDirectory(t->buildPath);
        cmd = QLatin1String("gcc");
        gcc.start(cmd, QStringList(QLatin1String("--version")));
        QVERIFY(gcc.waitForFinished());
        output = gcc.readAllStandardOutput();
        error = gcc.readAllStandardError();
        int pos = output.indexOf('\n');
        if (pos != -1)
            output = output.left(pos);
        qDebug() << "Extracting GCC version from: " << output;
        if (output.contains(QByteArray("SUSE Linux"))) {
            pos = output.indexOf(')');
            output = output.mid(pos + 1).trimmed();
            pos = output.indexOf(' ');
            output = output.left(pos);
        } else {
            pos = output.lastIndexOf(' ');
            output = output.mid(pos + 1);
        }
        int pos1 = output.indexOf('.');
        int major = output.left(pos1++).toInt();
        int pos2 = output.indexOf('.', pos1 + 1);
        int minor = output.mid(pos1, pos2++ - pos1).toInt();
        int patch = output.mid(pos2).toInt();
        m_gccVersion = 10000 * major + 100 * minor + patch;
        qDebug() << "GCC version: " << m_gccVersion;

        if (data.neededGccVersion.min > m_gccVersion)
            MSKIP_SINGLE("Need minimum GCC version "
                + QByteArray::number(data.neededGccVersion.min));
        if (data.neededGccVersion.max < m_gccVersion)
            MSKIP_SINGLE("Need maximum GCC version "
                + QByteArray::number(data.neededGccVersion.max));
    }

    if (data.neededMsvcVersion.isRestricted && m_debuggerEngine == CdbEngine) {
        if (data.neededMsvcVersion.min > m_msvcVersion)
            MSKIP_SINGLE("Need minimum Msvc version "
                         + QByteArray::number(data.neededMsvcVersion.min));
        if (data.neededMsvcVersion.max < m_msvcVersion)
            MSKIP_SINGLE("Need maximum Msvc version "
                         + QByteArray::number(data.neededMsvcVersion.max));
    }

    if (!data.configTest.isEmpty()) {
        QProcess configTest;
        configTest.start(data.configTest);
        QVERIFY(configTest.waitForFinished());
        output = configTest.readAllStandardOutput();
        error = configTest.readAllStandardError();
        if (configTest.exitCode()) {
            MSKIP_SINGLE("Configure test failed: '"
                + data.configTest.toUtf8() + "' " + output + ' ' + error);
        }
    }

    QFile proFile(t->buildPath + '/' + data.projectFile);
    QVERIFY(proFile.open(QIODevice::ReadWrite));
    if (data.allProfile.isEmpty()) {
        proFile.write("SOURCES = ");
        proFile.write(data.mainFile.toUtf8());
        proFile.write("\nTARGET = doit\n");
        proFile.write("\nCONFIG -= app_bundle\n");
        proFile.write("\nCONFIG -= release\n");
        proFile.write("\nCONFIG += debug\n");
        proFile.write("\nCONFIG += console\n");
        if (data.useQt)
            proFile.write("QT -= widgets gui\n");
        else
            proFile.write("CONFIG -= qt\n");
        if (m_useGLibCxxDebug)
            proFile.write("DEFINES += _GLIBCXX_DEBUG\n");
        if (m_debuggerEngine == GdbEngine && m_debuggerVersion < 70500)
            proFile.write("QMAKE_CXXFLAGS += -gdwarf-3\n");
        proFile.write(data.profileExtra.toUtf8());
    } else {
        proFile.write(data.allProfile.toUtf8());
    }
    proFile.close();

    QFile source(t->buildPath + QLatin1Char('/') + data.mainFile);
    QVERIFY(source.open(QIODevice::ReadWrite));
    QString fullCode = QString() +
            "\n\n#if defined(_MSC_VER)" + (data.useQt ?
                "\n#include <qt_windows.h>" :
                "\n#define NOMINMAX\n#include <Windows.h>") +
                "\n#define BREAK [](){ DebugBreak(); }();"
                "\n\nvoid unused(const void *first,...) { (void) first; }"
            "\n#else"
                "\n#include <stdint.h>\n";

    if (m_debuggerEngine == LldbEngine)
//#ifdef Q_OS_MAC
//        fullCode += "\n#define BREAK do { asm(\"int $3\"); } while (0)";
//#else
        fullCode += "\n#define BREAK int *nullPtr = 0; *nullPtr = 0;";
//#endif
    else
        fullCode += "\n#define BREAK do { asm(\"int $3\"); } while (0)";

    fullCode += "\n"
                "\nstatic volatile int64_t unused_dummy;\n"
                "\nvoid __attribute__((optimize(\"O0\"))) unused(const void *first,...)\n"
                "\n{\n"
                "\n    unused_dummy += (int64_t)first;\n"
                "\n}\n"
            "\n#endif"
            "\n"
            "\n\n" + data.includes +
            "\n\n" + (data.useQHash ?
                "\n#include <QByteArray>"
                "\n#if QT_VERSION >= 0x050900"
                "\n#include <QHash>"
                "\nvoid initHashSeed() { qSetGlobalQHashSeed(0); }"
                "\n#elif QT_VERSION >= 0x050000"
                "\nQT_BEGIN_NAMESPACE"
                "\nQ_CORE_EXPORT extern QBasicAtomicInt qt_qhash_seed; // from qhash.cpp"
                "\nQT_END_NAMESPACE"
                "\nvoid initHashSeed() { qt_qhash_seed.store(0); }"
                "\n#else"
                "\nvoid initHashSeed() {}"
                "\n#endif" : "") +
            "\n\nint main(int argc, char *argv[])"
            "\n{"
            "\n    unused(&argc, &argv);\n"
            "\n    int skipall = 0; unused(&skipall);\n"
            "\n    int qtversion = " + (data.useQt ? "QT_VERSION" : "0") + "; unused(&qtversion);"
            "\n#ifdef __GNUC__"
            "\n    int gccversion = 10000 * __GNUC__ + 100 * __GNUC_MINOR__; unused(&gccversion);"
            "\n#else"
            "\n    int gccversion = 0; unused(&gccversion);"
            "\n#endif"
            "\n#ifdef __clang__"
            "\n    int clangversion = 10000 * __clang_major__ + 100 * __clang_minor__; unused(&clangversion);"
            "\n    gccversion = 0;"
            "\n#else"
            "\n    int clangversion = 0; unused(&clangversion);"
            "\n#endif"
            "\n#ifdef BOOST_VERSION"
            "\n    int boostversion = BOOST_VERSION; unused(&boostversion);"
            "\n#else"
            "\n    int boostversion = 0; unused(&boostversion);"
            "\n#endif"
            "\n" + (data.useQHash ? "initHashSeed();" : "") +
            "\n" + data.code +
            "\n    BREAK;"
            "\n    return 0;"
            "\n}\n";
    if (!data.allCode.isEmpty())
        fullCode = data.allCode;
    // MSVC 14 requires a BOM on an UTF-8 encoded sourcefile
    // see: https://msdn.microsoft.com/en-us//library/xwy0e8f2.aspx
    source.write("\xef\xbb\xbf", 3);
    source.write(fullCode.toUtf8());
    source.close();

    QProcess qmake;
    qmake.setWorkingDirectory(t->buildPath);
    cmd = m_qmakeBinary;
    //qDebug() << "Starting qmake: " << cmd;
    QStringList options;
#ifdef Q_OS_MAC
    if (m_qtVersion && m_qtVersion < 0x050000)
        options << "-spec" << "unsupported/macx-clang";
#endif
    qmake.start(cmd, options);
    QVERIFY(qmake.waitForFinished());
    output = qmake.readAllStandardOutput();
    error = qmake.readAllStandardError();
    //qDebug() << "stdout: " << output;

    if (data.allProfile.isEmpty()) { // Nim...
        if (!error.isEmpty()) {
            qDebug() << error; QVERIFY(false);
        }
    }

    QProcess make;
    make.setWorkingDirectory(t->buildPath);
    make.setProcessEnvironment(m_env);

    make.start(m_makeBinary, QStringList());
    QVERIFY(make.waitForFinished());
    output = make.readAllStandardOutput();
    error = make.readAllStandardError();
    //qDebug() << "stdout: " << output;
    if (make.exitCode()) {
        qDebug() << error;
        qDebug() << "\n------------------ CODE --------------------";
        qDebug() << fullCode;
        qDebug() << "\n------------------ CODE --------------------";
        qDebug() << "Project file: " << proFile.fileName();
    }

    if (data.neededDwarfVersion.isRestricted) {
        QProcess readelf;
        readelf.setWorkingDirectory(t->buildPath);
        readelf.start("readelf", {"-wi", "doit"});
        QVERIFY(readelf.waitForFinished());
        output = readelf.readAllStandardOutput();
        error = readelf.readAllStandardError();
        int pos1 = output.indexOf("Version:") + 8;
        int pos2 = output.indexOf("\n", pos1);
        int dwarfVersion = output.mid(pos1, pos2 - pos1).toInt();
        //qDebug() << "OUT: " << output;
        //qDebug() << "ERR: " << error;
        qDebug() << "DWARF Version : " << dwarfVersion;

        if (data.neededDwarfVersion.min > dwarfVersion)
            MSKIP_SINGLE("Need minimum DWARF version "
                + QByteArray::number(data.neededDwarfVersion.min));
        if (data.neededDwarfVersion.max < dwarfVersion)
            MSKIP_SINGLE("Need maximum DWARF version "
                + QByteArray::number(data.neededDwarfVersion.max));
    }

    QByteArray dumperDir = DUMPERDIR;

    QSet<QString> expandedINames;
    expandedINames.insert("local");
    foreach (const Check &check, data.checks) {
        QString parent = check.iname;
        while (true) {
            parent = parentIName(parent);
            if (parent.isEmpty())
                break;
            expandedINames.insert("local." + parent);
        }
    }

    QString expanded;
    QString expandedq;
    foreach (const QString &iname, expandedINames) {
        if (!expanded.isEmpty()) {
            expanded.append(',');
            expandedq.append(',');
        }
        expanded += iname;
        expandedq += '\'' + iname + '\'';
    }

    QString exe = m_debuggerBinary;
    QStringList args;
    QString cmds;

    if (m_debuggerEngine == GdbEngine) {
        const QFileInfo gdbBinaryFile(exe);
        const QString uninstalledData = gdbBinaryFile.absolutePath()
            + "/data-directory/python";

        args << "-i" << "mi" << "-quiet" << "-nx";
        QByteArray nograb = "-nograb";

        cmds = "set confirm off\n"
#ifdef Q_OS_WIN
                "file debug/doit\n"
#else
                "file doit\n"
#endif
                "set print object on\n"
                "set auto-load python-scripts off\n";

        cmds += "python sys.path.insert(1, '" + dumperDir + "')\n"
                "python sys.path.append('" + uninstalledData + "')\n"
                "python from gdbbridge import *\n"
                "python theDumper.setupDumpers()\n"
                "run " + nograb + "\n"
                "up " + QString::number(data.skipLevels) + "\n"
                "python theDumper.fetchVariables({"
                    "'token':2,'fancy':1,'forcens':1,"
                    "'autoderef':1,'dyntype':1,'passexceptions':1,"
                    "'testing':1,'qobjectnames':1,"
                    "'expanded':[" + expandedq + "]})\n";

        cmds += "quit\n";

    } else if (m_debuggerEngine == CdbEngine) {
        QString cdbextPath = m_env.value("_NT_DEBUGGER_EXTENSION_PATH");
        const int frameNumber = cdbextPath.contains("qtcreatorcdbext64") ? 2 : 1;
        args << QLatin1String("-aqtcreatorcdbext.dll")
             << QLatin1String("-G")
             << QLatin1String("-xi")
             << QLatin1String("0x4000001f")
             << QLatin1String("-c")
             << QLatin1String("g")
             << QLatin1String("debug\\doit.exe");
        cmds += "!qtcreatorcdbext.script sys.path.insert(1, '" + dumperDir + "')\n"
                "!qtcreatorcdbext.script from cdbbridge import *\n"
                "!qtcreatorcdbext.script theDumper = Dumper()\n"
                "!qtcreatorcdbext.script theDumper.setupDumpers()\n"
                ".frame " + QString::number(frameNumber) + "\n"
                "!qtcreatorcdbext.pid\n"
                "!qtcreatorcdbext.script -t 42 theDumper.fetchVariables({"
                "'token':2,'fancy':1,'forcens':1,"
                "'autoderef':1,'dyntype':1,'passexceptions':0,"
                "'testing':1,'qobjectnames':1,"
                "'expanded':[" + expandedq + "]})\n"
                "q\n";
    } else if (m_debuggerEngine == LldbEngine) {
        QFile fullLldb(t->buildPath + QLatin1String("/lldbcommand.txt"));
        fullLldb.setPermissions(QFile::ReadOwner|QFile::WriteOwner|QFile::ExeOwner|QFile::ReadGroup|QFile::ReadOther);
        fullLldb.open(QIODevice::WriteOnly);
        fullLldb.write((exe + ' ' + args.join(' ') + '\n').toUtf8());

        cmds = "sc import sys\n"
               "sc sys.path.insert(1, '" + dumperDir + "')\n"
               "sc from lldbbridge import *\n"
              // "sc print(dir())\n"
               "sc Tester('" + t->buildPath.toLatin1() + "/doit', {'fancy':1,'forcens':1,"
                    "'autoderef':1,'dyntype':1,'passexceptions':1,"
                    "'testing':1,'qobjectnames':1,"
                    "'expanded':[" + expandedq + "]})\n"
               "quit\n";

        fullLldb.write(cmds.toUtf8());
        fullLldb.close();
    }

    t->input = cmds;

    QProcessEnvironment env = m_env;
    if (data.useDebugImage)
        env.insert("DYLD_IMAGE_SUFFIX", "_debug");

    QProcess debugger;
    debugger.setProcessEnvironment(env);
    debugger.setWorkingDirectory(t->buildPath);
    debugger.start(exe, args);
    QVERIFY(debugger.waitForStarted());
    // FIXME: next line is necessary for LLDB <= 310 - remove asap
    debugger.waitForReadyRead(1000);
    debugger.write(cmds.toLocal8Bit());
    QVERIFY(debugger.waitForFinished());
    output = debugger.readAllStandardOutput();
    QByteArray fullOutput = output;
    //qDebug() << "stdout: " << output;
    error = debugger.readAllStandardError();
    if (!error.isEmpty())
        qDebug() << error;

    if (keepTemp()) {
        QFile logger(t->buildPath + QLatin1String("/output.txt"));
        logger.open(QIODevice::ReadWrite);
        logger.write("=== STDOUT ===\n");
        logger.write(output);
        logger.write("\n=== STDERR ===\n");
        logger.write(error);
    }

    Context context(m_debuggerEngine);
    QByteArray contents;
    GdbMi actual;
    if (m_debuggerEngine == GdbEngine) {
        int posDataStart = output.indexOf("data=");
        if (posDataStart == -1) {
            qDebug() << "NO \"data=\" IN OUTPUT: " << output;
            QVERIFY(posDataStart != -1);
        }
        contents = output.mid(posDataStart);
        contents.replace("\\\"", "\"");

        actual.fromStringMultiple(contents);
        context.nameSpace = actual["qtnamespace"].data();
        actual = actual["data"];
        //qDebug() << "FOUND NS: " << context.nameSpace;

    } else if (m_debuggerEngine == LldbEngine) {
        //qDebug() << "GOT OUTPUT: " << output;
        int pos = output.indexOf("data=[{");
        QVERIFY(pos != -1);
        output = output.mid(pos);
        contents = output;

        int posNameSpaceStart = output.indexOf("@NS@");
        if (posNameSpaceStart == -1)
            qDebug() << "OUTPUT: " << output;
        QVERIFY(posNameSpaceStart != -1);
        posNameSpaceStart += sizeof("@NS@") - 1;
        int posNameSpaceEnd = output.indexOf("@", posNameSpaceStart);
        QVERIFY(posNameSpaceEnd != -1);
        context.nameSpace = output.mid(posNameSpaceStart, posNameSpaceEnd - posNameSpaceStart);
        //qDebug() << "FOUND NS: " << context.nameSpace;
        if (context.nameSpace == "::")
            context.nameSpace.clear();
        contents.replace("\\\"", "\"");
        actual.fromString(contents);
    } else {
        QByteArray localsAnswerStart("<qtcreatorcdbext>|R|42|");
        QByteArray locals("|script|");
        int localsBeginPos = output.indexOf(locals, output.indexOf(localsAnswerStart));
        if (localsBeginPos == -1)
            qDebug() << "OUTPUT: " << output;
        QVERIFY(localsBeginPos != -1);
        do {
            const int msgStart = localsBeginPos + locals.length();
            const int msgEnd = output.indexOf("\n", msgStart);
            contents += output.mid(msgStart, msgEnd - msgStart);
            localsBeginPos = output.indexOf(localsAnswerStart, msgEnd);
            if (localsBeginPos != -1)
                localsBeginPos = output.indexOf(locals, localsBeginPos);
        } while (localsBeginPos != -1);
        actual.fromString(contents);
        actual = actual["result"]["data"];
    }

    WatchItem local;
    local.iname = "local";

    foreach (const GdbMi &child, actual.children()) {
        const QString iname = child["iname"].data();
        if (iname == "local.qtversion")
            context.qtVersion = child["value"].toInt();
        else if (iname == "local.gccversion")
            context.gccVersion = child["value"].toInt();
        else if (iname == "local.clangversion")
            context.clangVersion = child["value"].toInt();
        else if (iname == "local.boostversion")
            context.boostVersion = child["value"].toInt();
        else if (iname == "local.skipall") {
            bool skipAll = child["value"].toInt();
            if (skipAll) {
                MSKIP_SINGLE("This test is excluded in this test machine configuration.");
                return;
            }
        } else {
            WatchItem *item = new WatchItem;
            item->parse(child, true);
            local.appendChild(item);
        }
    }


    //qDebug() << "QT VERSION " << QByteArray::number(context.qtVersion, 16);
    QSet<QString> seenINames;
    bool ok = true;

    for (int i = data.checks.size(); --i >= 0; ) {
        Check check = data.checks.at(i);
        QString iname = "local." + check.iname;
        WatchItem *item = static_cast<WatchItem *>(local.findAnyChild([iname](Utils::TreeItem *item) {
            return static_cast<WatchItem *>(item)->internalName() == iname;
        }));
        if (item) {
            seenINames.insert(iname);
            //qDebug() << "CHECKS" << i << check.iname;
            data.checks.removeAt(i);
            if (check.matches(m_debuggerEngine, m_debuggerVersion, context)) {
                //qDebug() << "USING MATCHING TEST FOR " << iname;
                QString name = item->realName();
                QString type = item->type;
                if (!check.expectedName.matches(name, context)) {
                    qDebug() << "INAME        : " << iname;
                    qDebug() << "NAME ACTUAL  : " << name;
                    qDebug() << "NAME EXPECTED: " << check.expectedName.name;
                    ok = false;
                }
                if (!check.expectedValue.matches(item->value, context)) {
                    qDebug() << "INAME         : " << iname;
                    qDebug() << "VALUE ACTUAL  : " << item->value << toHex(item->value);
                    qDebug() << "VALUE EXPECTED: " << check.expectedValue.value << toHex(check.expectedValue.value);
                    ok = false;
                }
                if (!check.expectedType.matches(type, context)) {
                    qDebug() << "INAME        : " << iname;
                    qDebug() << "TYPE ACTUAL  : " << type;
                    qDebug() << "TYPE EXPECTED: " << check.expectedType.type;
                    ok = false;
                }
            } else {
                qDebug() << "SKIPPING NON-MATCHING TEST FOR " << iname;
            }
        } else {
            qDebug() << "NOT SEEN: " << check.iname;
        }
    }

    if (!data.checks.isEmpty()) {
        for (int i = data.checks.size(); --i >= 0; ) {
            Check check = data.checks.at(i);
            if (!check.matches(m_debuggerEngine, m_debuggerVersion, context))
                data.checks.removeAt(i);
        }
    }

    if (!data.checks.isEmpty()) {
        qDebug() << "SOME TESTS NOT EXECUTED: ";
        foreach (const Check &check, data.checks) {
            if (check.optionallyPresent) {
                qDebug() << "  OPTIONAL TEST NOT FOUND FOR INAME: " << check.iname << " IGNORED.";
            } else {
                qDebug() << "  COMPULSORY TEST NOT FOUND FOR INAME: " << check.iname;
                ok = false;
            }
        }
        qDebug() << "SEEN INAMES " << seenINames;
        qDebug() << "EXPANDED     : " << expanded;
    }

    for (int i = data.requiredMessages.size(); --i >= 0; ) {
        RequiredMessage check = data.requiredMessages.at(i);
        if (fullOutput.contains(check.message.toLatin1())) {
            qDebug() << " EXPECTED MESSAGE TO BE MISSING, BUT FOUND: " << check.message;
            ok = false;
        }
    }

    if (ok) {
        m_keepTemp = false;
    } else {
        local.forAllChildren([](WatchItem *item) { qDebug() << item->internalName(); });

        int pos1 = 0, pos2 = -1;
        while (true) {
            pos1 = fullOutput.indexOf("bridgemessage={msg=", pos2 + 1);
            if (pos1 == -1)
                break;
            pos1 += 21;
            pos2 = fullOutput.indexOf("\"}", pos1 + 1);
            if (pos2 == -1)
                break;
            qDebug() << "MSG: " << fullOutput.mid(pos1, pos2 - pos1 - 1);
        }
        qDebug() << "CONTENTS     : " << contents;
        qDebug() << "FULL OUTPUT  : " << fullOutput.data();
        qDebug() << "Qt VERSION   : " << QString::number(context.qtVersion, 16);
        if (m_debuggerEngine != CdbEngine)
            qDebug() << "GCC VERSION   : " << context.gccVersion;
        qDebug() << "BUILD DIR    : " << t->buildPath;
    }
    QVERIFY(ok);
    disarm();
}

void tst_Dumpers::dumper_data()
{
    QTest::addColumn<Data>("data");

    QString fooData =
            "#include <QHash>\n"
            "#include <QMap>\n"
            "#include <QObject>\n"
            "#include <QString>\n"
            "#include <QDebug>\n"

            "//#define DEBUG_FOO(s) qDebug() << s << this->a << \" AT \" "
                "<< qPrintable(\"0x\" + QString::number(quintptr(this), 16));\n"
            "#define DEBUG_FOO(s)\n"
            "class Foo\n"
            "{\n"
            "public:\n"
            "    Foo(int i = 0)\n"
            "        : a(i), b(2)\n"
            "    {\n"
            "       for (int j = 0; j < 6; ++j)\n"
            "           x[j] = 'a' + j;\n"
            "        DEBUG_FOO(\"CREATE\")\n"
            "    }\n"
            "    Foo(const Foo &f)\n"
            "    {\n"
            "        copy(f);\n"
            "        DEBUG_FOO(\"COPY\");\n"
            "    }\n"
            "    void operator=(const Foo &f)\n"
            "    {\n"
            "        copy(f);\n"
            "        DEBUG_FOO(\"ASSIGN\");\n"
            "    }\n"
            "\n"
            "    virtual ~Foo()\n"
            "    {\n"
            "        a = 5;\n"
            "    }\n"
            "    void copy(const Foo &f)\n"
            "    {\n"
            "        a = f.a;\n"
            "        b = f.b;\n"
            "        m = f.m;\n"
            "        h = f.h;\n"
            "       for (int j = 0; j < 6; ++j)\n"
            "           x[j] = f.x[j];\n"
            "    }\n"
            "    void doit()\n"
            "    {\n"
            "        static QObject ob;\n"
            "        m[\"1\"] = \"2\";\n"
            "        h[&ob] = m.begin();\n"
            "        --b;\n"
            "    }\n"
            "public:\n"
            "    int a, b;\n"
            "    char x[6];\n"
            "private:\n"
            "    typedef QMap<QString, QString> Map;\n"
            "    Map m;\n"
            "    QHash<QObject *, Map::iterator> h;\n"
            "};\n";

    QString nsData =
            "namespace nsA {\n"
            "namespace nsB {\n"
           " struct SomeType\n"
           " {\n"
           "     SomeType(int a) : a(a) {}\n"
           "     int a;\n"
           " };\n"
           " } // namespace nsB\n"
           " } // namespace nsA\n";

    QTest::newRow("QBitArray")
            << Data("#include <QBitArray>\n",
                    "QBitArray ba0;\n"
                    "unused(&ba0);\n"
                    "QBitArray ba1(20, true);\n"
                    "ba1.setBit(1, false);\n"
                    "ba1.setBit(3, false);\n"
                    "ba1.setBit(16, false);\n"
                    "unused(&ba1);\n")

               + CoreProfile()

               + Check("ba0", "<0 items>", "@QBitArray")
               + Check("ba1", "<20 items>", "@QBitArray")
                // Cdb has "proper" "false"/"true"
               + Check("ba1.0", "[0]", "1", "bool")
               + Check("ba1.1", "[1]", "0", "bool")
               + Check("ba1.2", "[2]", "1", "bool")
               + Check("ba1.3", "[3]", "0", "bool")
               + Check("ba1.15", "[15]", "1", "bool")
               + Check("ba1.16", "[16]", "0", "bool")
               + Check("ba1.17", "[17]", "1", "bool");

    QTest::newRow("QByteArray")
            << Data("#include <QByteArray>\n"
                    "#include <QString>\n"
                    "#include <string>\n",

                    "QByteArray ba0;\n\n"

                    "QByteArray ba1 = \"Hello\\\"World\";\n"
                    "ba1 += char(0);\n"
                    "ba1 += 1;\n"
                    "ba1 += 2;\n\n"

                    "QByteArray ba2;\n"
                    "for (int i = 256; --i >= 0; )\n"
                    "    ba2.append(char(i));\n"
                    "QString s(10000, 'x');\n"
                    "std::string ss(10000, 'c');\n\n"

                    "const char *str1 = \"\\356\";\n"
                    "const char *str2 = \"\\xee\";\n"
                    "const char *str3 = \"\\\\ee\";\n"
                    "QByteArray buf1(str1);\n"
                    "QByteArray buf2(str2);\n"
                    "QByteArray buf3(str3);\n\n"

                    "char data[] = { 'H', 'e', 'l', 'l', 'o' };\n"
                    "QByteArray ba4 = QByteArray::fromRawData(data, 4);\n"
                    "QByteArray ba5 = QByteArray::fromRawData(data + 1, 4);\n\n"

                    "unused(&buf1, &buf2, &buf3);\n"
                    "unused(&ba0, &ba1, &ba2, &ba4, &ba5, &s, &ss);\n")

               + CoreProfile()
               + Check("ba0", "ba0", "\"\"", "@QByteArray")

               + Check("ba1", Value(QString("\"Hello\"World")
                      + QChar(0) + QChar(1) + QChar(2) + '"'), "@QByteArray")
               + Check("ba1.0", "[0]", "72", "char")
               + Check("ba1.11", "[11]", "0", "char")
               + Check("ba1.12", "[12]", "1", "char")
               + Check("ba1.13", "[13]", "2", "char")

               + CheckType("ba2", "@QByteArray")
               + Check("s", Value('"' + QString(100, QChar('x')) + '"'), "@QString")
               + Check("ss", Value('"' + QString(100, QChar('c')) + '"'), "std::string")

               + Check("buf1", Value("\"" + QString(1, QChar(0xee)) + "\""), "@QByteArray")
               + Check("buf2", Value("\"" + QString(1, QChar(0xee)) + "\""), "@QByteArray")
               + Check("buf3", "\"\\ee\"", "@QByteArray")
               + CheckType("str1", "char *")

               + Check("ba4", "\"Hell\"", "@QByteArray")
               + Check("ba5", "\"ello\"", "@QByteArray");


    QTest::newRow("QChar")
            << Data("#include <QString>\n",
                    "QString s = QLatin1String(\"x\");\n"
                    "QChar c = s.at(0);\n"
                    "unused(&c);\n")

               + CoreProfile()

               + Check("c", "120", "@QChar");


    QTest::newRow("QFlags")
            << Data("#include <QFlags>\n"
                    "enum Foo { a = 0x1, b = 0x2 };\n"
                    "Q_DECLARE_FLAGS(FooFlags, Foo)\n"
                    "Q_DECLARE_OPERATORS_FOR_FLAGS(FooFlags)\n",
                    "FooFlags f1(a);\n"
                    "FooFlags f2(a | b);\n")
               + CoreProfile()
               + Check("f1", "a (1)", TypeDef("QFlags<enum Foo>", "FooFlags"))
               + Check("f2", "(a | b) (3)", "FooFlags") % GdbEngine;

    QTest::newRow("QDateTime")
            << Data("#include <QDateTime>\n",

                    "QDate d0;\n"
                    "QDate d1;\n"
                    "d1.setDate(1980, 1, 1);\n"
                    "unused(&d0, &d1);\n"

                    "QTime t0;\n"
                    "QTime t1(13, 15, 32);\n"
                    "unused(&t0, &t1);\n\n"

                    "QDateTime dt0;\n"
                    "QDateTime dt1(QDate(1980, 1, 1), QTime(13, 15, 32), Qt::UTC);\n"
                    "unused(&dt0, &dt1);\n")

               + CoreProfile()

               + Check("d0", "(invalid)", "@QDate")
               + Check("d1", "Tue Jan 1 1980", "@QDate")
               + Check("d1.(ISO)", "\"1980-01-01\"", "@QString") % Optional()
               + Check("d1.toString", "\"Tue Jan 1 1980\"", "@QString") % Optional()
               + CheckType("d1.(Locale)", "@QString") % Optional()
               + CheckType("d1.(SystemLocale)", "@QString") % Optional()

               + Check("t0", "(invalid)", "@QTime")
               + Check("t1", "13:15:32", "@QTime")
               + Check("t1.(ISO)", "\"13:15:32\"", "@QString") % Optional()
               + Check("t1.toString", "\"13:15:32\"", "@QString") % Optional()
               + CheckType("t1.(Locale)", "@QString") % Optional()
               + CheckType("t1.(SystemLocale)", "@QString") % Optional()

               + Check("dt0", "(invalid)", "@QDateTime")
               + Check("dt1", Value4("Tue Jan 1 13:15:32 1980"), "@QDateTime")
               + Check("dt1", Value5("Tue Jan 1 13:15:32 1980 GMT"), "@QDateTime")
               + Check("dt1.(ISO)",
                    "\"1980-01-01T13:15:32Z\"", "@QString") % Optional()
               + CheckType("dt1.(Locale)", "@QString") % Optional()
               + CheckType("dt1.(SystemLocale)", "@QString") % Optional()
               + Check("dt1.toString",
                    Value4("\"Tue Jan 1 13:15:32 1980\""), "@QString") % Optional()
               + Check("dt1.toString",
                    Value5("\"Tue Jan 1 13:15:32 1980 GMT\""), "@QString") % Optional();
               //+ Check("dt1.toUTC",
               //     Value4("Tue Jan 1 13:15:32 1980"), "@QDateTime") % Optional()
               //+ Check("dt1.toUTC",
               //     Value5("Tue Jan 1 13:15:32 1980 GMT"), "@QDateTime") % Optional();

#ifdef Q_OS_WIN
    QString tempDir = "\"C:/Program Files\"";
#else
    QString tempDir = "\"/tmp\"";
#endif
    QTest::newRow("QDir")
            << Data("#include <QDir>\n",
                    "QDir dir(" + tempDir + ");\n"
                    "QString s = dir.absolutePath();\n"
                    "QFileInfoList fi = dir.entryInfoList();\n"
                    "unused(&dir, &s, &fi);\n")

               + CoreProfile()
               + QtVersion(0x50300)

               + Check("dir", tempDir, "@QDir")
            // + Check("dir.canonicalPath", tempDir, "@QString")
               + Check("dir.absolutePath", tempDir, "@QString") % Optional();


    QTest::newRow("QFileInfo")
#ifdef Q_OS_WIN
            << Data("#include <QFile>\n"
                    "#include <QFileInfo>\n",
                    "QFile file(\"C:\\\\Program Files\\\\t\");\n"
                    "file.setObjectName(\"A QFile instance\");\n"
                    "QFileInfo fi(\"C:\\\\Program Files\\\\tt\");\n"
                    "QString s = fi.absoluteFilePath();\n")
               + CoreProfile()
               + Check("fi", "\"C:/Program Files/tt\"", "QFileInfo")
               + Check("file", "\"C:\\Program Files\\t\"", "QFile")
               + Check("s", "\"C:/Program Files/tt\"", "QString");
#else
            << Data("#include <QFile>\n"
                    "#include <QFileInfo>\n",
                    "QFile file(\"/tmp/t\");\n"
                    "file.setObjectName(\"A QFile instance\");\n"
                    "QFileInfo fi(\"/tmp/tt\");\n"
                    "QString s = fi.absoluteFilePath();\n")
               + CoreProfile()
               + Check("fi", "\"/tmp/tt\"", "@QFileInfo")
               + Check("file", "\"/tmp/t\"", "@QFile")
               + Check("s", "\"/tmp/tt\"", "@QString");
#endif


    QTest::newRow("QFixed")
            << Data("#include <private/qfixed_p.h>\n",
                    "QFixed f(1234);\n")
               + Qt5
               + GuiPrivateProfile()
               + Check("f", "78976/64 = 1234.0", "@QFixed");


    QTest::newRow("QHash")
            << Data("#include <QHash>\n"
                    "#include <QByteArray>\n"
                    "#include <QPointer>\n"
                    "#include <QString>\n"
                    "#include <QList>\n" + fooData,

                    "QHash<QString, QList<int> > h1;\n"
                    "h1.insert(\"Hallo\", QList<int>());\n"
                    "h1.insert(\"Welt\", QList<int>() << 1);\n"
                    "h1.insert(\"!\", QList<int>() << 1 << 2);\n\n"

                    "QHash<int, float> h2;\n"
                    "h2[0]  = 33.0;\n"
                    "h2[11] = 11.0;\n"
                    "h2[22] = 22.0;\n\n"

                    "QHash<QString, int> h3;\n"
                    "h3[\"22.0\"] = 22.0;\n"

                    "QHash<QByteArray, float> h4;\n"
                    "h4[\"22.0\"] = 22.0;\n"

                    "QHash<int, QString> h5;\n"
                    "h5[22] = \"22.0\";\n\n"

                    "QHash<QString, Foo> h6;\n"
                    "h6[\"22.0\"] = Foo(22);\n"

                    "QObject ob;\n"
                    "QHash<QString, QPointer<QObject> > h7;\n"
                    "h7.insert(\"Hallo\", QPointer<QObject>(&ob));\n"
                    "h7.insert(\"Welt\", QPointer<QObject>(&ob));\n"
                    "h7.insert(\".\", QPointer<QObject>(&ob));\n\n"

                    "typedef QHash<int, float> Hash;\n"
                    "Hash h8;\n"
                    "h8[11] = 11.0;\n"
                    "h8[22] = 22.0;\n"
                    "h8[33] = 33.0;\n"
                    "Hash::iterator it1 = h8.begin();\n"
                    "Hash::iterator it2 = it1; ++it2;\n"
                    "Hash::iterator it3 = it2; ++it3;\n\n")

               + CoreProfile()

               + Check("h1", "<3 items>", "@QHash<@QString, @QList<int> >")
               + Check("h1.0.key", Value4("\"Hallo\""), "@QString")
               + Check("h1.0.key", Value5("\"Welt\""), "@QString")
               + Check("h1.0.value", Value4("<0 items>"), "@QList<int>")
               + Check("h1.0.value", Value5("<1 items>"), "@QList<int>")
               + Check("h1.1.key", "key", Value4("\"Welt\""), "@QString")
               + Check("h1.1.key", "key", Value5("\"Hallo\""), "@QString")
               + Check("h1.1.value", "value", Value4("<1 items>"), "@QList<int>")
               + Check("h1.1.value", "value", Value5("<0 items>"), "@QList<int>")
               + Check("h1.2.key", "key", "\"!\"", "@QString")
               + Check("h1.2.value", "value", "<2 items>", "@QList<int>")
               + Check("h1.2.value.0", "[0]", "1", "int")
               + Check("h1.2.value.1", "[1]", "2", "int")

               + Check("h2", "<3 items>", "@QHash<int, float>")
               + Check("h2.0", "[0] 0", FloatValue("33"), "")
               + Check("h2.1", "[1] 22", FloatValue("22"), "")
               + Check("h2.2", "[2] 11", FloatValue("11"), "")

               + Check("h3", "<1 items>", "@QHash<@QString, int>")
               + Check("h3.0.key", "key", "\"22.0\"", "@QString")
               + Check("h3.0.value", "22", "int")

               + Check("h4", "<1 items>", "@QHash<@QByteArray, float>")
               + Check("h4.0.key", "\"22.0\"", "@QByteArray")
               + Check("h4.0.value", FloatValue("22"), "float")

               + Check("h5", "<1 items>", "@QHash<int, @QString>")
               + Check("h5.0.key", "22", "int")
               + Check("h5.0.value", "\"22.0\"", "@QString")

               + Check("h6", "<1 items>", "@QHash<@QString, Foo>")
               + Check("h6.0.key", "\"22.0\"", "@QString")
               + CheckType("h6.0.value", "Foo")
               + Check("h6.0.value.a", "22", "int")

               + CoreProfile()
               + Check("h7", "<3 items>", "@QHash<@QString, @QPointer<@QObject>>")
               + Check("h7.0.key", Value4("\"Hallo\""), "@QString")
               + Check("h7.0.key", Value5("\"Welt\""), "@QString")
               + CheckType("h7.0.value", "@QPointer<@QObject>")
               //+ CheckType("h7.0.value.o", "@QObject")
               + Check("h7.2.key", "\".\"", "@QString")
               + CheckType("h7.2.value", "@QPointer<@QObject>")

               + Check("h8", "<3 items>", TypeDef("QHash<int,float>", "Hash"))
               + Check("h8.0", "[0] 22", FloatValue("22"), "")
               + Check("it1.key", "22", "int")
               + Check("it1.value", FloatValue("22"), "float")
               + Check("it3.key", "33", "int")
               + Check("it3.value", FloatValue("33"), "float");


    QTest::newRow("QHostAddress")
            << Data("#include <QHostAddress>\n",
                    "QHostAddress ha1(129u * 256u * 256u * 256u + 130u);\n\n"
                    "QHostAddress ha2;\n"
                    "ha2.setAddress(\"127.0.0.1\");\n\n"
                    "QIPv6Address addr;\n"
                    "addr.c[0] = 0;\n"
                    "addr.c[1] = 1;\n"
                    "addr.c[2] = 2;\n"
                    "addr.c[3] = 3;\n"
                    "addr.c[4] = 5;\n"
                    "addr.c[5] = 6;\n"
                    "addr.c[6] = 0;\n"
                    "addr.c[7] = 0;\n"
                    "addr.c[8] = 8;\n"
                    "addr.c[9] = 9;\n"
                    "addr.c[10] = 10;\n"
                    "addr.c[11] = 11;\n"
                    "addr.c[12] = 0;\n"
                    "addr.c[13] = 0;\n"
                    "addr.c[14] = 0;\n"
                    "addr.c[15] = 0;\n"
                    "QHostAddress ha3(addr);\n"
                    "unused(&ha1, &ha2, &ha3);\n")

               + NetworkProfile()

               + Check("ha1", "129.0.0.130", "@QHostAddress")
               + Check("ha2", ValuePattern(".*127.0.0.1.*"), "@QHostAddress")
               + Check("addr", "1:203:506:0:809:a0b:0:0", "@QIPv6Address")
               + Check("addr.3", "[3]", "3", "unsigned char");


    QTest::newRow("QImage")
            << Data("#include <QImage>\n"
                    "#include <QApplication>\n"
                    "#include <QPainter>\n",

                    "QApplication app(argc, argv);\n"
                    "QImage im(QSize(200, 200), QImage::Format_RGB32);\n"
                    "im.fill(QColor(200, 100, 130).rgba());\n\n"
                    "QPainter pain;\n"
                    "pain.begin(&im);\n"
                    "pain.drawLine(2, 2, 130, 130);\n"
                    "pain.end();\n"
                    "QPixmap pm = QPixmap::fromImage(im);\n"
                    "unused(&app, &pm);\n")

               + GuiProfile()

               + Check("im", "(200x200)", "@QImage")
               + CheckType("pain", "@QPainter")
               + Check("pm", "(200x200)", "@QPixmap");


    QTest::newRow("QLinkedList")
            << Data("#include <QLinkedList>\n"
                    "#include <string>\n"
                    + fooData,

                    "QLinkedList<float> l0;\n"
                    "unused(&l0);\n\n"

                    "QLinkedList<int> l1;\n"
                    "l1.append(101);\n"
                    "l1.append(102);\n"
                    "unused(&l1);\n\n"

                    "QLinkedList<uint> l2;\n"
                    "l2.append(103);\n"
                    "l2.append(104);\n"
                    "unused(&l2);\n\n"

                    "QLinkedList<Foo *> l3;\n"
                    "l3.append(new Foo(1));\n"
                    "l3.append(0);\n"
                    "l3.append(new Foo(3));\n"
                    "unused(&l3);\n\n"

                    "QLinkedList<qulonglong> l4;\n"
                    "l4.append(42);\n"
                    "l4.append(43);\n"
                    "unused(&l4);\n\n"

                    "QLinkedList<Foo> l5;\n"
                    "l5.append(Foo(1));\n"
                    "l5.append(Foo(2));\n"
                    "unused(&l5);\n\n"

                    "QLinkedList<std::string> l6;\n"
                    "l6.push_back(\"aa\");\n"
                    "l6.push_back(\"bb\");\n"
                    "unused(&l6);\n\n")

               + CoreProfile()

               + Check("l0", "<0 items>", "@QLinkedList<float>")

               + Check("l1", "<2 items>", "@QLinkedList<int>")
               + Check("l1.0", "[0]", "101", "int")
               + Check("l1.1", "[1]", "102", "int")

               + Check("l2", "<2 items>", "@QLinkedList<unsigned int>")
               + Check("l2.0", "[0]", "103", "unsigned int")
               + Check("l2.1", "[1]", "104", "unsigned int")

               + Check("l3", "<3 items>", "@QLinkedList<Foo*>")
               + CheckType("l3.0", "[0]", "Foo")
               + Check("l3.0.a", "1", "int")
               + Check("l3.1", "[1]", "0x0", "Foo *")
               + CheckType("l3.2", "[2]", "Foo")
               + Check("l3.2.a", "3", "int")

               + Check("l4", "<2 items>", TypeDef("@QLinkedList<unsigned __int64>",
                                                  "@QLinkedList<unsigned long long>"))
               + Check("l4.0", "[0]", "42", TypeDef("unsigned int64", "unsigned long long"))
               + Check("l4.1", "[1]", "43", TypeDef("unsigned int64", "unsigned long long"))


               + Check("l5", "<2 items>", "@QLinkedList<Foo>")
               + CheckType("l5.0", "[0]", "Foo")
               + Check("l5.0.a", "1", "int")
               + CheckType("l5.1", "[1]", "Foo")
               + Check("l5.1.a", "2", "int")

               + Check("l6", "<2 items>", "@QLinkedList<std::string>")
               + Check("l6.0", "[0]", "\"aa\"", "std::string")
               + Check("l6.1", "[1]", "\"bb\"", "std::string");


    QTest::newRow("QList")
            << Data("#include <QList>\n"
                    "#include <QChar>\n"
                    "#include <QStringList>\n"
                    "#include <string>\n",

                    "QList<int> l0;\n"
                    "unused(&l0);\n\n"

                    "QList<int> l1;\n"
                    "for (int i = 0; i < 10; ++i)\n"
                    "    l1.push_back(i);\n"
                    "unused(&l1);\n\n"

                    "QList<int> l2;\n"
                    "l2.append(0);\n"
                    "l2.append(1);\n"
                    "l2.append(2);\n"
                    "l2.takeFirst();\n"
                    "unused(&l2);\n\n"

                    "QList<QString> l3;\n"
                    "l3.append(\"0\");\n"
                    "l3.append(\"1\");\n"
                    "l3.append(\"2\");\n"
                    "l3.takeFirst();\n"
                    "unused(&l3);\n\n"

                    "QStringList l4;\n"
                    "l4.append(\"0\");\n"
                    "l4.append(\"1\");\n"
                    "l4.append(\"2\");\n"
                    "l4.takeFirst();\n"
                    "unused(&l4);\n\n"

                    "QList<int *> l5;\n"
                    "l5.append(new int(1));\n"
                    "l5.append(new int(2));\n"
                    "l5.append(0);\n"
                    "unused(&l5);\n\n"

                    "QList<int *> l6;\n"
                    "unused(&l6);\n\n"

                    "QList<uint> l7;\n"
                    "l7.append(101);\n"
                    "l7.append(102);\n"
                    "l7.append(102);\n"
                    "unused(&l7);\n\n"

                    "QStringList sl;\n"
                    "sl.append(\"aaa\");\n"
                    "QList<QStringList> l8;\n"
                    "l8.append(sl);\n"
                    "l8.append(sl);\n"
                    "unused(&l8, &sl);\n\n"

                    "QList<ushort> l9;\n"
                    "l9.append(101);\n"
                    "l9.append(102);\n"
                    "l9.append(102);\n"
                    "unused(&l9);\n\n"

                    "QList<QChar> l10;\n"
                    "l10.append(QChar('a'));\n"
                    "l10.append(QChar('b'));\n"
                    "l10.append(QChar('c'));\n"
                    "unused(&l10);\n\n"

                    "QList<qulonglong> l11;\n"
                    "l11.append(100);\n"
                    "l11.append(101);\n"
                    "l11.append(102);\n"
                    "unused(&l11);\n\n"

                    "QList<std::string> l12, l13;\n"
                    "l13.push_back(\"aa\");\n"
                    "l13.push_back(\"bb\");\n"
                    "l13.push_back(\"cc\");\n"
                    "l13.push_back(\"dd\");")

               + CoreProfile()

               + BigArrayProfile()

               + Check("l0", "<0 items>", "@QList<int>")

               + Check("l1", "<10 items>", "@QList<int>")
               + Check("l1.0", "[0]", "0", "int")
               + Check("l1.9", "[9]", "9", "int")

               + Check("l2", "<2 items>", "@QList<int>")
               + Check("l2.0", "[0]", "1", "int")

               + Check("l3", "<2 items>", "@QList<@QString>")
               + Check("l3.0", "[0]", "\"1\"", "@QString")

               + Check("l4", "<2 items>", "@QStringList")
               + Check("l4.0", "[0]", "\"1\"", "@QString")

               + Check("l5", "<3 items>", "@QList<int*>")
               + CheckType("l5.0", "[0]", "int")
               + CheckType("l5.1", "[1]", "int")

               + Check("l5.2", "[2]", "0x0", "int*")

               + Check("l6", "<0 items>", "@QList<int*>")

               + Check("l7", "<3 items>", "@QList<unsigned int>")
               + Check("l7.0", "[0]", "101", "unsigned int")
               + Check("l7.2", "[2]", "102", "unsigned int")

               + Check("l8", "<2 items>", "@QList<@QStringList>")
               + Check("sl", "<1 items>", "@QStringList")
               + Check("l8.1", "[1]", "<1 items>", "@QStringList")
               + Check("l8.1.0", "[0]", "\"aaa\"", "@QString")

               + Check("l9", "<3 items>", "@QList<unsigned short>")
               + Check("l9.0", "[0]", "101", "unsigned short")
               + Check("l9.2", "[2]", "102", "unsigned short")

               + Check("l10", "<3 items>", "@QList<@QChar>")
               + Check("l10.0", "[0]", "97", "@QChar")
               + Check("l10.2", "[2]", "99", "@QChar")

               + Check("l11", "<3 items>", TypeDef("@QList<unsigned __int64>",
                                                   "@QList<unsigned long long>"))
               + Check("l11.0", "[0]", "100", TypeDef("unsigned int64", "unsigned long long"))
               + Check("l11.2", "[2]", "102", TypeDef("unsigned int64", "unsigned long long"))

               + Check("l12", "<0 items>", "@QList<std::string>")
               + Check("l13", "<4 items>", "@QList<std::string>")
               + CheckType("l13.0", "[0]", "std::string")
               + Check("l13.3", "[3]" ,"\"dd\"", "std::string");


   QTest::newRow("QListFoo")
            << Data("#include <QList>\n" + fooData,

                    "QList<Foo> l0, l1;\n"
                    "for (int i = 0; i < 100; ++i)\n"
                    "    l1.push_back(i + 15);\n\n"

                    "QList<int> l = QList<int>() << 1 << 2 << 3;\n"
                    "typedef std::reverse_iterator<QList<int>::iterator> Reverse;\n"
                    "Reverse rit(l.end());\n"
                    "Reverse rend(l.begin());\n"
                    "QList<int> r;\n"
                    "while (rit != rend)\n"
                    "    r.append(*rit++);\n")

               + CoreProfile()

               + Check("l0", "<0 items>", "@QList<Foo>")
               + Check("l1", "<100 items>", "@QList<Foo>")
               + Check("l1.0", "[0]", "", "Foo")
               + Check("l1.99", "[99]", "", "Foo")

               + Check("l", "<3 items>", "@QList<int>")
               + Check("l.0", "[0]", "1", "int")
               + Check("l.1", "[1]", "2", "int")
               + Check("l.2", "[2]", "3", "int")
               + Check("r", "<3 items>", "@QList<int>")
               + Check("r.0", "[0]", "3", "int")
               + Check("r.1", "[1]", "2", "int")
               + Check("r.2", "[2]", "1", "int")
               + Check("rend", "", TypeDef("std::reverse_iterator<QList<int>::iterator>", "Reverse"))
               + Check("rit", "", TypeDef("std::reverse_iterator<QList<int>::iterator>", "Reverse"));


   QTest::newRow("QLocale")
           << Data("#include <QLocale>\n",
                   "QLocale loc0;\n"
                   "QLocale loc = QLocale::system();\n"
                   "QLocale::MeasurementSystem m = loc.measurementSystem();\n"
                   "QLocale loc1(\"en_US\");\n"
                   "QLocale::MeasurementSystem m1 = loc1.measurementSystem();\n"
                   "unused(&loc0, &loc, &m, &loc1, &m1);\n")
              + CoreProfile()
              + NoCdbEngine
              + CheckType("loc", "@QLocale")
              + CheckType("m", "@QLocale::MeasurementSystem")
              + Check("loc1", "\"en_US\"", "@QLocale")
              + Check("loc1.country", "@QLocale::UnitedStates (225)", "@QLocale::Country") % Qt5
              + Check("loc1.language", "@QLocale::English (31)", "@QLocale::Language") % Qt5
              + Check("loc1.numberOptions", "@QLocale::DefaultNumberOptions (0)", "@QLocale::NumberOptions")
                    % QtVersion(0x0507000)  // New in 15b5b3b3f
              + Check("loc1.decimalPoint", "46", "@QChar") % Qt5   // .
              + Check("loc1.exponential", "101", "@QChar")  % Qt5  // e
              + Check("loc1.percent", "37", "@QChar")  % Qt5       // %
              + Check("loc1.zeroDigit", "48", "@QChar") % Qt5      // 0
              + Check("loc1.groupSeparator", "44", "@QChar") % Qt5 // ,
              + Check("loc1.negativeSign", "45", "@QChar")  % Qt5  // -
              + Check("loc1.positiveSign", "43", "@QChar") % Qt5   // +
              + Check("m1", ValuePattern(".*Imperial.*System (1)"),
                      TypePattern(".*MeasurementSystem")) % Qt5;


   QTest::newRow("QMap")
           << Data("#include <QMap>\n"
                   "#include <QObject>\n"
                   "#include <QPointer>\n"
                   "#include <QStringList>\n" + fooData + nsData,

                   "QMap<uint, QStringList> m0;\n"
                   "unused(&m0);\n\n"

                   "QMap<uint, QStringList> m1;\n"
                   "m1[11] = QStringList() << \"11\";\n"
                   "m1[22] = QStringList() << \"22\";\n\n"

                   "QMap<uint, float> m2;\n"
                   "m2[11] = 31.0;\n"
                   "m2[22] = 32.0;\n\n"

                   "typedef QMap<uint, QStringList> T;\n"
                   "T m3;\n"
                   "m3[11] = QStringList() << \"11\";\n"
                   "m3[22] = QStringList() << \"22\";\n\n"

                   "QMap<QString, float> m4;\n"
                   "m4[\"22.0\"] = 22.0;\n\n"

                   "QMap<int, QString> m5;\n"
                   "m5[22] = \"22.0\";\n\n"

                   "QMap<QString, Foo> m6;\n"
                   "m6[\"22.0\"] = Foo(22);\n"
                   "m6[\"33.0\"] = Foo(33);\n\n"

                   "QMap<QString, QPointer<QObject> > m7;\n"
                   "QObject ob;\n"
                   "m7.insert(\"Hallo\", QPointer<QObject>(&ob));\n"
                   "m7.insert(\"Welt\", QPointer<QObject>(&ob));\n"
                   "m7.insert(\".\", QPointer<QObject>(&ob));\n\n"

                   "QList<nsA::nsB::SomeType *> x;\n"
                   "x.append(new nsA::nsB::SomeType(1));\n"
                   "x.append(new nsA::nsB::SomeType(2));\n"
                   "x.append(new nsA::nsB::SomeType(3));\n"
                   "QMap<QString, QList<nsA::nsB::SomeType *> > m8;\n"
                   "m8[\"foo\"] = x;\n"
                   "m8[\"bar\"] = x;\n"
                   "m8[\"1\"] = x;\n"
                   "m8[\"2\"] = x;\n\n")

              + CoreProfile()

              + Check("m0", "<0 items>", "@QMap<unsigned int, @QStringList>")

              + Check("m1", "<2 items>", "@QMap<unsigned int, @QStringList>")
              + Check("m1.0.key", "11", "unsigned int")
              + Check("m1.0.value", "<1 items>", "@QStringList")
              + Check("m1.0.value.0", "[0]", "\"11\"", "@QString")
              + Check("m1.1.key", "22", "unsigned int")
              + Check("m1.1.value", "<1 items>", "@QStringList")
              + Check("m1.1.value.0", "[0]", "\"22\"", "@QString")

              + Check("m2", "<2 items>", "@QMap<unsigned int, float>")
              + Check("m2.0", "[0] 11", FloatValue("31.0"), "")
              + Check("m2.1", "[1] 22", FloatValue("32.0"), "")

              + Check("m3", "<2 items>", TypeDef("QMap<unsigned int,QStringList>", "T"))

              + Check("m4", "<1 items>", "@QMap<@QString, float>")
              + Check("m4.0.key", "\"22.0\"", "@QString")
              + Check("m4.0.value", FloatValue("22"), "float")

              + Check("m5", "<1 items>", "@QMap<int, @QString>")
              + Check("m5.0.key", "22", "int")
              + Check("m5.0.value", "\"22.0\"", "@QString")

              + Check("m6", "<2 items>", "@QMap<@QString, Foo>")
              + Check("m6.0.key", "\"22.0\"", "@QString")
              + Check("m6.0.value", "", "Foo")
              + Check("m6.0.value.a", "22", "int")
              + Check("m6.1.key", "\"33.0\"", "@QString")
              + Check("m6.1.value", "", "Foo")
              + Check("m6.1.value.a", "33", "int")

              + Check("m7", "<3 items>", "@QMap<@QString, @QPointer<@QObject>>")
              + Check("m7.0.key", "\".\"", "@QString")
              + Check("m7.0.value", "", "@QPointer<@QObject>")
              //+ Check("m7.0.value.o", Pointer(), "@QObject")
              // FIXME: it's '.wp' in Qt 5
              + Check("m7.1.key", "\"Hallo\"", "@QString")
              + Check("m7.2.key", "\"Welt\"", "@QString")

              + Check("m8", "<4 items>", "@QMap<@QString, @QList<nsA::nsB::SomeType*>>")
              + Check("m8.0.key", "\"1\"", "@QString")
              + Check("m8.0.value", "<3 items>", "@QList<nsA::nsB::SomeType*>")
              + Check("m8.0.value.0", "[0]", "", "nsA::nsB::SomeType")
              + Check("m8.0.value.0.a", "1", "int")
              + Check("m8.0.value.1", "[1]", "", "nsA::nsB::SomeType")
              + Check("m8.0.value.1.a", "2", "int")
              + Check("m8.0.value.2", "[2]", "", "nsA::nsB::SomeType")
              + Check("m8.0.value.2.a", "3", "int")
              + Check("m8.3.key", "\"foo\"", "@QString")
              + Check("m8.3.value", "<3 items>", "@QList<nsA::nsB::SomeType*>")
              + Check("m8.3.value.2", "[2]", "", "nsA::nsB::SomeType")
              + Check("m8.3.value.2.a", "3", "int")
              + Check("x", "<3 items>", "@QList<nsA::nsB::SomeType*>");


   QTest::newRow("QMultiMap")
           << Data("#include <QMultiMap>\n"
                   "#include <QObject>\n"
                   "#include <QPointer>\n"
                   "#include <QString>\n" + fooData,

                   "QMultiMap<int, int> m0;\n"
                   "unused(&m0);\n\n"

                   "QMultiMap<uint, float> m1;\n"
                   "m1.insert(11, 11.0);\n"
                   "m1.insert(22, 22.0);\n"
                   "m1.insert(22, 33.0);\n"
                   "m1.insert(22, 34.0);\n"
                   "m1.insert(22, 35.0);\n"
                   "m1.insert(22, 36.0);\n\n"

                   "QMultiMap<QString, float> m2;\n"
                   "m2.insert(\"22.0\", 22.0);\n\n"

                   "QMultiMap<int, QString> m3;\n"
                   "m3.insert(22, \"22.0\");\n\n"

                   "QMultiMap<QString, Foo> m4;\n"
                   "m4.insert(\"22.0\", Foo(22));\n"
                   "m4.insert(\"33.0\", Foo(33));\n"
                   "m4.insert(\"22.0\", Foo(22));\n\n"

                   "QObject ob;\n"
                   "QMultiMap<QString, QPointer<QObject> > m5;\n"
                   "m5.insert(\"Hallo\", QPointer<QObject>(&ob));\n"
                   "m5.insert(\"Welt\", QPointer<QObject>(&ob));\n"
                   "m5.insert(\".\", QPointer<QObject>(&ob));\n"
                   "m5.insert(\".\", QPointer<QObject>(&ob));\n\n")

              + CoreProfile()

              + Check("m0", "<0 items>", "@QMultiMap<int, int>")

              + Check("m1", "<6 items>", "@QMultiMap<unsigned int, float>")
              + Check("m1.0", "[0] 11", FloatValue("11"), "")
              + Check("m1.5", "[5] 22", FloatValue("22"), "")

              + Check("m2", "<1 items>", "@QMultiMap<@QString, float>")
              + Check("m2.0.key", "\"22.0\"", "@QString")
              + Check("m2.0.value", FloatValue("22"), "float")

              + CoreProfile()
              + Check("m3", "<1 items>", "@QMultiMap<int, @QString>")
              + Check("m3.0.key", "22", "int")
              + Check("m3.0.value", "\"22.0\"", "@QString")

              + CoreProfile()
              + Check("m4", "<3 items>", "@QMultiMap<@QString, Foo>")
              + Check("m4.0.key", "\"22.0\"", "@QString")
              + Check("m4.0.value", "", "Foo")
              + Check("m4.0.value.a", "22", "int")

              + Check("m5", "<4 items>", "@QMultiMap<@QString, @QPointer<@QObject>>")
              + Check("m5.0.key", "\".\"", "@QString")
              + Check("m5.0.value", "", "@QPointer<@QObject>")
              + Check("m5.1.key", "\".\"", "@QString")
              + Check("m5.2.key", "\"Hallo\"", "@QString")
              + Check("m5.3.key", "\"Welt\"", "@QString");


   QTest::newRow("QObject1")
           << Data("#include <QObject>\n",
                   "QObject parent;\n"
                   "parent.setObjectName(\"A Parent\");\n"
                   "QObject child(&parent);\n"
                   "child.setObjectName(\"A Child\");\n"
                   "QObject::connect(&child, SIGNAL(destroyed()), &parent, SLOT(deleteLater()));\n"
                   "QObject::disconnect(&child, SIGNAL(destroyed()), &parent, SLOT(deleteLater()));\n"
                   "child.setObjectName(\"A renamed Child\");\n")

              + CoreProfile()

              + Check("child", "\"A renamed Child\"", "@QObject")
              + Check("parent", "\"A Parent\"", "@QObject");


    QTest::newRow("QObject2")
            << Data("#include <QWidget>\n"
                    "#include <QApplication>\n"
                    "#include <QMetaObject>\n"
                    "#include <QMetaMethod>\n"
                    "#include <QVariant>\n\n"
                    "namespace Bar {\n"
                    "    struct Ui { Ui() { w = 0; } QWidget *w; };\n"
                    "    class TestObject : public QObject\n"
                    "    {\n"
                    "        Q_OBJECT\n"
                    "    public:\n"
                    "        TestObject(QObject *parent = 0)\n"
                    "            : QObject(parent)\n"
                    "        {\n"
                    "            m_ui = new Ui;\n"
                    "            m_ui->w = new QWidget;\n"
                    "        }\n"
                    "        Q_PROPERTY(QString myProp1 READ myProp1 WRITE setMyProp1)\n"
                    "        QString myProp1() const { return m_myProp1; }\n"
                    "        Q_SLOT void setMyProp1(const QString&mt) { m_myProp1 = mt; }\n"
                    "        Q_PROPERTY(QByteArray myProp2 READ myProp2 WRITE setMyProp2)\n"
                    "        QByteArray myProp2() const { return m_myProp2; }\n"
                    "        Q_SLOT void setMyProp2(const QByteArray&mt) { m_myProp2 = mt; }\n"
                    "        Q_PROPERTY(long myProp3 READ myProp3)\n"
                    "        long myProp3() const { return 54; }\n"
                    "        Q_PROPERTY(int myProp4 READ myProp4)\n"
                    "        int myProp4() const { return 44; }\n"
                    "    public:\n"
                    "        Ui *m_ui;\n"
                    "        QString m_myProp1;\n"
                    "        QByteArray m_myProp2;\n"
                    "    };\n"
                    "} // namespace Bar\n"
                    "#include <main.moc>\n",
                    ""
                    "QApplication app(argc, argv); unused(&app);\n"
                    "Bar::TestObject test;\n"
                    "test.setObjectName(\"Name\");\n"
                    "test.setMyProp1(\"Hello\");\n"
                    "test.setMyProp2(\"World\");\n"
                    "test.setProperty(\"New\", QVariant(QByteArray(\"Stuff\")));\n"
                    "test.setProperty(\"Old\", QVariant(QString(\"Cruft\")));\n"
                    "QString s = test.myProp1();\n"
                    "s += QString::fromLatin1(test.myProp2());\n"
                    "\n"
                    "const QMetaObject *mo = test.metaObject();\n"
                    "QMetaMethod mm0; unused(&mm0); \n"
                    "const QMetaObject smo = test.staticMetaObject;\n"
                    "QMetaMethod mm = mo->method(0); unused(&mm);\n"
                    "\n"
                    "QMetaEnum me0;\n"
                    "QMetaEnum me = mo->enumerator(0); unused(&me);\n"
                    "\n"
                    "QMetaProperty mp0;\n"
                    "QMetaProperty mp = mo->property(0); unused(&mp);\n"
                    "\n"
                    "QMetaClassInfo mci0;\n"
                    "QMetaClassInfo mci = mo->classInfo(0); unused(&mci);\n"
                    "\n"
                    "int n = mo->methodCount();\n"
                    "QVector<QMetaMethod> v(n);\n"
                    "for (int i = 0; i < n; ++i)\n"
                    "    v[i] = mo->method(i);\n")
               + GuiProfile()
               + Check("s", "\"HelloWorld\"", "@QString")
               + Check("test", "\"Name\"", "Bar::TestObject")
               + Check("test.[properties]", "<6 items>", "")
#ifndef Q_OS_WIN
               + Check("test.[properties].myProp1",
                    "\"Hello\"", "@QVariant (QString)")
               + Check("test.[properties].myProp2",
                    "\"World\"", "@QVariant (QByteArray)")
               + Check("test.[properties].myProp3", "54", "@QVariant (long)")
               + Check("test.[properties].myProp4", "44", "@QVariant (int)")
#endif
               + Check("test.[properties].4", "\"New\"",
                    "\"Stuff\"", "@QVariant (QByteArray)")
               + Check("test.[properties].5", "\"Old\"",
                    "\"Cruft\"", "@QVariant (QString)")
               + Check5("mm", "destroyed", "@QMetaMethod")
               + Check4("mm", "destroyed(QObject*)", "@QMetaMethod")
               + Check("mm.handle", "14", TypeDef("unsigned int", "uint"))
               + Check("mp", "objectName", "@QMetaProperty");

    QTest::newRow("QObject3")
            << Data("#include <QWidget>\n"
                    "#include <QList>\n"
                    "#include <QStringList>\n"
                    "#include <QVariant>\n"
                    "#include <QApplication>\n",

                    "QApplication app(argc, argv);\n"
                    "QWidget ob;\n"
                    "ob.setObjectName(\"An Object\");\n"
                    "ob.setProperty(\"USER DEFINED 1\", 44);\n"
                    "ob.setProperty(\"USER DEFINED 2\", QStringList() << \"FOO\" << \"BAR\");\n"
                    ""
                    "QObject ob1, ob2;\n"
                    "ob1.setObjectName(\"Another Object\");\n"
                    "QObject::connect(&ob, SIGNAL(destroyed()), &ob1, SLOT(deleteLater()));\n"
                    "QObject::connect(&ob, SIGNAL(destroyed()), &ob1, SLOT(deleteLater()));\n"
                    "//QObject::connect(&app, SIGNAL(lastWindowClosed()), &ob, SLOT(deleteLater()));\n"
                    ""
                    "QList<QObject *> obs;\n"
                    "obs.append(&ob);\n"
                    "obs.append(&ob1);\n"
                    "obs.append(0);\n"
                    "obs.append(&app);\n"
                    "ob2.setObjectName(\"A Subobject\");\n"
                    "unused(&ob, &ob1, &ob2);\n")

               + GuiProfile()

               + Check("ob", "\"An Object\"", "@QWidget")
               + Check("ob1", "\"Another Object\"", "@QObject")
               + Check("ob2", "\"A Subobject\"", "@QObject")
               + Check("ob.[extra].[connections].0.0.receiver", "\"Another Object\"",
                       "@QObject") % NoCdbEngine;


    QString senderData =
            "    class Sender : public QObject\n"
            "    {\n"
            "        Q_OBJECT\n"
            "    public:\n"
            "        Sender() { setObjectName(\"Sender\"); }\n"
            "        void doEmit() { emit aSignal(); }\n"
            "    signals:\n"
            "        void aSignal();\n"
            "    };\n"
            "\n"
            "    class Receiver : public QObject\n"
            "    {\n"
            "        Q_OBJECT\n"
            "    public:\n"
            "        Receiver() { setObjectName(\"Receiver\"); }\n"
            "    public slots:\n"
            "        void aSlot() {\n"
            "            QObject *s = sender();\n"
            "            if (s) {\n"
            "                qDebug() << \"SENDER: \" << s;\n"
            "            } else {\n"
            "                qDebug() << \"NO SENDER\";\n"
            "            }\n"
            "        }\n"
            "    };\n";

    QTest::newRow("QObjectData")
            << Data("#include <QObject>\n"
                    "#include <QStringList>\n"
                    "#include <private/qobject_p.h>\n"
                    "    class DerivedObjectPrivate : public QObjectPrivate\n"
                    "    {\n"
                    "    public:\n"
                    "        DerivedObjectPrivate() {\n"
                    "            m_extraX = 43;\n"
                    "            m_extraY.append(\"xxx\");\n"
                    "            m_extraZ = 1;\n"
                    "        }\n"
                    "        int m_extraX;\n"
                    "        QStringList m_extraY;\n"
                    "        uint m_extraZ : 1;\n"
                    "        bool m_extraA : 1;\n"
                    "        bool m_extraB;\n"
                    "    };\n"
                    "\n"
                    "    class DerivedObject : public QObject\n"
                    "    {\n"
                    "        Q_OBJECT\n"
                    "\n"
                    "    public:\n"
                    "        DerivedObject() : QObject(*new DerivedObjectPrivate, 0) {}\n"
                    "\n"
                    "        Q_PROPERTY(int x READ x WRITE setX)\n"
                    "        Q_PROPERTY(QStringList y READ y WRITE setY)\n"
                    "        Q_PROPERTY(uint z READ z WRITE setZ)\n"
                    "\n"
                    "        int x() const;\n"
                    "        void setX(int x);\n"
                    "        QStringList y() const;\n"
                    "        void setY(QStringList y);\n"
                    "        uint z() const;\n"
                    "        void setZ(uint z);\n"
                    "\n"
                    "    private:\n"
                    "        Q_DECLARE_PRIVATE(DerivedObject)\n"
                    "    };\n"
                    "\n"
                    "    int DerivedObject::x() const\n"
                    "    {\n"
                    "        Q_D(const DerivedObject);\n"
                    "        return d->m_extraX;\n"
                    "    }\n"
                    "\n"
                    "    void DerivedObject::setX(int x)\n"
                    "    {\n"
                    "        Q_D(DerivedObject);\n"
                    "        d->m_extraX = x;\n"
                    "        d->m_extraA = !d->m_extraA;\n"
                    "        d->m_extraB = !d->m_extraB;\n"
                    "    }\n"
                    "\n"
                    "    QStringList DerivedObject::y() const\n"
                    "    {\n"
                    "        Q_D(const DerivedObject);\n"
                    "        return d->m_extraY;\n"
                    "    }\n"
                    "\n"
                    "    void DerivedObject::setY(QStringList y)\n"
                    "    {\n"
                    "        Q_D(DerivedObject);\n"
                    "        d->m_extraY = y;\n"
                    "    }\n"
                    "\n"
                    "    uint DerivedObject::z() const\n"
                    "    {\n"
                    "        Q_D(const DerivedObject);\n"
                    "        return d->m_extraZ;\n"
                    "    }\n"
                    "\n"
                    "    void DerivedObject::setZ(uint z)\n"
                    "    {\n"
                    "        Q_D(DerivedObject);\n"
                    "        d->m_extraZ = z;\n"
                    "    }\n"
                    "#include \"main.moc\"\n",
                    "DerivedObject ob;\n"
                    "ob.setX(26);\n")
              + CoreProfile()
              + CorePrivateProfile();
// FIXME:
//              + Check("ob.properties.x", "26", "@QVariant (int)");


    QTest::newRow("QRegExp")
            << Data("#include <QRegExp>\n"
                    "#include <QStringList>\n",
                    "QRegExp re(QString(\"a(.*)b(.*)c\"));\n"
                    "QString str1 = \"a1121b344c\";\n"
                    "QString str2 = \"Xa1121b344c\";\n"
                    "int pos1 = re.indexIn(str1); unused(&pos1);\n"
                    "int pos2 = re.indexIn(str2); unused(&pos2);\n"
                    "QStringList caps = re.capturedTexts(); unused(&caps);\n")
               + CoreProfile()
               + Check("re", "\"a(.*)b(.*)c\"", "@QRegExp")
               + Check("re.captures.0", "[0]", "\"a1121b344c\"", "@QString")
               + Check("re.captures.1", "[1]", "\"1121\"", "@QString")
               + Check("re.captures.2", "[2]", "\"344\"", "@QString")
               + Check("str2", "\"Xa1121b344c\"", "@QString")
               + Check("pos1", "0", "int")
               + Check("pos2", "1", "int")
               + Check("caps", "<3 items>", "@QStringList");


    QTest::newRow("QRect")
            << Data("#include <QRect>\n"
                    "#include <QRectF>\n"
                    "#include <QPoint>\n"
                    "#include <QPointF>\n"
                    "#include <QSize>\n"
                    "#include <QSizeF>\n"
                    "#include <QString> // Dummy for namespace\n",

                    "QString dummy;\n"
                    "unused(&dummy);\n"

                    "QRect rect0, rect;\n"
                    "rect = QRect(100, 100, 200, 200);\n"
                    "QRectF rectf0, rectf;\n"
                    "rectf = QRectF(100.25, 100.25, 200.5, 200.5);\n"
                    "unused(&rect0, &rect);\n\n"

                    "QPoint p0, p;\n"
                    "p = QPoint(100, 200);\n"
                    "QPointF pf0, pf;\n"
                    "pf = QPointF(100.5, 200.5);\n"
                    "unused(&p0, &p);\n"

                    "QSize s0, s;\n"
                    "QSizeF sf0, sf;\n"
                    "sf = QSizeF(100.5, 200.5);\n"
                    "s = QSize(100, 200);\n"
                    "unused(&s0, &s);\n")

               + CoreProfile()

               + Check("rect0", "0x0+0+0", "@QRect")
               + Check("rect", "200x200+100+100", "@QRect")
               + Check("rectf0", "0.0x0.0+0.0+0.0", "@QRectF")
               + Check("rectf", "200.5x200.5+100.25+100.25", "@QRectF")

               + Check("p0", "(0, 0)", "@QPoint")
               + Check("p", "(100, 200)", "@QPoint")
               + Check("pf0", "(0.0, 0.0)", "@QPointF")
               + Check("pf", "(100.5, 200.5)", "@QPointF")

               + Check("s0", "(-1, -1)", "@QSize")
               + Check("s", "(100, 200)", "@QSize")
               + Check("sf0", "(-1.0, -1.0)", "@QSizeF")
               + Check("sf", "(100.5, 200.5)", "@QSizeF");


    QTest::newRow("QRegion")
            << Data("#include <QRegion>\n"
                    "#include <QVector>\n",
                    "QRegion region, region0, region1, region2;\n"
                    "region0 = region;\n"
                    "region += QRect(100, 100, 200, 200);\n"
                    "region1 = region;\n"
                    "QVector<QRect> rects = region1.rects(); // Warm up internal cache.\n"
                    "region += QRect(300, 300, 400, 500);\n"
                    "region2 = region;\n"
                    "unused(&region0, &region1, &region2, &rects);\n")

               + GuiProfile()

               + Check("region0", "<0 items>", "@QRegion")
               + Check("region1", "<1 items>", "@QRegion")
               + Check("region1.extents", "200x200+100+100", "@QRect")
               + Check("region1.innerArea", "40000", "int")
               + Check("region1.innerRect", "200x200+100+100", "@QRect")
               + Check("region1.numRects", "1", "int")
               // This seems to be 0(!) items on Linux, 1 on Mac
               // + Check("region1.rects", "<1 items>", "@QVector<@QRect>")
               + Check("region2", "<2 items>", "@QRegion")
               + Check("region2.extents", "600x700+100+100", "@QRect")
               + Check("region2.innerArea", "200000", "int")
               + Check("region2.innerRect", "400x500+300+300", "@QRect")
               + Check("region2.numRects", "2", "int")
               + Check("region2.rects", "<2 items>", "@QVector<@QRect>");


    QTest::newRow("QSettings")
            << Data("#include <QSettings>\n"
                    "#include <QCoreApplication>\n"
                    "#include <QVariant>\n",
                    "QCoreApplication app(argc, argv);\n"
                    "QSettings settings(\"/tmp/test.ini\", QSettings::IniFormat);\n"
                    "QVariant value = settings.value(\"item1\", \"\").toString();\n"
                    "unused(&value);\n")
               + CoreProfile()
               + Check("settings", "", "@QSettings")
               //+ Check("settings.@1", "[@QObject]", "", "@QObject")
               + Check("value", "\"\"", "@QVariant (QString)");


    QTest::newRow("QSet")
            << Data("#include <QSet>\n"
                    "#include <QString>\n\n"
                    "#include <QObject>\n"
                    "#include <QPointer>\n"
                    "QT_BEGIN_NAMESPACE\n"
                    "uint qHash(const QMap<int, int> &) { return 0; }\n"
                    "uint qHash(const double & f) { return int(f); }\n"
                    "uint qHash(const QPointer<QObject> &p) { return (ulong)p.data(); }\n"
                    "QT_END_NAMESPACE\n",

                    "QSet<int> s1;\n"
                    "s1.insert(11);\n"
                    "s1.insert(22);\n\n"

                    "QSet<QString> s2;\n"
                    "s2.insert(\"11.0\");\n"
                    "s2.insert(\"22.0\");\n\n"

                    "QObject ob;\n"
                    "QSet<QPointer<QObject> > s3;\n"
                    "QPointer<QObject> ptr(&ob);\n"
                    "s3.insert(ptr);\n"
                    "s3.insert(ptr);\n"
                    "s3.insert(ptr);\n")

               + CoreProfile()
               + Check("s1", "<2 items>", "@QSet<int>")
               + Check("s1.0", "[0]", "22", "int")
               + Check("s1.1", "[1]", "11", "int")

               + Check("s2", "<2 items>", "@QSet<@QString>")
               + Check("s2.0", "[0]", Value4("\"11.0\""), "@QString")
               + Check("s2.0", "[0]", Value5("\"22.0\""), "@QString")
               + Check("s2.1", "[1]", Value4("\"22.0\""), "@QString")
               + Check("s2.1", "[1]", Value5("\"11.0\""), "@QString")

               + Check("s3", "<1 items>", "@QSet<@QPointer<@QObject>>")
               + Check("s3.0", "[0]", "", "@QPointer<@QObject>");


    QString sharedData =
            "    class EmployeeData : public QSharedData\n"
            "    {\n"
            "    public:\n"
            "        EmployeeData() : id(-1) { name.clear(); }\n"
            "        EmployeeData(const EmployeeData &other)\n"
            "            : QSharedData(other), id(other.id), name(other.name) { }\n"
            "        ~EmployeeData() { }\n"
            "\n"
            "        int id;\n"
            "        QString name;\n"
            "    };\n"
            "\n"
            "    class Employee\n"
            "    {\n"
            "    public:\n"
            "        Employee() { d = new EmployeeData; }\n"
            "        Employee(int id, QString name) {\n"
            "            d = new EmployeeData;\n"
            "            setId(id);\n"
            "            setName(name);\n"
            "        }\n"
            "        Employee(const Employee &other)\n"
            "              : d (other.d)\n"
            "        {\n"
            "        }\n"
            "        void setId(int id) { d->id = id; }\n"
            "        void setName(QString name) { d->name = name; }\n"
            "\n"
            "        int id() const { return d->id; }\n"
            "        QString name() const { return d->name; }\n"
            "\n"
            "       private:\n"
            "         QSharedDataPointer<EmployeeData> d;\n"
            "    };\n";

    QTest::newRow("QAtomicPointer")
            << Data("#include <QAtomicPointer>\n"
                    "#include <QStringList>\n\n"
                    "template <class T> struct Pointer : QAtomicPointer<T> {\n"
                    "    Pointer(T *value = 0) : QAtomicPointer<T>(value) {}\n"
                    "};\n\n"
                    "struct SomeStruct {\n"
                    "    int a = 1;\n"
                    "    long b = 2;\n"
                    "    double c = 3.0;\n"
                    "    QString d = \"4\";\n"
                    "    QList<QString> e = {\"5\", \"6\" };\n"
                    "};\n\n"
                    "typedef Pointer<SomeStruct> SomeStructPointer;\n\n",

                    "SomeStruct *s = new SomeStruct; unused(s);\n"
                    "SomeStructPointer p(s); unused(p);\n"
                    "Pointer<SomeStruct> pp(s); unused(pp);\n"
                    "QAtomicPointer<SomeStruct> ppp(s); unused(ppp);\n")
                + CoreProfile()
                + Cxx11Profile()
                + MsvcVersion(1900)
                + Check("p.@1.a", "1", "int")
                + Check("p.@1.e", "<2 items>", "@QList<@QString>")
                + Check("pp.@1.a", "1", "int")
                + Check("ppp.a", "1", "int");


    QTest::newRow("QScopedPointer")
            << Data("#include <QScopedPointer>\n"
                    "#include <QString>\n",

                    "QScopedPointer<int> ptr10; unused(&ptr10);\n"
                    "QScopedPointer<int> ptr11(new int(32)); unused(&ptr11);\n\n"

                    "QScopedPointer<QString> ptr20; unused(&ptr20);\n"
                    "QScopedPointer<QString> ptr21(new QString(\"ABC\")); unused(&ptr21);\n\n")

               + CoreProfile()

               + Check("ptr10", "(null)", "@QScopedPointer<int>")
               + Check("ptr11", "32", "@QScopedPointer<int>")

               + Check("ptr20", "(null)", "@QScopedPointer<@QString>")
               + Check("ptr21", "\"ABC\"", "@QScopedPointer<@QString>");


    QTest::newRow("QSharedPointer")
            << Data("#include <QSharedPointer>\n"
                    "#include <QString>\n" + fooData,

                    "QSharedPointer<int> ptr10;\n"
                    "QSharedPointer<int> ptr11 = ptr10;\n"
                    "QSharedPointer<int> ptr12 = ptr10;\n"
                    "unused(&ptr10, &ptr11, &ptr12);\n\n"

                    "QSharedPointer<QString> ptr20(new QString(\"hallo\"));\n"
                    "QSharedPointer<QString> ptr21 = ptr20;\n"
                    "QSharedPointer<QString> ptr22 = ptr20;\n"
                    "unused(&ptr20, &ptr21, &ptr22);\n\n"

                    "QSharedPointer<int> ptr30(new int(43));\n"
                    "QWeakPointer<int> ptr31(ptr30);\n"
                    "QWeakPointer<int> ptr32 = ptr31;\n"
                    "QWeakPointer<int> ptr33 = ptr32;\n"
                    "unused(&ptr30, &ptr31, &ptr32);\n\n"

                    "QSharedPointer<QString> ptr40(new QString(\"hallo\"));\n"
                    "QWeakPointer<QString> ptr41(ptr40);\n"
                    "QWeakPointer<QString> ptr42 = ptr40;\n"
                    "QWeakPointer<QString> ptr43 = ptr40;\n"
                    "unused(&ptr40, &ptr41, &ptr42, &ptr43);\n\n"

                    "QSharedPointer<Foo> ptr50(new Foo(1));\n"
                    "QWeakPointer<Foo> ptr51(ptr50);\n"
                    "QWeakPointer<Foo> ptr52 = ptr50;\n"
                    "QWeakPointer<Foo> ptr53 = ptr50;\n"
                    "unused(&ptr50, &ptr51, &ptr52, &ptr53);\n")

               + CoreProfile()

               + Check("ptr10", "(null)", "@QSharedPointer<int>")
               + Check("ptr11", "(null)", "@QSharedPointer<int>")
               + Check("ptr12", "(null)", "@QSharedPointer<int>")

               + Check("ptr20", "\"hallo\"", "@QSharedPointer<@QString>")
               + Check("ptr20.data", "\"hallo\"", "@QString")
               + Check("ptr20.weakref", "3", "int")
               + Check("ptr20.strongref", "3", "int")
               + Check("ptr21.data", "\"hallo\"", "@QString")
               + Check("ptr22.data", "\"hallo\"", "@QString")

               + Check("ptr30", "43", "@QSharedPointer<int>")
               + Check("ptr30.data", "43", "int")
               + Check("ptr30.weakref", "4", "int")
               + Check("ptr30.strongref", "1", "int")
               + Check("ptr33", "43", "@QWeakPointer<int>")
               + Check("ptr33.data", "43", "int")

               + Check("ptr40", "\"hallo\"", "@QSharedPointer<@QString>")
               + Check("ptr40.data", "\"hallo\"", "@QString")
               + Check("ptr43", "\"hallo\"", "@QWeakPointer<@QString>")

               + Check("ptr50", "", "@QSharedPointer<Foo>")
               + Check("ptr50.data", "", "Foo")
               + Check("ptr53", "", "@QWeakPointer<Foo>");


    QTest::newRow("QFiniteStack")
            << Data("#include <stdlib.h>\n" // Needed on macOS.
                    "#include <private/qfinitestack_p.h>\n" + fooData,

                    "QFiniteStack<int> s1;\n"
                    "s1.allocate(2);\n"
                    "s1.push(1);\n"
                    "s1.push(2);\n\n"

                    "QFiniteStack<int> s2;\n"
                    "s2.allocate(100000);\n"
                    "for (int i = 0; i != 10000; ++i)\n"
                    "    s2.push(i);\n\n"

                    "QFiniteStack<Foo *> s3;\n"
                    "s3.allocate(10);\n"
                    "s3.push(new Foo(1));\n"
                    "s3.push(0);\n"
                    "s3.push(new Foo(2));\n"
                    "unused(&s3);\n\n"

                    "QFiniteStack<Foo> s4;\n"
                    "s4.allocate(10);\n"
                    "s4.push(1);\n"
                    "s4.push(2);\n"
                    "s4.push(3);\n"
                    "s4.push(4);\n\n"

                    "QFiniteStack<bool> s5;\n"
                    "s5.allocate(10);\n"
                    "s5.push(true);\n"
                    "s5.push(false);\n\n")

               + QmlPrivateProfile()

               + BigArrayProfile()

               + Check("s1", "<2 items>", "@QFiniteStack<int>")
               + Check("s1.0", "[0]", "1", "int")
               + Check("s1.1", "[1]", "2", "int")

               + Check("s2", "<10000 items>", "@QFiniteStack<int>")
               + Check("s2.0", "[0]", "0", "int")
               + Check("s2.8999", "[8999]", "8999", "int")

               + Check("s3", "<3 items>", "@QFiniteStack<Foo*>")
               + Check("s3.0", "[0]", "", "Foo")
               + Check("s3.0.a", "1", "int")
               + Check("s3.1", "[1]", "0x0", "Foo *")
               + Check("s3.2", "[2]", "", "Foo")
               + Check("s3.2.a", "2", "int")

               + Check("s4", "<4 items>", "@QFiniteStack<Foo>")
               + Check("s4.0", "[0]", "", "Foo")
               + Check("s4.0.a", "1", "int")
               + Check("s4.3", "[3]", "", "Foo")
               + Check("s4.3.a", "4", "int")

               + Check("s5", "<2 items>", "@QFiniteStack<bool>")
               + Check("s5.0", "[0]", "1", "bool") // 1 -> true is done on display
               + Check("s5.1", "[1]", "0", "bool");


/*
    QTest::newRow("QStandardItemModel")
            << Data("#include <QStandardItemModel>\n",

                    "QStandardItemModel m;\n"
                    "QStandardItem *i1, *i2, *i11;\n"
                    "m.appendRow(QList<QStandardItem *>()\n"
                    "     << (i1 = new QStandardItem(\"1\")) "
                    "       << (new QStandardItem(\"a\")) "
                    "       << (new QStandardItem(\"a2\")));\n"
                    "QModelIndex mi = i1->index();\n"
                    "m.appendRow(QList<QStandardItem *>()\n"
                    "     << (i2 = new QStandardItem(\"2\")) "
                    "       << (new QStandardItem(\"b\")));\n"
                    "i1->appendRow(QList<QStandardItem *>()\n"
                    "     << (i11 = new QStandardItem(\"11\")) "
                    "       << (new QStandardItem(\"aa\")));\n"
                    "unused(&i1, &i2, &i11, &m, &mi);\n")

               + GdbEngine
               + GuiProfile()

               + Check("i1", "", "@QStandardItem")
               + Check("i11", "", "@QStandardItem")
               + Check("i2", "", "@QStandardItem")
               + Check("m", "", "@QStandardItemModel")
               + Check("mi", "\"1\"", "@QModelIndex");
*/


    QTest::newRow("QStack")
            << Data("#include <QStack>\n" + fooData,

                    "QStack<int> s1;\n"
                    "s1.append(1);\n"
                    "s1.append(2);\n"
                    "unused(&s1);\n\n"

                    "QStack<int> s2;\n"
                    "for (int i = 0; i != 10000; ++i)\n"
                    "    s2.append(i);\n"
                    "unused(&s2);\n\n"

                    "QStack<Foo *> s3;\n"
                    "s3.append(new Foo(1));\n"
                    "s3.append(0);\n"
                    "s3.append(new Foo(2));\n"
                    "unused(&s3);\n\n"

                    "QStack<Foo> s4;\n"
                    "s4.append(1);\n"
                    "s4.append(2);\n"
                    "s4.append(3);\n"
                    "s4.append(4);\n"
                    "unused(&s4);\n\n"

                    "QStack<bool> s5;\n"
                    "s5.append(true);\n"
                    "s5.append(false);\n"
                    "unused(&s5);\n\n")

               + CoreProfile()

               + BigArrayProfile()

               + Check("s1", "<2 items>", "@QStack<int>")
               + Check("s1.0", "[0]", "1", "int")
               + Check("s1.1", "[1]", "2", "int")

               + Check("s2", "<10000 items>", "@QStack<int>")
               + Check("s2.0", "[0]", "0", "int")
               + Check("s2.8999", "[8999]", "8999", "int")

               + Check("s3", "<3 items>", "@QStack<Foo*>")
               + Check("s3.0", "[0]", "", "Foo")
               + Check("s3.0.a", "1", "int")
               + Check("s3.1", "[1]", "0x0", "Foo *")
               + Check("s3.2", "[2]", "", "Foo")
               + Check("s3.2.a", "2", "int")

               + Check("s4", "<4 items>", "@QStack<Foo>")
               + Check("s4.0", "[0]", "", "Foo")
               + Check("s4.0.a", "1", "int")
               + Check("s4.3", "[3]", "", "Foo")
               + Check("s4.3.a", "4", "int")

               + Check("s5", "<2 items>", "@QStack<bool>")
               + Check("s5.0", "[0]", "1", "bool") // 1 -> true is done on display
               + Check("s5.1", "[1]", "0", "bool");


    QTest::newRow("QTimeZone")
            << Data("#include <QTimeZone>\n",
                    "QTimeZone tz0;\n"
                    "QTimeZone tz1(\"UTC+05:00\");\n"
                    "unused(&tz0, &tz1);\n")

               + CoreProfile()
               + QtVersion(0x50200)

               + Check("tz0", "(null)", "@QTimeZone")
               + Check("tz1", "\"UTC+05:00\"", "@QTimeZone");


    QTest::newRow("QUrl")
            << Data("#include <QUrl>",
                    "QUrl url0;\n"
                    "QUrl url1 = QUrl::fromEncoded(\"http://foo@qt-project.org:10/have_fun\");\n"
                    "int port = url1.port();\n"
                    "QString path = url1.path();\n"
                    "unused(&url0, &url1, &port, &path);\n")

               + CoreProfile()

               + Check("url0", "<invalid>", "@QUrl")
               + Check("url1", UnsubstitutedValue("\"http://foo@qt-project.org:10/have_fun\""), "@QUrl")
               + Check("url1.port", "10", "int")
               + Check("url1.scheme", "\"http\"", "?QString")
               + Check("url1.userName", "\"foo\"", "?QString")
               + Check("url1.password", "\"\"", "?QString")
               + Check("url1.host", "\"qt-project.org\"", "?QString")
               + Check("url1.path", "\"/have_fun\"", "?QString")
               + Check5("url1.query", "\"\"", "?QString")
               + Check4("url1.query", "\"\"", "?QByteArray")
               + Check("url1.fragment", "\"\"", "?QString");


    QTest::newRow("QUuid")
            << Data("#include <QUuid>",
                    "QUuid uuid1(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11);\n"
                    "QUuid uuid2(0xfffffffeu, 0xfffd, 0xfffc, 0xfb, "
                    "  0xfa, 0xf9, 0xf8, 0xf7, 0xf6, 0xf5, 0xf4);\n"
                    "unused(&uuid1, &uuid2);\n")
               + CoreProfile()
               + Check("uuid1", "{00000001-0002-0003-0405-060708090a0b}", "@QUuid")
               + Check("uuid2", "{fffffffe-fffd-fffc-fbfa-f9f8f7f6f5f4}", "@QUuid");


    QString expected1 = "\"AAA";
    expected1.append(QChar('\t'));
    expected1.append(QChar('\r'));
    expected1.append(QChar('\n'));
    expected1.append(QChar(0));
    expected1.append(QChar(1));
    expected1.append("BBB\"");

    QChar oUmlaut = QLatin1Char(char(0xf6));

    QTest::newRow("QString")
            << Data("#include <QByteArray>\n"
                    "#include <QString>\n"
                    "#include <QStringList>\n"
                    "#include <QStringRef>\n",

                    "QByteArray s0 = \"Hello\";\n"
                    "s0.prepend(\"Prefix: \");\n"
                    "unused(&s0);\n\n"

                    "QByteArray s1 = \"AAA\";\n"
                    "s1 += '\\t';\n"
                    "s1 += '\\r';\n"
                    "s1 += '\\n';\n"
                    "s1 += char(0);\n"
                    "s1 += char(1);\n"
                    "s1 += \"BBB\";\n"
                    "unused(&s1);\n\n"

                    "QChar data[] = { 'H', 'e', 'l', 'l', 'o' };\n"
                    "QString s2 = QString::fromRawData(data, 4);\n"
                    "QString s3 = QString::fromRawData(data + 1, 4);\n"
                    "unused(&data, &s2, &s3);\n\n"

                    "QString s4 = \"Hello \";\n"
                    "QString s5(\"String Test\");\n"
                    "QString *s6 = new QString(\"Pointer String Test\");\n"
                    "unused(&s4, &s5, &s6);\n\n"

                    "const wchar_t *w = L\"aöa\";\n"
                    "QString s7;\n"
                    "if (sizeof(wchar_t) == 4)\n"
                    "    s7 = QString::fromUcs4((uint *)w);\n"
                    "else\n"
                    "    s7 = QString::fromUtf16((ushort *)w);\n"
                    "unused(&w, &s7);\n\n"

                    "QString str = \"Hello\";\n"
                    "QStringRef s8(&str, 1, 2);\n"
                    "QStringRef s9;\n"
                    "unused(&s8, &s9);\n\n"

                    "QStringList l;\n"
                    "l << \"Hello \";\n"
                    "l << \" big, \";\n"
                    "l.takeFirst();\n"
                    "l << \" World \";\n\n"

                    "QString str1(\"Hello Qt\"); unused(&str1);\n"
                    "QString str2(\"Hello\\nQt\"); unused(&str2);\n"
                    "QString str3(\"Hello\\rQt\"); unused(&str3);\n"
                    "QString str4(\"Hello\\tQt\"); unused(&str4);\n\n"

                    "#if QT_VERSION > 0x50000\n"
                    "static const QStaticStringData<3> qstring_literal = {\n"
                    "    Q_STATIC_STRING_DATA_HEADER_INITIALIZER(3),\n"
                    "    QT_UNICODE_LITERAL(u\"ABC\") };\n"
                    "QStringDataPtr holder = { qstring_literal.data_ptr() };\n"
                    "const QString qstring_literal_temp(holder); unused(&holder);\n\n"

                    "QStaticStringData<1> sd;\n"
                    "sd.data[0] = 'Q';\n"
                    "sd.data[1] = 0;\n"
                    "#endif\n")

               + CoreProfile()
               + MsvcVersion(1900)
               + Check("s0", "\"Prefix: Hello\"", "@QByteArray")
               + Check("s1", expected1, "@QByteArray")
               + Check("s2", "\"Hell\"", "@QString")
               + Check("s3", "\"ello\"", "@QString")

               + Check("s4", "\"Hello \"", "@QString")
               + Check("s5", "\"String Test\"", "@QString")
               + Check("s6", "\"Pointer String Test\"", "@QString")

               + Check("s7", QString::fromLatin1("\"a%1a\"").arg(oUmlaut), "@QString")
               + CheckType("w", "w", "wchar_t *")

               + Check("s8", "\"el\"", "@QStringRef")
               + Check("s9", "(null)", "@QStringRef")

               + Check("l", "<2 items>", "@QStringList")
               + Check("l.0", "[0]", "\" big, \"", "@QString")
               + Check("l.1", "[1]", "\" World \"", "@QString")

               + Check("str1", "\"Hello Qt\"", "@QString")
               + Check("str2", "\"Hello\nQt\"", "@QString")
               + Check("str3", "\"Hello\rQt\"", "@QString")
               + Check("str4", "\"Hello\tQt\"", "@QString")

               + Check("holder", "", "@QStringDataPtr") % Qt5
               + Check("holder.ptr", "\"ABC\"", TypeDef("@QTypedArrayData<unsigned short>",
                                                        "@QStringData")) % Qt5
               + Check("sd", "\"Q\"", "@QStaticStringData<1>") % Qt5;


    QTest::newRow("QStringReference")
            << Data("#include <QString>\n"
                    "void stringRefTest(const QString &refstring) {\n"
                    "   BREAK;\n"
                    "   unused(&refstring);\n"
                    "}\n",
                    "stringRefTest(QString(\"Ref String Test\"));\n")

               + CoreProfile()

               + Check("refstring", "\"Ref String Test\"", "@QString &") % NoCdbEngine
               + Check("refstring", "\"Ref String Test\"", "@QString") % CdbEngine;


    QTest::newRow("QText")
            << Data("#include <QApplication>\n"
                    "#include <QTextCursor>\n"
                    "#include <QTextDocument>\n",
                    "QApplication app(argc, argv);\n"
                    "QTextDocument doc;\n"
                    "doc.setPlainText(\"Hallo\\nWorld\");\n"
                    "QTextCursor tc;\n"
                    "tc = doc.find(\"all\");\n"
                    "int pos = tc.position();\n"
                    "int anc = tc.anchor();\n"
                    "unused(&pos, &anc);\n")
               + GuiProfile()
               + CheckType("doc", "@QTextDocument")
               + Check("tc", "4", "@QTextCursor")
               + Check("pos", "4", "int")
               + Check("anc", "1", "int");

    QTest::newRow("QThread")
            << Data("#include <QThread>\n"
                    "struct Thread : QThread\n"
                    "{\n"
                    "    void run()\n"
                    "    {\n"
                    "        auto mo = &QThread::metaObject;\n"
                    "        auto mc = &QThread::qt_metacast;\n"
                    "        auto p0 = (*(void***)this)[0]; unused(&p0);\n"
                    "        auto p1 = (*(void***)this)[1]; unused(&p1);\n"
                    "        auto p2 = (*(void***)this)[2]; unused(&p2);\n"
                    "        auto p3 = (*(void***)this)[3]; unused(&p3);\n"
                    "        auto p4 = (*(void***)this)[4]; unused(&p4);\n"
                    "        auto p5 = (*(void***)this)[5]; unused(&p5);\n"
                    "        if (m_id == 3) {\n"
                    "            BREAK;\n"
                    "        }\n"
                    "    }\n"
                    "    int m_id;\n"
                    "};\n",

                    "const int N = 14;\n"
                    "Thread thread[N];\n"
                    "for (int i = 0; i != N; ++i) {\n"
                    "    thread[i].m_id = i;\n"
                    "    thread[i].setObjectName(\"This is thread #\" + QString::number(i));\n"
                    "    thread[i].start();\n"
                    "}\n"
                    "for (int i = 0; i != N; ++i) {\n"
                    "    thread[i].wait();\n"
                    "}\n")

               + CoreProfile()

               + CheckType("this", "Thread")
               + Check("this.@1", "[@QThread]", "\"This is thread #3\"", "@QThread");
               //+ Check("this.@1.@1", "[@QObject]", "\"This is thread #3\"", "@QObject");


    QTest::newRow("QVariant")
            << Data("#include <QMap>\n"
                    "#include <QStringList>\n"
                    "#include <QVariant>\n"
                    // This checks user defined types in QVariants\n";
                    "typedef QMap<uint, QStringList> MyType;\n"
                    "Q_DECLARE_METATYPE(QStringList)\n"
                    "Q_DECLARE_METATYPE(MyType)\n"
                    "#if QT_VERSION < 0x050000\n"
                    "Q_DECLARE_METATYPE(QList<int>)\n"
                    "#endif\n",

                    "QVariant v0;\n"
                    "unused(&v0);\n\n"

                    "QVariant::Type t1 = QVariant::String;\n"
                    "QVariant v1 = QVariant(t1, (void*)0);\n"
                    //"*(QString*)v1.data() = QString(\"Some string\");\n"
                    "unused(&v1);\n\n"

                    "MyType my;\n"
                    "my[1] = (QStringList() << \"Hello\");\n"
                    "my[3] = (QStringList() << \"World\");\n"
                    "QVariant v2;\n"
                    "v2.setValue(my);\n"
                    "int t = QMetaType::type(\"MyType\");\n"
                    "const char *s = QMetaType::typeName(t);\n"
                    "unused(&v2, &t, &s);\n\n"

                    "QList<int> list;\n"
                    "list << 1 << 2 << 3;\n"
                    "QVariant v3 = qVariantFromValue(list);\n"
                    "unused(&list, &v3);\n\n")

               + CoreProfile()

               + Check("v0", "(invalid)", "@QVariant (invalid)")

               //+ Check("v1", "\"Some string\"", "@QVariant (QString)")
               + CheckType("v1", "@QVariant (QString)")

               + Check("my", "<2 items>", TypeDef("QMap<unsigned int,QStringList>", "MyType"))
               + Check("my.0.key", "1", "unsigned int")
               + Check("my.0.value", "<1 items>", "@QStringList")
               + Check("my.0.value.0", "[0]", "\"Hello\"", "@QString")
               + Check("my.1.key", "3", "unsigned int")
               + Check("my.1.value", "<1 items>", "@QStringList")
               + Check("my.1.value.0", "[0]", "\"World\"", "@QString")
               //+ CheckType("v2", "@QVariant (MyType)")
               + Check("my", "<2 items>", TypeDef("QMap<unsigned int,QStringList>", "MyType"))
               + Check("v2.data.0.key", "1", "unsigned int") % NoCdbEngine
               + Check("v2.data.0.value", "<1 items>", "@QStringList") % NoCdbEngine
               + Check("v2.data.0.value.0", "[0]", "\"Hello\"", "@QString") % NoCdbEngine
               + Check("v2.data.1.key", "3", "unsigned int") % NoCdbEngine
               + Check("v2.data.1.value", "<1 items>", "@QStringList") % NoCdbEngine
               + Check("v2.data.1.value.0", "[0]", "\"World\"", "@QString") % NoCdbEngine

               + Check("list", "<3 items>", "@QList<int>")
               + Check("list.0", "[0]", "1", "int")
               + Check("list.1", "[1]", "2", "int")
               + Check("list.2", "[2]", "3", "int")
               //+ Check("v3", "", "@QVariant (@QList<int>)")
               + Check("v3.data", "<3 items>", TypePattern(".*QList<int>")) % NoCdbEngine
               + Check("v3.data.0", "[0]", "1", "int") % NoCdbEngine
               + Check("v3.data.1", "[1]", "2", "int") % NoCdbEngine
               + Check("v3.data.2", "[2]", "3", "int") % NoCdbEngine;


    QTest::newRow("QVariant2")
            << Data("#include <QApplication>\n"
                    "#include <QBitArray>\n"
                    "#include <QDateTime>\n"
                    "#include <QLocale>\n"
                    "#include <QMap>\n"
                    "#include <QRectF>\n"
                    "#include <QRect>\n"
                    "#include <QStringList>\n"
                    "#include <QUrl>\n"
                    "#include <QVariant>\n"
                    "#include <QFont>\n"
                    "#include <QPixmap>\n"
                    "#include <QBrush>\n"
                    "#include <QColor>\n"
                    "#include <QPalette>\n"
                    "#include <QIcon>\n"
                    "#include <QImage>\n"
                    "#include <QPolygon>\n"
                    "#include <QRegion>\n"
                    "#include <QBitmap>\n"
                    "#include <QCursor>\n"
                    "#include <QSizePolicy>\n"
                    "#include <QKeySequence>\n"
                    "#include <QPen>\n"
                    "#include <QTextLength>\n"
                    "#include <QTextFormat>\n"
                    "#include <QTransform>\n"
                    "#include <QMatrix4x4>\n"
                    "#include <QVector2D>\n"
                    "#include <QVector3D>\n"
                    "#include <QVector4D>\n"
                    "#include <QPolygonF>\n"
                    "#include <QQuaternion>\n"
                    "#if QT_VERSION < 0x050000\n"
                    "Q_DECLARE_METATYPE(QPolygonF)\n"
                    "Q_DECLARE_METATYPE(QPen)\n"
                    "Q_DECLARE_METATYPE(QTextLength)\n"
                    "#endif\n",
                    "QApplication app(argc, argv);\n"
                    "QRect r(100, 200, 300, 400);\n"
                    "QPen pen;\n"
                    "QRectF rf(100.5, 200.5, 300.5, 400.5);\n"
                    "QUrl url = QUrl::fromEncoded(\"http://foo@qt-project.org:10/have_fun\");\n"
                     "QVariant var0; unused(&var0);                                  // Type 0, invalid\n"
                     "QVariant var1(true); unused(&var1);                            // 1, bool\n"
                     "QVariant var2(2); unused(&var2);                               // 2, int\n"
                     "QVariant var3(3u); unused(&var3);                              // 3, uint\n"
                     "QVariant var4(qlonglong(4)); unused(&var4);                    // 4, qlonglong\n"
                     "QVariant var5(qulonglong(5)); unused(&var5);                   // 5, qulonglong\n"
                     "QVariant var6(double(6.0)); unused(&var6);                     // 6, double\n"
                     "QVariant var7(QChar(7)); unused(&var7);                        // 7, QChar\n"
                     "QVariant var8 = QVariantMap(); unused(&var8);                  // 8, QVariantMap\n"
                     "QVariant var9 = QVariantList(); unused(&var9);                 // 9, QVariantList\n"
                     "QVariant var10 = QString(\"Hello 10\"); unused(&var10);        // 10, QString\n"
                     "QVariant var11 = QStringList() << \"Hello\" << \"World\"; unused(&var11); // 11, QStringList\n"
                     "QVariant var12 = QByteArray(\"array\"); unused(&var12);        // 12 QByteArray\n"
                     "QVariant var13 = QBitArray(1, true); unused(&var13);           // 13 QBitArray\n"
                     "QVariant var14 = QDate(); unused(&var14);                      // 14 QDate\n"
                     "QVariant var15 = QTime(); unused(&var15);                      // 15 QTime\n"
                     "QDateTime dateTime(QDate(1980, 1, 1), QTime(13, 15, 32), Qt::UTC);\n"
                     "QVariant var16 = dateTime; unused(&var16);                     // 16 QDateTime\n"
                     "QVariant var17 = url; unused(&url, &var17);                    // 17 QUrl\n"
                     "QVariant var18 = QLocale(\"en_US\"); unused(&var18);           // 18 QLocale\n"
                     "QVariant var19(r); unused(&var19);                             // 19 QRect\n"
                     "QVariant var20(rf); unused(&var20);                            // 20 QRectF\n"
                     "QVariant var21 = QSize(); unused(&var21);                      // 21 QSize\n"
                     "QVariant var22 = QSizeF(); unused(&var22);                     // 22 QSizeF\n"
                     "QVariant var23 = QLine(); unused(&var23);                      // 23 QLine\n"
                     "QVariant var24 = QLineF(); unused(&var24);                     // 24 QLineF\n"
                     "QVariant var25 = QPoint(); unused(&var25);                     // 25 QPoint\n"
                     "QVariant var26 = QPointF(); unused(&var26);                    // 26 QPointF\n"
                     "QVariant var27 = QRegExp(); unused(&var27);                    // 27 QRegExp\n"
                     "QVariant var28 = QVariantHash(); unused(&var28);               // 28 QVariantHash\n"
                     "QVariant var31 = QVariant::fromValue<void *>(&r); unused(&var31);         // 31 void *\n"
                     "QVariant var32 = QVariant::fromValue<long>(32); unused(&var32);           // 32 long\n"
                     "QVariant var33 = QVariant::fromValue<short>(33); unused(&var33);          // 33 short\n"
                     "QVariant var34 = QVariant::fromValue<char>(34); unused(&var34);           // 34 char\n"
                     "QVariant var35 = QVariant::fromValue<unsigned long>(35); unused(&var35);  // 35 unsigned long\n"
                     "QVariant var36 = QVariant::fromValue<unsigned short>(36); unused(&var36); // 36 unsigned short\n"
                     "QVariant var37 = QVariant::fromValue<unsigned char>(37); unused(&var37);  // 37 unsigned char\n"
                     "QVariant var38 = QVariant::fromValue<float>(38); unused(&var38);          // 38 float\n"
                     "QVariant var64 = QFont(); unused(&var64);                      // 64 QFont\n"
                     "QPixmap pixmap(QSize(1, 2)); unused(&pixmap);\n"
                     "QVariant var65 = pixmap; unused(&var65);                       // 65 QPixmap\n"
                     "QVariant var66 = QBrush(); unused(&var66);                     // 66 QBrush\n"
                     "QVariant var67 = QColor(); unused(&var67);                     // 67 QColor\n"
                     "QVariant var68 = QPalette(); unused(&var68);                   // 68 QPalette\n"
                     "QVariant var69 = QIcon(); unused(&var69);                      // 69 QIcon\n"
                     "QImage image(1, 2, QImage::Format_RGB32);\n"
                     "QVariant var70 = image; unused(&var70);                        // 70 QImage\n"
                     "QVariant var71 = QPolygon(); unused(&var71);                   // 71 QPolygon\n"
                     "QRegion reg; reg += QRect(1, 2, 3, 4);\n"
                     "QVariant var72 = reg; unused(&var72, &reg);                    // 72 QRegion\n"
                     "QBitmap bitmap; unused(&bitmap);\n"
                     "QVariant var73 = bitmap; unused(&var73);                       // 73 QBitmap\n"
                     "QVariant var74 = QCursor(); unused(&var74);                    // 74 QCursor\n"
                     "QVariant var75 = QKeySequence(); unused(&var75);               // 75 QKeySequence\n"
                     "QVariant var76 = pen; unused(&pen, &var76);                    // 76 QPen\n"
                     "QVariant var77 = QTextLength(); unused(&var77);                // 77 QTextLength\n"
                     "QVariant var78 = QTextFormat(); unused(&var78);                // 78 QTextFormat\n"
                     "QVariant var80 = QTransform(); unused(&var80);                 // 80 QTransform\n"
                     "QVariant var81 = QMatrix4x4(); unused(&var81);                 // 81 QMatrix4x4\n"
                     "QVariant var82 = QVector2D(); unused(&var82);                  // 82 QVector2D\n"
                     "QVariant var83 = QVector3D(); unused(&var83);                  // 83 QVector3D\n"
                     "QVariant var84 = QVector4D(); unused(&var84);                  // 84 QVector4D\n"
                     "QVariant var85 = QQuaternion(); unused(&var85);                // 85 QQuaternion\n"
                     "QVariant var86 = QVariant::fromValue<QPolygonF>(QPolygonF()); unused(&var86);\n"
                    )

               + GuiProfile()

               + Check("var0", "(invalid)", "@QVariant (invalid)")
               + Check("var1", "true", "@QVariant (bool)")
               + Check("var2", "2", "@QVariant (int)")
               + Check("var3", "3", "@QVariant (uint)")
               + Check("var4", "4", "@QVariant (qlonglong)")
               + Check("var5", "5", "@QVariant (qulonglong)")
               + Check("var6", "6.0", "@QVariant (double)")
               + Check("var7", "7", "@QVariant (QChar)")
               + Check("var8", "<0 items>", "@QVariant (QVariantMap)")
               + Check("var9", "<0 items>", "@QVariant (QVariantList)")
               + Check("var10", "\"Hello 10\"", "@QVariant (QString)")
               + Check("var11", "<2 items>", "@QVariant (QStringList)")
               + Check("var11.1", "[1]", "\"World\"", "@QString")
               + Check("var12", "\"array\"", "@QVariant (QByteArray)")
               + Check("var13", "<1 items>", "@QVariant (QBitArray)")
               + Check("var14", "(invalid)", "@QVariant (QDate)")
               + Check("var15", "(invalid)", "@QVariant (QTime)")
//               + Check("var16", "(invalid)", "@QVariant (QDateTime)")
               + Check("var17", UnsubstitutedValue("\"http://foo@qt-project.org:10/have_fun\""), "@QVariant (QUrl)")
               + Check("var17.port", "10", "int")
//               + Check("var18", "\"en_US\"", "@QVariant (QLocale)")
               + Check("var19", "300x400+100+200", "@QVariant (QRect)")
               + Check("var20", "300.5x400.5+100.5+200.5", "@QVariant (QRectF)")
               + Check("var21", "(-1, -1)", "@QVariant (QSize)")
               + Check("var22", "(-1.0, -1.0)", "@QVariant (QSizeF)")
               + Check("var23", "", "@QVariant (QLine)")
               + Check("var24", "", "@QVariant (QLineF)")
               + Check("var25", "(0, 0)", "@QVariant (QPoint)")
               + Check("var26", "(0.0, 0.0)", "@QVariant (QPointF)")
               + Check("var27", "\"\"", "@QVariant (QRegExp)")
               + Check("var28", "<0 items>", "@QVariant (QVariantHash)")
               + CheckType("var31", "@QVariant (void *)")
               + Check("var32", "32", "@QVariant (long)")
               + Check("var33", "33", "@QVariant (short)")
               + Check("var34", "34", "@QVariant (char)")
               + Check("var35", "35", "@QVariant (unsigned long)")
               + Check("var36", "36", "@QVariant (unsigned short)")
               + Check("var37", "37", "@QVariant (unsigned char)")
               + Check("var38", FloatValue("38.0"), "@QVariant (float)")
               + Check("var64", "", "@QVariant (QFont)")
               + Check("var65", "(1x2)", "@QVariant (QPixmap)")
               + Check("var66", "", "@QVariant (QBrush)")
               + Check("var67", "", "@QVariant (QColor)")
               + Check("var68", "", "@QVariant (QPalette)")
               + Check("var69", "", "@QVariant (QIcon)")
               + Check("var70", "(1x2)", "@QVariant (QImage)")
               + Check("var71", "<0 items>", "@QVariant (QPolygon)")
               //+ Check("var72", "", "@QVariant (QRegion)")    FIXME
               + Check("var73", "", "@QVariant (QBitmap)")
               + Check("var74", "", "@QVariant (QCursor)")
               + Check("var75", "", "@QVariant (QKeySequence)") % NoLldbEngine // FIXME
               + Check("var76", "", "@QVariant (QPen)")
               + Check("var77", "", "@QVariant (QTextLength)")
               //+ Check("var78", Value5(""), "@QVariant (QTextFormat)")
               + Check("var80", "", "@QVariant (QTransform)")
               + Check("var81", "", "@QVariant (QMatrix4x4)")
               + Check("var82", "", "@QVariant (QVector2D)")
               + Check("var83", "", "@QVariant (QVector3D)")
               + Check("var84", "", "@QVariant (QVector4D)")
               + Check("var85", "", "@QVariant (QQuaternion)")
               + Check5("var86", "<0 items>", "@QVariant (QPolygonF)");


    QTest::newRow("QVariant4")
            << Data("#include <QHostAddress>\n"
                    "#include <QVariant>\n"
                    "Q_DECLARE_METATYPE(QHostAddress)\n",
                    "QVariant var;\n"
                    "QHostAddress ha;\n"
                    "ha.setAddress(\"127.0.0.1\");\n"
                    "var.setValue(ha);\n"
                    "QHostAddress ha1 = var.value<QHostAddress>();\n"
                    "unused(&ha1);\n")

               + NetworkProfile()

               + Check("ha", ValuePattern(".*127.0.0.1.*"), "@QHostAddress")
               + Check("ha.a", "2130706433", TypeDef("unsigned int", "@quint32"))
               + Check("ha.ipString", ValuePattern(".*127.0.0.1.*"), "@QString")
                    % QtVersion(0, 0x50800)
               //+ Check("ha.protocol", "@QAbstractSocket::IPv4Protocol (0)",
               //        "@QAbstractSocket::NetworkLayerProtocol") % GdbEngine
               //+ Check("ha.protocol", "IPv4Protocol",
               //        "@QAbstractSocket::NetworkLayerProtocol") % LldbEngine
               + Check("ha.scopeId", "\"\"", "@QString")
               + Check("ha1", ValuePattern(".*127.0.0.1.*"), "@QHostAddress")
               + Check("ha1.a", "2130706433", TypeDef("unsigned int", "@quint32"))
               + Check("ha1.ipString", "\"127.0.0.1\"", "@QString")
                    % QtVersion(0, 0x50800)
               //+ Check("ha1.protocol", "@QAbstractSocket::IPv4Protocol (0)",
               //        "@QAbstractSocket::NetworkLayerProtocol") % GdbEngine
               //+ Check("ha1.protocol", "IPv4Protocol",
               //        "@QAbstractSocket::NetworkLayerProtocol") % LldbEngine
               + Check("ha1.scopeId", "\"\"", "@QString")
               + Check("var", "", "@QVariant (@QHostAddress)") % NoCdbEngine
               + Check("var.data", ValuePattern(".*127.0.0.1.*"), "@QHostAddress") % NoCdbEngine;


    QTest::newRow("QVariantList")
            << Data("#include <QVariantList>\n",

                    "QVariantList vl0;\n"
                    "unused(&vl0);\n\n"

                    "QVariantList vl1;\n"
                    "vl1.append(QVariant(1));\n"
                    "vl1.append(QVariant(2));\n"
                    "vl1.append(QVariant(\"Some String\"));\n"
                    "vl1.append(QVariant(21));\n"
                    "vl1.append(QVariant(22));\n"
                    "vl1.append(QVariant(\"2Some String\"));\n"
                    "unused(&vl1);\n\n"

                    "QVariantList vl2;\n"
                    "vl2.append(\"one\");\n"
                    "QVariant v = vl2;\n\n"
                    "unused(&vl2, &v);\n\n")

               + CoreProfile()

               + Check("vl0", "<0 items>", TypeDef("@QList<@QVariant>", "@QVariantList"))

               + Check("vl1", "<6 items>", TypeDef("@QList<@QVariant>", "@QVariantList"))
               + CheckType("vl1.0", "[0]", "@QVariant (int)")
               + CheckType("vl1.2", "[2]", "@QVariant (QString)")

               + Check("v", "<1 items>", "@QVariant (QVariantList)")
               + Check("v.0", "[0]", "\"one\"", "@QVariant (QString)");


    QTest::newRow("QVariantMap")
            << Data("#include <QVariantMap>\n",

                    "QVariantMap vm0;\n\n"
                    "unused(&vm0);\n\n"

                    "QVariantMap vm1;\n"
                    "vm1[\"a\"] = QVariant(1);\n"
                    "vm1[\"b\"] = QVariant(2);\n"
                    "vm1[\"c\"] = QVariant(\"Some String\");\n"
                    "vm1[\"d\"] = QVariant(21);\n"
                    "vm1[\"e\"] = QVariant(22);\n"
                    "vm1[\"f\"] = QVariant(\"2Some String\");\n"
                    "unused(&vm1);\n\n"

                    "QVariant v = vm1;\n"
                    "unused(&v);\n")

               + CoreProfile()

               + Check("vm0", "<0 items>", TypeDef("@QMap<@QString,@QVariant>", "@QVariantMap"))

               + Check("vm1", "<6 items>", TypeDef("@QMap<@QString,@QVariant>", "@QVariantMap"))
               + Check("vm1.0.key", "\"a\"", "@QString")
               + Check("vm1.0.value", "1", "@QVariant (int)")
               + Check("vm1.5.key", "\"f\"", "@QString")
               + Check("vm1.5.value", "\"2Some String\"", "@QVariant (QString)")

               + Check("v", "<6 items>", "@QVariant (QVariantMap)")
               + Check("v.0.key", "\"a\"", "@QString");


    QTest::newRow("QVariantHash")
            << Data("#include <QVariant>\n",

                    "QVariantHash h0;\n"
                    "unused(&h0);\n\n"

                    "QVariantHash h1;\n"
                    "h1[\"one\"] = \"vone\";\n"
                    "unused(&h1);\n\n"

                    "QVariant v = h1;\n"
                    "unused( &v);\n")

               + CoreProfile()

               + Check("h0", "<0 items>", TypeDef("@QHash<@QString,@QVariant>", "@QVariantHash"))

               + Check("h1", "<1 items>", TypeDef("@QHash<@QString,@QVariant>", "@QVariantHash"))
               + Check("h1.0.key", "\"one\"", "@QString")
               + Check("h1.0.value", "\"vone\"", "@QVariant (QString)")

               + Check("v", "<1 items>", "@QVariant (QVariantHash)")
               + Check("v.0.key", "\"one\"", "@QString");


    QTest::newRow("QVector")
            << Data("#include <QVector>\n" + fooData,

                    "QVector<int> v1(10000);\n"
                    "for (int i = 0; i != v1.size(); ++i)\n"
                    "     v1[i] = i * i;\n\n"
                    "unused(&v1);\n\n"

                    "QVector<Foo> v2;\n"
                    "v2.append(1);\n"
                    "v2.append(2);\n"
                    "unused(&v2);\n\n"

                    "typedef QVector<Foo> FooVector;\n"
                    "FooVector v3;\n"
                    "v3.append(1);\n"
                    "v3.append(2);\n"
                    "unused(&v3);\n\n"

                    "QVector<Foo *> v4;\n"
                    "v4.append(new Foo(1));\n"
                    "v4.append(0);\n"
                    "v4.append(new Foo(5));\n"
                    "unused(&v4);\n\n"

                    "QVector<bool> v5;\n"
                    "v5.append(true);\n"
                    "v5.append(false);\n"
                    "unused(&v5);\n\n"

                    "QVector<QList<int> > v6;\n"
                    "v6.append(QList<int>() << 1);\n"
                    "v6.append(QList<int>() << 2 << 3);\n"
                    "QVector<QList<int> > *pv = &v6;\n"
                    "unused(&v6, &pv);\n\n")

               + CoreProfile()

               + BigArrayProfile()

               + Check("v1", "<10000 items>", "@QVector<int>")
               + Check("v1.0", "[0]", "0", "int")
               + Check("v1.8999", "[8999]", "80982001", "int")

               + Check("v2", "<2 items>", "@QVector<Foo>")
               + Check("v2.0", "[0]", "", "Foo")
               + Check("v2.0.a", "1", "int")
               + Check("v2.1", "[1]", "", "Foo")
               + Check("v2.1.a", "2", "int")

               + Check("v3", "<2 items>", TypeDef("QVector<Foo>", "FooVector"))
               + Check("v3.0", "[0]", "", "Foo")
               + Check("v3.0.a", "1", "int")
               + Check("v3.1", "[1]", "", "Foo")
               + Check("v3.1.a", "2", "int")

               + Check("v4", "<3 items>", "@QVector<Foo*>")
               + CheckType("v4.0", "[0]", "Foo")
               + Check("v4.0.a", "1", "int")
               + Check("v4.1", "[1]", "0x0", "Foo *")
               + CheckType("v4.2", "[2]", "Foo")
               + Check("v4.2.a", "5", "int")

               + Check("v5", "<2 items>", "@QVector<bool>")
               + Check("v5.0", "[0]", "1", "bool")
               + Check("v5.1", "[1]", "0", "bool")

               + CheckType("pv", "@QVector<@QList<int>>")
               + Check("pv.0", "[0]", "<1 items>", "@QList<int>")
               + Check("pv.0.0", "[0]", "1", "int")
               + Check("pv.1", "[1]", "<2 items>", "@QList<int>")
               + Check("pv.1.0", "[0]", "2", "int")
               + Check("pv.1.1", "[1]", "3", "int")
               + Check("v6", "<2 items>", "@QVector<@QList<int>>")
               + Check("v6.0", "[0]", "<1 items>", "@QList<int>")
               + Check("v6.0.0", "[0]", "1", "int")
               + Check("v6.1", "[1]", "<2 items>", "@QList<int>")
               + Check("v6.1.0", "[0]", "2", "int")
               + Check("v6.1.1", "[1]", "3", "int");

    QTest::newRow("QVarLengthArray")
            << Data("#include <QVarLengthArray>\n" + fooData,

                    "QVarLengthArray<int> v1(10000);\n"
                    "for (int i = 0; i != v1.size(); ++i)\n"
                    "     v1[i] = i * i;\n\n"
                    "unused(&v1);\n\n"

                    "QVarLengthArray<Foo> v2;\n"
                    "v2.append(1);\n"
                    "v2.append(2);\n"
                    "unused(&v2);\n\n"

                    "typedef QVarLengthArray<Foo> FooVector;\n"
                    "FooVector v3;\n"
                    "v3.append(1);\n"
                    "v3.append(2);\n"
                    "unused(&v3);\n\n"

                    "QVarLengthArray<Foo *> v4;\n"
                    "v4.append(new Foo(1));\n"
                    "v4.append(0);\n"
                    "v4.append(new Foo(5));\n"
                    "unused(&v4);\n\n"

                    "QVarLengthArray<bool> v5;\n"
                    "v5.append(true);\n"
                    "v5.append(false);\n"
                    "unused(&v5);\n\n"

                    "QVarLengthArray<QList<int> > v6;\n"
                    "v6.append(QList<int>() << 1);\n"
                    "v6.append(QList<int>() << 2 << 3);\n"
                    "QVarLengthArray<QList<int> > *pv = &v6;\n"
                    "unused(&v6, &pv);\n\n")

               + CoreProfile()

               + BigArrayProfile()

               + Check("v1", "<10000 items>", "@QVarLengthArray<int, 256>")
               + Check("v1.0", "[0]", "0", "int")
               + Check("v1.8999", "[8999]", "80982001", "int")

               + Check("v2", "<2 items>", "@QVarLengthArray<Foo, 256>")
               + Check("v2.0", "[0]", "", "Foo")
               + Check("v2.0.a", "1", "int")
               + Check("v2.1", "[1]", "", "Foo")
               + Check("v2.1.a", "2", "int")

               + Check("v3", "<2 items>", TypeDef("@QVarLengthArray<Foo,256>", "FooVector"))
               + Check("v3.0", "[0]", "", "Foo")
               + Check("v3.0.a", "1", "int")
               + Check("v3.1", "[1]", "", "Foo")
               + Check("v3.1.a", "2", "int")

               + Check("v4", "<3 items>", "@QVarLengthArray<Foo*, 256>")
               + CheckType("v4.0", "[0]", "Foo")
               + Check("v4.0.a", "1", "int")
               + Check("v4.1", "[1]", "0x0", "Foo *")
               + CheckType("v4.2", "[2]", "Foo")
               + Check("v4.2.a", "5", "int")

               + Check("v5", "<2 items>", "@QVarLengthArray<bool, 256>")
               + Check("v5.0", "[0]", "1", "bool")
               + Check("v5.1", "[1]", "0", "bool")

               + CheckType("pv", "@QVarLengthArray<@QList<int>, 256>")
               + Check("pv.0", "[0]", "<1 items>", "@QList<int>")
               + Check("pv.0.0", "[0]", "1", "int")
               + Check("pv.1", "[1]", "<2 items>", "@QList<int>")
               + Check("pv.1.0", "[0]", "2", "int")
               + Check("pv.1.1", "[1]", "3", "int")
               + Check("v6", "<2 items>", "@QVarLengthArray<@QList<int>, 256>")
               + Check("v6.0", "[0]", "<1 items>", "@QList<int>")
               + Check("v6.0.0", "[0]", "1", "int")
               + Check("v6.1", "[1]", "<2 items>", "@QList<int>")
               + Check("v6.1.0", "[0]", "2", "int")
               + Check("v6.1.1", "[1]", "3", "int");


    QTest::newRow("QXmlAttributes")
            << Data("#include <QXmlAttributes>\n",
                    "QXmlAttributes atts;\n"
                    "atts.append(\"name1\", \"uri1\", \"localPart1\", \"value1\");\n"
                    "atts.append(\"name2\", \"uri2\", \"localPart2\", \"value2\");\n"
                    "atts.append(\"name3\", \"uri3\", \"localPart3\", \"value3\");\n")

               + CoreProfile()
               + Profile("QT += xml\n")

               + Check("atts", "<3 items>", "@QXmlAttributes")
               + Check("atts.0", "[0]", "", "@QXmlAttributes::Attribute")
               + Check("atts.0.localname", "\"localPart1\"", "@QString")
               + Check("atts.0.qname", "\"name1\"", "@QString")
               + Check("atts.0.uri", "\"uri1\"", "@QString")
               + Check("atts.0.value", "\"value1\"", "@QString")
               + Check("atts.1", "[1]", "", "@QXmlAttributes::Attribute")
               + Check("atts.1.localname", "\"localPart2\"", "@QString")
               + Check("atts.1.qname", "\"name2\"", "@QString")
               + Check("atts.1.uri", "\"uri2\"", "@QString")
               + Check("atts.1.value", "\"value2\"", "@QString")
               + Check("atts.2", "[2]", "", "@QXmlAttributes::Attribute")
               + Check("atts.2.localname", "\"localPart3\"", "@QString")
               + Check("atts.2.qname", "\"name3\"", "@QString")
               + Check("atts.2.uri", "\"uri3\"", "@QString")
               + Check("atts.2.value", "\"value3\"", "@QString");


    QTest::newRow("StdArray")
            << Data("#include <array>\n"
                    "#include <QString>\n",
                    "std::array<int, 4> a = { { 1, 2, 3, 4} };\n"
                    "std::array<QString, 4> b = { { \"1\", \"2\", \"3\", \"4\"} };\n"
                    "unused(&a, &b);\n")

               + CoreProfile()
               + Cxx11Profile()
               + MacLibCppProfile()

               + Check("a", "<4 items>", TypePattern("std::array<int, 4.*>"))
               + Check("a.0", "[0]", "1", "int")
               + Check("b", "<4 items>", TypePattern("std::array<@QString, 4.*>"))
               + Check("b.0", "[0]", "\"1\"", "@QString");


    QTest::newRow("StdComplex")
            << Data("#include <complex>\n",
                    "std::complex<double> c(1, 2);\n"
                    "unused(&c);\n")
               + Check("c", "(1.0, 2.0)", "std::complex<double>")
               + Check("c.real", FloatValue("1.0"), "double")
               + Check("c.imag", FloatValue("2.0"), "double");


    QTest::newRow("CComplex")
            << Data("#include <complex.h>\n",
                    "// Doesn't work when compiled as C++.\n"
                    "double complex a = 0;\n"
                    "double _Complex b = 0;\n"
                    "unused(&a, &b);\n")

               + ForceC()
               + GdbVersion(70500)
               + NoCdbEngine

               + Check("a", "0 + 0 * I", "complex double") % GdbEngine
               + Check("b", "0 + 0 * I", "complex double") % GdbEngine
               + Check("a", "0 + 0i", "_Complex double") % LldbEngine
               + Check("b", "0 + 0i", "_Complex double") % LldbEngine;


    QTest::newRow("StdDeque")
            << Data("#include <deque>\n",

                    "std::deque<int> deque0;\n\n"

                    "std::deque<int> deque1;\n"
                    "deque1.push_back(1);\n"
                    "deque1.push_back(2);\n"

                    "std::deque<int *> deque2;\n"
                    "deque2.push_back(new int(1));\n"
                    "deque2.push_back(0);\n"
                    "deque2.push_back(new int(2));\n"
                    "deque2.push_back(new int(3));\n"
                    "deque2.pop_back();\n"
                    "deque2.pop_front();\n")

               + Check("deque0", "<0 items>", "std::deque<int>")

               + Check("deque1", "<2 items>", "std::deque<int>")
               + Check("deque1.0", "[0]", "1", "int")
               + Check("deque1.1", "[1]", "2", "int")

               + Check("deque2", "<2 items>", "std::deque<int *>")
               + Check("deque2.0", "[0]", "0x0", "int *")
               + Check("deque2.1", "[1]", "2", "int");


    QTest::newRow("StdDequeQt")
            << Data("#include <deque>\n" + fooData,

                    "std::deque<Foo> deque0;\n"
                    "unused(&deque0);\n\n"

                    "std::deque<Foo> deque1;\n"
                    "deque1.push_back(1);\n"
                    "deque1.push_front(2);\n"

                    "std::deque<Foo *> deque2;\n"
                    "deque2.push_back(new Foo(1));\n"
                    "deque2.push_back(new Foo(2));\n")

               + CoreProfile()

               + Check("deque0", "<0 items>", "std::deque<Foo>")

               + Check("deque1", "<2 items>", "std::deque<Foo>")
               + Check("deque1.0", "[0]", "", "Foo")
               + Check("deque1.0.a", "2", "int")
               + Check("deque1.1", "[1]", "", "Foo")
               + Check("deque1.1.a", "1", "int")

               + Check("deque2", "<2 items>", "std::deque<Foo*>")
               + Check("deque2.0", "[0]", "", "Foo")
               + Check("deque2.0.a", "1", "int")
               + Check("deque2.1", "[1]", "", "Foo")
               + Check("deque2.1.a", "2", "int");


    QTest::newRow("StdHashSet")
            << Data("#include <hash_set>\n"
                    "namespace  __gnu_cxx {\n"
                    "template<> struct hash<std::string> {\n"
                    "  size_t operator()(const std::string &x) const { return __stl_hash_string(x.c_str()); }\n"
                    "};\n"
                    "}\n\n"
                    "using namespace __gnu_cxx;\n\n",
                    "hash_set<int> h;\n"
                    "h.insert(1);\n"
                    "h.insert(194);\n"
                    "h.insert(2);\n"
                    "h.insert(3);\n\n"
                    "hash_set<std::string> h2;\n"
                    "h2.insert(\"1\");\n"
                    "h2.insert(\"194\");\n"
                    "h2.insert(\"2\");\n"
                    "h2.insert(\"3\");\n")

               + GdbEngine

               + Profile("QMAKE_CXXFLAGS += -Wno-deprecated")
               + Check("h", "<4 items>", "__gnu__cxx::hash_set<int>")
               + Check("h.0", "[0]", "194", "int")
               + Check("h.1", "[1]", "1", "int")
               + Check("h.2", "[2]", "2", "int")
               + Check("h.3", "[3]", "3", "int")
               + Check("h2", "<4 items>", "__gnu__cxx::hash_set<std::string>")
               + Check("h2.0", "[0]", "\"194\"", "std::string")
               + Check("h2.1", "[1]", "\"1\"", "std::string")
               + Check("h2.2", "[2]", "\"2\"", "std::string")
               + Check("h2.3", "[3]", "\"3\"", "std::string");


    QTest::newRow("StdList")
            << Data("#include <list>\n"

                    "struct Base { virtual ~Base() {} };\n"
                    "template<class T>\n"
                    "struct Derived : public std::list<T>, Base {};\n",

                    "std::list<int> l0;\n"
                    "unused(&l0);\n\n"

                    "std::list<int> l1;\n"
                    "for (int i = 0; i < 10000; ++i)\n"
                    "    l1.push_back(i);\n"

                    "std::list<bool> l2;\n"
                    "l2.push_back(true);\n"
                    "l2.push_back(false);\n"
                    "unused(&l2);\n\n"

                    "std::list<int *> l3;\n"
                    "l3.push_back(new int(1));\n"
                    "l3.push_back(0);\n"
                    "l3.push_back(new int(2));\n"
                    "unused(&l3);\n\n"

                    "Derived<int> l4;\n"
                    "l4.push_back(1);\n"
                    "l4.push_back(2);\n"
                    "unused(&l4);\n\n")

               + BigArrayProfile()

               + Check("l0", "<0 items>", "std::list<int>")

               //+ Check("l1", "<at least 1000 items>", "std::list<int>")
               + Check("l1", ValuePattern("<.*1000.* items>"), "std::list<int>") // Matches both above.
               + Check("l1.0", "[0]", "0", "int")
               + Check("l1.1", "[1]", "1", "int")
               + Check("l1.999", "[999]", "999", "int")

               + Check("l2", "<2 items>", "std::list<bool>")
               + Check("l2.0", "[0]", "1", "bool")
               + Check("l2.1", "[1]", "0", "bool")

               + Check("l3", "<3 items>", "std::list<int*>")
               + Check("l3.0", "[0]", "1", "int")
               + Check("l3.1", "[1]", "0x0", "int *")
               + Check("l3.2", "[2]", "2", "int")

               + Check("l4.@1.0", "[0]", "1", "int")
               + Check("l4.@1.1", "[1]", "2", "int");


    QTest::newRow("StdListQt")
            << Data("#include <list>\n" + fooData,

                    "std::list<Foo> l1;\n"
                    "l1.push_back(15);\n"
                    "l1.push_back(16);\n"
                    "unused(&l1);\n\n"

                    "std::list<Foo *> l2;\n"
                    "l2.push_back(new Foo(1));\n"
                    "l2.push_back(0);\n"
                    "l2.push_back(new Foo(2));\n"
                    "unused(&l2);\n")

               + CoreProfile()
               + Check("l1", "<2 items>", "std::list<Foo>")
               + Check("l1.0", "[0]", "", "Foo")
               + Check("l1.0.a", "15", "int")
               + Check("l1.1", "[1]", "", "Foo")
               + Check("l1.1.a", "16", "int")

               + Check("l2", "<3 items>", "std::list<Foo*>")
               + Check("l2.0", "[0]", "", "Foo")
               + Check("l2.0.a", "1", "int")
               + Check("l2.1", "[1]", "0x0", "Foo *")
               + Check("l2.2", "[2]", "", "Foo")
               + Check("l2.2.a", "2", "int");


    QTest::newRow("StdMap")
            << Data("#include <map>\n"
                    "#include <string>\n",

                    "std::map<unsigned int, unsigned int> map1;\n"
                    "map1[11] = 1;\n"
                    "map1[22] = 2;\n"
                    "unused(&map1);\n\n"

                    "std::map<unsigned int, float> map2;\n"
                    "map2[11] = 11.0;\n"
                    "map2[22] = 22.0;\n"

                    "typedef std::map<int, float> Map;\n"
                    "Map map3;\n"
                    "map3[11] = 11.0;\n"
                    "map3[22] = 22.0;\n"
                    "map3[33] = 33.0;\n"
                    "map3[44] = 44.0;\n"
                    "map3[55] = 55.0;\n"
                    "map3[66] = 66.0;\n"
                    "Map::iterator it1 = map3.begin();\n"
                    "Map::iterator it2 = it1; ++it2;\n"
                    "Map::iterator it3 = it2; ++it3;\n"
                    "Map::iterator it4 = it3; ++it4;\n"
                    "Map::iterator it5 = it4; ++it5;\n"
                    "Map::iterator it6 = it5; ++it6;\n"
                    "unused(&it6);\n"

                    "std::multimap<unsigned int, float> map4;\n"
                    "map4.insert(std::pair<unsigned int, float>(11, 11.0));\n"
                    "map4.insert(std::pair<unsigned int, float>(22, 22.0));\n"
                    "map4.insert(std::pair<unsigned int, float>(22, 23.0));\n"
                    "map4.insert(std::pair<unsigned int, float>(22, 24.0));\n"
                    "map4.insert(std::pair<unsigned int, float>(22, 25.0));\n"

                    "std::map<short, long long> map5;\n"
                    "map5[12] = 42;\n"

                    "std::map<short, std::string> map6;\n"
                    "map6[12] = \"42\";\n")

               + Check("map1", "<2 items>", "std::map<unsigned int, unsigned int>")
               + Check("map1.0", "[0] 11", "1", "")
               + Check("map1.1", "[1] 22", "2", "")

               + Check("map2", "<2 items>", "std::map<unsigned int, float>")
               + Check("map2.0", "[0] 11", FloatValue("11"), "")
               + Check("map2.1", "[1] 22", FloatValue("22"), "")

               + Check("map3", "<6 items>", TypeDef("std::map<int, float>", "Map"))
               + Check("map3.0", "[0] 11", FloatValue("11"), "")
               + Check("it1.first", "11", "int") % NoCdbEngine
               + Check("it1.second", FloatValue("11"), "float") % NoCdbEngine
               + Check("it6.first", "66", "int") % NoCdbEngine
               + Check("it6.second", FloatValue("66"), "float") % NoCdbEngine
               + Check("it1.0", "11", FloatValue("11"), "") % CdbEngine
               + Check("it6.0", "66", FloatValue("66"), "") % CdbEngine

               + Check("map4", "<5 items>", "std::multimap<unsigned int, float>")
               + Check("map4.0", "[0] 11", FloatValue("11"), "")
               + Check("map4.4", "[4] 22", FloatValue("25"), "")

               + Check("map5", "<1 items>", TypeDef("std::map<short, __int64>",
                                                    "std::map<short, long long>"))
               + Check("map5.0", "[0] 12", "42", "")

               + Check("map6", "<1 items>", "std::map<short, std::string>")
               + Check("map6.0", "[0] 12", "\"42\"", "");


    QTest::newRow("StdMapQt")
            << Data("#include <map>\n"
                    "#include <QPointer>\n"
                    "#include <QObject>\n"
                    "#include <QStringList>\n"
                    "#include <QString>\n" + fooData,

                    "std::map<QString, Foo> map1;\n"
                    "map1[\"22.0\"] = Foo(22);\n"
                    "map1[\"33.0\"] = Foo(33);\n"
                    "map1[\"44.0\"] = Foo(44);\n"
                    "unused(&map1);\n\n"

                    "std::map<const char *, Foo> map2;\n"
                    "map2[\"22.0\"] = Foo(22);\n"
                    "map2[\"33.0\"] = Foo(33);\n"
                    "unused(&map2);\n\n"

                    "std::map<uint, QStringList> map3;\n"
                    "map3[11] = QStringList() << \"11\";\n"
                    "map3[22] = QStringList() << \"22\";\n"
                    "unused(&map3);\n\n"

                    "typedef std::map<uint, QStringList> T;\n"
                    "T map4;\n"
                    "map4[11] = QStringList() << \"11\";\n"
                    "map4[22] = QStringList() << \"22\";\n"
                    "unused(&map4);\n\n"

                    "std::map<QString, float> map5;\n"
                    "map5[\"11.0\"] = 11.0;\n"
                    "map5[\"22.0\"] = 22.0;\n"
                    "unused(&map5);\n\n"

                    "std::map<int, QString> map6;\n"
                    "map6[11] = \"11.0\";\n"
                    "map6[22] = \"22.0\";\n"
                    "unused(&map6);\n\n"

                    "QObject ob;\n"
                    "std::map<QString, QPointer<QObject> > map7;\n"
                    "map7[\"Hallo\"] = QPointer<QObject>(&ob);\n"
                    "map7[\"Welt\"] = QPointer<QObject>(&ob);\n"
                    "map7[\".\"] = QPointer<QObject>(&ob);\n"
                    "unused(&map7);\n")

               + CoreProfile()

               + Check("map1", "<3 items>", "std::map<@QString, Foo>")
               + Check("map1.0", "[0] \"22.0\"", "", "")
               + Check("map1.0.first", "\"22.0\"", "@QString")
               + Check("map1.0.second", "", "Foo")
               + Check("map1.0.second.a", "22", "int")
               + Check("map1.1", "[1] \"33.0\"", "", "")
               + Check("map1.2.first", "\"44.0\"", "@QString")
               + Check("map1.2.second", "", "Foo")
               + Check("map1.2.second.a", "44", "int")

               + Check("map2", "<2 items>", "std::map<char const*, Foo>")
               + Check("map2.0", "[0] \"22.0\"", "", "")
               + Check("map2.0.first", "\"22.0\"", "char *")
               + Check("map2.0.first.0", "[0]", "50", "char")
               + Check("map2.0.second", "", "Foo")
               + Check("map2.0.second.a", "22", "int")
               + Check("map2.1", "[1] \"33.0\"", "", "")
               + Check("map2.1.first", "\"33.0\"", "char *")
               + Check("map2.1.first.0", "[0]", "51", "char")
               + Check("map2.1.second", "", "Foo")
               + Check("map2.1.second.a", "33", "int")

               + Check("map3", "<2 items>", "std::map<unsigned int, @QStringList>")
               + Check("map3.0", "[0] 11", "<1 items>", "")
               + Check("map3.0.first", "11", "unsigned int")
               + Check("map3.0.second", "<1 items>", "@QStringList")
               + Check("map3.0.second.0", "[0]", "\"11\"", "@QString")
               + Check("map3.1", "[1] 22", "<1 items>", "")
               + Check("map3.1.first", "22", "unsigned int")
               + Check("map3.1.second", "<1 items>", "@QStringList")
               + Check("map3.1.second.0", "[0]", "\"22\"", "@QString")

               + Check("map4.1.second.0", "[0]", "\"22\"", "@QString")

               + Check("map5", "<2 items>", "std::map<@QString, float>")
               + Check("map5.0", "[0] \"11.0\"", FloatValue("11"), "")
               + Check("map5.0.first", "\"11.0\"", "@QString")
               + Check("map5.0.second", FloatValue("11"), "float")
               + Check("map5.1", "[1] \"22.0\"", FloatValue("22"), "")
               + Check("map5.1.first", "\"22.0\"", "@QString")
               + Check("map5.1.second", FloatValue("22"), "float")

               + Check("map6", "<2 items>", "std::map<int, @QString>")
               + Check("map6.0", "[0] 11", "\"11.0\"", "")
               + Check("map6.0.first", "11", "int")
               + Check("map6.0.second", "\"11.0\"", "@QString")
               + Check("map6.1", "[1] 22", "\"22.0\"", "")
               + Check("map6.1.first", "22", "int")
               + Check("map6.1.second", "\"22.0\"", "@QString")

               + Check("map7", "<3 items>", "std::map<@QString, @QPointer<@QObject>>")
               + Check("map7.0", "[0] \".\"", "", "")
               + Check("map7.0.first", "\".\"", "@QString")
               + Check("map7.0.second", "", "@QPointer<@QObject>")
               + Check("map7.2.first", "\"Welt\"", "@QString");


    QTest::newRow("StdUniquePtr")
            << Data("#include <memory>\n"
                    "#include <string>\n" + fooData,
                    "std::unique_ptr<int> p0; unused(&p0);\n\n"
                    "std::unique_ptr<int> p1(new int(32)); unused(&p1);\n\n"
                    "std::unique_ptr<Foo> p2(new Foo); unused(&p2);\n\n"
                    "std::unique_ptr<std::string> p3(new std::string(\"ABC\")); unused(&p3);\n\n")

               + CoreProfile()
               + Cxx11Profile()
               + MacLibCppProfile()

               + Check("p0", "(null)", "std::unique_ptr<int, std::default_delete<int> >")
               + Check("p1", "32", "std::unique_ptr<int, std::default_delete<int> >")
               + Check("p2", Pointer(), "std::unique_ptr<Foo, std::default_delete<Foo> >")
               + Check("p3", "\"ABC\"", "std::unique_ptr<std::string, std::default_delete<std::string> >");


    QTest::newRow("StdOnce")
            << Data("#include <mutex>\n",
                    "std::once_flag x; unused(&x);\n")
               + Cxx11Profile()
               + Check("x", "0", "std::once_flag");


    QTest::newRow("StdSharedPtr")
            << Data("#include <memory>\n"
                    "#include <string>\n" + fooData,
                    "std::shared_ptr<int> pi(new int(32)); unused(&pi);\n"
                    "std::shared_ptr<Foo> pf(new Foo); unused(&pf);\n"
                    "std::shared_ptr<std::string> ps(new std::string(\"ABC\")); "
                    "unused(&ps);\n"
                    "std::weak_ptr<int> wi = pi; unused(&wi);\n"
                    "std::weak_ptr<Foo> wf = pf; unused(&wf);\n"
                    "std::weak_ptr<std::string> ws = ps; unused(&ws);\n")

               + CoreProfile()
               + Cxx11Profile()
               + MacLibCppProfile()

               + Check("pi", "32", "std::shared_ptr<int>")
               + Check("pf", Pointer(), "std::shared_ptr<Foo>")
               + Check("ps", "\"ABC\"", "std::shared_ptr<std::string>")
               + Check("wi", "32", "std::weak_ptr<int>")
               + Check("wf", Pointer(), "std::weak_ptr<Foo>")
               + Check("ws", "\"ABC\"", "std::weak_ptr<std::string>")
               + Check("ps", "\"ABC\"", "std::shared_ptr<std::string>");

    QTest::newRow("StdSharedPtr2")
            << Data("#include <memory>\n"
                    "struct A {\n"
                    "    virtual ~A() {}\n"
                    "    int *m_0 = (int *)0;\n"
                    "    int *m_1 = (int *)1;\n"
                    "    int *m_2 = (int *)2;\n"
                    "    int x = 3;\n"
                    "};\n",
                    "std::shared_ptr<A> a(new A);\n"
                    "A *inner = a.get(); unused(inner);\n")
               + Cxx11Profile()
               + Check("inner.m_0", "0x0", "int *")
               + Check("inner.m_1", "0x1", "int *")
               + Check("inner.m_2", "0x2", "int *")
               + Check("inner.x", "3", "int")
               + Check("a.m_0", "0x0", "int *")
               + Check("a.m_1", "0x1", "int *")
               + Check("a.m_2", "0x2", "int *")
               + Check("a.x", "3", "int");

    QTest::newRow("StdSet")
            << Data("#include <set>\n",

                    "std::set<double> s0;\n"
                    "unused(&s0);\n\n"

                    "std::set<int> s1;\n"
                    "s1.insert(11);\n"
                    "s1.insert(22);\n"
                    "s1.insert(33);\n"
                    "unused(&s1);\n\n"

                    "typedef std::set<int> Set;\n"
                    "Set s2;\n"
                    "s2.insert(11.0);\n"
                    "s2.insert(22.0);\n"
                    "s2.insert(33.0);\n"
                    "Set::iterator it1 = s2.begin();\n"
                    "Set::iterator it2 = it1; ++it2;\n"
                    "Set::iterator it3 = it2; ++it3;\n"
                    "unused(&it3);\n\n"

                    "std::multiset<int> s3;\n"
                    "s3.insert(1);\n"
                    "s3.insert(1);\n"
                    "s3.insert(2);\n"
                    "s3.insert(3);\n"
                    "s3.insert(3);\n"
                    "s3.insert(3);\n")

               + Check("s0", "<0 items>", "std::set<double>")

               + Check("s1", "<3 items>", "std::set<int>")

               + Check("s2", "<3 items>", TypeDef("std::set<int>", "Set"))
               + Check("it1.value", "11", "int")
               + Check("it3.value", "33", "int")

               + Check("s3", "<6 items>", "std::multiset<int>")
               + Check("s3.0", "[0]", "1", "int")
               + Check("s3.5", "[5]", "3", "int");


    QTest::newRow("StdSetQt")
            << Data("#include <set>\n"
                    "#include <QPointer>\n"
                    "#include <QObject>\n"
                    "#include <QString>\n",

                    "std::set<QString> set1;\n"
                    "set1.insert(\"22.0\");\n"

                    "QObject ob;\n"
                    "std::set<QPointer<QObject> > set2;\n"
                    "QPointer<QObject> ptr(&ob);\n"
                    "set2.insert(ptr);\n"

                    "unused(&ptr, &ob, &set1, &set2);\n")

               + CoreProfile()
               + Check("set1", "<1 items>", "std::set<@QString>")
               + Check("set1.0", "[0]", "\"22.0\"", "@QString")

               + Check("set2", "<1 items>", "std::set<@QPointer<@QObject>, "
                    "std::less<@QPointer<@QObject>>, std::allocator<@QPointer<@QObject>>>")
               + Check("ob", "", "@QObject")
               + Check("ptr", "", "@QPointer<@QObject>");


    QTest::newRow("StdStack")
            << Data("#include <stack>\n",
                    "std::stack<int *> s0, s1;\n"
                    "s1.push(new int(1));\n"
                    "s1.push(0);\n"
                    "s1.push(new int(2));\n"

                    "std::stack<int> s2, s3;\n"
                    "s3.push(1);\n"
                    "s3.push(2);\n"
                    "unused(&s0, &s1, &s2, &s3);\n")

               + Check("s0", "<0 items>", "std::stack<int*>")

               + Check("s1", "<3 items>", "std::stack<int*>")
               + Check("s1.0", "[0]", "1", "int")
               + Check("s1.1", "[1]", "0x0", "int *")
               + Check("s1.2", "[2]", "2", "int")

               + Check("s2", "<0 items>", "std::stack<int>")

               + Check("s3", "<2 items>", "std::stack<int>")
               + Check("s3.0", "[0]", "1", "int")
               + Check("s3.1", "[1]", "2", "int");


    QTest::newRow("StdStackQt")
            << Data("#include <stack>\n" + fooData,
                    "std::stack<Foo *> s0, s1;\n"
                    "std::stack<Foo> s2, s3;\n"
                    "s1.push(new Foo(1));\n"
                    "s1.push(new Foo(2));\n"
                    "s3.push(1);\n"
                    "s3.push(2);\n"
                    "unused(&s0, &s1, &s2, &s3);\n")
               + CoreProfile()
               + Check("s0", "<0 items>", "std::stack<Foo*>")
               + Check("s1", "<2 items>", "std::stack<Foo*>")
               + Check("s1.0", "[0]", "", "Foo")
               + Check("s1.0.a", "1", "int")
               + Check("s1.1", "[1]", "", "Foo")
               + Check("s1.1.a", "2", "int")
               + Check("s2", "<0 items>", "std::stack<Foo>")
               + Check("s3", "<2 items>", "std::stack<Foo>")
               + Check("s3.0", "[0]", "", "Foo")
               + Check("s3.0.a", "1", "int")
               + Check("s3.1", "[1]", "", "Foo")
               + Check("s3.1.a", "2", "int");


    QTest::newRow("StdString")
            << Data("#include <string>\n",
                    "std::string str0, str;\n"
                    "std::wstring wstr0, wstr;\n"
                    "str += \"b\";\n"
                    "wstr += wchar_t('e');\n"
                    "str += \"d\";\n"
                    "wstr += wchar_t('e');\n"
                    "str += \"e\";\n"
                    "str += \"b\";\n"
                    "str += \"d\";\n"
                    "str += \"e\";\n"
                    "wstr += wchar_t('e');\n"
                    "wstr += wchar_t('e');\n"
                    "str += \"e\";\n"
                    "unused(&str0, &str, &wstr0, &wstr);\n")
               + Check("str0", "\"\"", "std::string")
               + Check("wstr0", "\"\"", "std::wstring")
               + Check("str", "\"bdebdee\"", "std::string")
               + Check("wstr", "\"eeee\"", "std::wstring");


    QTest::newRow("StdStringQt")
            << Data("#include <string>\n"
                    "#include <vector>\n"
                    "#include <QList>\n",
                    "std::string str = \"foo\";\n"
                    "std::vector<std::string> v;\n"
                    "QList<std::string> l0, l;\n"
                    "v.push_back(str);\n"
                    "v.push_back(str);\n"
                    "l.push_back(str);\n"
                    "l.push_back(str);\n"
                    "unused(&v, &l);\n")
               + CoreProfile()
               + Check("l0", "<0 items>", "@QList<std::string>")
               + Check("l", "<2 items>", "@QList<std::string>")
               + Check("str", "\"foo\"", "std::string")
               + Check("v", "<2 items>", "std::vector<std::string>")
               + Check("v.0", "[0]", "\"foo\"", "std::string");


    QTest::newRow("StdValArray")
            << Data("#include <valarray>\n"
                    "#include <list>\n",

                    "std::valarray<double> v0, v1 = { 1, 0, 2 };\n"
                    "unused(&v0, &v1);\n\n"

                    "std::valarray<int *> v2, v3 = { new int(1), 0, new int(2) };\n"
                    "unused(&v2, &v3);\n\n"

                    "std::valarray<int> v4 = { 1, 2, 3, 4 };\n"
                    "unused(&v4);\n\n"

                    "std::list<int> list;\n"
                    "std::list<int> list1 = { 45 };\n"
                    "std::valarray<std::list<int> *> v5 = {\n"
                    "   new std::list<int>(list), 0,\n"
                    "   new std::list<int>(list1), 0\n"
                    "};\n"
                    "unused(&v5);\n\n"

                    "std::valarray<bool> b0;\n"
                    "unused(&b0);\n\n"

                    "std::valarray<bool> b1 = { true, false, false, true, false };\n"
                    "unused(&b1);\n\n"

                    "std::valarray<bool> b2(true, 65);\n"
                    "unused(&b2);\n\n"

                    "std::valarray<bool> b3(300);\n"
                    "unused(&b3);\n")

               + Cxx11Profile()

               + Check("v0", "<0 items>", "std::valarray<double>")
               + Check("v1", "<3 items>", "std::valarray<double>")
               + Check("v1.0", "[0]", FloatValue("1"), "double")
               + Check("v1.1", "[1]", FloatValue("0"), "double")
               + Check("v1.2", "[2]", FloatValue("2"), "double")

               + Check("v2", "<0 items>", "std::valarray<int*>")
               + Check("v3", "<3 items>", "std::valarray<int*>")
               + Check("v3.0", "[0]", "1", "int")
               + Check("v3.1", "[1]", "0x0", "int *")
               + Check("v3.2", "[2]", "2", "int")

               + Check("v4", "<4 items>", "std::valarray<int>")
               + Check("v4.0", "[0]", "1", "int")
               + Check("v4.3", "[3]", "4", "int")

               + Check("list1", "<1 items>", "std::list<int>")
               + Check("list1.0", "[0]", "45", "int")
               + Check("v5", "<4 items>", "std::valarray<std::list<int>*>")
               + Check("v5.0", "[0]", "<0 items>", "std::list<int>")
               + Check("v5.2", "[2]", "<1 items>", "std::list<int>")
               + Check("v5.2.0", "[0]", "45", "int")
               + Check("v5.3", "[3]", "0x0", "std::list<int> *")

               + Check("b0", "<0 items>", "std::valarray<bool>")
               + Check("b1", "<5 items>", "std::valarray<bool>")

               + Check("b1.0", "[0]", "1", "bool")
               + Check("b1.1", "[1]", "0", "bool")
               + Check("b1.2", "[2]", "0", "bool")
               + Check("b1.3", "[3]", "1", "bool")
               + Check("b1.4", "[4]", "0", "bool")

               + Check("b2", "<65 items>", "std::valarray<bool>")
               + Check("b2.0", "[0]", "1", "bool")
               + Check("b2.64", "[64]", "1", "bool")

               + Check("b3", "<300 items>", "std::valarray<bool>")
               + Check("b3.0", "[0]", "0", "bool")
               + Check("b3.299", "[299]", "0", "bool");


    QTest::newRow("StdVector")
            << Data("#include <vector>\n"
                    "#include <list>\n",

                    "std::vector<double> v0, v1;\n"
                    "v1.push_back(1);\n"
                    "v1.push_back(0);\n"
                    "v1.push_back(2);\n"
                    "unused(&v0, &v1);\n\n"

                    "std::vector<int *> v2; unused(&v2);\n"

                    "std::vector<int *> v3; unused(&v3);\n\n"
                    "v3.push_back(new int(1));\n"
                    "v3.push_back(0);\n"
                    "v3.push_back(new int(2));\n\n"

                    "std::vector<int> v4; unused(&v4);\n"
                    "v4.push_back(1);\n"
                    "v4.push_back(2);\n"
                    "v4.push_back(3);\n"
                    "v4.push_back(4);\n\n"

                    "std::vector<std::list<int> *> v5;\n"
                    "std::list<int> list;\n"
                    "v5.push_back(new std::list<int>(list));\n"
                    "v5.push_back(0);\n"
                    "list.push_back(45);\n"
                    "v5.push_back(new std::list<int>(list));\n"
                    "v5.push_back(0);\n"
                    "unused(&v5);\n\n"

                    "std::vector<bool> b0;\n"
                    "unused(&b0);\n\n"

                    "std::vector<bool> b1;\n"
                    "b1.push_back(true);\n"
                    "b1.push_back(false);\n"
                    "b1.push_back(false);\n"
                    "b1.push_back(true);\n"
                    "b1.push_back(false);\n"
                    "unused(&b1);\n\n"

                    "std::vector<bool> b2(65, true);\n"
                    "unused(&b2);\n\n"

                    "std::vector<bool> b3(300);\n"
                    "unused(&b3);\n")

               + Check("v0", "<0 items>", "std::vector<double>")
               + Check("v1", "<3 items>", "std::vector<double>")
               + Check("v1.0", "[0]", FloatValue("1"), "double")
               + Check("v1.1", "[1]", FloatValue("0"), "double")
               + Check("v1.2", "[2]", FloatValue("2"), "double")

               + Check("v2", "<0 items>", "std::vector<int*>")
               + Check("v3", "<3 items>", "std::vector<int*>")
               + Check("v3.0", "[0]", "1", "int")
               + Check("v3.1", "[1]", "0x0", "int *")
               + Check("v3.2", "[2]", "2", "int")

               + Check("v4", "<4 items>", "std::vector<int>")
               + Check("v4.0", "[0]", "1", "int")
               + Check("v4.3", "[3]", "4", "int")

               + Check("list", "<1 items>", "std::list<int>")
               + Check("list.0", "[0]", "45", "int")
               + Check("v5", "<4 items>", "std::vector<std::list<int>*>")
               + Check("v5.0", "[0]", "<0 items>", "std::list<int>")
               + Check("v5.2", "[2]", "<1 items>", "std::list<int>")
               + Check("v5.2.0", "[0]", "45", "int")
               + Check("v5.3", "[3]", "0x0", "std::list<int> *")

               + Check("b0", "<0 items>", "std::vector<bool>")
               + Check("b1", "<5 items>", "std::vector<bool>")
               + Check("b1.0", "[0]", "1", "bool")
               + Check("b1.1", "[1]", "0", "bool")
               + Check("b1.2", "[2]", "0", "bool")
               + Check("b1.3", "[3]", "1", "bool")
               + Check("b1.4", "[4]", "0", "bool")

               + Check("b2", "<65 items>", "std::vector<bool>")
               + Check("b2.0", "[0]", "1", "bool")
               + Check("b2.64", "[64]", "1", "bool")

               + Check("b3", "<300 items>", "std::vector<bool>")
               + Check("b3.0", "[0]", "0", "bool")
               + Check("b3.299", "[299]", "0", "bool");


    QTest::newRow("StdVectorQt")
            << Data("#include <vector>\n" + fooData,

                    "std::vector<Foo *> v1;\n"
                    "v1.push_back(new Foo(1));\n"
                    "v1.push_back(0);\n"
                    "v1.push_back(new Foo(2));\n"

                    "std::vector<Foo> v2;\n"
                    "v2.push_back(1);\n"
                    "v2.push_back(2);\n"
                    "v2.push_back(3);\n"
                    "v2.push_back(4);\n"
                    "unused(&v1, &v2);\n")

               + CoreProfile()
               + Check("v1", "<3 items>", "std::vector<Foo*>")
               + Check("v1.0", "[0]", "", "Foo")
               + Check("v1.0.a", "1", "int")
               + Check("v1.1", "[1]", "0x0", "Foo *")
               + Check("v1.2", "[2]", "", "Foo")
               + Check("v1.2.a", "2", "int")

               + Check("v2", "<4 items>", "std::vector<Foo>")
               + Check("v2.0", "[0]", "", "Foo")
               + Check("v2.1.a", "2", "int")
               + Check("v2.3", "[3]", "", "Foo");


    QTest::newRow("StdUnorderedMap")
            << Data("#include <unordered_map>\n"
                    "#include <string>\n",

                    "std::unordered_map<unsigned int, unsigned int> map1;\n"
                    "map1[11] = 1;\n"
                    "map1[22] = 2;\n"
                    "unused(&map1);\n\n"

                    "std::unordered_map<std::string, float> map2;\n"
                    "map2[\"11.0\"] = 11.0;\n"
                    "map2[\"22.0\"] = 22.0;\n"
                    "unused(&map2);\n"

                    "std::unordered_multimap<int, std::string> map3;\n"
                    "map3.insert({1, \"Foo\"});\n"
                    "map3.insert({1, \"Bar\"});\n"
                    "unused(&map3);\n" )

               + Cxx11Profile()

               + Check("map1", "<2 items>", "std::unordered_map<unsigned int, unsigned int>")
               + Check("map1.0", "[0] 22", "2", "") % NoCdbEngine
               + Check("map1.1", "[1] 11", "1", "") % NoCdbEngine
               + Check("map1.0", "11", "1", "std::pair<unsigned int const ,unsigned int>") % CdbEngine
               + Check("map1.1", "22", "2", "std::pair<unsigned int const ,unsigned int>") % CdbEngine

               + Check("map2", "<2 items>", "std::unordered_map<std::string, float>")
               + Check("map2.0", "[0] \"22.0\"", FloatValue("22.0"), "") % NoCdbEngine
               + Check("map2.0.first", "\"22.0\"", "std::string")        % NoCdbEngine
               + Check("map2.0.second", FloatValue("22"), "float")       % NoCdbEngine
               + Check("map2.1", "[1] \"11.0\"", FloatValue("11.0"), "") % NoCdbEngine
               + Check("map2.1.first", "\"11.0\"", "std::string")        % NoCdbEngine
               + Check("map2.1.second", FloatValue("11"), "float")       % NoCdbEngine
               + Check("map2.0", "\"11.0\"", FloatValue("11.0"),
                       "std::pair<std::string, float>")             % CdbEngine
               + Check("map2.0.first", "\"11.0\"", "std::string")   % CdbEngine
               + Check("map2.0.second", FloatValue("11"), "float")  % CdbEngine
               + Check("map2.1", "\"22.0\"", FloatValue("22.0"),
                       "std::pair<std::string, float>")             % CdbEngine
               + Check("map2.1.first", "\"22.0\"", "std::string")   % CdbEngine
               + Check("map2.1.second", FloatValue("22"), "float")  % CdbEngine

               + Check("map3", "<2 items>", "std::unordered_multimap<int, std::string>")
               + Check("map3.0", "[0] 1", "\"Bar\"", "") % NoCdbEngine
               + Check("map3.1", "[1] 1", "\"Foo\"", "") % NoCdbEngine
               + Check("map3.0", "1", "\"Foo\"", "std::pair<int const ,std::string>") % CdbEngine
               + Check("map3.1", "1", "\"Bar\"", "std::pair<int const ,std::string>") % CdbEngine;


    QTest::newRow("StdUnorderedSet")
            << Data("#include <unordered_set>\n",
                    "std::unordered_set<int> set1;\n"
                    "set1.insert(11);\n"
                    "set1.insert(22);\n"
                    "set1.insert(33);\n"
                    "unused(&set1);\n"

                    "std::unordered_multiset<int> set2;\n"
                    "set2.insert(42);\n"
                    "set2.insert(42);\n"
                    "unused(&set2);\n")

               + Cxx11Profile()

               + Check("set1", "<3 items>", "std::unordered_set<int>")
               + Check("set1.0", "[0]", "33", "int") % NoCdbEngine
               + Check("set1.0", "[0]", "11", "int") % CdbEngine
               + Check("set1.1", "[1]", "22", "int")
               + Check("set1.2", "[2]", "11", "int") % NoCdbEngine
               + Check("set1.2", "[2]", "33", "int") % CdbEngine

               + Check("set2", "<2 items>", "std::unordered_multiset<int>")
               + Check("set2.0", "[0]", "42", "int")
               + Check("set2.1", "[1]", "42", "int");


//    class Goo
//    {
//    public:
//       Goo(const QString &str, const int n) : str_(str), n_(n) {}
//    private:
//       QString str_;
//       int n_;
//    };

//    typedef QList<Goo> GooList;

//    QTest::newRow("NoArgumentName(int i, int, int k)
//
//        // This is not supposed to work with the compiled dumpers");
//        "GooList list;
//        "list.append(Goo("Hello", 1));
//        "list.append(Goo("World", 2));

//        "QList<Goo> l2;
//        "l2.append(Goo("Hello", 1));
//        "l2.append(Goo("World", 2));

//         + Check("i", "1", "int");
//         + Check("k", "3", "int");
//         + Check("list <2 items> noargs::GooList");
//         + Check("list.0", "noargs::Goo");
//         + Check("list.0.n_", "1", "int");
//         + Check("list.0.str_ "Hello" QString");
//         + Check("list.1", "noargs::Goo");
//         + Check("list.1.n_", "2", "int");
//         + Check("list.1.str_ "World" QString");
//         + Check("l2 <2 items> QList<noargs::Goo>");
//         + Check("l2.0", "noargs::Goo");
//         + Check("l2.0.n_", "1", "int");
//         + Check("l2.0.str_ "Hello" QString");
//         + Check("l2.1", "noargs::Goo");
//         + Check("l2.1.n_", "2", "int");
//         + Check("l2.1.str_ "World" QString");


//"void foo() {}\n"
//"void foo(int) {}\n"
//"void foo(QList<int>) {}\n"
//"void foo(QList<QVector<int> >) {}\n"
//"void foo(QList<QVector<int> *>) {}\n"
//"void foo(QList<QVector<int *> *>) {}\n"
//"\n"
//"template <class T>\n"
//"void foo(QList<QVector<T> *>) {}\n"
//"\n"
//"\n"
//"namespace namespc {\n"
//"\n"
//"    class MyBase : public QObject\n"
//"    {\n"
//"    public:\n"
//"        MyBase() {}\n"
//"        virtual void doit(int i)\n"
//"        {\n"
//"           n = i;\n"
//"        }\n"
//"    protected:\n"
//"        int n;\n"
//"    };\n"
//"\n"
//"    namespace nested {\n"
//"\n"
//"        class MyFoo : public MyBase\n"
//"        {\n"
//"        public:\n"
//"            MyFoo() {}\n"
//"            virtual void doit(int i)\n"
//"            {\n"
//"                // Note there's a local 'n' and one in the base class");\n"
//"                n = i;\n"
//"            }\n"
//"        protected:\n"
//"            int n;\n"
//"        };\n"
//"\n"
//"        class MyBar : public MyFoo\n"
//"        {\n"
//"        public:\n"
//"            virtual void doit(int i)\n"
//"            {\n"
//"               n = i + 1;\n"
//"            }\n"
//"        };\n"
//"\n"
//"        namespace {\n"
//"\n"
//"            class MyAnon : public MyBar\n"
//"            {\n"
//"            public:\n"
//"                virtual void doit(int i)\n"
//"                {\n"
//"                   n = i + 3;\n"
//"                }\n"
//"            };\n"
//"\n"
//"            namespace baz {\n"
//"\n"
//"                class MyBaz : public MyAnon\n"
//"                {\n"
//"                public:\n"
//"                    virtual void doit(int i)\n"
//"                    {\n"
//"                       n = i + 5;\n"
//"                    }\n"
//"                };\n"
//"\n"
//"            } // namespace baz\n"
//"\n"
//"        } // namespace anon\n"
//"\n"
//"    } // namespace nested\n"
//"\n"
//    QTest::newRow("Namespace()
//    {
//        // This checks whether classes with "special" names are
//        // properly displayed");
//        "using namespace nested;\n"
//        "MyFoo foo;\n"
//        "MyBar bar;\n"
//        "MyAnon anon;\n"
//        "baz::MyBaz baz;\n"
//         + CheckType("anon namespc::nested::(anonymous namespace)::MyAnon");
//         + Check("bar", "namespc::nested::MyBar");
//         + CheckType("baz namespc::nested::(anonymous namespace)::baz::MyBaz");
//         + Check("foo", "namespc::nested::MyFoo");
//        // Continue");
//        // step into the doit() functions

//        baz.doit(1);\n"
//        anon.doit(1);\n"
//        foo.doit(1);\n"
//        bar.doit(1);\n"
//        unused();\n"
//    }


    const FloatValue ff("5.88545355e-44");
    QTest::newRow("AnonymousStruct")
            << Data("",
                    "union {\n"
                    "     struct { int i; int b; };\n"
                    "     struct { float f; };\n"
                    "     double d;\n"
                    " } a = { { 42, 43 } };\n (void)a;")

               + Check("a.d", FloatValue("9.1245819032257467e-313"), "double")

               //+ Check("a.b", "43", "int") % GdbVersion(0, 70699)
               //+ Check("a.i", "42", "int") % GdbVersion(0, 70699)
               //+ Check("a.f", ff, "float") % GdbVersion(0, 70699)

               + Check("a.#1.b", "43", "int") % NoCdbEngine
               + Check("a.#1.i", "42", "int") % NoCdbEngine
               + Check("a.#2.f", ff, "float") % NoCdbEngine
               + Check("a.b", "43", "int") % CdbEngine
               + Check("a.i", "42", "int") % CdbEngine
               + Check("a.f", ff, "float") % CdbEngine;


    QTest::newRow("Chars")
            << Data("#include <qglobal.h>\n",
                    "char c = -12;\n"
                    "signed char sc = -12;\n"
                    "unsigned char uc = -12;\n"
                    "qint8 qs = -12;\n"
                    "quint8 qu  = -12;\n"
                    "unused(&c, &sc, &uc, &qs, &qu);\n")

               + CoreProfile()
               + Check("c", "-12", "char")  // on all our platforms char is signed.
               + Check("sc", "-12", TypeDef("char", "signed char"))
               + Check("uc", "244", "unsigned char")
               + Check("qs", "-12", TypeDef("char", "@qint8"))
               + Check("qu", "244", TypeDef("unsigned char", "@quint8"));


    QTest::newRow("CharArrays")
            << Data("",
                    "char s[] = \"aöa\";\n"
                    "char t[] = \"aöax\";\n"
                    "wchar_t w[] = L\"aöa\";\n"
                    "unused(&s, &t, &w);\n")

               + CheckType("s", "char [5]") % NoCdbEngine
               + CheckType("s", "char [4]") % CdbEngine
               + Check("s.0", "[0]", "97", "char")
               + CheckType("t", "char [6]") % NoCdbEngine
               + CheckType("t", "char [5]") % CdbEngine
               + Check("t.0", "[0]", "97", "char")
               + CheckType("w", "wchar_t [4]");


    QTest::newRow("CharPointers")
            << Data("",
                    "const char *s = \"aöa\";\n"
                    "const char *t = \"a\\xc3\\xb6\";\n"
                    "unsigned char uu[] = { 'a', 153 /* ö Latin1 */, 'a' };\n"
                    "const unsigned char *u = uu;\n"
                    "const wchar_t *w = L\"aöa\";\n"
                    "unused(&s, &t, &uu, &u, &w);\n")

               + CheckType("u", "unsigned char *")
               + CheckType("uu", "unsigned char [3]")
               + CheckType("s", "char *")
               + CheckType("t", "char *")
               + CheckType("w", "wchar_t *");

        // All: Select UTF-8 in "Change Format for Type" in L&W context menu");
        // Windows: Select UTF-16 in "Change Format for Type" in L&W context menu");
        // Other: Select UCS-6 in "Change Format for Type" in L&W context menu");


    QTest::newRow("GccExtensions")
            << Data("",
                   "char v[8] = { 1, 2 };\n"
                   "char w __attribute__ ((vector_size (8))) = { 1, 2 };\n"
                   "int y[2] = { 1, 2 };\n"
                   "int z __attribute__ ((vector_size (8))) = { 1, 2 };\n"
                   "unused(&v, &w, &y, &z);\n")
               + NoCdbEngine
               + Check("v.0", "[0]", "1", "char")
               + Check("v.1", "[1]", "2", "char")
               + Check("w.0", "[0]", "1", "char")
               + Check("w.1", "[1]", "2", "char")
               + Check("y.0", "[0]", "1", "int")
               + Check("y.1", "[1]", "2", "int")
               + Check("z.0", "[0]", "1", "int")
               + Check("z.1", "[1]", "2", "int");


    QTest::newRow("Int")
            << Data("#include <qglobal.h>\n"
                    "#include <limits.h>\n"
                    "#include <QString>\n",
                    "quint64 u64 = ULLONG_MAX;\n"
                    "qint64 s64 = LLONG_MAX;\n"
                    "quint32 u32 = UINT_MAX;\n"
                    "qint32 s32 = INT_MAX;\n"
                    "quint64 u64s = 0;\n"
                    "qint64 s64s = LLONG_MIN;\n"
                    "quint32 u32s = 0;\n"
                    "qint32 s32s = INT_MIN;\n"
                    "QString dummy; // needed to get namespace\n"
                    "unused(&u64, &s64, &u32, &s32, &u64s, &s64s, &u32s, &s32s, &dummy);\n")
               + CoreProfile()
               + Check("u64", "18446744073709551615", TypeDef("unsigned int64", "@quint64"))
               + Check("s64", "9223372036854775807", TypeDef("int64", "@qint64"))
               + Check("u32", "4294967295", TypeDef("unsigned int", "@quint32"))
               + Check("s32", "2147483647", TypeDef("int", "@qint32"))
               + Check("u64s", "0", TypeDef("unsigned int64", "@quint64"))
               + Check("s64s", "-9223372036854775808", TypeDef("int64", "@qint64"))
               + Check("u32s", "0", TypeDef("unsigned int", "@quint32"))
               + Check("s32s", "-2147483648", TypeDef("int", "@qint32"));


    QTest::newRow("Float")
            << Data("#include <QFloat16>\n",
                    "qfloat16 f1 = 45.3f; unused(&f1);\n"
                    "qfloat16 f2 = 45.1f; unused(&f2);\n")
               + CoreProfile()
               + QtVersion(0x50900)
               // Using numpy:
               // + Check("f1", "45.281", "@qfloat16")
               // + Check("f2", "45.094", "@qfloat16");
               + Check("f1", "45.28125", "@qfloat16")
               + Check("f2", "45.09375", "@qfloat16");


    QTest::newRow("Enum")
            << Data("\n"
                    "enum Foo { a = -1000, b, c = 1, d };\n",
                    "Foo fa = a; unused(&fa);\n"
                    "Foo fb = b; unused(&fb);\n"
                    "Foo fc = c; unused(&fc);\n"
                    "Foo fd = d; unused(&fd);\n")
                + Check("fa", "a (-1000)", "Foo")
                + Check("fb", "b (-999)", "Foo")
                + Check("fc", "c (1)", "Foo")
                + Check("fd", "d (2)", "Foo");


    QTest::newRow("EnumFlags")
            << Data("\n"
                    "enum Flags { one = 1, two = 2, four = 4 };\n",
                    "Flags fone = one; unused(&fone);\n"
                    "Flags fthree = (Flags)(one|two); unused(&fthree);\n"
                    "Flags fmixed = (Flags)(two|8); unused(&fmixed);\n"
                    "Flags fbad = (Flags)(24); unused(&fbad);\n")
               + GdbEngine
               + Check("fone", "one (1)", "Flags")
               + Check("fthree", "(one | two) (3)", "Flags")
               + Check("fmixed", "(two | unknown:8) (10)", "Flags")
               + Check("fbad", "(unknown:24) (24)", "Flags");


    QTest::newRow("Array")
            << Data("",
                    "double a1[3][3];\n"
                    "for (int i = 0; i != 3; ++i)\n"
                    "    for (int j = 0; j != 3; ++j)\n"
                    "        a1[i][j] = i + j;\n"
                    "unused(&a1);\n\n"

                    "char a2[20] = { 0 };\n"
                    "a2[0] = 'a';\n"
                    "a2[1] = 'b';\n"
                    "a2[2] = 'c';\n"
                    "a2[3] = 'd';\n"
                    "a2[4] = 0;\n"
                    "unused(&a2);\n")

               + Check("a1", Pointer(), "double [3][3]")
               + Check("a1.0", "[0]", Pointer(), "double [3]")
               + Check("a1.0.0", "[0]", FloatValue("0"), "double")
               + Check("a1.0.2", "[2]", FloatValue("2"), "double")
               + Check("a1.2", "[2]", Pointer(), "double [3]")

               + Check("a2", Value("\"abcd" + QString(16, 0) + '"'), "char [20]")
               + Check("a2.0", "[0]", "97", "char")
               + Check("a2.3", "[3]", "100", "char");


    QTest::newRow("ArrayQt")
            << Data("#include <QByteArray>\n"
                    "#include <QString>\n" + fooData,

                    "QString a1[20];\n"
                    "a1[0] = \"a\";\n"
                    "a1[1] = \"b\";\n"
                    "a1[2] = \"c\";\n"
                    "a1[3] = \"d\";\n\n"

                    "QByteArray a2[20];\n"
                    "a2[0] = \"a\";\n"
                    "a2[1] = \"b\";\n"
                    "a2[2] = \"c\";\n"
                    "a2[3] = \"d\";\n\n"

                    "Foo a3[10];\n"
                    "for (int i = 0; i < 5; ++i)\n"
                    "    a3[i].a = i;\n")

               + CoreProfile()
               + CheckType("a1", "@QString [20]")
               + Check("a1.0", "[0]", "\"a\"", "@QString")
               + Check("a1.3", "[3]", "\"d\"", "@QString")
               + Check("a1.4", "[4]", "\"\"", "@QString")
               + Check("a1.19", "[19]", "\"\"", "@QString")

               + CheckType("a2", "@QByteArray [20]")
               + Check("a2.0", "[0]", "\"a\"", "@QByteArray")
               + Check("a2.3", "[3]", "\"d\"", "@QByteArray")
               + Check("a2.4", "[4]", "\"\"", "@QByteArray")
               + Check("a2.19", "[19]", "\"\"", "@QByteArray")

               + CheckType("a3", "Foo [10]")
               + Check("a3.0", "[0]", "", "Foo")
               + Check("a3.9", "[9]", "", "Foo");


    QTest::newRow("Bitfields")
            << Data("",
                    "struct S\n"
                    "{\n"
                    "    S() : x(2), y(3), z(39), c(1), b(0), f(5), d(6), i(7) {}\n"
                    "    unsigned int x : 3;\n"
                    "    unsigned int y : 4;\n"
                    "    unsigned int z : 18;\n"
                    "    bool c : 1;\n"
                    "    bool b;\n"
                    "    float f;\n"
                    "    double d;\n"
                    "    int i;\n"
                    "} s;\n"
                    "unused(&s);\n")

               + Check("s", "", "S") % NoCdbEngine
               + Check("s.b", "0", "bool")
               + Check("s.c", "1", "bool : 1") % NoCdbEngine
               + Check("s.c", "1", "bool") % CdbEngine
               + Check("s.f", FloatValue("5"), "float")
               + Check("s.d", FloatValue("6"), "double")
               + Check("s.i", "7", "int")
               + Check("s.x", "2", "unsigned int : 3") % NoCdbEngine
               + Check("s.y", "3", "unsigned int : 4") % NoCdbEngine
               + Check("s.z", "39", "unsigned int : 18") % NoCdbEngine
               + Check("s.x", "2", "unsigned int") % CdbEngine
               + Check("s.y", "3", "unsigned int") % CdbEngine
               + Check("s.z", "39", "unsigned int") % CdbEngine;


    QTest::newRow("Function")
            << Data("#include <QByteArray>\n"
                    "struct Function\n"
                    "{\n"
                    "    Function(QByteArray var, QByteArray f, double min, double max)\n"
                    "      : var(var), f(f), min(min), max(max) {}\n"
                    "    QByteArray var;\n"
                    "    QByteArray f;\n"
                    "    double min;\n"
                    "    double max;\n"
                    "};\n",
                    "// In order to use this, switch on the 'qDump__Function' in dumper.py\n"
                    "Function func(\"x\", \"sin(x)\", 0, 1);\n"
                    "func.max = 10;\n"
                    "func.f = \"cos(x)\";\n"
                    "func.max = 7;\n")

               + CoreProfile()

               + Check("func", "", "Function")
               + Check("func.min", FloatValue("0"), "double")
               + Check("func.var", "\"x\"", "@QByteArray")
               + Check("func", "", "Function")
               + Check("func.f", "\"cos(x)\"", "@QByteArray")
               + Check("func.max", FloatValue("7"), "double");


//    QTest::newRow("AlphabeticSorting")
//                  << Data(
//        "// This checks whether alphabetic sorting of structure\n"
//        "// members work");\n"
//        "struct Color\n"
//        "{\n"
//        "    int r,g,b,a;\n"
//        "    Color() { r = 1, g = 2, b = 3, a = 4; }\n"
//        "};\n"
//        "    Color c;\n"
//         + Check("c", "basic::Color");
//         + Check("c.a", "4", "int");
//         + Check("c.b", "3", "int");
//         + Check("c.g", "2", "int");
//         + Check("c.r", "1", "int");

//        // Manual: Toogle "Sort Member Alphabetically" in context menu
//        // Manual: of "Locals and Expressions" view");
//        // Manual: Check that order of displayed members changes");

    QTest::newRow("Typedef")
            << Data("#include <QtGlobal>\n"
                    "namespace ns {\n"
                    "    typedef unsigned long long vl;\n"
                    "    typedef vl verylong;\n"
                    "}\n"
                    "typedef quint32 myType1;\n"
                    "typedef unsigned int myType2;\n",
                    "myType1 t1 = 0;\n"
                    "myType2 t2 = 0;\n"
                    "ns::vl j = 1000;\n"
                    "ns::verylong k = 1000;\n"
                    "unused(&t1, &t2, &j, &k);\n")

                + CoreProfile()
                + BigArrayProfile()
                + NoCdbEngine

                + Check("j", "1000", "ns::vl")
                + Check("k", "1000", "ns::verylong")
                + Check("t1", "0", "myType1")
                + Check("t2", "0", "myType2");


    QTest::newRow("Typedef2")
            << Data("#include <vector>\n"
                    "template<typename T> using TVector = std::vector<T>;\n",
                    "std::vector<bool> b1(10); unused(&b1);\n"
                    "std::vector<int> b2(10); unused(&b2);\n"
                    "TVector<bool> b3(10); unused(&b3);\n"
                    "TVector<int> b4(10); unused(&b4);\n"
                    "TVector<bool> b5(10); unused(&b5);\n"
                    "TVector<int> b6(10); unused(&b6);\n")

                + NoCdbEngine

                // The test is for a gcc bug
                // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=80466,
                // so check for any gcc version vs any other compiler
                // (identified by gccversion == 0).
                + Check("b1", "<10 items>", "std::vector<bool>")
                + Check("b2", "<10 items>", "std::vector<int>")
                + Check("b3", "<10 items>", "TVector") % GccVersion(1)
                + Check("b4", "<10 items>", "TVector") % GccVersion(1)
                + Check("b5", "<10 items>", "TVector<bool>") % GccVersion(0, 0)
                + Check("b6", "<10 items>", "TVector<int>") % GccVersion(0, 0);


    QTest::newRow("Struct")
            << Data(fooData,

                    "Foo f(3);\n"
                    "unused(&f);\n\n"

                    "Foo *p = new Foo();\n"
                    "unused(&p);\n")

               + CoreProfile()
               + Check("f", "", "Foo")
               + Check("f.a", "3", "int")
               + Check("f.b", "2", "int")

               + Check("p", Pointer(), "Foo")
               + Check("p.a", "0", "int")
               + Check("p.b", "2", "int");

    QTest::newRow("This")
            << Data("struct Foo {\n"
                    "    Foo() : x(143) {}\n"
                    "    int foo() {\n"
                    "        BREAK;\n"
                    "        return x;\n"
                    "    }\n\n"
                    "    int x;\n"
                    "};\n",
                    "Foo f;\n"
                    "f.foo();\n")

               + Check("this", "", "Foo")
               + Check("this.x", "143", "int");

    QTest::newRow("Union")
            << Data("union U { int a; int b; };", "U u;\n"
                    "unused(&u);\n")
               + Check("u", "", "U")
               + CheckType("u.a", "int")
               + CheckType("u.b", "int");

//    QTest::newRow("TypeFormats")
//                  << Data(
//    "// These tests should result in properly displayed umlauts in the\n"
//    "// Locals and Expressions view. It is only support on gdb with Python");\n"
//    "const char *s = "aöa";\n"
//    "const wchar_t *w = L"aöa";\n"
//    "QString u;\n"
//    "// All: Select UTF-8 in "Change Format for Type" in L&W context menu");\n"
//    "// Windows: Select UTF-16 in "Change Format for Type" in L&W context menu");\n"
//    "// Other: Select UCS-6 in "Change Format for Type" in L&W context menu");\n"
//    "if (sizeof(wchar_t) == 4)\n"
//    "    u = QString::fromUcs4((uint *)w);\n"
//    "else\n"
//    "    u = QString::fromUtf16((ushort *)w);\n"
//         + CheckType("s char *");
//         + Check("u "" QString");
//         + CheckType("w wchar_t *");


    QTest::newRow("PointerTypedef")
            << Data("typedef void *VoidPtr;\n"
                    "typedef const void *CVoidPtr;\n"
                    "struct A {};\n",
                    "A a;\n"
                    "VoidPtr p = &a;\n"
                    "CVoidPtr cp = &a;\n"
                    "unused(&a, &p, &cp);\n")
               + Check("a", "", "A")
               + Check("cp", Pointer(), TypeDef("void*", "CVoidPtr"))
               + Check("p", Pointer(), TypeDef("void*", "VoidPtr"));


    QTest::newRow("Reference")
            << Data("#include <string>\n"
                    "#include <QString>\n"

                    "using namespace std;\n"
                    "string fooxx() { return \"bababa\"; }\n"
                    + fooData,

                    "int a1 = 43;\n"
                    "const int &b1 = a1;\n"
                    "typedef int &Ref1;\n"
                    "const int c1 = 44;\n"
                    "const Ref1 d1 = a1;\n"
                    "unused(&a1, &b1, &c1, &d1);\n\n"

                    "string a2 = \"hello\";\n"
                    "const string &b2 = fooxx();\n"
                    "typedef string &Ref2;\n"
                    "const string c2= \"world\";\n"
                    "const Ref2 d2 = a2;\n"
                    "unused(&a2, &b2, &c2, &d2);\n\n"

                    "QString a3 = QLatin1String(\"hello\");\n"
                    "const QString &b3 = a3;\n"
                    "typedef QString &Ref3;\n"
                    "const Ref3 d3 = const_cast<Ref3>(a3);\n"
                    "unused(&a3, &b3, &d3);\n\n"

                    "Foo a4(12);\n"
                    "const Foo &b4 = a4;\n"
                    "typedef Foo &Ref4;\n"
                    "const Ref4 d4 = const_cast<Ref4>(a4);\n"
                    "unused(&a4, &b4, &d4);\n")

               + CoreProfile()
               + NoCdbEngine // The Cdb has no information about references

               + Check("a1", "43", "int")
               + Check("b1", "43", "int &")
               + Check("c1", "44", "int")
               + Check("d1", "43", "Ref1")

               + Check("a2", "\"hello\"", "std::string")
               + Check("b2", "\"bababa\"", TypePattern("(std::)?string &")) // Clang...
               + Check("c2", "\"world\"", "std::string")
               + Check("d2", "\"hello\"", "Ref2") % NoLldbEngine

               + Check("a3", "\"hello\"", "@QString")
               + Check("b3", "\"hello\"", "@QString &")
               + Check("d3", "\"hello\"", "Ref3")

               + Check("a4", "", "Foo")
               + Check("a4.a", "12", "int")
               + Check("b4", "", "Foo &")
               + Check("b4.a", "12", "int")
               //+ Check("d4", "\"hello\"", "Ref4");  FIXME: We get "Foo &" instead
               + Check("d4.a", "12", "int");

    QTest::newRow("DynamicReference")
            << Data("struct BaseClass { virtual ~BaseClass() {} };\n"
                    "struct DerivedClass : BaseClass {};\n",
                    "DerivedClass d;\n"
                    "BaseClass *b1 = &d;\n"
                    "BaseClass &b2 = d;\n"
                    "unused(&d, &b1, &b2);\n")
               + NoCdbEngine // The Cdb has no information about references
               + CheckType("b1", "DerivedClass") // autoderef
               + CheckType("b2", "DerivedClass &");


/*
    // FIXME: The QDateTime dumper for 5.3 is really too slow.
    QTest::newRow("LongEvaluation1")
            << Data("#include <QDateTime>",
                    "QDateTime time = QDateTime::currentDateTime();\n"
                    "const int N = 10000;\n"
                    "QDateTime bigv[N];\n"
                    "for (int i = 0; i < N; ++i) {\n"
                    "    bigv[i] = time;\n"
                    "    time = time.addDays(1);\n"
                    "}\n"
                    "unused(&bigv[10]);\n")
             + Check("N", "10000", "int")
             + CheckType("bigv", "@QDateTime [10000]")
             + CheckType("bigv.0", "[0]", "@QDateTime")
             + CheckType("bigv.9999", "[9999]", "@QDateTime");
*/

    QTest::newRow("LongEvaluation2")
            << Data("",
                    "const int N = 10000;\n"
                    "int bigv[N];\n"
                    "for (int i = 0; i < N; ++i)\n"
                    "    bigv[i] = i;\n"
                    "unused(&bigv[10]);\n")
             + BigArrayProfile()
             + Check("N", "10000", "int")
             + CheckType("bigv", "int [10000]")
             + Check("bigv.0", "[0]", "0", "int")
             + Check("bigv.9999", "[9999]", "9999", "int");

//    QTest::newRow("Fork")
//                  << Data(
//        "QProcess proc;\n"
//        "proc.start("/bin/ls");\n"
//        "proc.waitForFinished();\n"
//        "QByteArray ba = proc.readAllStandardError();\n"
//        "ba.append('x');\n"
//         + Check("ba "x" QByteArray");
//         + Check("proc  QProcess");


    QTest::newRow("MemberFunctionPointer")
            << Data("struct Class\n"
                    "{\n"
                    "    Class() : a(34) {}\n"
                    "    int testFunctionPointerHelper(int x) { return x; }\n"
                    "    int a;\n"
                    "};\n"
                    "typedef int (Class::*func_t)(int);\n"
                    "typedef int (Class::*member_t);\n",

                    "Class x;\n"
                    "func_t f = &Class::testFunctionPointerHelper;\n"
                    "int a1 = (x.*f)(43);\n"
                    "unused(&a1);\n"

                    "member_t m = &Class::a;\n"
                    "int a2 = x.*m;\n"
                    "unused(&a2);\n")

             + CheckType("f", TypeDef("<function>", "func_t"))
             + CheckType("m", TypeDef("int*", "member_t"));


  QTest::newRow("PassByReference")
           << Data(fooData +
                   "void testPassByReference(Foo &f) {\n"
                   "   BREAK;\n"
                   "   int dummy = 2;\n"
                   "   unused(&f, &dummy);\n"
                   "}\n",
                   "Foo f(12);\n"
                   "testPassByReference(f);\n")
               + CoreProfile()
               + NoCdbEngine // The Cdb has no information about references
               + CheckType("f", "Foo &")
               + Check("f.a", "12", "int");


    QTest::newRow("BigInt")
            << Data("#include <QString>\n"
                    "#include <limits>\n",
                    "qint64 a = Q_INT64_C(0xF020304050607080);\n"
                    "quint64 b = Q_UINT64_C(0xF020304050607080);\n"
                    "quint64 c = std::numeric_limits<quint64>::max() - quint64(1);\n"
                    "qint64 d = c;\n"
                    "QString dummy;\n"
                    "unused(&a, &b, &c, &d, &dummy);\n")
               + CoreProfile()
               + Check("a", "-1143861252567568256", TypeDef("int64", "@qint64"))
               + Check("b", "17302882821141983360", TypeDef("unsigned int64", "@quint64"))
               + Check("c", "18446744073709551614", TypeDef("unsigned int64", "@quint64"))
               + Check("d", "-2",                   TypeDef("int64", "@qint64"));


    QTest::newRow("Hidden")
            << Data("#include <QString>\n",
                    "int n = 1;\n"
                    "{\n"
                    "    QString n = \"2\";\n"
                    "    {\n"
                    "        double n = 3.5;\n"
                    "        BREAK;\n"
                    "        unused(&n);\n"
                    "    }\n"
                    "    unused(&n);\n"
                    "}\n"
                    "unused(&n);\n")
               + CoreProfile()
               + Check("n", FloatValue("3.5"), "double")
               + Check("n@1", "\"2\"", "@QString")
               + Check("n@2", "1", "int");


    const Data rvalueData = Data(
                    "#include <utility>\n"
                    "struct X { X() : a(2), b(3) {} int a, b; };\n"
                    "X testRValueReferenceHelper1() { return X(); }\n"
                    "X testRValueReferenceHelper2(X &&x) { return x; }\n",
                    "X &&x1 = testRValueReferenceHelper1();\n"
                    "X &&x2 = testRValueReferenceHelper2(std::move(x1));\n"
                    "X &&x3 = testRValueReferenceHelper2(testRValueReferenceHelper1());\n"
                    "X y1 = testRValueReferenceHelper1();\n"
                    "X y2 = testRValueReferenceHelper2(std::move(y1));\n"
                    "X y3 = testRValueReferenceHelper2(testRValueReferenceHelper1());\n"
                    "unused(&x1, &x2, &x3, &y1, &y2, &y3);\n")
               + Cxx11Profile()
               + Check("y1", "", "X")
               + Check("y2", "", "X")
               + Check("y3", "", "X");

    QTest::newRow("RValueReference2")
            << Data(rvalueData)
               + DwarfProfile(2)
               + NoCdbEngine // The Cdb has no information about references
               + Check("x1", "", "X &")
               + Check("x2", "", "X &")
               + Check("x3", "", "X &");

    // GCC emits rvalue references with DWARF-4, i.e. after 4.7.4.
    // GDB doesn't understand them,
    // https://sourceware.org/bugzilla/show_bug.cgi?id=14441
    // and produces: 'x1 = <unknown type in [...]/doit, CU 0x0, DIE 0x355>'
    // LLdb reports plain references.

    QTest::newRow("RValueReference4")
            << Data(rvalueData)
               + DwarfProfile(4)
               + LldbEngine
               + Check("x1", "", "X &")
               + Check("x2", "", "X &")
               + Check("x3", "", "X &");

    QTest::newRow("SSE")
            << Data("#include <xmmintrin.h>\n"
                    "#include <stddef.h>\n",
                    "float a[4];\n"
                    "float b[4];\n"
                    "int i;\n"
                    "for (i = 0; i < 4; i++) {\n"
                    "    a[i] = 2 * i;\n"
                    "    b[i] = 2 * i;\n"
                    "}\n"
                    "__m128 sseA, sseB;\n"
                    "sseA = _mm_loadu_ps(a);\n"
                    "sseB = _mm_loadu_ps(b);\n"
                    "unused(&i, &sseA, &sseB);\n")
               + Profile("QMAKE_CXXFLAGS += -msse2")
               + CheckType("sseA", "__m128")
               + Check("sseA.2", "[2]", FloatValue("4"), "float")
               + CheckType("sseB", "__m128");


    QTest::newRow("BoostOptional")
            << Data("#include <boost/optional.hpp>\n"
                    "#include <QStringList>\n",

                    "boost::optional<int> i0, i1;\n"
                    "i1 = 1;\n"
                    "unused(&i0, &i1);\n\n"

                    "boost::optional<QStringList> sl0, sl;\n"
                    "sl = (QStringList() << \"xxx\" << \"yyy\");\n"
                    "sl.get().append(\"zzz\");\n"
                    "unused(&sl);\n")

             + CoreProfile()
             + BoostProfile()

             + Check("i0", "<uninitialized>", "boost::optional<int>")
             + Check("i1", "1", "boost::optional<int>")

             + Check("sl", "<3 items>", "boost::optional<@QStringList>");


    QTest::newRow("BoostSharedPtr")
            << Data("#include <QStringList>\n"
                    "#include <boost/shared_ptr.hpp>\n",
                    "boost::shared_ptr<int> s;\n"
                    "boost::shared_ptr<int> i(new int(43));\n"
                    "boost::shared_ptr<int> j = i;\n"
                    "boost::shared_ptr<QStringList> sl(new QStringList(QStringList() << \"HUH!\"));\n"
                    "unused(&s, &i, &j, &sl);\n")

             + CoreProfile()
             + BoostProfile()

             + Check("s", "(null)", "boost::shared_ptr<int>")
             + Check("i", "43", "boost::shared_ptr<int>")
             + Check("j", "43", "boost::shared_ptr<int>")
             + Check("sl", "<1 items>", " boost::shared_ptr<@QStringList>")
             + Check("sl.0", "[0]", "\"HUH!\"", "@QString");


    QTest::newRow("BoostGregorianDate")
            << Data("#include <boost/date_time.hpp>\n"
                    "#include <boost/date_time/gregorian/gregorian.hpp>\n",
                    "using namespace boost;\n"
                    "using namespace gregorian;\n"
                    "date d(2005, Nov, 29);\n"
                    "date d0 = d;\n"
                    "date d1 = d += months(1);\n"
                    "date d2 = d += months(1);\n"
                    "// snap-to-end-of-month behavior kicks in:\n"
                    "date d3 = d += months(1);\n"
                    "// Also end of the month (expected in boost)\n"
                    "date d4 = d += months(1);\n"
                    "// Not where we started (expected in boost)\n"
                    "date d5 = d -= months(4);\n"
                    "unused(&d1, &d2, &d3, &d4, &d5);\n")
             + BoostProfile()
             + Check("d0", "Tue Nov 29 2005", "boost::gregorian::date")
             + Check("d1", "Thu Dec 29 2005", "boost::gregorian::date")
             + Check("d2", "Sun Jan 29 2006", "boost::gregorian::date")
             + Check("d3", "Tue Feb 28 2006", "boost::gregorian::date")
             + Check("d4", "Fri Mar 31 2006", "boost::gregorian::date")
             + Check("d5", "Wed Nov 30 2005", "boost::gregorian::date");


    QTest::newRow("BoostPosixTimeTimeDuration")
            << Data("#include <boost/date_time.hpp>\n"
                    "#include <boost/date_time/gregorian/gregorian.hpp>\n"
                    "#include <boost/date_time/posix_time/posix_time.hpp>\n",
                    "using namespace boost;\n"
                    "using namespace posix_time;\n"
                    "time_duration d1(1, 0, 0);\n"
                    "time_duration d2(0, 1, 0);\n"
                    "time_duration d3(0, 0, 1);\n"
                    "unused(&d1, &d2, &d3);\n")
             + BoostProfile()
             + Check("d1", "01:00:00", "boost::posix_time::time_duration")
             + Check("d2", "00:01:00", "boost::posix_time::time_duration")
             + Check("d3", "00:00:01", "boost::posix_time::time_duration");


    QTest::newRow("BoostBimap")
            << Data("#include <boost/bimap.hpp>\n",
                    "typedef boost::bimap<int, int> B;\n"
                    "B b;\n"
                    "b.left.insert(B::left_value_type(1, 2));\n"
                    "B::left_const_iterator it = b.left.begin();\n"
                    "int l = it->first;\n"
                    "int r = it->second;\n"
                    "unused(&l, &r);\n")
             + BoostProfile()
             + Check("b", "<1 items>", TypeDef("boost::bimaps::bimap<int,int,boost::mpl::na,"
                                               "boost::mpl::na,boost::mpl::na>", "B"));


    QTest::newRow("BoostPosixTimePtime")
            << Data("#include <boost/date_time.hpp>\n"
                    "#include <boost/date_time/gregorian/gregorian.hpp>\n"
                    "#include <boost/date_time/posix_time/posix_time.hpp>\n"
                    "using namespace boost;\n"
                    "using namespace gregorian;\n"
                    "using namespace posix_time;\n",
                    "ptime p1(date(2002, 1, 10), time_duration(1, 0, 0));\n"
                    "ptime p2(date(2002, 1, 10), time_duration(0, 0, 0));\n"
                    "ptime p3(date(1970, 1, 1), time_duration(0, 0, 0));\n"
                    "unused(&p1, &p2, &p3);\n")
             + BoostProfile()
             + Check("p1", "Thu Jan 10 01:00:00 2002", "boost::posix_time::ptime")
             + Check("p2", "Thu Jan 10 00:00:00 2002", "boost::posix_time::ptime")
             + Check("p3", "Thu Jan 1 00:00:00 1970", "boost::posix_time::ptime");


/*
    FIXME

    QTest::newRow("BoostList")
            << Data("#include <boost/container/list.hpp>\n",
                    "typedef std::pair<int, double> p;\n"
                    "boost::container::list<p> l;\n"
                    "l.push_back(p(13, 61));\n"
                    "l.push_back(p(14, 64));\n"
                    "l.push_back(p(15, 65));\n"
                    "l.push_back(p(16, 66));\n")
             + BoostProfile()
             + Check("l", "<4 items>", TypePattern("boost::container::list<std::pair<int,double>.*>"))
             + Check("l.2.second", FloatValue("65"), "double");
*/


    QTest::newRow("BoostUnorderedSet")
            << Data("#include <boost/unordered_set.hpp>\n"
                    "#include <boost/version.hpp>\n"
                    "#include <string>\n",

                    "boost::unordered_set<int> s1;\n"
                    "s1.insert(11);\n"
                    "s1.insert(22);\n\n"

                    "boost::unordered_set<std::string> s2;\n"
                    "s2.insert(\"abc\");\n"
                    "s2.insert(\"def\");\n")

               + BoostProfile()

               + Check("s1", "<2 items>", "boost::unordered::unordered_set<int>") % BoostVersion(1 * 100000 + 54 * 100)
               + Check("s1.0", "[0]", "22", "int") % BoostVersion(1 * 100000 + 54 * 100)
               + Check("s1.1", "[1]", "11", "int") % BoostVersion(1 * 100000 + 54 * 100)

               + Check("s2", "<2 items>", "boost::unordered::unordered_set<std::string>") % BoostVersion(1 * 100000 + 54 * 100)
               + Check("s2.0", "[0]", "\"def\"", "std::string") % BoostVersion(1 * 100000 + 54 * 100)
               + Check("s2.1", "[1]", "\"abc\"", "std::string") % BoostVersion(1 * 100000 + 54 * 100);


    QTest::newRow("Eigen")
         << Data("#ifdef HAS_EIGEN\n"
                "#include <Eigen/Core>\n"
                "#endif\n",
                "#ifdef HAS_EIGEN\n"
                "using namespace Eigen;\n"
                "Vector3d zero = Vector3d::Zero();\n"
                "Matrix3d constant = Matrix3d::Constant(5);\n"

                "MatrixXd dynamicMatrix(5, 2);\n"
                "dynamicMatrix << 1, 2, 3, 4, 5, 6, 7, 8, 9, 10;\n"

                "Matrix<double, 2, 5, ColMajor> colMajorMatrix;\n"
                "colMajorMatrix << 1, 2, 3, 4, 5, 6, 7, 8, 9, 10;\n"

                "Matrix<double, 2, 5, RowMajor> rowMajorMatrix;\n"
                "rowMajorMatrix << 1, 2, 3, 4, 5, 6, 7, 8, 9, 10;\n"

                "VectorXd vector(3);\n"
                "vector << 1, 2, 3;\n"
                "#else\n"
                "skipall = true;\n"
                "#endif\n")

            + EigenProfile()

            + Check("colMajorMatrix", "(2 x 5), ColumnMajor",
                    "Eigen::Matrix<double, 2, 5, 0, 2, 5>")
            + Check("colMajorMatrix.[1,0]", FloatValue("6"), "double")
            + Check("colMajorMatrix.[1,1]", FloatValue("7"), "double")

            + Check("rowMajorMatrix", "(2 x 5), RowMajor",
                    "Eigen::Matrix<double, 2, 5, 1, 2, 5>")
            + Check("rowMajorMatrix.[1,0]", FloatValue("6"), "double")
            + Check("rowMajorMatrix.[1,1]", FloatValue("7"), "double")

            + Check("dynamicMatrix", "(5 x 2), ColumnMajor", "Eigen::MatrixXd")
            + Check("dynamicMatrix.[2,0]", FloatValue("5"), "double")
            + Check("dynamicMatrix.[2,1]", FloatValue("6"), "double")

            + Check("constant", "(3 x 3), ColumnMajor", "Eigen::Matrix3d")
            + Check("constant.[0,0]", FloatValue("5"), "double")

            + Check("zero", "(3 x 1), ColumnMajor", "Eigen::Vector3d")
            + Check("zero.1", "[1]", FloatValue("0"), "double")

            + Check("vector", "(3 x 1), ColumnMajor", "Eigen::VectorXd")
            + Check("vector.1", "[1]", FloatValue("2"), "double");


    // https://bugreports.qt.io/browse/QTCREATORBUG-3611
    QTest::newRow("Bug3611")
        << Data("",
                "typedef unsigned char byte;\n"
                "byte f = '2';\n"
                "int *x = (int*)&f;\n")
         + Check("f", "50", TypeDef("unsigned char", "byte"));


    // https://bugreports.qt.io/browse/QTCREATORBUG-4904
    QTest::newRow("Bug4904")
        << Data("#include <QMap>\n"
                "struct CustomStruct {\n"
                "    int id;\n"
                "    double dvalue;\n"
                "};",
                "QMap<int, CustomStruct> map;\n"
                "CustomStruct cs1;\n"
                "cs1.id = 1;\n"
                "cs1.dvalue = 3.14;\n"
                "CustomStruct cs2 = cs1;\n"
                "cs2.id = -1;\n"
                "map.insert(cs1.id, cs1);\n"
                "map.insert(cs2.id, cs2);\n"
                "QMap<int, CustomStruct>::iterator it = map.begin();\n")
         + CoreProfile()
         + Check("map", "<2 items>", "@QMap<int, CustomStruct>")
         + Check("map.0.key", "-1", "int")
         + CheckType("map.0.value", "CustomStruct")
         + Check("map.0.value.dvalue", FloatValue("3.14"), "double")
         + Check("map.0.value.id", "-1", "int");


#if 0
      // https://bugreports.qt.io/browse/QTCREATORBUG-5106
      QTest::newRow("Bug5106")
          << Data("struct A5106 {\n"
                  "        A5106(int a, int b) : m_a(a), m_b(b) {}\n"
                  "        virtual int test() { return 5; }\n"
                  "        int m_a, m_b;\n"
                  "};\n"

                  "struct B5106 : public A5106 {\n"
                  "        B5106(int c, int a, int b) : A5106(a, b), m_c(c) {}\n"
                  "        virtual int test() {\n"
                  "            BREAK;\n"
                  "        }\n"
                  "        int m_c;\n"
                  "};\n",
                  "B5106 b(1,2,3);\n"
                  "b.test();\n"
                  "b.A5106::test();\n")
            + Check(?)
#endif


    // https://bugreports.qt.io/browse/QTCREATORBUG-5184

    // Note: The report there shows type field "QUrl &" instead of QUrl");
    // It's unclear how this can happen. It should never have been like
    // that with a stock 7.2 and any version of Creator");
    QTest::newRow("Bug5184")
        << Data("#include <QUrl>\n"
                "#include <QNetworkRequest>\n"
                "void helper(const QUrl &url, int qtversion)\n"
                "{\n"
                "    QNetworkRequest request(url);\n"
                "    QList<QByteArray> raw = request.rawHeaderList();\n"
                "    BREAK;\n"
                "    unused(&url);\n"
                "}\n",
               " QUrl url(QString(\"http://127.0.0.1/\"));\n"
               " helper(url, qtversion);\n")
           + NetworkProfile()
           + Check("raw", "<0 items>", "@QList<@QByteArray>")
           + CheckType("request", "@QNetworkRequest")
           + Check("url", "\"http://127.0.0.1/\"", "@QUrl &") % NoCdbEngine
           + Check("url", "\"http://127.0.0.1/\"", "@QUrl") % CdbEngine;


    // https://bugreports.qt.io/browse/QTCREATORBUG-5799
    QTest::newRow("Bug5799")
        << Data("typedef struct { int m1; int m2; } S1;\n"
                "struct S2 : S1 { };\n"
                "typedef struct S3 { int m1; int m2; } S3;\n"
                "struct S4 : S3 { };\n",
                "S2 s2;\n"
                "s2.m1 = 5;\n"
                "S4 s4;\n"
                "s4.m1 = 5;\n"
                "S1 a1[10];\n"
                "typedef S1 Array[10];\n"
                "Array a2;\n"
                "unused(&s2, &s4, &a1, &a2);\n")
             + CheckType("a1", "S1 [10]")
             + CheckType("a2", TypeDef("S1 [10]", "Array"))
             + CheckType("s2", "S2")
             + CheckType("s2.@1", "[S1]", "S1")
             + Check("s2.@1.m1", "5", "int")
             + CheckType("s2.@1.m2", "int")
             + CheckType("s4", "S4")
             + CheckType("s4.@1", "[S3]", "S3")
             + Check("s4.@1.m1", "5", "int")
             + CheckType("s4.@1.m2", "int");


    // https://bugreports.qt.io/browse/QTCREATORBUG-6465
    QTest::newRow("Bug6465")
        << Data("",
                "typedef char Foo[20];\n"
                "Foo foo = \"foo\";\n"
                "char bar[20] = \"baz\";\n")
        + CheckType("bar", "char[20]");


#ifndef Q_OS_WIN
    // https://bugreports.qt.io/browse/QTCREATORBUG-6857
    QTest::newRow("Bug6857")
        << Data("#include <QFile>\n"
                "#include <QString>\n"
                "struct MyFile : public QFile {\n"
                "    MyFile(const QString &fileName)\n"
                "        : QFile(fileName) {}\n"
                "};\n",

                "MyFile file(\"/tmp/tt\");\n"
                "file.setObjectName(\"A file\");\n")

         + CoreProfile()
         + QtVersion(0x50000)

         + Check("file", "\"A file\"", "MyFile")
         + Check("file.@1", "[@QFile]", "\"/tmp/tt\"", "@QFile");
        // FIXME: The classname in the iname is sub-optimal.
         //+ Check("file.@1.[QFileDevice]", "[@QFileDevice]", "\"A file\"", "@QFileDevice");
         //+ Check("file.@1.@1", "[QFile]", "\"A file\"", "@QObject");
#endif


#if 0
    QTest::newRow("Bug6858")
            << Data("#include <QFile>\n"
                    "#include <QString>\n"
                    "class MyFile : public QFile\n"
                    "{\n"
                    "public:\n"
                    "    MyFile(const QString &fileName)\n"
                    "        : QFile(fileName) {}\n"
                    "};\n",
                    "MyFile file(\"/tmp/tt\");\n"
                    "file.setObjectName(\"Another file\");\n"
                    "QFile *pfile = &file;\n")
         + Check("pfile", "\"Another file\"", "MyFile")
         + Check("pfile.@1", "\"/tmp/tt\"", "@QFile")
         + Check("pfile.@1.@1", "\"Another file\"", "@QObject");
#endif

//namespace bug6863 {

//    class MyObject : public QObject\n"
//    {\n"
//        Q_OBJECT\n"
//    public:\n"
//        MyObject() {}\n"
//    };\n"
//\n"
//    void setProp(QObject *obj)\n"
//    {\n"
//        obj->setProperty("foo", "bar");
//         + Check("obj.[QObject].properties", "<2", "items>");
//        // Continue");
//        unused(&obj);
//    }

//    QTest::newRow("6863")
//                                     << Data(
//    {
//        QFile file("/tmp/tt");\n"
//        setProp(&file);\n"
//        MyObject obj;\n"
//        setProp(&obj);\n"
//    }\n"

//}


    QTest::newRow("Bug6933")
            << Data("struct Base\n"
                    "{\n"
                    "    Base() : a(21) {}\n"
                    "    virtual ~Base() {}\n"
                    "    int a;\n"
                    "};\n"
                    "struct Derived : public Base\n"
                    "{\n"
                    "    Derived() : b(42) {}\n"
                    "    int b;\n"
                    "};\n",
                    "Derived d;\n"
                    "Base *b = &d;\n"
                    "unused(&d, &b);\n")
               + Check("b.@1.a", "a", "21", "int")
               + Check("b.b", "b", "42", "int");



    // https://bugreports.qt.io/browse/QTCREATORBUG-17823
    QTest::newRow("Bug17823")
            << Data("struct Base1\n"
                    "{\n"
                    "    virtual ~Base1() {}\n"
                    "    int foo = 42;\n"
                    "};\n\n"
                    "struct Base2\n"
                    "{\n"
                    "    virtual ~Base2() {}\n"
                    "    int bar = 43;\n"
                    "};\n\n"
                    "struct Derived : Base1, Base2\n"
                    "{\n"
                    "    int baz = 84;\n"
                    "};\n\n"
                    "struct Container\n"
                    "{\n"
                    "    Container(Base2 *b) : b2(b) {}\n"
                    "    Base2 *b2;\n"
                    "};\n",

                    "Derived d;\n"
                    "Container c(&d); // c.b2 has wrong address\n"
                    "unused(&c);\n"
                    "Base2 *b2 = &d; // This has the right address\n"
                    "unused(&b2);\n")
                + Check("c.b2.@1.foo", "42", "int")
                + Check("c.b2.@2.bar", "43", "int")
                + Check("c.b2.baz", "84", "int")

                + Check("d.@1.foo", "42", "int")
                + Check("d.@2.bar", "43", "int")
                + Check("d.baz", "84", "int");



    // http://www.qtcentre.org/threads/42170-How-to-watch-data-of-actual-type-in-debugger
    QTest::newRow("QC42170")
        << Data("struct Object {\n"
                "    Object(int id_) : id(id_) {}\n"
                "    virtual ~Object() {}\n"
                "    int id;\n"
                "};\n\n"
                "struct Point : Object\n"
                "{\n"
                "    Point(double x_, double y_) : Object(1), x(x_), y(y_) {}\n"
                "    double x, y;\n"
                "};\n\n"
                "struct Circle : Point\n"
                "{\n"
                "    Circle(double x_, double y_, double r_) : Point(x_, y_), r(r_) { id = 2; }\n"
                "    double r;\n"
                "};\n\n"
                "void helper(Object *obj)\n"
                "{\n"
                "    BREAK;\n"
                "    unused(obj);\n"
                "}\n",
                "Circle *circle = new Circle(1.5, -2.5, 3.0);\n"
                "Object *obj = circle;\n"
                "helper(circle);\n"
                "helper(obj);\n")
            + NoCdbEngine
            + CheckType("obj", "Circle");



    // http://www.qtcentre.org/threads/41700-How-to-watch-STL-containers-iterators-during-debugging
    QTest::newRow("QC41700")
        << Data("#include <map>\n"
                "#include <list>\n"
                "#include <string>\n"
                "using namespace std;\n"
                "typedef map<string, list<string> > map_t;\n",
                "map_t m;\n"
                "m[\"one\"].push_back(\"a\");\n"
                "m[\"one\"].push_back(\"b\");\n"
                "m[\"one\"].push_back(\"c\");\n"
                "m[\"two\"].push_back(\"1\");\n"
                "m[\"two\"].push_back(\"2\");\n"
                "m[\"two\"].push_back(\"3\");\n"
                "map_t::const_iterator it = m.begin();\n")
         + Check("m", "<2 items>", TypeDef("std::map<std::string, std::list<std::string>>","map_t"))
         + Check("m.0.first", "\"one\"", "std::string")
         + Check("m.0.second", "<3 items>", "std::list<std::string>")
         + Check("m.0.second.0", "[0]", "\"a\"", "std::string")
         + Check("m.0.second.1", "[1]", "\"b\"", "std::string")
         + Check("m.0.second.2", "[2]", "\"c\"", "std::string")
         + Check("m.1.first", "\"two\"", "std::string")
         + Check("m.1.second", "<3 items>", "std::list<std::string>")
         + Check("m.1.second.0", "[0]", "\"1\"", "std::string")
         + Check("m.1.second.1", "[1]", "\"2\"", "std::string")
         + Check("m.1.second.2", "[2]", "\"3\"", "std::string")
         + CheckType("it",  TypeDef("std::_Tree_const_iterator<std::_Tree_val<"
                                    "std::_Tree_simple_types<std::pair<"
                                    "std::string const ,std::list<std::string>>>>>",
                                    "std::map<std::string, std::list<std::string> >::const_iterator"))
         + Check("it.first", "\"one\"", "std::string") % NoCdbEngine
         + Check("it.second", "<3 items>", "std::list<std::string>") % NoCdbEngine
         + Check("it.0.first", "\"one\"", "std::string") % CdbEngine
         + Check("it.0.second", "<3 items>", "std::list<std::string>") % CdbEngine;

    QTest::newRow("Varargs")
            << Data("#include <stdarg.h>\n"
                    "void test(const char *format, ...)\n"
                    "{\n"
                    "    va_list arg;\n"
                    "    va_start(arg, format);\n"
                    "    int i = va_arg(arg, int);\n"
                    "    double f = va_arg(arg, double);\n"
                    "    va_end(arg);\n"
                    "    BREAK;\n"
                    "    unused(&i, &f);\n"
                    "}\n",
                    "test(\"abc\", 1, 2.0);\n")
               + Check("format", "\"abc\"", "char *")
               + Check("i", "1", "int")
               + Check("f", FloatValue("2"), "double");


    QString inheritanceData =
        "struct Empty {};\n"
        "struct Data { Data() : a(42) {} int a; };\n"
        "struct VEmpty {};\n"
        "struct VData { VData() : v(42) {} int v; };\n"

        "struct S1 : Empty, Data, virtual VEmpty, virtual VData\n"
        "    { S1() : i1(1) {} int i1; };\n"
        "struct S2 : Empty, Data, virtual VEmpty, virtual VData\n"
        "    { S2() : i2(1) {} int i2; };\n"
        "struct Combined : S1, S2 { Combined() : c(1) {} int c; };\n"

        "struct T1 : virtual VEmpty, virtual VData\n"
        "    { T1() : i1(1) {} int i1; };\n"
        "struct T2 : virtual VEmpty, virtual VData\n"
        "    { T2() : i2(1) {} int i2; };\n"
        "struct TT : T1, T2 { TT() : c(1) {} int c; };\n"

        "struct A { int a = 1; char aa = 'a'; };\n"
        "struct B : virtual A { int b = 2; float bb = 2; };\n"
        "struct C : virtual A { int c = 3; double cc = 3; };\n"
        "struct D : virtual B, virtual C { int d = 4; };\n";

    QTest::newRow("Inheritance")
            << Data(inheritanceData,
                    "Combined c;\n"
                    "c.S1::a = 42;\n"
                    "c.S2::a = 43;\n"
                    "c.S1::v = 44;\n"
                    "c.S2::v = 45;\n"
                    "unused(&c.S2::v);\n"
                    "TT tt;\n"
                    "tt.T1::v = 44;\n"
                    "tt.T2::v = 45;\n"
                    "unused(&tt.T2::v);\n"
                    "D dd; unused(&dd);\n"
                    "D *dp = new D; unused(&dp);\n"
                    "D &dr = dd; unused(&dr);\n")
                + Cxx11Profile()
                + Check("c.c", "1", "int")
                + Check("c.@1.@2.a", "42", "int") % NoLldbEngine
                + Check("c.@1.@4.v", "45", "int") % NoLldbEngine
                + Check("c.@2.@2.a", "43", "int") % NoLldbEngine
                + Check("c.@2.@4.v", "45", "int") % NoLldbEngine
                + Check("c.@1.@1.a", "42", "int") % LldbEngine
                + Check("c.@1.@2.v", "45", "int") % LldbEngine
                + Check("c.@2.@1.a", "43", "int") % LldbEngine
                + Check("c.@2.@2.v", "45", "int") % LldbEngine
                + Check("tt.c", "1", "int")
                + Check("tt.@1.@2.v", "45", "int") % NoLldbEngine
                + Check("tt.@2.@2.v", "45", "int") % NoLldbEngine
                + Check("tt.@1.@1.v", "45", "int") % LldbEngine
                + Check("tt.@2.@1.v", "45", "int") % LldbEngine

                + Check("dd.@1.@1.a", "1", "int") // B::a
                // C::a - fails with command line LLDB 3.8/360.x
                + Check("dd.@2.@1.a", "1", "int") % NoLldbEngine // C::a
                + Check("dd.@1.b", "2", "int")
                + Check("dd.@2.c", "3", "int")
                + Check("dd.d", "4", "int")

                + Check("dp.@1.@1.a", "1", "int") // B::a
                + Check("dp.@2.@1.a", "1", "int") % NoLldbEngine // C::a
                + Check("dp.@1.b", "2", "int")
                + Check("dp.@2.c", "3", "int")
                + Check("dp.d", "4", "int")

                + Check("dr.@1.@1.a", "1", "int") // B::a
                + Check("dr.@2.@1.a", "1", "int") % NoLldbEngine // C::a
                + Check("dr.@1.b", "2", "int")
                + Check("dr.@2.c", "3", "int")
                + Check("dr.d", "4", "int");


    QTest::newRow("Gdb13393")
            << Data(
                   "struct Base {\n"
                   "    Base() : a(1) {}\n"
                   "    virtual ~Base() {}  // Enforce type to have RTTI\n"
                   "    int a;\n"
                   "};\n"
                   "struct Derived : public Base {\n"
                   "    Derived() : b(2) {}\n"
                   "    int b;\n"
                   "};\n"
                   "struct S\n"
                   "{\n"
                   "    Base *ptr;\n"
                   "    const Base *ptrConst;\n"
                   "    Base &ref;\n"
                   "    const Base &refConst;\n"
                   "        S(Derived &d)\n"
                   "            : ptr(&d), ptrConst(&d), ref(d), refConst(d)\n"
                   "        {}\n"
                   "    };\n"
                   ,
                   "Derived d;\n"
                   "S s(d);\n"
                   "Base *ptr = &d;\n"
                   "const Base *ptrConst = &d;\n"
                   "Base &ref = d;\n"
                   "const Base &refConst = d;\n"
                   "Base **ptrToPtr = &ptr;\n"
                   "#if USE_BOOST\n"
                   "boost::shared_ptr<Base> sharedPtr(new Derived());\n"
                   "#else\n"
                   "int sharedPtr = 1;\n"
                   "#endif\n"
                   "unused(&ptrConst, &ref, &refConst, &ptrToPtr, &sharedPtr, &s);\n")

               + GdbEngine
               + GdbVersion(70500)
               + BoostProfile()

               + Check("d", "", "Derived")
               + Check("d.@1", "[Base]", "", "Base")
               + Check("d.b", "2", "int")
               + Check("ptr", "", "Derived")
               + Check("ptr.@1", "[Base]", "", "Base")
               + Check("ptr.@1.a", "1", "int")
               + Check("ptrConst", "", "Derived")
               + Check("ptrConst.@1", "[Base]", "", "Base")
               + Check("ptrConst.b", "2", "int")
               + Check("ptrToPtr", "", "Derived")
               //+ CheckType("ptrToPtr.[vptr]", " ")
               + Check("ptrToPtr.@1.a", "1", "int")
               + Check("ref", "", "Derived &")
               //+ CheckType("ref.[vptr]", "")
               + Check("ref.@1.a", "1", "int")
               + Check("refConst", "", "Derived &")
               //+ CheckType("refConst.[vptr]", "")
               + Check("refConst.@1.a", "1", "int")
               + Check("s", "", "S")
               + Check("s.ptr", "", "Derived")
               + Check("s.ptrConst", "", "Derived")
               + Check("s.ref", "", "Derived &")
               + Check("s.refConst", "", "Derived &")
               + Check("sharedPtr", "1", "int");


    // http://sourceware.org/bugzilla/show_bug.cgi?id=10586. fsf/MI errors out
    // on -var-list-children on an anonymous union. mac/MI was fixed in 2006");
    // The proposed fix has been reported to crash gdb steered from eclipse");
    // http://sourceware.org/ml/gdb-patches/2011-12/msg00420.html
    QTest::newRow("Gdb10586")
            << Data("",
                    "struct Test {\n"
                    "    struct { int a; float b; };\n"
                    "    struct { int c; float d; };\n"
                    "} v = {{1, 2}, {3, 4}};\n"
                    "unused(&v);\n")
               + Check("v", "", "Test") % NoCdbEngine
               + Check("v", "", TypePattern("main::.*::Test")) % CdbEngine
               //+ Check("v.a", "1", "int") % GdbVersion(0, 70699)
               //+ Check("v.0.a", "1", "int") % GdbVersion(70700)
               + Check("v.#1.a", "1", "int") % NoCdbEngine
               + Check("v.a", "1", "int") % CdbEngine;


    QTest::newRow("Gdb10586eclipse")
            << Data("",
                    "struct { int x; struct { int a; }; struct { int b; }; } "
                    "   v = {1, {2}, {3}};\n"
                    "struct S { int x, y; } n = {10, 20};\n"
                    "unused(&v, &n);\n")

               + Check("v", "", "{...}") % GdbEngine
               + Check("v", "", TypePattern(".*anonymous .*")) % LldbEngine
               + Check("v", "", TypePattern(".*<unnamed-type-.*")) % CdbEngine
               + Check("n", "", "S") % NoCdbEngine
               + Check("n", "", TypePattern("main::.*::S")) % CdbEngine
               //+ Check("v.a", "2", "int") % GdbVersion(0, 70699)
               //+ Check("v.0.a", "2", "int") % GdbVersion(70700)
               + Check("v.#1.a", "2", "int") % NoCdbEngine
               + Check("v.a", "2", "int") % CdbEngine
               //+ Check("v.b", "3", "int") % GdbVersion(0, 70699)
               //+ Check("v.1.b", "3", "int") % GdbVersion(70700)
               + Check("v.#2.b", "3", "int") % NoCdbEngine
               + Check("v.b", "3", "int") % CdbEngine
               + Check("v.x", "1", "int")
               + Check("n.x", "10", "int")
               + Check("n.y", "20", "int");


    QTest::newRow("StdInt")
            << Data("#include <stdint.h>\n",
                    "uint8_t u8 = 64;\n"
                    "int8_t s8 = 65;\n"
                    "uint16_t u16 = 66;\n"
                    "int16_t s16 = 67;\n"
                    "uint32_t u32 = 68;\n"
                    "int32_t s32 = 69;\n"
                    "unused(&u8, &s8, &u16, &s16, &u32, &s32);\n")
               + Check("u8", "64", TypeDef("unsigned char", "uint8_t"))
               + Check("s8", "65", TypeDef("char", "int8_t"))
               + Check("u16", "66", TypeDef("unsigned short", "uint16_t"))
               + Check("s16", "67", TypeDef("short", "int16_t"))
               + Check("u32", "68", TypeDef("unsigned int", "uint32_t"))
               + Check("s32", "69", TypeDef("int", "int32_t"));

    QTest::newRow("QPolygon")
            << Data("#include <QGraphicsScene>\n"
                    "#include <QGraphicsPolygonItem>\n"
                    "#include <QApplication>\n",
                    "QApplication app(argc, argv);\n"
                    "QGraphicsScene sc;\n"
                    "QPolygonF pol;\n"
                    "pol.append(QPointF(1, 2));\n"
                    "pol.append(QPointF(2, 2));\n"
                    "pol.append(QPointF(3, 3));\n"
                    "pol.append(QPointF(2, 4));\n"
                    "pol.append(QPointF(1, 4));\n"
                    "QGraphicsPolygonItem *p = sc.addPolygon(pol);\n"
                    "unused(&app, &p);\n")
               + GuiProfile()
               + Check("pol", "<5 items>", "@QPolygonF")
               + Check("p", "<5 items>", "@QGraphicsPolygonItem");

    QTest::newRow("QJson")
            << Data("#include <QString>\n"
                    "#if QT_VERSION >= 0x050000\n"
                    "#include <QJsonObject>\n"
                    "#include <QJsonArray>\n"
                    "#include <QJsonValue>\n"
                    "#include <QVariantMap>\n"
                    "#endif\n",
                    "#if QT_VERSION >= 0x050000\n"
                    "QJsonObject ob = QJsonObject::fromVariantMap({\n"
                    "    {\"a\", 1},\n"
                    "    {\"bb\", 2},\n"
                    "    {\"ccc\", \"hallo\"},\n"
                    "    {\"s\", \"ssss\"}\n"
                    "});\n"
                    "ob.insert(QLatin1String(\"d\"), QJsonObject::fromVariantMap({{\"ddd\", 1234}}));\n"
                    "\n"
                    "QJsonArray a;\n"
                    "a.append(QJsonValue(1));\n"
                    "a.append(QJsonValue(\"asd\"));\n"
                    "a.append(QJsonValue(QString::fromLatin1(\"cdfer\")));\n"
                    "a.append(QJsonValue(1.4));\n"
                    "a.append(QJsonValue(true));\n"
                    "a.append(ob);\n"
                    "\n"
                    "QJsonArray b;\n"
                    "b.append(QJsonValue(1));\n"
                    "b.append(a);\n"
                    "b.append(QJsonValue(2));\n"
                    "\n"
                    "QJsonArray c;\n"
                    "for (unsigned int i = 0; i < 32; ++i) {\n"
                    "    c.append(QJsonValue(qint64(1u << i) - 1));\n"
                    "    c.append(QJsonValue(qint64(1u << i)));\n"
                    "    c.append(QJsonValue(qint64(1u << i) + 1));\n"
                    "}\n"
                    "for (unsigned int i = 0; i < 32; ++i) {\n"
                    "    c.append(QJsonValue(-qint64(1u << i) + 1));\n"
                    "    c.append(QJsonValue(-qint64(1u << i)));\n"
                    "    c.append(QJsonValue(-qint64(1u << i) - 1));\n"
                    "}\n"
                    "\n"
                    "unused(&ob,&b,&a);\n"
                    "#endif\n")
            + Cxx11Profile()
            + CoreProfile()
            + QtVersion(0x50000)
            + MsvcVersion(1900)
            + Check("a",                  "<6 items>",  "@QJsonArray")
            + Check("a.0",   "[0]",       "1",            "QJsonValue (Number)")
            + Check("a.1",   "[1]",       "\"asd\"",      "QJsonValue (String)")
            + Check("a.2",   "[2]",       "\"cdfer\"",    "QJsonValue (String)")
            + Check("a.3",   "[3]",       "1.4",          "QJsonValue (Number)")
            + Check("a.4",   "[4]",       "true",         "QJsonValue (Bool)")
            + Check("a.5",   "[5]",       "<5 items>",    "QJsonValue (Object)")
            + Check("a.5.0",    "\"a\"",     "1",            "QJsonValue (Number)")
            + Check("a.5.1",    "\"bb\"",    "2",            "QJsonValue (Number)")
            + Check("a.5.2",    "\"ccc\"",   "\"hallo\"",    "QJsonValue (String)")
            + Check("a.5.3",    "\"d\"",     "<1 items>",    "QJsonValue (Object)")
            + Check("a.5.4",    "\"s\"",     "\"ssss\"",     "QJsonValue (String)")
            + Check("b",     "b",        "<3 items>" ,  "@QJsonArray")
            + Check("b.0",   "[0]",       "1",             "QJsonValue (Number)")
            + Check("b.1",   "[1]",       "<6 items>",     "QJsonValue (Array)")
            + Check("b.1.0",    "[0]",       "1",             "QJsonValue (Number)")
            + Check("b.1.1",    "[1]",       "\"asd\"",       "QJsonValue (String)")
            + Check("b.1.2",    "[2]",       "\"cdfer\"",     "QJsonValue (String)")
            + Check("b.1.3",    "[3]",       "1.4",           "QJsonValue (Number)")
            + Check("b.1.4",    "[4]",       "true",          "QJsonValue (Bool)")
            + Check("b.1.5",    "[5]",       "<5 items>",     "QJsonValue (Object)")
            + Check("b.2",   "[2]",       "2",             "QJsonValue (Number)")
            + Check("c",     "c",        "<192 items>", "@QJsonArray")
            + Check("c.0",   "[0]",       "0.0",           "QJsonValue (Number)")
            + Check("c.1",   "[1]",       "1",             "QJsonValue (Number)")
            + Check("c.78",  "[78]",      "67108863",      "QJsonValue (Number)")
            + Check("c.79",  "[79]",      "67108864.0",    "QJsonValue (Number)")
            + Check("c.94",  "[94]",      "2147483648.0",  "QJsonValue (Number)")
            + Check("c.95",  "[95]",      "2147483649.0",  "QJsonValue (Number)")
            + Check("c.96",  "[96]",      "0.0",           "QJsonValue (Number)")
            + Check("c.97",  "[97]",      "-1",            "QJsonValue (Number)")
            + Check("c.174", "[174]",     "-67108863",     "QJsonValue (Number)")
            + Check("c.175", "[175]",     "-67108864.0",   "QJsonValue (Number)")
            + Check("ob",   "ob",      "<5 items>",     "@QJsonObject")
            + Check("ob.0", "\"a\"",    "1",              "QJsonValue (Number)")
            + Check("ob.1", "\"bb\"",   "2",              "QJsonValue (Number)")
            + Check("ob.2", "\"ccc\"",  "\"hallo\"",      "QJsonValue (String)")
            + Check("ob.3", "\"d\"",    "<1 items>",      "QJsonValue (Object)")
            + Check("ob.4", "\"s\"",    "\"ssss\"",       "QJsonValue (String)");

    QTest::newRow("QV4")
            << Data("#include <private/qv4value_p.h>\n"
                    "#include <private/qjsvalue_p.h>\n"
                    "#include <QCoreApplication>\n"
                    "#include <QJSEngine>\n",
                    "QCoreApplication app(argc, argv);\n"
                    "QJSEngine eng;\n\n"
                    "//QV4::Value q0; unused(&q0); // Uninitialized data.\n\n"
                    "//QV4::Value q1; unused(&q1); // Upper 32 bit uninitialized.\n"
                    "//q1.setInt_32(1);\n\n"
                    "QV4::Value q2; unused(&q2);\n"
                    "q2.setDouble(2.5);\n\n"
                    "QJSValue v10; unused(&v10);\n"
                    "QJSValue v11 = QJSValue(true); unused(&v11);\n"
                    "QJSValue v12 = QJSValue(1); unused(&v12);\n"
                    "QJSValue v13 = QJSValue(2.5); unused(&v13);\n"
                    "QJSValue v14 = QJSValue(QLatin1String(\"latin1\")); unused(&v14);\n"
                    "QJSValue v15 = QJSValue(QString(\"utf16\")); unused(&v15);\n"
                    "QJSValue v16 = QJSValue(bool(true)); unused(&v16);\n"
                    "QJSValue v17 = eng.newArray(100); unused(&v17);\n"
                    "QJSValue v18 = eng.newObject(); unused(&v18);\n\n"
                    "v18.setProperty(\"PropA\", 1);\n"
                    "v18.setProperty(\"PropB\", 2.5);\n"
                    "v18.setProperty(\"PropC\", v10);\n\n"
                    "QV4::Value s11, *p11 = QJSValuePrivate::valueForData(&v11, &s11); unused(&p11);\n"
                    "QV4::Value s12, *p12 = QJSValuePrivate::valueForData(&v12, &s12); unused(&p12);\n"
                    "QV4::Value s13, *p13 = QJSValuePrivate::valueForData(&v13, &s13); unused(&p13);\n"
                    "QV4::Value s14, *p14 = QJSValuePrivate::valueForData(&v14, &s14); unused(&p14);\n"
                    "QV4::Value s15, *p15 = QJSValuePrivate::valueForData(&v15, &s15); unused(&p15);\n"
                    "QV4::Value s16, *p16 = QJSValuePrivate::valueForData(&v16, &s16); unused(&p16);\n"
                    "QV4::Value s17, *p17 = QJSValuePrivate::valueForData(&v17, &s17); unused(&p17);\n"
                    "QV4::Value s18, *p18 = QJSValuePrivate::valueForData(&v18, &s18); unused(&p18);\n"
                    )
            + QmlPrivateProfile()
            + QtVersion(0x50000)
            + Check("q2", FloatValue("2.5"), "@QV4::Value (double)")
            //+ Check("v10", "(null)", "@QJSValue (null)") # Works in GUI. Why?
            + Check("v11", "true", "@QJSValue (bool)")
            + Check("v12", "1", "@QJSValue (int)")
            + Check("v13", FloatValue("2.5"), "@QJSValue (double)")
            + Check("v14", "\"latin1\"", "@QJSValue (QString)")
            + Check("v14.2", "[2]", "116", "@QChar")
            + Check("v15", "\"utf16\"", "@QJSValue (QString)")
            + Check("v15.1", "[1]", "116", "@QChar");


    QTest::newRow("QStandardItem")
            << Data("#include <QStandardItemModel>",
                    "QStandardItemModel m;\n"
                    "QStandardItem *root = m.invisibleRootItem();\n"
                    "for (int i = 0; i < 4; ++i) {\n"
                    "    QStandardItem *item = new QStandardItem(QString(\"item %1\").arg(i));\n"
                    "    item->setData(123);\n"
                    "    root->appendRow(item);\n"
                    "}\n")
            + GuiProfile()
            + Check("root.[children].0.[values].0.role", "Qt::DisplayRole (0)", "@Qt::ItemDataRole")
            + Check("root.[children].0.[values].0.value", "\"item 0\"", "@QVariant (QString)");


    QTest::newRow("Internal1")
            << Data("struct QtcDumperTest_FieldAccessByIndex { int d[3] = { 10, 11, 12 }; };\n",
                    "QtcDumperTest_FieldAccessByIndex d; unused(&d);\n")
            + MsvcVersion(1900)
            + Check("d", "12", "QtcDumperTest_FieldAccessByIndex");


    QTest::newRow("Internal2")
            << Data("struct Foo { int bar = 15; }; \n"
                    "struct QtcDumperTest_PointerArray {\n"
                    "   Foo *foos = new Foo[10];\n"
                    "};\n\n",
                    "QtcDumperTest_PointerArray tc; unused(&tc);\n")
            + Check("tc.0.bar", "15", "int")
            + Check("tc.1.bar", "15", "int")
            + Check("tc.2.bar", "15", "int")
            + Check("tc.3.bar", "15", "int");

    QTest::newRow("ArrayOfFunctionPointers")
            << Data("typedef int (*FP)(int *); \n"
                    "int func(int *param) { unused(param); return 0; }  \n",
                    "FP fps[5]; fps[0] = func; fps[0](0); unused(&fps);\n")
            + RequiredMessage("Searching for type int (*)(int *) across all target modules, this could be very slow")
            + LldbEngine;

    QTest::newRow("Sql")
            << Data("#include <QSqlField>\n"
                    "#include <QSqlDatabase>\n"
                    "#include <QSqlQuery>\n"
                    "#include <QSqlRecord>\n",
                    "QSqlDatabase db = QSqlDatabase::addDatabase(\"QSQLITE\");\n"
                    "db.setDatabaseName(\":memory:\");\n"
                    "Q_ASSERT(db.open());\n"
                    "QSqlQuery query;\n"
                    "query.exec(\"create table images (itemid int, file varchar(20))\");\n"
                    "query.exec(\"insert into images values(1, 'qt-logo.png')\");\n"
                    "query.exec(\"insert into images values(2, 'qt-creator.png')\");\n"
                    "query.exec(\"insert into images values(3, 'qt-project.png')\");\n"
                    "query.exec(\"select * from images\");\n"
                    "query.next();\n"
                    "QSqlRecord rec = query.record();\n"
                    "QSqlField f1 = rec.field(0);\n"
                    "QSqlField f2 = rec.field(1);\n"
                    "QSqlField f3 = rec.field(2);\n"
                    "unused(&f1, &f2, &f3);\n")
            + SqlProfile()
            + Check("f1", "1", "@QSqlField (qlonglong)")
            + Check("f2", "\"qt-logo.png\"", "@QSqlField (QString)")
            + Check("f3", "(invalid)", "@QSqlField (invalid)");
#if 0
#ifdef Q_OS_LINUX
    // Hint: To open a failing test in Creator, do:
    //  touch qt_tst_dumpers_Nim_.../dummy.nimproject
    //  qtcreator qt_tst_dumpers_Nim_*/dummy.nimproject
    Data nimData;
    nimData.configTest = "which nim";
    nimData.allProfile =
        "CONFIG -= qt\n"
        "SOURCES += main.nim\n"
        "# Prevents linking\n"
        "TARGET=\n"
        "# Overwrites qmake-generated 'all' target.\n"
        "all.commands = nim c --debugInfo --lineDir:on --out:doit main.nim\n"
        "all.depends = main.nim\n"
        "all.CONFIG = phony\n\n"
        "QMAKE_EXTRA_TARGETS += all\n";
    nimData.allCode =
        "type Mirror = ref object\n"
        "  tag:int\n"
        "  other:array[0..1, Mirror]\n\n"
        "proc mainProc =\n"
        "  var name: string = \"Hello World\"\n"
        "  var i: int = 43\n"
        "  var j: int\n"
        "  var x: seq[int]\n"
        "  x = @[1, 2, 3, 4, 5, 6]\n\n"
        "  j = i + name.len()\n"
        "  # Crash it.\n"
        "  var m1 = Mirror(tag:1)\n"
        "  var m2 = Mirror(tag:2)\n"
        "  var m3 = Mirror(tag:3)\n\n"
        "  m1.other[0] = m2; m1.other[1] = m3\n"
        "  m2.other[0] = m1; m2.other[1] = m3\n"
        "  m3.other[0] = m1; m3.other[1] = m2\n\n"
        "  for i in 1..30000:\n"
        //"    echo i\n"
        "    var mx : Mirror; mx.deepCopy(m1)\n"
        "    m1 = mx\n\n"
        "if isMainModule:\n"
        "  mainProc()\n";
    nimData.mainFile = "main.nim";
    nimData.skipLevels = 15;

    QTest::newRow("Nim")
        << nimData
        + GdbEngine
        + Check("name", "\"Hello World\"", "NimStringDesc")
        + Check("x", "<6 items>", TypePattern("TY.*NI.6..")) // Something like "TY95019 (NI[6])"
        + Check("x.2", "[2]", "3", "NI");
#endif
#endif
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    tst_Dumpers test;
    return QTest::qExec(&test, argc, argv);
}

#include "tst_dumpers.moc"
