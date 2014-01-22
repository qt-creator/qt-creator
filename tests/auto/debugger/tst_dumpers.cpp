/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "debuggerprotocol.h"
#include "simplifytype.h"
#include "watchdata.h"
#include "watchutils.h"

#ifdef Q_CC_MSVC
#include <utils/environment.h>
#include <utils/qtcprocess.h>
#include <utils/fileutils.h>
#include <utils/synchronousprocess.h>
#endif // Q_CC_MSVC

#include "temporarydir.h"

#include <QtTest>
#include <math.h>

#if  QT_VERSION >= 0x050000
#define MSKIP_SINGLE(x) do { disarm(); QSKIP(x); } while (0)
#else
#define MSKIP_SINGLE(x) do { disarm(); QSKIP(x, SkipSingle); } while (0)
#endif

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

static void setupCdb(QString *makeBinary, QProcessEnvironment *environment)
{
    QByteArray envBat = qgetenv("QTC_MSVC_ENV_BAT");
    QMap <QString, QString> envPairs;
    Utils::Environment env = Utils::Environment::systemEnvironment();
    QVERIFY(generateEnvironmentSettings(env, QString::fromLatin1(envBat), QString(), envPairs));
    for (QMap<QString,QString>::const_iterator envIt = envPairs.begin(); envIt != envPairs.end(); ++envIt)
            env.set(envIt.key(), envIt.value());
    const QByteArray cdbextPath = CDBEXT_PATH "\\qtcreatorcdbext64";
    QVERIFY(QFile::exists(QString::fromLatin1(cdbextPath + QByteArray("\\qtcreatorcdbext.dll"))));
    env.appendOrSet(QLatin1String("_NT_DEBUGGER_EXTENSION_PATH"),
                         QString::fromLatin1(cdbextPath),
                         QLatin1String(";"));
    *makeBinary = env.searchInPath(QLatin1String("nmake.exe"));
    *environment = env.toProcessEnvironment();
}

#else

static void setupCdb(QString *, QProcessEnvironment *) {}

#endif // Q_CC_MSVC


static QByteArray noValue = "\001";

static QString toHex(const QString &str)
{
    QString encoded;
    foreach (const QChar c, str) {
        encoded += QString::fromLatin1("%1")
            .arg(c.unicode(), 2, 16, QLatin1Char('0'));
    }
    return encoded;
}

struct Context
{
    Context() : qtVersion(0), gccVersion(0) {}

    QByteArray nameSpace;
    int qtVersion;
    int gccVersion;
};

struct Name
{
    Name() : name(noValue) {}
    Name(const char *str) : name(str) {}
    Name(const QByteArray &ba) : name(ba) {}

    bool matches(const QByteArray &actualName0, const Context &context) const
    {
        QByteArray actualName = actualName0;
        QByteArray expectedName = name;
        expectedName.replace("@Q", context.nameSpace + 'Q');
        return actualName == expectedName;
    }

    QByteArray name;
};

static Name nameFromIName(const QByteArray &iname)
{
    int pos = iname.lastIndexOf('.');
    return Name(pos == -1 ? iname : iname.mid(pos + 1));
}

static QByteArray parentIName(const QByteArray &iname)
{
    int pos = iname.lastIndexOf('.');
    return pos == -1 ? QByteArray() : iname.left(pos);
}

struct ValueBase
{
    ValueBase()
      : hasPtrSuffix(false), isFloatValue(false), substituteNamespace(true),
        qtVersion(0), minimalGccVersion(0)
    {}

    bool hasPtrSuffix;
    bool isFloatValue;
    bool substituteNamespace;
    int qtVersion;
    int minimalGccVersion;
};

struct Value : public ValueBase
{
    Value() : value(QString::fromLatin1(noValue)) {}
    Value(const char *str) : value(QLatin1String(str)) {}
    Value(const QByteArray &ba) : value(QString::fromLatin1(ba.data(), ba.size())) {}
    Value(const QString &str) : value(str) {}

    bool matches(const QString &actualValue0, const Context &context) const
    {
        if (value == QString::fromLatin1(noValue))
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
        if (minimalGccVersion && context.gccVersion) {
            if (minimalGccVersion >= context.gccVersion) {
                //QWARN("Current GCC is too old for this test.")
                return true;
            }
        }
        QString actualValue = actualValue0;
        if (actualValue == QLatin1String(" "))
            actualValue.clear(); // FIXME: Remove later.
        QString expectedValue = value;
        if (substituteNamespace)
            expectedValue.replace(QLatin1Char('@'), QString::fromLatin1(context.nameSpace));

        if (hasPtrSuffix)
            return actualValue.startsWith(expectedValue + QLatin1String(" @0x"))
                || actualValue.startsWith(expectedValue + QLatin1String("@0x"));

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

    void setMinimalGccVersion(int version) { minimalGccVersion = version; }

    QString value;
};

struct Pointer : Value
{
    Pointer() { hasPtrSuffix = true; }
    Pointer(const QByteArray &value) : Value(value) { hasPtrSuffix = true; }
};

struct FloatValue : Value
{
    FloatValue() { isFloatValue = true; }
    FloatValue(const QByteArray &value) : Value(value) { isFloatValue = true; }
};

struct Value4 : Value
{
    Value4(const QByteArray &value) : Value(value) { qtVersion = 4; }
};

struct Value5 : Value
{
    Value5(const QByteArray &value) : Value(value) { qtVersion = 5; }
};

struct UnsubstitutedValue : Value
{
    UnsubstitutedValue(const QByteArray &value) : Value(value) { substituteNamespace = false; }
};

struct Type
{
    Type() : qtVersion(0), isPattern(false) {}
    Type(const char *str) : type(str), qtVersion(0), isPattern(false) {}
    Type(const QByteArray &ba) : type(ba), qtVersion(0), isPattern(false) {}

    bool matches(const QByteArray &actualType0, const Context &context) const
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
        QByteArray actualType =
            simplifyType(QString::fromLatin1(actualType0)).toLatin1();
        actualType.replace(' ', "");
        actualType.replace("const", "");
        QByteArray expectedType = type;
        expectedType.replace(' ', "");
        expectedType.replace("const", "");
        expectedType.replace('@', context.nameSpace);
        if (isPattern) {
            QString actual = QString::fromLatin1(actualType);
            QString expected = QString::fromLatin1(expectedType);
            return QRegExp(expected).exactMatch(actual);
        }
        return actualType == expectedType;
    }
    QByteArray type;
    int qtVersion;
    bool isPattern;
};

struct Type4 : Type
{
    Type4(const QByteArray &ba) : Type(ba) { qtVersion = 4; }
};

struct Type5 : Type
{
    Type5(const QByteArray &ba) : Type(ba) { qtVersion = 5; }
};

struct Pattern : Type
{
    Pattern(const QByteArray &ba) : Type(ba) { isPattern = true; }
};

enum DebuggerEngine
{
    GdbEngine = 0x01,
    CdbEngine = 0x02,
    LldbEngine = 0x04,

    AllEngines = GdbEngine | CdbEngine | LldbEngine,

    NoCdbEngine = AllEngines & (~CdbEngine),
    NoLldbEngine = AllEngines & (~LldbEngine),
    NoGdbEngine = AllEngines & (~GdbEngine)
};

struct CheckBase
{
    CheckBase() : enginesForCheck(AllEngines) {}
    mutable int enginesForCheck;
};

struct Check : CheckBase
{
    Check() {}

    Check(const QByteArray &iname, const Value &value, const Type &type)
        : iname(iname), expectedName(nameFromIName(iname)),
          expectedValue(value), expectedType(type)
    {}

    Check(const QByteArray &iname, const Name &name,
         const Value &value, const Type &type)
        : iname(iname), expectedName(name),
          expectedValue(value), expectedType(type)
    {}

    bool matchesEngine(DebuggerEngine engine) const
    {
        return (engine & enginesForCheck);
    }

    const Check &setEngines(int debuggerEngine)
    {
        enginesForCheck = debuggerEngine;
        return *this;
    }

    const Check &setForLldbOnly() const
    {
        enginesForCheck = LldbEngine;
        return *this;
    }

    const Check &setForGdbOnly() const
    {
        enginesForCheck = GdbEngine;
        return *this;
    }

    const Check &setForCdbOnly()
    {
        enginesForCheck = CdbEngine;
        return *this;
    }

    QByteArray iname;
    Name expectedName;
    Value expectedValue;
    Type expectedType;
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

struct Profile
{
    Profile(const QByteArray &contents) : contents(contents) {}

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
      : Profile("macx:INCLUDEPATH += /usr/local/include")
    {}
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

struct VersionBase
{
    // Minimum and maximum are inclusive.
    VersionBase(int minimum = 0, int maximum = INT_MAX)
    {
        isRestricted = minimum != 0 || maximum != INT_MAX;
        max = maximum;
        min = minimum;
    }

    bool isRestricted;
    int min;
    int max;
};

struct QtVersion : VersionBase
{
    QtVersion(int minimum = 0, int maximum = INT_MAX)
        : VersionBase(minimum, maximum)
    {}
};

struct GccVersion : VersionBase
{
    GccVersion(int minimum = 0, int maximum = INT_MAX)
        : VersionBase(minimum, maximum)
    {}
};

struct GdbVersion : VersionBase
{
    GdbVersion(int minimum = 0, int maximum = INT_MAX)
        : VersionBase(minimum, maximum)
    {}
};

struct LldbVersion : VersionBase
{
    LldbVersion(int minimum = 0, int maximum = INT_MAX)
        : VersionBase(minimum, maximum)
    {}
};

struct ForceC {};
struct EigenProfile {};
struct UseDebugImage {};

struct CoreProfile {};
struct CorePrivateProfile {};
struct GuiProfile {};
struct NetworkProfile {};

struct DataBase
{
    DataBase()
      : useQt(false), useQHash(false),
        forceC(false), engines(GdbEngine | CdbEngine | LldbEngine),
        glibcxxDebug(false), useDebugImage(false)
    {}

    mutable bool useQt;
    mutable bool useQHash;
    mutable bool forceC;
    mutable int engines;
    mutable bool glibcxxDebug;
    mutable bool useDebugImage;
    mutable GdbVersion neededGdbVersion;
    mutable LldbVersion neededLldbVersion;
    mutable QtVersion neededQtVersion;
    mutable GccVersion neededGccVersion;
};

class Data : public DataBase
{
public:
    Data() {}
    Data(const QByteArray &code) : code(code) {}

    Data(const QByteArray &includes, const QByteArray &code)
        : includes(includes), code(code)
    {}

    const Data &operator%(const Check &check) const
    {
        checks.append(check);
        return *this;
    }

    const Data &operator%(const Profile &profile) const
    {
        profileExtra += profile.contents;
        return *this;
    }

    const Data &operator%(const QtVersion &qtVersion) const
    {
        neededQtVersion = qtVersion;
        return *this;
    }

    const Data &operator%(const GccVersion &gccVersion) const
    {
        neededGccVersion = gccVersion;
        return *this;
    }

    const Data &operator%(const GdbVersion &gdbVersion) const
    {
        neededGdbVersion = gdbVersion;
        return *this;
    }

    const Data &operator%(const LldbVersion &lldbVersion) const
    {
        neededLldbVersion = lldbVersion;
        return *this;
    }

    const Data &operator%(const DebuggerEngine &enginesForTest) const
    {
        engines = enginesForTest;
        return *this;
    }

    const Data &operator%(const EigenProfile &) const
    {
        profileExtra +=
            "INCLUDEPATH += /usr/include/eigen2\n"
            "INCLUDEPATH += /usr/include/eigen3\n";

        return *this;
    }

    const Data &operator%(const UseDebugImage &) const
    {
        useDebugImage = true;
        return *this;
    }

    const Data &operator%(const CoreProfile &) const
    {
        profileExtra +=
            "CONFIG += QT\n"
            "QT += core\n";

        useQt = true;
        useQHash = true;

        return *this;
    }

    const Data &operator%(const NetworkProfile &) const
    {
        profileExtra +=
            "CONFIG += QT\n"
            "QT += core network\n";

        useQt = true;
        useQHash = true;

        return *this;
    }

    const Data &operator%(const GuiProfile &) const
    {
        this->operator%(CoreProfile());
        profileExtra +=
            "QT += gui\n"
            "greaterThan(QT_MAJOR_VERSION, 4):QT *= widgets\n";

        return *this;
    }

    const Data &operator%(const CorePrivateProfile &) const
    {
        this->operator%(CoreProfile());
        profileExtra +=
            "greaterThan(QT_MAJOR_VERSION, 4) {\n"
            "  QT += core-private\n"
            "  CONFIG += no_private_qt_headers_warning\n"
            "}";

        return *this;
    }

    const Data &operator%(const ForceC &) const
    {
        forceC = true;
        return *this;
    }

public:
    mutable QByteArray profileExtra;
    mutable QByteArray includes;
    mutable QByteArray code;
    mutable QList<Check> checks;
};

struct TempStuff
{
    TempStuff() : buildTemp(QLatin1String("qt_tst_dumpers_"))
    {
        buildPath = QDir::currentPath() + QLatin1Char('/')  + buildTemp.path();
        buildTemp.setAutoRemove(false);
        QVERIFY(!buildPath.isEmpty());
    }

    QByteArray input;
    QTemporaryDir buildTemp;
    QString buildPath;
};

Q_DECLARE_METATYPE(Data)

class tst_Dumpers : public QObject
{
    Q_OBJECT

public:
    tst_Dumpers()
    {
        t = 0;
        m_keepTemp = true;
        m_forceKeepTemp = false;
        m_gdbVersion = 0;
        m_gdbBuildVersion = 0;
        m_lldbVersion = 0;
        m_qtVersion = 0;
        m_gccVersion = 0;
        m_isMacGdb = false;
        m_isQnxGdb = false;
        m_useGLibCxxDebug = false;
    }

private slots:
    void initTestCase();
    void dumper();
    void dumper_data();
    void init();
    void cleanup();

private:
    void disarm() { t->buildTemp.setAutoRemove(!keepTemp()); }
    bool keepTemp() const { return m_keepTemp || m_forceKeepTemp; }
    TempStuff *t;
    QByteArray m_debuggerBinary;
    QByteArray m_qmakeBinary;
    QProcessEnvironment m_env;
    DebuggerEngine m_debuggerEngine;
    QString m_makeBinary;
    bool m_keepTemp;
    bool m_forceKeepTemp;
    int m_gdbVersion; // 7.5.1 -> 70501
    int m_gdbBuildVersion;
    int m_lldbVersion;
    int m_qtVersion; // 5.2.0 -> 50200
    int m_gccVersion;
    bool m_isMacGdb;
    bool m_isQnxGdb;
    bool m_useGLibCxxDebug;
};

void tst_Dumpers::initTestCase()
{
    m_debuggerBinary = qgetenv("QTC_DEBUGGER_PATH_FOR_TEST");
    if (m_debuggerBinary.isEmpty())
        m_debuggerBinary = "gdb";
    qDebug() << "Debugger           : " << m_debuggerBinary.constData();

    m_debuggerEngine = GdbEngine;
    if (m_debuggerBinary.endsWith("cdb.exe"))
        m_debuggerEngine = CdbEngine;

    QString base = QFileInfo(QString::fromLatin1(m_debuggerBinary)).baseName();
    if (base.startsWith(QLatin1String("lldb")))
        m_debuggerEngine = LldbEngine;

    m_qmakeBinary = qgetenv("QTC_QMAKE_PATH_FOR_TEST");
    if (m_qmakeBinary.isEmpty())
        m_qmakeBinary = "qmake";
    qDebug() << "QMake              : " << m_qmakeBinary.constData();

    m_useGLibCxxDebug = qgetenv("QTC_USE_GLIBCXXDEBUG_FOR_TEST").toInt();
    qDebug() << "Use _GLIBCXX_DEBUG : " << m_useGLibCxxDebug;

    m_forceKeepTemp = qgetenv("QTC_KEEP_TEMP_FOR_TEST").toInt();
    qDebug() << "Force keep temp    : " << m_forceKeepTemp;

    if (m_debuggerEngine == GdbEngine) {
        QProcess debugger;
        debugger.start(QString::fromLatin1(m_debuggerBinary)
                       + QLatin1String(" -i mi -quiet -nx"));
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
        int pos1 = version.indexOf(QLatin1String("&\"show version\\n"));
        QVERIFY(pos1 != -1);
        pos1 += 20;
        int pos2 = version.indexOf(QLatin1String("~\"Copyright (C) "), pos1);
        QVERIFY(pos2 != -1);
        pos2 -= 4;
        version = version.mid(pos1, pos2 - pos1);
        extractGdbVersion(version, &m_gdbVersion,
            &m_gdbBuildVersion, &m_isMacGdb, &m_isQnxGdb);
        m_env = QProcessEnvironment::systemEnvironment();
        m_makeBinary = QLatin1String("make");
        qDebug() << "Gdb version        : " << m_gdbVersion;
    } else if (m_debuggerEngine == CdbEngine) {
        setupCdb(&m_makeBinary, &m_env);
    } else if (m_debuggerEngine == LldbEngine) {
        QProcess debugger;
        QString cmd = QString::fromUtf8(m_debuggerBinary + " -v");
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
        m_lldbVersion = ba.toInt();
        if (!m_lldbVersion) {
            if (output.startsWith("lldb version")) {
                int pos1 = output.indexOf('.', 13);
                int major = output.mid(13, pos1++ - 13).toInt();
                int pos2 = output.indexOf(' ', pos1);
                int minor = output.mid(pos1, pos2++ - pos1).toInt();
                m_lldbVersion = 100 * major + 10 * minor;
            }
        }

        qDebug() << "Lldb version       :" << output << ba << m_lldbVersion;
        QVERIFY(m_lldbVersion);

        m_env = QProcessEnvironment::systemEnvironment();
        m_makeBinary = QLatin1String("make");
    }
}

void tst_Dumpers::init()
{
    t = new TempStuff();
}

void tst_Dumpers::cleanup()
{
    if (!t->buildTemp.autoRemove()) {
        QFile logger(t->buildPath + QLatin1String("/input.txt"));
        logger.open(QIODevice::ReadWrite);
        logger.write(t->input);
    }
    delete t;
}

void tst_Dumpers::dumper()
{
    QFETCH(Data, data);

    if (!(data.engines & m_debuggerEngine))
        MSKIP_SINGLE("The test is excluded for this debugger engine.");

    if (data.neededGdbVersion.isRestricted && m_debuggerEngine == GdbEngine) {
        if (data.neededGdbVersion.min > m_gdbVersion)
            MSKIP_SINGLE("Need minimum GDB version "
                + QByteArray::number(data.neededGdbVersion.min));
        if (data.neededGdbVersion.max < m_gdbVersion)
            MSKIP_SINGLE("Need maximum GDB version "
                + QByteArray::number(data.neededGdbVersion.max));
    }

    if (data.neededLldbVersion.isRestricted && m_debuggerEngine == LldbEngine) {
        if (data.neededLldbVersion.min > m_lldbVersion)
            MSKIP_SINGLE("Need minimum LLDB version "
                + QByteArray::number(data.neededLldbVersion.min));
        if (data.neededLldbVersion.max < m_lldbVersion)
            MSKIP_SINGLE("Need maximum LLDB version "
                + QByteArray::number(data.neededLldbVersion.max));
    }

    QString cmd;
    QByteArray output;
    QByteArray error;

    if (data.neededQtVersion.isRestricted) {
        QProcess qmake;
        qmake.setWorkingDirectory(t->buildPath);
        cmd = QString::fromLatin1(m_qmakeBinary);
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
        m_qtVersion = 10000 * major + 100 * minor + patch;

        if (data.neededQtVersion.min > m_qtVersion)
            MSKIP_SINGLE("Need minimum Qt version "
                + QByteArray::number(data.neededQtVersion.min));
        if (data.neededQtVersion.max < m_qtVersion)
            MSKIP_SINGLE("Need maximum Qt version "
                + QByteArray::number(data.neededQtVersion.max));
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
        pos = output.lastIndexOf(' ');
        output = output.mid(pos + 1);
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

    const char *mainFile = data.forceC ? "main.c" : "main.cpp";

    QFile proFile(t->buildPath + QLatin1String("/doit.pro"));
    QVERIFY(proFile.open(QIODevice::ReadWrite));
    proFile.write("SOURCES = ");
    proFile.write(mainFile);
    proFile.write("\nTARGET = doit\n");
    proFile.write("\nCONFIG -= app_bundle\n");
    proFile.write("\nCONFIG -= release\n");
    proFile.write("\nCONFIG += debug\n");
    if (data.useQt)
        proFile.write("QT -= widgets gui\n");
    else
        proFile.write("CONFIG -= QT\n");
    if (m_useGLibCxxDebug)
        proFile.write("DEFINES += _GLIBCXX_DEBUG\n");
    proFile.write(data.profileExtra);
    proFile.close();

    QFile source(t->buildPath + QLatin1Char('/') + QLatin1String(mainFile));
    QVERIFY(source.open(QIODevice::ReadWrite));
    QByteArray fullCode = QByteArray() +
            "\n\nvoid unused(const void *first,...) { (void) first; }"
            "\n\n#if defined(_MSC_VER)" + (data.useQt ?
                "\n#include <qt_windows.h>" :
                "\n#include <Windows.h>") +
            "\n#define BREAK do { DebugBreak(); } while (0)"
            "\n#else"
            "\n#define BREAK do { asm(\"int $3\"); } while (0)"
            "\n#endif"
            "\n"
            "\n\n" + data.includes +
            "\n\n" + (data.useQHash ?
                "\n#include <QByteArray>"
                "\n#if QT_VERSION >= 0x050000"
                "\nQT_BEGIN_NAMESPACE"
                "\nQ_CORE_EXPORT extern QBasicAtomicInt qt_qhash_seed; // from qhash.cpp"
                "\nQT_END_NAMESPACE"
                "\n#endif" : "") +
            "\n\nint main(int argc, char *argv[])"
            "\n{"
            "\n    int qtversion = " + (data.useQt ? "QT_VERSION" : "0") + ";"
            "\n#ifdef __GNUC__"
            "\n    int gccversion = 0x10000 * __GNUC__ + 0x100 * __GNUC_MINOR__;"
            "\n#else"
            "\n    int gccversion = 0;"
            "\n#endif"
            "\n" + (data.useQHash ?
                "\n#if QT_VERSION >= 0x050000"
                "\nqt_qhash_seed.store(0);"
                "\n#endif\n" : "") +
            "\n    unused(&argc, &argv, &qtversion, &gccversion);\n"
            "\n" + data.code +
            "\n    BREAK;"
            "\n    return 0;"
            "\n}\n";
    source.write(fullCode);
    source.close();

    QProcess qmake;
    qmake.setWorkingDirectory(t->buildPath);
    cmd = QString::fromLatin1(m_qmakeBinary);
    //qDebug() << "Starting qmake: " << cmd;
    qmake.start(cmd);
    QVERIFY(qmake.waitForFinished());
    output = qmake.readAllStandardOutput();
    error = qmake.readAllStandardError();
    //qDebug() << "stdout: " << output;
    if (!error.isEmpty()) { qDebug() << error; QVERIFY(false); }

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
        qDebug() << ".pro: " << qPrintable(proFile.fileName());
    }

    QByteArray dumperDir = DUMPERDIR;

    QSet<QByteArray> expandedINames;
    expandedINames.insert("local");
    foreach (const Check &check, data.checks) {
        QByteArray parent = check.iname;
        while (true) {
            parent = parentIName(parent);
            if (parent.isEmpty())
                break;
            expandedINames.insert("local." + parent);
        }
    }

    QByteArray expanded;
    foreach (const QByteArray &iname, expandedINames) {
        if (!expanded.isEmpty())
            expanded.append(',');
        expanded += iname;
    }

    QByteArray exe;
    QStringList args;
    QByteArray cmds;

    if (m_debuggerEngine == GdbEngine) {
        exe = m_debuggerBinary;

        const QFileInfo gdbBinaryFile(QString::fromLatin1(exe));
        const QByteArray uninstalledData = gdbBinaryFile.absolutePath().toLocal8Bit() + "/data-directory/python";

        args << QLatin1String("-i")
             << QLatin1String("mi")
             << QLatin1String("-quiet")
             << QLatin1String("-nx");
        QByteArray nograb = "-nograb";

        cmds = "set confirm off\n"
                "file doit\n"
                "set print object on\n"
                "set auto-load python-scripts no\n";

        cmds += "python sys.path.insert(1, '" + dumperDir + "')\n"
                "python sys.path.append('" + uninstalledData + "')\n"
                "python from gdbbridge import *\n"
                "run " + nograb + "\n"
                "python print('@%sS@%s@' % ('N', theDumper.qtNamespace()))\n"
                "bb options:fancy,autoderef,dyntype,pe vars: expanded:" + expanded + " typeformats:\n";

        cmds += "quit\n";

    } else if (m_debuggerEngine == CdbEngine) {
        exe = m_debuggerBinary;
        args << QLatin1String("-aqtcreatorcdbext.dll")
             << QLatin1String("-lines")
             << QLatin1String("-G")
             << QLatin1String("-c.idle_cmd")
             << QLatin1String("!qtcreatorcdbext.idle")
             << QLatin1String("debug\\doit.exe");
        cmds = "l+t\n"
               "l+s\n"
               "sxi 0x4000001f\n"
               "g\n"
               "gu\n"
               "!qtcreatorcdbext.expandlocals -t 0 -c 0 " + expanded + "\n";
        int token = 0;
        QStringList sortediNames;
        foreach (QByteArray iName, expandedINames)
            sortediNames << QString::fromLatin1(iName);
        sortediNames.sort();
        foreach (QString iName, sortediNames)
            cmds += "!qtcreatorcdbext.locals -t " + QByteArray::number(++token)
                    + " -c 0 " + iName.toLatin1() + "\n";
        cmds += "q\n";
    } else if (m_debuggerEngine == LldbEngine) {
        exe = "python";
        args << QLatin1String(dumperDir + "/lldbbridge.py")
             << QString::fromUtf8(m_debuggerBinary)
             << t->buildPath + QLatin1String("/doit")
             << QString::fromUtf8(expanded);
        //qDebug() << exe.constData() << ' ' << qPrintable(args.join(QLatin1Char(' ')));
    }

    t->input = cmds;

    QProcessEnvironment env = m_env;
    if (data.useDebugImage)
        env.insert(QLatin1String("DYLD_IMAGE_SUFFIX"), QLatin1String("_debug"));

    QProcess debugger;
    debugger.setProcessEnvironment(env);
    debugger.setWorkingDirectory(t->buildPath);
    debugger.start(QString::fromLatin1(exe), args);
    QVERIFY(debugger.waitForStarted());
    debugger.write(cmds);
    QVERIFY(debugger.waitForFinished());
    output = debugger.readAllStandardOutput();
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

    Context context;
    QByteArray contents;
    if (m_debuggerEngine == GdbEngine) {
        int posDataStart = output.indexOf("data=");
        QVERIFY(posDataStart != -1);
        int posDataEnd = output.indexOf(",typeinfo", posDataStart);
        QVERIFY(posDataEnd != -1);
        contents = output.mid(posDataStart, posDataEnd - posDataStart);

        int posNameSpaceStart = output.indexOf("@NS@");
        QVERIFY(posNameSpaceStart != -1);
        posNameSpaceStart += sizeof("@NS@") - 1;
        int posNameSpaceEnd = output.indexOf("@", posNameSpaceStart);
        QVERIFY(posNameSpaceEnd != -1);
        context.nameSpace = output.mid(posNameSpaceStart, posNameSpaceEnd - posNameSpaceStart);
        //qDebug() << "FOUND NS: " << context.nameSpace;
        if (context.nameSpace == "::")
            context.nameSpace.clear();
        contents.replace("\\\"", "\"");
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
    } else {
        const QByteArray locals = "|locals|";
        int pos1 = output.indexOf(locals);
        QVERIFY(pos1 != -1);
        do {
            pos1 += locals.length();
            int pos2 = output.indexOf("\n", pos1);
            QVERIFY(pos2 != -1);
            contents += output.mid(pos1, pos2 - pos1);
            pos1 = output.indexOf(locals, pos2);
        } while (pos1 != -1);
    }

    GdbMi actual;
    actual.fromString(contents);
    WatchData local;
    local.iname = "local";

    QList<WatchData> list;
    foreach (const GdbMi &child, actual.children()) {
        WatchData dummy;
        dummy.iname = child["iname"].data();
        dummy.name = QLatin1String(child["name"].data());
        if (dummy.iname == "local.qtversion")
            context.qtVersion = child["value"].toInt();
        else if (dummy.iname == "local.gccversion")
            context.gccVersion = child["value"].toInt();
        else
            parseWatchData(expandedINames, dummy, child, &list);
    }

    //qDebug() << "QT VERSION " << QByteArray::number(context.qtVersion, 16);
    QSet<QByteArray> seenINames;
    bool ok = true;
    foreach (const WatchData &item, list) {
        seenINames.insert(item.iname);
        //qDebug() << "NUM CHECKS" << data.checks.size();
        for (int i = data.checks.size(); --i >= 0; ) {
            Check check = data.checks.at(i);
            //qDebug() << "CHECKS" << i << check.iname;
            if ("local." + check.iname == item.iname) {
                data.checks.removeAt(i);
                if (check.matchesEngine(m_debuggerEngine)) {
                    //qDebug() << "USING MATCHING TEST FOR " << item.iname;
                    if (!check.expectedName.matches(item.name.toLatin1(), context)) {
                        qDebug() << "INAME        : " << item.iname;
                        qDebug() << "NAME ACTUAL  : " << item.name.toLatin1();
                        qDebug() << "NAME EXPECTED: " << check.expectedName.name;
                        ok = false;
                    }
                    if (!check.expectedValue.matches(item.value, context)) {
                        qDebug() << "INAME         : " << item.iname;
                        qDebug() << "VALUE ACTUAL  : " << item.value << toHex(item.value);
                        qDebug() << "VALUE EXPECTED: "
                            << check.expectedValue.value << toHex(check.expectedValue.value);
                        ok = false;
                    }
                    if (!check.expectedType.matches(item.type, context)) {
                        qDebug() << "INAME        : " << item.iname;
                        qDebug() << "TYPE ACTUAL  : " << item.type;
                        qDebug() << "TYPE EXPECTED: " << check.expectedType.type;
                        ok = false;
                    }
                } else {
                    qDebug() << "SKIPPING NON-MATCHING TEST FOR " << item.iname;
                }
            }
        }
    }

    if (!data.checks.isEmpty()) {
        for (int i = data.checks.size(); --i >= 0; ) {
            Check check = data.checks.at(i);
            if (!check.matchesEngine(m_debuggerEngine))
                data.checks.removeAt(i);
        }
    }

    if (!data.checks.isEmpty()) {
        bool fail = false;
        qDebug() << "SOME TESTS NOT EXECUTED: ";
        foreach (const Check &check, data.checks) {
            qDebug() << "  TEST NOT FOUND FOR INAME: " << check.iname;
            if (!fail && check.expectedValue.qtVersion != 0)
                fail = true;
        }
        qDebug() << "SEEN INAMES " << seenINames;
        qDebug() << "EXPANDED     : " << expanded;
        ok = false;
    }
    if (ok) {
        m_keepTemp = false;
    } else {
        qDebug() << "CONTENTS     : " << contents;
        qDebug() << "Qt VERSION   : " << qPrintable(QString::number(context.qtVersion, 16));
        if (m_debuggerEngine != CdbEngine)
            qDebug() << "GCC VERSION   : " << qPrintable(QString::number(context.gccVersion, 16));
        qDebug() << "BUILD DIR    : " << qPrintable(t->buildPath);
    }
    QVERIFY(ok);
    disarm();
}

void tst_Dumpers::dumper_data()
{
    QTest::addColumn<Data>("data");

    QByteArray fooData =
            "#include <QHash>\n"
            "#include <QMap>\n"
            "#include <QObject>\n"
            "#include <QString>\n"
            "class Foo\n"
            "{\n"
            "public:\n"
            "    Foo(int i = 0)\n"
            "        : a(i), b(2)\n"
            "    {}\n"
            "    virtual ~Foo()\n"
            "    {\n"
            "        a = 5;\n"
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

    QByteArray nsData =
            "namespace nsA {\n"
            "namespace nsB {\n"
           " struct SomeType\n"
           " {\n"
           "     SomeType(int a) : a(a) {}\n"
           "     int a;\n"
           " };\n"
           " } // namespace nsB\n"
           " } // namespace nsA\n";

    QTest::newRow("AnonymousStructGdb")
            << Data("union {\n"
                    "     struct { int i; int b; };\n"
                    "     struct { float f; };\n"
                    "     double d;\n"
                    " } a = { { 42, 43 } };\n (void)a;")
               % GdbEngine
               % CheckType("a", "a", "union {...}")
               % Check("a.b", "43", "int")
               % Check("a.d", FloatValue("9.1245819032257467e-313"), "double")
               % Check("a.f", FloatValue("5.88545355e-44"), "float")
               % Check("a.i", "42", "int");

    // FIXME: Merge with GDB case
    QTest::newRow("AnonymousStructLldb")
            << Data("union {\n"
                    "     struct { int i; int b; };\n"
                    "     struct { float f; };\n"
                    "     double d;\n"
                    " } a = { { 42, 43 } };\n (void)a;")
               //% CheckType("a", "a", "union {...}")
               % LldbEngine
               % Check("a.#1.b", "43", "int")
               % Check("a.d", FloatValue("9.1245819032257467e-313"), "double")
               % Check("a.#2.f", FloatValue("5.88545355e-44"), "float")
               % Check("a.#1.i", "42", "int");


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

               % CoreProfile()
               % Check("ba0", "ba0", "\"\"", "@QByteArray")

               % Check("ba1", QByteArray("\"Hello\"World")
                      + char(0) + char(1) + char(2) + '"', "@QByteArray").setEngines(
                  GdbEngine | LldbEngine)
               % Check("ba1", QByteArray("\"Hello\"World...\""), "@QByteArray").setForCdbOnly()
               % Check("ba1.0", "[0]", "72", "char")
               % Check("ba1.11", "[11]", "0", "char")
               % Check("ba1.12", "[12]", "1", "char")
               % Check("ba1.13", "[13]", "2", "char")

               % CheckType("ba2", "@QByteArray")
               % Check("s", '"' + QByteArray(10000, 'x') + '"', "@QString")
               % Check("ss", '"' + QByteArray(10000, 'c') + '"', "std::string")

               % Check("buf1", "\"" + QByteArray(1, (char)0xee) + "\"", "@QByteArray")
               % Check("buf2", "\"" + QByteArray(1, (char)0xee) + "\"", "@QByteArray")
               % Check("buf3", "\"\\ee\"", "@QByteArray")
               % CheckType("str1", "char *")

               % Check("ba4", "\"Hell\"", "@QByteArray")
               % Check("ba5", "\"ello\"", "@QByteArray");


    QTest::newRow("QChar")
            << Data("#include <QString>\n",
                    "QString s = QLatin1String(\"x\");\n"
                    "QChar c = s.at(0);\n"
                    "unused(&c);\n")
               % CoreProfile()
               % Check("c", "'x' (120)", "@QChar").setForCdbOnly()
               % Check("c", "120", "@QChar").setEngines(GdbEngine | LldbEngine);


    QTest::newRow("QDate")
            << Data("#include <QDate>\n",
                    "QDate date0;\n"
                    "QDate date1;\n"
                    "date1.setDate(1980, 1, 1);\n"
                    "unused(&date0, &date1);\n")
               % CoreProfile()
               % Check("date0", "(invalid)", "@QDate")
               % Check("date1", "Tue Jan 1 1980", "@QDate")
               % Check("date1.(ISO)", "\"1980-01-01\"", "@QString").setEngines(NoCdbEngine)
               % CheckType("date1.(Locale)", "@QString").setEngines(NoCdbEngine)
               % CheckType("date1.(SystemLocale)", "@QString").setEngines(NoCdbEngine)
               % Check("date1.toString", "\"Tue Jan 1 1980\"", "@QString").setEngines(NoCdbEngine);


    QTest::newRow("QTime")
            << Data("#include <QTime>\n",
                    "QTime time0;\n"
                    "QTime time1(13, 15, 32);")
               % CoreProfile()
               % Check("time0", "(invalid)", "@QTime")
               % Check("time1", "13:15:32", "@QTime")
               % Check("time1.(ISO)", "\"13:15:32\"", "@QString").setEngines(NoCdbEngine)
               % CheckType("time1.(Locale)", "@QString").setEngines(NoCdbEngine)
               % CheckType("time1.(SystemLocale)", "@QString").setEngines(NoCdbEngine)
               % Check("time1.toString", "\"13:15:32\"", "@QString").setEngines(NoCdbEngine);


    QTest::newRow("QDateTime")
            << Data("#include <QDateTime>\n",
                    "QDateTime date0;\n"
                    "QDateTime date1(QDate(1980, 1, 1), QTime(13, 15, 32), Qt::UTC);\n"
                    "unused(&date0);\n")
               % CoreProfile()
               % Check("date0", "(invalid)", "@QDateTime")
               % Check("date1", Value4("Tue Jan 1 13:15:32 1980"), "@QDateTime")
               % Check("date1", Value5("Tue Jan 1 13:15:32 1980 GMT"), "@QDateTime")
               % Check("date1.(ISO)", "\"1980-01-01T13:15:32Z\"", "@QString").setEngines(NoCdbEngine)
               % CheckType("date1.(Locale)", "@QString").setEngines(NoCdbEngine)
               % CheckType("date1.(SystemLocale)", "@QString").setEngines(NoCdbEngine)
               % Check("date1.toString", Value4("\"Tue Jan 1 13:15:32 1980\""), "@QString").setEngines(NoCdbEngine)
               % Check("date1.toString", Value5("\"Tue Jan 1 13:15:32 1980 GMT\""), "@QString").setEngines(NoCdbEngine)
               % Check("date1.toUTC", Value4("Tue Jan 1 13:15:32 1980"), "@QDateTime").setEngines(NoCdbEngine)
               % Check("date1.toUTC", Value5("Tue Jan 1 13:15:32 1980 GMT"), "@QDateTime").setEngines(NoCdbEngine);

#ifdef Q_OS_WIN
    QByteArray tempDir = "\"C:/Program Files\"";
#else
    QByteArray tempDir = "\"/tmp\"";
#endif
    QTest::newRow("QDir")
            << Data("#include <QDir>\n",
                    "QDir dir(" + tempDir + ");\n"
                    "QString s = dir.absolutePath();\n"
                    "QFileInfoList fi = dir.entryInfoList();\n"
                    "unused(&dir, &s, &fi);\n")
               % CoreProfile()
               % Check("dir", tempDir, "@QDir")
               % Check("dir.absolutePath", tempDir, "@QString").setEngines(NoCdbEngine);
            // % Check("dir.canonicalPath", tempDir, "@QString");

    QTest::newRow("QFileInfo")
#ifdef Q_OS_WIN
            << Data("#include <QFile>\n"
                    "#include <QFileInfo>\n",
                    "QFile file(\"C:\\\\Program Files\\\\t\");\n"
                    "file.setObjectName(\"A QFile instance\");\n"
                    "QFileInfo fi(\"C:\\\\Program Files\\\\tt\");\n"
                    "QString s = fi.absoluteFilePath();\n")
               % Check("fi", "\"C:/Program Files/tt\"", "QFileInfo")
               % Check("file", "\"C:\\Program Files\\t\"", "QFile")
               % Check("s", "\"C:/Program Files/tt\"", "QString");
#else
            << Data("#include <QFile>\n"
                    "#include <QFileInfo>\n",
                    "QFile file(\"/tmp/t\");\n"
                    "file.setObjectName(\"A QFile instance\");\n"
                    "QFileInfo fi(\"/tmp/tt\");\n"
                    "QString s = fi.absoluteFilePath();\n")
               % CoreProfile()
               % Check("fi", "\"/tmp/tt\"", "@QFileInfo")
               % Check("file", "\"/tmp/t\"", "@QFile")
               % Check("s", "\"/tmp/tt\"", "@QString");
#endif

    QTest::newRow("QHash1")
            << Data("#include <QHash>\n"
                    "#include <QString>\n"
                    "#include <QList>\n",
                    "QHash<QString, QList<int> > hash;\n"
                    "hash.insert(\"Hallo\", QList<int>());\n"
                    "hash.insert(\"Welt\", QList<int>() << 1);\n"
                    "hash.insert(\"!\", QList<int>() << 1 << 2);\n")
               % CoreProfile()
               % Check("hash", "<3 items>", "@QHash<@QString, @QList<int> >")
               % Check("hash.0", "[0]", "", "@QHashNode<@QString, @QList<int>>")
               % Check("hash.0.key", Value4("\"Hallo\""), "@QString")
               % Check("hash.0.key", Value5("\"Welt\""), "@QString")
               % Check("hash.0.value", Value4("<0 items>"), "@QList<int>")
               % Check("hash.0.value", Value5("<1 items>"), "@QList<int>")
               % Check("hash.1", "[1]", "", "@QHashNode<@QString, @QList<int>>")
               % Check("hash.1.key", "key", Value4("\"Welt\""), "@QString")
               % Check("hash.1.key", "key", Value5("\"Hallo\""), "@QString")
               % Check("hash.1.value", "value", Value4("<1 items>"), "@QList<int>")
               % Check("hash.1.value", "value", Value5("<0 items>"), "@QList<int>")
               % Check("hash.2", "[2]", "", "@QHashNode<@QString, @QList<int>>")
               % Check("hash.2.key", "key", "\"!\"", "@QString")
               % Check("hash.2.value", "value", "<2 items>", "@QList<int>")
               % Check("hash.2.value.0", "[0]", "1", "int")
               % Check("hash.2.value.1", "[1]", "2", "int");

    QTest::newRow("QHash2")
            << Data("#include <QHash>\n",
                    "QHash<int, float> hash;\n"
                    "hash[11] = 11.0;\n"
                    "hash[22] = 22.0;\n")
               % CoreProfile()
               % Check("hash", "hash", "<2 items>", "@QHash<int, float>")
               % Check("hash.22", "[22]", "22", "float")
               % Check("hash.11", "[11]", "11", "float");

    QTest::newRow("QHash3")
            << Data("#include <QString>\n"
                    "#include <QHash>\n",
                    "QHash<QString, int> hash;\n"
                    "hash[\"22.0\"] = 22.0;\n"
                    "hash[\"123.0\"] = 22.0;\n"
                    "hash[\"111111ss111128.0\"] = 28.0;\n"
                    "hash[\"11124.0\"] = 22.0;\n"
                    "hash[\"1111125.0\"] = 22.0;\n"
                    "hash[\"11111126.0\"] = 22.0;\n"
                    "hash[\"111111127.0\"] = 27.0;\n"
                    "hash[\"111111111128.0\"] = 28.0;\n"
                    "hash[\"111111111111111111129.0\"] = 29.0;\n")
               % CoreProfile()
               % Check("hash", "hash", "<9 items>", "@QHash<@QString, int>")
               % Check("hash.0", "[0]", "", "@QHashNode<@QString, int>")
               % Check("hash.0.key", "key", Value4("\"123.0\""), "@QString")
               % Check("hash.0.key", "key", Value5("\"111111111128.0\""), "@QString")
               % Check("hash.0.value", Value4("22"), "int")
               % Check("hash.0.value", Value5("28"), "int")
               % Check("hash.8", "[8]", "", "@QHashNode<@QString, int>")
               % Check("hash.8.key", "key", Value4("\"11124.0\""), "@QString")
               % Check("hash.8.key", "key", Value5("\"123.0\""), "@QString")
               % Check("hash.8.value", "value", Value4("22"), "int")
               % Check("hash.8.value", "value", Value5("22"), "int");

    QTest::newRow("QHash4")
            << Data("#include <QByteArray>\n"
                    "#include <QHash>\n",
                    "QHash<QByteArray, float> hash;\n"
                    "hash[\"22.0\"] = 22.0;\n"
                    "hash[\"123.0\"] = 22.0;\n"
                    "hash[\"111111ss111128.0\"] = 28.0;\n"
                    "hash[\"11124.0\"] = 22.0;\n"
                    "hash[\"1111125.0\"] = 22.0;\n"
                    "hash[\"11111126.0\"] = 22.0;\n"
                    "hash[\"111111127.0\"] = 27.0;\n"
                    "hash[\"111111111128.0\"] = 28.0;\n"
                    "hash[\"111111111111111111129.0\"] = 29.0;\n")
               % CoreProfile()
               % Check("hash", "<9 items>", "@QHash<@QByteArray, float>")
               % Check("hash.0", "[0]", "", "@QHashNode<@QByteArray, float>")
               % Check("hash.0.key", Value4("\"123.0\""), "@QByteArray")
               % Check("hash.0.key", Value5("\"111111111128.0\""), "@QByteArray")
               % Check("hash.0.value", Value4("22"), "float")
               % Check("hash.0.value", Value5("28"), "float")
               % Check("hash.8", "[8]", "", "@QHashNode<@QByteArray, float>")
               % Check("hash.8.key", Value4("\"11124.0\""), "@QByteArray")
               % Check("hash.8.key", Value5("\"123.0\""), "@QByteArray")
               % Check("hash.8.value", Value4("22"), "float")
               % Check("hash.8.value", Value5("22"), "float");

    QTest::newRow("QHash5")
            << Data("#include <QString>\n"
                    "#include <QHash>\n",
                    "QHash<int, QString> hash;\n"
                    "hash[22] = \"22.0\";\n")
               % CoreProfile()
               % Check("hash", "<1 items>", "@QHash<int, @QString>")
               % Check("hash.0.key", "22", "int")
               % Check("hash.0.value", "\"22.0\"", "@QString");

    QTest::newRow("QHash6")
            << Data("#include <QHash>\n" + fooData,
                    "QHash<QString, Foo> hash;\n"
                    "hash[\"22.0\"] = Foo(22);\n"
                    "hash[\"33.0\"] = Foo(33);\n")
               % CoreProfile()
               % Check("hash", "<2 items>", "@QHash<@QString, Foo>")
               % Check("hash.0", "[0]", "", "@QHashNode<@QString, Foo>")
               % Check("hash.0.key", Value4("\"22.0\""), "@QString")
               % Check("hash.0.key", Value5("\"33.0\""), "@QString")
               % CheckType("hash.0.value", "Foo")
               % Check("hash.0.value.a", Value4("22"), "int")
               % Check("hash.0.value.a", Value5("33"), "int")
               % Check("hash.1", "[1]", "", "@QHashNode<@QString, Foo>")
               % Check("hash.1.key", Value4("\"33.0\""), "@QString")
               % Check("hash.1.key", Value5("\"22.0\""), "@QString")
               % CheckType("hash.1.value", "Foo");

    QTest::newRow("QHash7")
            << Data("#include <QHash>\n"
                    "#include <QPointer>\n",
                    "QObject ob;\n"
                    "QHash<QString, QPointer<QObject> > hash;\n"
                    "hash.insert(\"Hallo\", QPointer<QObject>(&ob));\n"
                    "hash.insert(\"Welt\", QPointer<QObject>(&ob));\n"
                    "hash.insert(\".\", QPointer<QObject>(&ob));\n")
               % CoreProfile()
               % Check("hash", "<3 items>", "@QHash<@QString, @QPointer<@QObject>>")
               % Check("hash.0", "[0]", "", "@QHashNode<@QString, @QPointer<@QObject>>")
               % Check("hash.0.key", Value4("\"Hallo\""), "@QString")
               % Check("hash.0.key", Value5("\"Welt\""), "@QString")
               % CheckType("hash.0.value", "@QPointer<@QObject>")
               //% CheckType("hash.0.value.o", "@QObject")
               % Check("hash.2", "[2]", "", "@QHashNode<@QString, @QPointer<@QObject>>")
               % Check("hash.2.key", "\".\"", "@QString")
               % CheckType("hash.2.value", "@QPointer<@QObject>");


    QTest::newRow("QHashIterator")
            << Data("#include <QHash>\n",
                    "typedef QHash<int, float> Hash;\n"
                    "Hash hash;\n"
                    "hash[11] = 11.0;\n"
                    "hash[22] = 22.0;\n"
                    "hash[33] = 33.0;\n"
                    "hash[44] = 44.0;\n"
                    "hash[55] = 55.0;\n"
                    "hash[66] = 66.0;\n"
                    "Hash::iterator it1 = hash.begin();\n"
                    "Hash::iterator it2 = it1; ++it2;\n"
                    "Hash::iterator it3 = it2; ++it3;\n"
                    "Hash::iterator it4 = it3; ++it4;\n"
                    "Hash::iterator it5 = it4; ++it5;\n"
                    "Hash::iterator it6 = it5; ++it6;\n")
               % CoreProfile()
               % Check("hash", "<6 items>", "Hash")
               % Check("hash.11", "[11]", "11", "float")
               % Check("it1.key", "55", "int")
               % Check("it1.value", "55", "float")
               % Check("it6.key", "33", "int")
               % Check("it6.value", "33", "float");


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
               % NetworkProfile()
               % Check("ha1", "129.0.0.130", "@QHostAddress")
               % Check("ha2", "\"127.0.0.1\"", "@QHostAddress")
               % Check("addr", "1:203:506:0:809:a0b:0:0", "@QIPv6Address");


    QTest::newRow("QImage")
            << Data("#include <QImage>\n"
                    "#include <QApplication>\n"
                    "#include <QPainter>\n",
                    "QApplication app(argc, argv);\n"
                    "QImage im(QSize(200, 200), QImage::Format_RGB32);\n"
                    "im.fill(QColor(200, 100, 130).rgba());\n"
                    "QPainter pain;\n"
                    "pain.begin(&im);\n"
                    "unused(&pain, &im);\n")
               % GuiProfile()
               % Check("im", "(200x200)", "@QImage")
               % CheckType("pain", "@QPainter");

    QTest::newRow("QPixmap")
            << Data("#include <QImage>\n"
                    "#include <QPainter>\n"
                    "#include <QApplication>\n",
                    "QApplication app(argc, argv);\n"
                    "QImage im(QSize(200, 200), QImage::Format_RGB32);\n"
                    "im.fill(QColor(200, 100, 130).rgba());\n"
                    "QPainter pain;\n"
                    "pain.begin(&im);\n"
                    "pain.drawLine(2, 2, 130, 130);\n"
                    "pain.end();\n"
                    "QPixmap pm = QPixmap::fromImage(im);\n"
                    "unused(&pm);\n")
               % GuiProfile()
               % Check("im", "(200x200)", "@QImage")
               % CheckType("pain", "@QPainter")
               % Check("pm", "(200x200)", "@QPixmap");

    QTest::newRow("QLinkedList")
            << Data("#include <QLinkedList>\n"
                    "#include <string>\n"
                    + fooData,

                    "QLinkedList<int> list1;\n"
                    "list1.append(101);\n"
                    "list1.append(102);\n\n"

                    "QLinkedList<uint> list2;\n"
                    "list2.append(103);\n"
                    "list2.append(104);\n\n"

                    "QLinkedList<Foo *> list3;\n"
                    "list3.append(new Foo(1));\n"
                    "list3.append(0);\n"
                    "list3.append(new Foo(3));\n\n"

                    "QLinkedList<qulonglong> list4;\n"
                    "list4.append(42);\n"
                    "list4.append(43);\n\n"

                    "QLinkedList<Foo> list5;\n"
                    "list5.append(Foo(1));\n"
                    "list5.append(Foo(2));\n\n"

                    "QLinkedList<std::string> list6;\n"
                    "list6.push_back(\"aa\");\n"
                    "list6.push_back(\"bb\");\n")

               % CoreProfile()

               % Check("list1", "<2 items>", "@QLinkedList<int>")
               % Check("list1.0", "[0]", "101", "int")
               % Check("list1.1", "[1]", "102", "int")

               % Check("list2", "<2 items>", "@QLinkedList<unsigned int>")
               % Check("list2.0", "[0]", "103", "unsigned int")
               % Check("list2.1", "[1]", "104", "unsigned int")

               % Check("list3", "<3 items>", "@QLinkedList<Foo*>")
               % CheckType("list3.0", "[0]", "Foo")
               % Check("list3.0.a", "1", "int")
               % Check("list3.1", "[1]", "0x0", "Foo *")
               % CheckType("list3.2", "[2]", "Foo")
               % Check("list3.2.a", "3", "int")

               % Check("list4", "<2 items>", "@QLinkedList<unsigned long long>")
               % Check("list4.0", "[0]", "42", "unsigned long long")
               % Check("list4.1", "[1]", "43", "unsigned long long")

               % Check("list5", "<2 items>", "@QLinkedList<Foo>")
               % CheckType("list5.0", "[0]", "Foo")
               % Check("list5.0.a", "1", "int")
               % CheckType("list5.1", "[1]", "Foo")
               % Check("list5.1.a", "2", "int")

               % Check("list6", "<2 items>", "@QLinkedList<std::string>")
               % Check("list6.0", "[0]", "\"aa\"", "std::string")
               % Check("list6.1", "[1]", "\"bb\"", "std::string");


    QTest::newRow("QListInt")
            << Data("#include <QList>\n",
                    "QList<int> big;\n"
                    "for (int i = 0; i < 10000; ++i)\n"
                    "    big.push_back(i);\n")
               % CoreProfile()
               % Check("big", "<10000 items>", "@QList<int>")
               % Check("big.0", "[0]", "0", "int")
               % Check("big.1999", "[1999]", "1999", "int");

    QTest::newRow("QListIntTakeFirst")
            << Data("#include <QList>\n",
                    "QList<int> l;\n"
                    "l.append(0);\n"
                    "l.append(1);\n"
                    "l.append(2);\n"
                    "l.takeFirst();\n")
               % CoreProfile()
               % Check("l", "<2 items>", "@QList<int>")
               % Check("l.0", "[0]", "1", "int");

    QTest::newRow("QListStringTakeFirst")
            << Data("#include <QList>\n"
                    "#include <QString>\n",
                    "QList<QString> l;\n"
                    "l.append(\"0\");\n"
                    "l.append(\"1\");\n"
                    "l.append(\"2\");\n"
                    "l.takeFirst();\n")
               % CoreProfile()
               % Check("l", "<2 items>", "@QList<@QString>")
               % Check("l.0", "[0]", "\"1\"", "@QString");

    QTest::newRow("QStringListTakeFirst")
            << Data("#include <QStringList>\n",
                    "QStringList l;\n"
                    "l.append(\"0\");\n"
                    "l.append(\"1\");\n"
                    "l.append(\"2\");\n"
                    "l.takeFirst();\n")
               % CoreProfile()
               % Check("l", "<2 items>", "@QStringList")
               % Check("l.0", "[0]", "\"1\"", "@QString");

    QTest::newRow("QListIntStar")
            << Data("#include <QList>\n",
                    "QList<int *> l0, l;\n"
                    "l.append(new int(1));\n"
                    "l.append(new int(2));\n"
                    "l.append(new int(3));\n")
               % CoreProfile()
               % Check("l0", "<0 items>", "@QList<int*>")
               % Check("l", "<3 items>", "@QList<int*>")
               % CheckType("l.0", "[0]", "int")
               % CheckType("l.2", "[2]", "int");

    QTest::newRow("QListUInt")
            << Data("#include <QList>\n",
                    "QList<uint> l0,l;\n"
                    "l.append(101);\n"
                    "l.append(102);\n"
                    "l.append(102);\n")
               % CoreProfile()
               % Check("l0", "<0 items>", "@QList<unsigned int>")
               % Check("l", "<3 items>", "@QList<unsigned int>")
               % Check("l.0", "[0]", "101", "unsigned int")
               % Check("l.2", "[2]", "102", "unsigned int");

    QTest::newRow("QListStringList")
            << Data("#include <QStringList>\n",
                    "QStringList l;\n"
                    "l.append(\"aaa\");\n"
                    "QList<QStringList> ll;\n"
                    "ll.append(l);\n"
                    "ll.append(l);\n")
               % CoreProfile()
               % Check("ll", "<2 items>", "@QList<@QStringList>")
               % Check("l", "<1 items>", "@QStringList")
               % Check("ll.1", "[1]", "<1 items>", "@QStringList")
               % Check("ll.1.0", "[0]", "\"aaa\"", "@QString");

    QTest::newRow("QListUShort")
            << Data("#include <QList>\n",
                    "QList<ushort> l0,l;\n"
                    "l.append(101);\n"
                    "l.append(102);\n"
                    "l.append(102);\n")
               % CoreProfile()
               % Check("l0", "<0 items>", "@QList<unsigned short>")
               % Check("l", "<3 items>", "@QList<unsigned short>")
               % Check("l.0", "[0]", "101", "unsigned short")
               % Check("l.2", "[2]", "102", "unsigned short");

    QTest::newRow("QListQChar")
            << Data("#include <QList>\n"
                    "#include <QChar>\n",
                    "QList<QChar> l0, l;\n"
                    "l.append(QChar('a'));\n"
                    "l.append(QChar('b'));\n"
                    "l.append(QChar('c'));\n")
               % CoreProfile()
               % Check("l0", "<0 items>", "@QList<@QChar>")
               % Check("l", "<3 items>", "@QList<@QChar>")
               % Check("l.0", "[0]", "97", "@QChar")
               % Check("l.2", "[2]", "99", "@QChar");

    QTest::newRow("QListQULongLong")
            << Data("#include <QList>\n",
                    "QList<qulonglong> l0, l;\n"
                    "l.append(101);\n"
                    "l.append(102);\n"
                    "l.append(102);\n")
               % CoreProfile()
               % Check("l0", "<0 items>", "@QList<unsigned long long>")
               % Check("l", "<3 items>", "@QList<unsigned long long>")
               % Check("l.0", "[0]", "101", "unsigned long long")
               % Check("l.2", "[2]", "102", "unsigned long long");

    QTest::newRow("QListStdString")
            << Data("#include <QList>\n"
                    "#include <string>\n",
                    "QList<std::string> l0, l;\n"
                    "l.push_back(\"aa\");\n"
                    "l.push_back(\"bb\");\n"
                    "l.push_back(\"cc\");\n"
                    "l.push_back(\"dd\");")
               % CoreProfile()
               % Check("l0", "<0 items>", "@QList<std::string>")
               % Check("l", "<4 items>", "@QList<std::string>")
               % CheckType("l.0", "[0]", "std::string")
               % Check("l.3", "[3]" ,"\"dd\"", "std::string");

   QTest::newRow("QListFoo")
            << Data("#include <QList>\n" + fooData,
                    "QList<Foo> l0, l;\n"
                    "for (int i = 0; i < 100; ++i)\n"
                    "    l.push_back(i + 15);\n")
               % CoreProfile()
               % Check("l0", "<0 items>", "@QList<Foo>")
               % Check("l", "<100 items>", "@QList<Foo>")
               % Check("l.0", "[0]", "", "Foo")
               % Check("l.99", "[99]", "", "Foo");

   QTest::newRow("QListReverse")
           << Data("#include <QList>\n",
                   "QList<int> l = QList<int>() << 1 << 2 << 3;\n"
                   "typedef std::reverse_iterator<QList<int>::iterator> Reverse;\n"
                   "Reverse rit(l.end());\n"
                   "Reverse rend(l.begin());\n"
                   "QList<int> r;\n"
                   "while (rit != rend)\n"
                   "    r.append(*rit++);\n")
              % CoreProfile()
              % Check("l", "<3 items>", "@QList<int>")
              % Check("l.0", "[0]", "1", "int")
              % Check("l.1", "[1]", "2", "int")
              % Check("l.2", "[2]", "3", "int")
              % Check("r", "<3 items>", "@QList<int>")
              % Check("r.0", "[0]", "3", "int")
              % Check("r.1", "[1]", "2", "int")
              % Check("r.2", "[2]", "1", "int")
              % Check("rend", "", "Reverse")
              % Check("rit", "", "Reverse");

   QTest::newRow("QLocale")
           << Data("#include <QLocale>\n",
                   "QLocale loc0;\n"
                   "QLocale loc = QLocale::system();\n"
                   "QLocale::MeasurementSystem m = loc.measurementSystem();\n"
                   "QLocale loc1(\"en_US\");\n"
                   "QLocale::MeasurementSystem m1 = loc1.measurementSystem();\n"
                   "unused(&loc0, &loc, &m, &loc1, &m1);\n")
              % CoreProfile()
              % Check("loc0", "\"en_US\"", "@QLocale")
              % CheckType("loc", "@QLocale")
              % CheckType("m", "@QLocale::MeasurementSystem")
              % Check("loc1", "\"en_US\"", "@QLocale")
              % Check("m1", Value5("@QLocale::ImperialUSSystem (1)"),
                    "@QLocale::MeasurementSystem").setForGdbOnly()
              % Check("m1", Value4("@QLocale::ImperialSystem (1)"),
                    "@QLocale::MeasurementSystem").setForGdbOnly()
              % Check("m1", Value5("ImperialUSSystem"),
                    "@QLocale::MeasurementSystem").setForLldbOnly()
              % Check("m1", Value4("ImperialSystem"),
                    "@QLocale::MeasurementSystem").setForLldbOnly();

   QTest::newRow("QMapUIntStringList")
           << Data("#include <QMap>\n"
                   "#include <QStringList>\n",
                   "QMap<uint, QStringList> map;\n"
                   "map[11] = QStringList() << \"11\";\n"
                   "map[22] = QStringList() << \"22\";\n")
              % CoreProfile()
              % Check("map", "<2 items>", "@QMap<unsigned int, @QStringList>")
              % Check("map.0", "[0]", "", "@QMapNode<unsigned int, @QStringList>")
              % Check("map.0.key", "11", "unsigned int")
              % Check("map.0.value", "<1 items>", "@QStringList")
              % Check("map.0.value.0", "[0]", "\"11\"", "@QString")
              % Check("map.1", "[1]", "", "@QMapNode<unsigned int, @QStringList>")
              % Check("map.1.key", "22", "unsigned int")
              % Check("map.1.value", "<1 items>", "@QStringList")
              % Check("map.1.value.0", "[0]", "\"22\"", "@QString");

   QTest::newRow("QMapUIntStringListTypedef")
           << Data("#include <QMap>\n"
                   "#include <QStringList>\n",
                   "typedef QMap<uint, QStringList> T;\n"
                   "T map;\n"
                   "map[11] = QStringList() << \"11\";\n"
                   "map[22] = QStringList() << \"22\";\n")
              % CoreProfile()
              % Check("map", "<2 items>", "T")
              % Check("map.0", "[0]", "", "@QMapNode<unsigned int, @QStringList>");

   QTest::newRow("QMapUIntFloat")
           << Data("#include <QMap>\n",
                   "QMap<uint, float> map;\n"
                   "map[11] = 31.0;\n"
                   "map[22] = 32.0;\n")
              % CoreProfile()
              % Check("map", "<2 items>", "@QMap<unsigned int, float>")
              % Check("map.0", "[0] 11", FloatValue("31.0"), "float")
              % Check("map.1", "[1] 22", FloatValue("32.0"), "float");

   QTest::newRow("QMapStringFloat")
           << Data("#include <QMap>\n"
                   "#include <QString>\n",
                   "QMap<QString, float> map;\n"
                   "map[\"22.0\"] = 22.0;\n")
              % CoreProfile()
              % Check("map", "<1 items>", "@QMap<@QString, float>")
              % Check("map.0", "[0]", "", "@QMapNode<@QString, float>")
              % Check("map.0.key", "\"22.0\"", "@QString")
              % Check("map.0.value", "22", "float");

   QTest::newRow("QMapIntString")
           << Data("#include <QMap>\n"
                   "#include <QString>\n",
                   "QMap<int, QString> map;\n"
                   "map[22] = \"22.0\";\n")
              % CoreProfile()
              % Check("map", "<1 items>", "@QMap<int, @QString>")
              % Check("map.0", "[0]", "", "@QMapNode<int, @QString>")
              % Check("map.0.key", "22", "int")
              % Check("map.0.value", "\"22.0\"", "@QString");

   QTest::newRow("QMapStringFoo")
           << Data("#include <QMap>\n" + fooData +
                   "#include <QString>\n",
                   "QMap<QString, Foo> map;\n"
                   "map[\"22.0\"] = Foo(22);\n"
                   "map[\"33.0\"] = Foo(33);\n")
              % CoreProfile()
              % Check("map", "<2 items>", "@QMap<@QString, Foo>")
              % Check("map.0", "[0]", "", "@QMapNode<@QString, Foo>")
              % Check("map.0.key", "\"22.0\"", "@QString")
              % Check("map.0.value", "", "Foo")
              % Check("map.0.value.a", "22", "int")
              % Check("map.1", "[1]", "", "@QMapNode<@QString, Foo>")
              % Check("map.1.key", "\"33.0\"", "@QString")
              % Check("map.1.value", "", "Foo")
              % Check("map.1.value.a", "33", "int");

   QTest::newRow("QMapStringPointer")
           << Data("#include <QMap>\n"
                   "#include <QObject>\n"
                   "#include <QPointer>\n"
                   "#include <QString>\n",
                   "QObject ob;\n"
                   "QMap<QString, QPointer<QObject> > map;\n"
                   "map.insert(\"Hallo\", QPointer<QObject>(&ob));\n"
                   "map.insert(\"Welt\", QPointer<QObject>(&ob));\n"
                   "map.insert(\".\", QPointer<QObject>(&ob));\n")
              % CoreProfile()
              % Check("map", "<3 items>", "@QMap<@QString, @QPointer<@QObject>>")
              % Check("map.0", "[0]", "", "@QMapNode<@QString, @QPointer<@QObject>>")
              % Check("map.0.key", "\".\"", "@QString")
              % Check("map.0.value", "", "@QPointer<@QObject>")
              //% Check("map.0.value.o", Pointer(), "@QObject")
              // FIXME: it's '.wp' in Qt 5
              % Check("map.1", "[1]", "", "@QMapNode<@QString, @QPointer<@QObject>>")
              % Check("map.1.key", "\"Hallo\"", "@QString")
              % Check("map.2", "[2]", "", "@QMapNode<@QString, @QPointer<@QObject>>")
              % Check("map.2.key", "\"Welt\"", "@QString");

   QTest::newRow("QMapStringList")
           << Data("#include <QMap>\n"
                   "#include <QList>\n"
                   "#include <QString>\n" + nsData,
                   "QList<nsA::nsB::SomeType *> x;\n"
                   "x.append(new nsA::nsB::SomeType(1));\n"
                   "x.append(new nsA::nsB::SomeType(2));\n"
                   "x.append(new nsA::nsB::SomeType(3));\n"
                   "QMap<QString, QList<nsA::nsB::SomeType *> > map;\n"
                   "map[\"foo\"] = x;\n"
                   "map[\"bar\"] = x;\n"
                   "map[\"1\"] = x;\n"
                   "map[\"2\"] = x;\n")
              % CoreProfile()
              % Check("map", "<4 items>", "@QMap<@QString, @QList<nsA::nsB::SomeType*>>")
              % Check("map.0", "[0]", "", "@QMapNode<@QString, @QList<nsA::nsB::SomeType*>>")
              % Check("map.0.key", "\"1\"", "@QString")
              % Check("map.0.value", "<3 items>", "@QList<nsA::nsB::SomeType*>")
              % Check("map.0.value.0", "[0]", "", "nsA::nsB::SomeType")
              % Check("map.0.value.0.a", "1", "int")
              % Check("map.0.value.1", "[1]", "", "nsA::nsB::SomeType")
              % Check("map.0.value.1.a", "2", "int")
              % Check("map.0.value.2", "[2]", "", "nsA::nsB::SomeType")
              % Check("map.0.value.2.a", "3", "int")
              % Check("map.3", "[3]", "", "@QMapNode<@QString, @QList<nsA::nsB::SomeType*>>")
              % Check("map.3.key", "\"foo\"", "@QString")
              % Check("map.3.value", "<3 items>", "@QList<nsA::nsB::SomeType*>")
              % Check("map.3.value.2", "[2]", "", "nsA::nsB::SomeType")
              % Check("map.3.value.2.a", "3", "int")
              % Check("x", "<3 items>", "@QList<nsA::nsB::SomeType*>");


   QTest::newRow("QMultiMapUintFloat")
           << Data("#include <QMap>\n",
                   "QMultiMap<uint, float> map;\n"
                   "map.insert(11, 11.0);\n"
                   "map.insert(22, 22.0);\n"
                   "map.insert(22, 33.0);\n"
                   "map.insert(22, 34.0);\n"
                   "map.insert(22, 35.0);\n"
                   "map.insert(22, 36.0);\n")
              % CoreProfile()
              % Check("map", "<6 items>", "@QMultiMap<unsigned int, float>")
              % Check("map.0", "[0] 11", "11", "float")
              % Check("map.5", "[5] 22", "22", "float");


   QTest::newRow("QMultiMapStringFloat")
           << Data("#include <QMap>\n"
                   "#include <QString>\n",
                   "QMultiMap<QString, float> map;\n"
                   "map.insert(\"22.0\", 22.0);\n")
              % CoreProfile()
              % Check("map", "<1 items>", "@QMultiMap<@QString, float>")
              % Check("map.0", "[0]", "", "@QMapNode<@QString, float>")
              % Check("map.0.key", "\"22.0\"", "@QString")
              % Check("map.0.value", "22", "float");

   QTest::newRow("QMultiMapIntString")
           << Data("#include <QMap>\n"
                   "#include <QString>\n",
                   "QMultiMap<int, QString> map;\n"
                   "map.insert(22, \"22.0\");\n")
              % CoreProfile()
              % Check("map", "<1 items>", "@QMultiMap<int, @QString>")
              % Check("map.0", "[0]", "", "@QMapNode<int, @QString>")
              % Check("map.0.key", "22", "int")
              % Check("map.0.value", "\"22.0\"", "@QString");

   QTest::newRow("QMultiMapStringFoo")
           << Data("#include <QMultiMap>\n" + fooData,
                   "QMultiMap<QString, Foo> map;\n"
                   "map.insert(\"22.0\", Foo(22));\n"
                   "map.insert(\"33.0\", Foo(33));\n"
                   "map.insert(\"22.0\", Foo(22));\n")
              % CoreProfile()
              % Check("map", "<3 items>", "@QMultiMap<@QString, Foo>")
              % Check("map.0", "[0]", "", "@QMapNode<@QString, Foo>")
              % Check("map.0.key", "\"22.0\"", "@QString")
              % Check("map.0.value", "", "Foo")
              % Check("map.0.value.a", "22", "int")
              % Check("map.2", "[2]", "", "@QMapNode<@QString, Foo>");

   QTest::newRow("QMultiMapStringPointer")
           << Data("#include <QMap>\n"
                   "#include <QObject>\n"
                   "#include <QPointer>\n"
                   "#include <QString>\n",
                   "QObject ob;\n"
                   "QMultiMap<QString, QPointer<QObject> > map;\n"
                   "map.insert(\"Hallo\", QPointer<QObject>(&ob));\n"
                   "map.insert(\"Welt\", QPointer<QObject>(&ob));\n"
                   "map.insert(\".\", QPointer<QObject>(&ob));\n"
                   "map.insert(\".\", QPointer<QObject>(&ob));\n")
              % CoreProfile()
              % Check("map", "<4 items>", "@QMultiMap<@QString, @QPointer<@QObject>>")
              % Check("map.0", "[0]", "", "@QMapNode<@QString, @QPointer<@QObject>>")
              % Check("map.0.key", "\".\"", "@QString")
              % Check("map.0.value", "", "@QPointer<@QObject>")
              % Check("map.1", "[1]", "", "@QMapNode<@QString, @QPointer<@QObject>>")
              % Check("map.1.key", "\".\"", "@QString")
              % Check("map.2", "[2]", "", "@QMapNode<@QString, @QPointer<@QObject>>")
              % Check("map.2.key", "\"Hallo\"", "@QString")
              % Check("map.3", "[3]", "", "@QMapNode<@QString, @QPointer<@QObject>>")
              % Check("map.3.key", "\"Welt\"", "@QString");


   QTest::newRow("QObject1")
           << Data("#include <QObject>\n",
                   "QObject parent;\n"
                   "parent.setObjectName(\"A Parent\");\n"
                   "QObject child(&parent);\n"
                   "child.setObjectName(\"A Child\");\n"
                   "QObject::connect(&child, SIGNAL(destroyed()), &parent, SLOT(deleteLater()));\n"
                   "QObject::disconnect(&child, SIGNAL(destroyed()), &parent, SLOT(deleteLater()));\n"
                   "child.setObjectName(\"A renamed Child\");\n")
              % CoreProfile()
              % Check("child", "\"A renamed Child\"", "@QObject")
              % Check("parent", "\"A Parent\"", "@QObject");

    QTest::newRow("QObject2")
            << Data("#include <QWidget>\n"
                    "#include <QApplication>\n"
                    "namespace Names {\n"
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
                    "} // namespace Names\n"
                    "#include <main.moc>\n",
                    ""
                    "QApplication app(argc, argv);\n"
                    "Names::Bar::TestObject test;\n"
                    "test.setMyProp1(\"Hello\");\n"
                    "test.setMyProp2(\"World\");\n"
                    "QString s = test.myProp1();\n"
                    "s += QString::fromLatin1(test.myProp2());\n"
                    "unused(&app, &test, &s);\n")
               % GuiProfile()
               % Check("s", "\"HelloWorld\"", "@QString")
               % Check("test", "", "Names::Bar::TestObject")
               % Check("test.[properties]", "<4 items>", "")
               % Check("test.[properties].myProp1", "\"Hello\"", "@QVariant (QString)")
               % Check("test.[properties].myProp2", "\"World\"", "@QVariant (QByteArray)")
               % Check("test.[properties].myProp3", "54", "@QVariant (long)")
               % Check("test.[properties].myProp4", "44", "@QVariant (int)");

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
               % GuiProfile()
               % Check("ob", "\"An Object\"", "@QWidget")
               % Check("ob1", "\"Another Object\"", "@QObject")
               % Check("ob2", "\"A Subobject\"", "@QObject");

    QByteArray senderData =
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
              % CoreProfile()
              % CorePrivateProfile();
// FIXME:
//              % Check("ob.properties.x", "26", "@QVariant (int)");


    QTest::newRow("QRegExp")
            << Data("#include <QRegExp>\n",
                    "QRegExp re(QString(\"a(.*)b(.*)c\"));\n"
                    "QString str1 = \"a1121b344c\";\n"
                    "QString str2 = \"Xa1121b344c\";\n"
                    "int pos2 = re.indexIn(str2);\n"
                    "int pos1 = re.indexIn(str1);\n"
                    "unused(&pos1, &pos2);\n")
               % CoreProfile()
               % Check("re", "\"a(.*)b(.*)c\"", "@QRegExp")
               % Check("str1", "\"a1121b344c\"", "@QString")
               % Check("str2", "\"Xa1121b344c\"", "@QString")
               % Check("pos1", "0", "int")
               % Check("pos2", "1", "int");


    QTest::newRow("QPoint")
            << Data("#include <QPoint>\n"
                    "#include <QPointF>\n"
                    "#include <QString> // Dummy for namespace\n",
                    "QString dummy;\n"
                    "QPoint s0, s;\n"
                    "s = QPoint(100, 200);\n"
                    "QPointF pf0, pf;\n"
                    "pf = QPointF(100.5, 200.5);\n"
                    "unused(&s0, &s, &dummy);\n")
               % CoreProfile()
               % Check("s0", "(0, 0)", "@QPoint")
               % Check("s", "(100, 200)", "@QPoint")
               % Check("pf0", "(0.0, 0.0)", "@QPointF")
               % Check("pf", "(100.5, 200.5)", "@QPointF");


    QTest::newRow("QRect")
            << Data("#include <QRect>\n"
                    "#include <QRectF>\n"
                    "#include <QString> // Dummy for namespace\n",
                    "QString dummy;\n"
                    "QRect rect0, rect;\n"
                    "rect = QRect(100, 100, 200, 200);\n"
                    "QRectF rectf0, rectf;\n"
                    "rectf = QRectF(100.25, 100.25, 200.5, 200.5);\n"
                    "unused(&rect0, &rect, &dummy);\n")
               % Check("rect0", "0x0+0+0", "@QRect")
               % Check("rect", "200x200+100+100", "@QRect")
               % Check("rectf0", "0.0x0.0+0.0+0.0", "@QRectF")
               % Check("rectf", "200.5x200.5+100.25+100.25", "@QRectF");


    QTest::newRow("QSize")
            << Data("#include <QSize>\n"
                    "#include <QSizeF>\n"
                    "#include <QString> // Dummy for namespace\n",
                    "QString dummy;\n"
                    "QSize s0, s;\n"
                    "QSizeF sf0, sf;\n"
                    "sf = QSizeF(100.5, 200.5);\n"
                    "s = QSize(100, 200);\n"
                    "unused(&s0, &s, &dummy);\n")
               % CoreProfile()
               % Check("s0", "(-1, -1)", "@QSize")
               % Check("s", "(100, 200)", "@QSize")
               % Check("sf0", "(-1.0, -1.0)", "@QSizeF")
               % Check("sf", "(100.5, 200.5)", "@QSizeF");


    QTest::newRow("QRegion")
            << Data("#include <QRegion>\n",
                    "QRegion region, region0, region1, region2;\n"
                    "region0 = region;\n"
                    "region += QRect(100, 100, 200, 200);\n"
                    "region1 = region;\n"
                    "region += QRect(300, 300, 400, 500);\n"
                    "region2 = region;\n"
                    "unused(&region0, &region1, &region2);\n")
               % GuiProfile()
               % Check("region0", Value4("<empty>"), "@QRegion")
               % Check("region0", Value5("<0 items>"), "@QRegion")
               % Check("region1", "<1 items>", "@QRegion")
               % Check("region1.extents", "200x200+100+100", "@QRect")
               % Check("region1.innerArea", "40000", "int")
               % Check("region1.innerRect", "200x200+100+100", "@QRect")
               % Check("region1.numRects", "1", "int")
               // This seems to be 0(!) items on Linux, 1 on Mac
               // % Check("region1.rects", "<1 items>", "@QVector<@QRect>")
               % Check("region2", "<2 items>", "@QRegion")
               % Check("region2.extents", "600x700+100+100", "@QRect")
               % Check("region2.innerArea", "200000", "int")
               % Check("region2.innerRect", "400x500+300+300", "@QRect")
               % Check("region2.numRects", "2", "int")
               % Check("region2.rects", "<2 items>", "@QVector<@QRect>");

    QTest::newRow("QSettings")
            << Data("#include <QSettings>\n"
                    "#include <QCoreApplication>\n"
                    "#include <QVariant>\n",
                    "QCoreApplication app(argc, argv);\n"
                    "QSettings settings(\"/tmp/test.ini\", QSettings::IniFormat);\n"
                    "QVariant value = settings.value(\"item1\", \"\").toString();\n")
               % CoreProfile()
               % Check("settings", "", "@QSettings")
               // FIXME
               //% Check("settings.[@QObject]", "", "@QObject")
               // FIXME
               % Check("value", "\"\"", "@QVariant (QString)");


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

               % CoreProfile()
               % Check("s1", "<2 items>", "@QSet<int>")
               % Check("s1.0", "[0]", "22", "int")
               % Check("s1.1", "[1]", "11", "int")

               % Check("s2", "<2 items>", "@QSet<@QString>")
               % Check("s2.0", "[0]", Value4("\"11.0\""), "@QString")
               % Check("s2.0", "[0]", Value5("\"22.0\""), "@QString")
               % Check("s2.1", "[1]", Value4("\"22.0\""), "@QString")
               % Check("s2.1", "[1]", Value5("\"11.0\""), "@QString")

               % Check("s3", "<1 items>", "@QSet<@QPointer<@QObject>>")
               % Check("s3.0", "[0]", "", "@QPointer<@QObject>");


    QByteArray sharedData =
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


    QTest::newRow("QSharedPointer1")
            << Data("#include <QSharedPointer>\n",
                    "QSharedPointer<int> ptr;\n"
                    "QSharedPointer<int> ptr2 = ptr;\n"
                    "QSharedPointer<int> ptr3 = ptr;\n"
                    "unused(&ptr, &ptr2, &ptr3);\n")
               % CoreProfile()
               % Check("ptr", "(null)", "@QSharedPointer<int>")
               % Check("ptr2", "(null)", "@QSharedPointer<int>")
               % Check("ptr3", "(null)", "@QSharedPointer<int>");

    QTest::newRow("QSharedPointer2")
            << Data("#include <QSharedPointer>\n",
                    "QSharedPointer<QString> ptr(new QString(\"hallo\"));\n"
                    "QSharedPointer<QString> ptr2 = ptr;\n"
                    "QSharedPointer<QString> ptr3 = ptr;\n"
                    "unused(&ptr, &ptr2, &ptr3);\n")
               % CoreProfile()
               % Check("ptr", "", "@QSharedPointer<@QString>")
               % Check("ptr.data", "\"hallo\"", "@QString")
               % Check("ptr.weakref", "3", "int")
               % Check("ptr.strongref", "3", "int")
               % Check("ptr2.data", "\"hallo\"", "@QString")
               % Check("ptr3.data", "\"hallo\"", "@QString");

    QTest::newRow("QSharedPointer3")
            << Data("#include <QSharedPointer>\n",
                    "QSharedPointer<int> iptr(new int(43));\n"
                    "QWeakPointer<int> ptr(iptr);\n"
                    "QWeakPointer<int> ptr2 = ptr;\n"
                    "QWeakPointer<int> ptr3 = ptr;\n"
                    "unused(&ptr, &ptr2, &ptr3);\n")
               % CoreProfile()
               % Check("iptr", "43", "@QSharedPointer<int>")
               % Check("iptr.data", "43", "int")
               % Check("iptr.weakref", "4", "int")
               % Check("iptr.strongref", "1", "int")
               % Check("ptr3", "43", "@QWeakPointer<int>")
               % Check("ptr3.data", "43", "int");

    QTest::newRow("QSharedPointer4")
            << Data("#include <QSharedPointer>\n"
                    "#include <QString>\n",
                    "QSharedPointer<QString> sptr(new QString(\"hallo\"));\n"
                    "QWeakPointer<QString> ptr(sptr);\n"
                    "QWeakPointer<QString> ptr2 = ptr;\n"
                    "QWeakPointer<QString> ptr3 = ptr;\n"
                    "unused(&ptr, &ptr2, &ptr3);\n")
               % CoreProfile()
               % Check("sptr", "", "@QSharedPointer<@QString>")
               % Check("sptr.data", "\"hallo\"", "@QString")
               % Check("ptr3", "", "@QWeakPointer<@QString>");

    QTest::newRow("QSharedPointer5")
            << Data("#include <QSharedPointer>\n" + fooData,
                    "QSharedPointer<Foo> fptr(new Foo(1));\n"
                    "QWeakPointer<Foo> ptr(fptr);\n"
                    "QWeakPointer<Foo> ptr2 = ptr;\n"
                    "QWeakPointer<Foo> ptr3 = ptr;\n"
                    "unused(&ptr, &ptr2, &ptr3);\n")
               % CoreProfile()
               % Check("fptr", "", "@QSharedPointer<Foo>")
               % Check("fptr.data", "", "Foo")
               % Check("ptr3", "", "@QWeakPointer<Foo>");

    QTest::newRow("QXmlAttributes")
            << Data("#include <QXmlAttributes>\n",
                    "QXmlAttributes atts;\n"
                    "atts.append(\"name1\", \"uri1\", \"localPart1\", \"value1\");\n"
                    "atts.append(\"name2\", \"uri2\", \"localPart2\", \"value2\");\n"
                    "atts.append(\"name3\", \"uri3\", \"localPart3\", \"value3\");\n")
               % CoreProfile()
               % Profile("QT += xml\n")
               % Check("atts", "", "@QXmlAttributes")
               % Check("atts.attList", "<3 items>", "@QXmlAttributes::AttributeList")
               % Check("atts.attList.0", "[0]", "", "@QXmlAttributes::Attribute")
               % Check("atts.attList.0.localname", "\"localPart1\"", "@QString")
               % Check("atts.attList.0.qname", "\"name1\"", "@QString")
               % Check("atts.attList.0.uri", "\"uri1\"", "@QString")
               % Check("atts.attList.0.value", "\"value1\"", "@QString")
               % Check("atts.attList.1", "[1]", "", "@QXmlAttributes::Attribute")
               % Check("atts.attList.1.localname", "\"localPart2\"", "@QString")
               % Check("atts.attList.1.qname", "\"name2\"", "@QString")
               % Check("atts.attList.1.uri", "\"uri2\"", "@QString")
               % Check("atts.attList.1.value", "\"value2\"", "@QString")
               % Check("atts.attList.2", "[2]", "", "@QXmlAttributes::Attribute")
               % Check("atts.attList.2.localname", "\"localPart3\"", "@QString")
               % Check("atts.attList.2.qname", "\"name3\"", "@QString")
               % Check("atts.attList.2.uri", "\"uri3\"", "@QString")
               % Check("atts.attList.2.value", "\"value3\"", "@QString");

    QTest::newRow("StdArray")
            << Data("#include <array>\n"
                    "#include <QString>\n",
                    "std::array<int, 4> a = { { 1, 2, 3, 4} };\n"
                    "std::array<QString, 4> b = { { \"1\", \"2\", \"3\", \"4\"} };\n"
                    "unused(&a, &b);\n")
               % CoreProfile()
               % Cxx11Profile()
               % MacLibCppProfile()
               % Check("a", "<4 items>", Pattern("std::array<int, 4.*>"))
               % Check("a.0", "[0]", "1", "int")
               % Check("b", "<4 items>", Pattern("std::array<@QString, 4.*>"))
               % Check("b.0", "[0]", "\"1\"", "@QString");

    QTest::newRow("StdComplex")
            << Data("#include <complex>\n",
                    "std::complex<double> c(1, 2);\n"
                    "unused(&c);\n")
               % Check("c", "(1.000000, 2.000000)", "std::complex<double>")
               % CheckType("c.real", "double");

    QTest::newRow("CComplex")
            << Data("#include <complex.h>\n",
                    "// Doesn't work when compiled as C++.\n"
                    "double complex a = 0;\n"
                    "double _Complex b = 0;\n"
                    "unused(&a, &b);\n")
               % ForceC()
               % Check("a", "0 + 0 * I", "complex double").setForGdbOnly()
               % Check("b", "0 + 0 * I", "complex double").setForGdbOnly()
               % Check("a", "0 + 0i", "_Complex double").setForLldbOnly()
               % Check("b", "0 + 0i", "_Complex double").setForLldbOnly();

    QTest::newRow("StdDequeInt")
            << Data("#include <deque>\n",
                    "std::deque<int> deque;\n"
                    "deque.push_back(1);\n"
                    "deque.push_back(2);\n")
               % Check("deque", "<2 items>", "std::deque<int>")
               % Check("deque.0", "[0]", "1", "int")
               % Check("deque.1", "[1]", "2", "int");

    QTest::newRow("StdDequeIntStar")
            << Data("#include <deque>\n",
                    "std::deque<int *> deque;\n"
                    "deque.push_back(new int(1));\n"
                    "deque.push_back(0);\n"
                    "deque.push_back(new int(2));\n"
                    "deque.push_back(new int(3));\n"
                    "deque.pop_back();\n"
                    "deque.pop_front();\n")
               % Check("deque", "<2 items>", "std::deque<int *>")
               % Check("deque.0", "[0]", "0x0", "int *")
               % Check("deque.1", "[1]", "2", "int");

    QTest::newRow("StdDequeFoo")
            << Data("#include <deque>\n" + fooData,
                    "std::deque<Foo> deque;\n"
                    "deque.push_back(1);\n"
                    "deque.push_front(2);\n")
               % Check("deque", "<2 items>", "std::deque<Foo>")
               % Check("deque.0", "[0]", "", "Foo")
               % Check("deque.0.a", "2", "int")
               % Check("deque.1", "[1]", "", "Foo")
               % Check("deque.1.a", "1", "int");

    QTest::newRow("StdDequeFooStar")
            << Data("#include <deque>\n" + fooData,
                    "std::deque<Foo *> deque;\n"
                    "deque.push_back(new Foo(1));\n"
                    "deque.push_back(new Foo(2));\n")
               % Check("deque", "<2 items>", "std::deque<Foo*>")
               % Check("deque.0", "[0]", "", "Foo")
               % Check("deque.0.a", "1", "int")
               % Check("deque.1", "[1]", "", "Foo")
               % Check("deque.1.a", "2", "int");

    QTest::newRow("StdHashSet")
            << Data("#include <hash_set>\n"
                    "using namespace __gnu_cxx;\n",
                    "hash_set<int> h;\n"
                    "h.insert(1);\n"
                    "h.insert(194);\n"
                    "h.insert(2);\n"
                    "h.insert(3);\n")
               % GdbEngine
               % Profile("QMAKE_CXXFLAGS += -Wno-deprecated")
               % Check("h", "<4 items>", "__gnu__cxx::hash_set<int>")
               % Check("h.0", "[0]", "194", "int")
               % Check("h.1", "[1]", "1", "int")
               % Check("h.2", "[2]", "2", "int")
               % Check("h.3", "[3]", "3", "int");

    QTest::newRow("StdListInt")
            << Data("#include <list>\n",
                    "std::list<int> list;\n"
                    "list.push_back(1);\n"
                    "list.push_back(2);\n")
               % Check("list", "<2 items>", "std::list<int>")
               % Check("list.0", "[0]", "1", "int")
               % Check("list.1", "[1]", "2", "int");

    QTest::newRow("StdListIntStar")
            << Data("#include <list>\n",
                    "std::list<int *> list;\n"
                    "list.push_back(new int(1));\n"
                    "list.push_back(0);\n"
                    "list.push_back(new int(2));\n")
               % Check("list", "<3 items>", "std::list<int*>")
               % Check("list.0", "[0]", "1", "int")
               % Check("list.1", "[1]", "0x0", "int *")
               % Check("list.2", "[2]", "2", "int");

    QTest::newRow("StdListIntBig")
            << Data("#include <list>\n",
                    "std::list<int> list;\n"
                    "for (int i = 0; i < 10000; ++i)\n"
                    "    list.push_back(i);\n")
               % Check("list", "<>1000 items>", "std::list<int>")
               % Check("list.0", "[0]", "0", "int")
               % Check("list.999", "[999]", "999", "int");

    QTest::newRow("StdListFoo")
            << Data("#include <list>\n" + fooData,
                    "std::list<Foo> list;\n"
                    "list.push_back(15);\n"
                    "list.push_back(16);\n")
               % Check("list", "<2 items>", "std::list<Foo>")
               % Check("list.0", "[0]", "", "Foo")
               % Check("list.0.a", "15", "int")
               % Check("list.1", "[1]", "", "Foo")
               % Check("list.1.a", "16", "int");

    QTest::newRow("StdListFooStar")
            << Data("#include <list>\n" + fooData,
                    "std::list<Foo *> list;\n"
                    "list.push_back(new Foo(1));\n"
                    "list.push_back(0);\n"
                    "list.push_back(new Foo(2));\n"
                    "unused(&list);\n")
               % Check("list", "<3 items>", "std::list<Foo*>")
               % Check("list.0", "[0]", "", "Foo")
               % Check("list.0.a", "1", "int")
               % Check("list.1", "[1]", "0x0", "Foo *")
               % Check("list.2", "[2]", "", "Foo")
               % Check("list.2.a", "2", "int");

    QTest::newRow("StdListBool")
            << Data("#include <list>\n",
                    "std::list<bool> list;\n"
                    "list.push_back(true);\n"
                    "list.push_back(false);\n"
                    "unused(&list);\n")
               % Check("list", "<2 items>", "std::list<bool>")
               % Check("list.0", "[0]", "true", "bool")
               % Check("list.1", "[1]", "false", "bool");

    QTest::newRow("StdMapStringFoo")
            << Data("#include <map>\n"
                    "#include <QString>\n" + fooData,
                    "std::map<QString, Foo> map;\n"
                    "map[\"22.0\"] = Foo(22);\n"
                    "map[\"33.0\"] = Foo(33);\n"
                    "map[\"44.0\"] = Foo(44);\n"
                    "unused(&map);\n")
               % Check("map", "<3 items>", "std::map<@QString, Foo>")
               % Check("map.0", "[0]", "", "std::pair<@QString const, Foo>")
               % Check("map.0.first", "\"22.0\"", "@QString")
               % Check("map.0.second", "", "Foo")
               % Check("map.0.second.a", "22", "int")
               % Check("map.1", "[1]", "", "std::pair<@QString const, Foo>")
               % Check("map.2.first", "\"44.0\"", "@QString")
               % Check("map.2.second", "", "Foo")
               % Check("map.2.second.a", "44", "int");

    QTest::newRow("StdMapCharStarFoo")
            << Data("#include <map>\n" + fooData,
                    "std::map<const char *, Foo> map;\n"
                    "map[\"22.0\"] = Foo(22);\n"
                    "map[\"33.0\"] = Foo(33);\n"
                    "unused(&map);\n")
               % Check("map", "<2 items>", "std::map<char const*, Foo>")
               % Check("map.0", "[0]", "", "std::pair<char const* const, Foo>")
               % CheckType("map.0.first", "char *")
// FIXME
               //% Check("map.0.first.0", "50", "char")
               % Check("map.0.second", "", "Foo")
               % Check("map.0.second.a", "22", "int")
               % Check("map.1", "[1]", "", "std::pair<char const* const, Foo>")
               % CheckType("map.1.first", "char *")
               //% Check("map.1.first.*first", "51 '3'", "char")
               % Check("map.1.second", "", "Foo")
               % Check("map.1.second.a", "33", "int");

    QTest::newRow("StdMapUIntUInt")
            << Data("#include <map>\n",
                    "std::map<unsigned int, unsigned int> map;\n"
                    "map[11] = 1;\n"
                    "map[22] = 2;\n"
                    "unused(&map);\n")
               % Check("map", "<2 items>", "std::map<unsigned int, unsigned int>")
               % Check("map.11", "[11]", "1", "unsigned int")
               % Check("map.22", "[22]", "2", "unsigned int");

    QTest::newRow("StdMapUIntStringList")
            << Data("#include <map>\n"
                    "#include <QStringList>\n",
                    "std::map<uint, QStringList> map;\n"
                    "map[11] = QStringList() << \"11\";\n"
                    "map[22] = QStringList() << \"22\";\n"
                    "unused(&map);\n")
               % Check("map", "<2 items>", "std::map<unsigned int, @QStringList>")
               % Check("map.0", "[0]", "", "std::pair<unsigned int const, @QStringList>")
               % Check("map.0.first", "11", "unsigned int")
               % Check("map.0.second", "<1 items>", "@QStringList")
               % Check("map.0.second.0", "[0]", "\"11\"", "@QString")
               % Check("map.1", "[1]", "", "std::pair<unsigned int const, @QStringList>")
               % Check("map.1.first", "22", "unsigned int")
               % Check("map.1.second", "<1 items>", "@QStringList")
               % Check("map.1.second.0", "[0]", "\"22\"", "@QString");

    QTest::newRow("StdMapUIntStringListTypedef")
            << Data("#include <map>\n"
                    "#include <QStringList>\n",
                    "typedef std::map<uint, QStringList> T;\n"
                    "T map;\n"
                    "map[11] = QStringList() << \"11\";\n"
                    "map[22] = QStringList() << \"22\";\n"
                    "unused(&map);\n")
               % Check("map.1.second.0", "[0]", "\"22\"", "@QString");

    QTest::newRow("StdMapUIntFloat")
            << Data("#include <map>\n",
                    "std::map<unsigned int, float> map;\n"
                    "map[11] = 11.0;\n"
                    "map[22] = 22.0;\n")
               % Check("map", "<2 items>", "std::map<unsigned int, float>")
               % Check("map.11", "[11]", "11", "float")
               % Check("map.22", "[22]", "22", "float");

    QTest::newRow("StdMapUIntFloatIterator")
            << Data("#include <map>\n",
                    "typedef std::map<int, float> Map;\n"
                    "Map map;\n"
                    "map[11] = 11.0;\n"
                    "map[22] = 22.0;\n"
                    "map[33] = 33.0;\n"
                    "map[44] = 44.0;\n"
                    "map[55] = 55.0;\n"
                    "map[66] = 66.0;\n"
                    "Map::iterator it1 = map.begin();\n"
                    "Map::iterator it2 = it1; ++it2;\n"
                    "Map::iterator it3 = it2; ++it3;\n"
                    "Map::iterator it4 = it3; ++it4;\n"
                    "Map::iterator it5 = it4; ++it5;\n"
                    "Map::iterator it6 = it5; ++it6;\n"
                    "unused(&it6);\n")
               % Check("map", "<6 items>", "Map")
               % Check("map.11", "[11]", "11", "float")
               % Check("it1.first", "11", "int")
               % Check("it1.second", "11", "float")
               % Check("it6.first", "66", "int")
               % Check("it6.second", "66", "float");

    QTest::newRow("StdMapStringFloat")
            << Data("#include <map>\n"
                    "#include <QString>\n",
                    "std::map<QString, float> map;\n"
                    "map[\"11.0\"] = 11.0;\n"
                    "map[\"22.0\"] = 22.0;\n"
                    "unused(&map);\n")
               % Check("map", "<2 items>", "std::map<@QString, float>")
               % Check("map.0", "[0]", "", "std::pair<@QString const, float>")
               % Check("map.0.first", "\"11.0\"", "@QString")
               % Check("map.0.second", "11", "float")
               % Check("map.1", "[1]", "", "std::pair<@QString const, float>")
               % Check("map.1.first", "\"22.0\"", "@QString")
               % Check("map.1.second", "22", "float");

    QTest::newRow("StdMapIntString")
            << Data("#include <map>\n"
                    "#include <QString>\n",
                    "std::map<int, QString> map;\n"
                    "map[11] = \"11.0\";\n"
                    "map[22] = \"22.0\";\n"
                    "unused(&map);\n")
               % Check("map", "<2 items>", "std::map<int, @QString>")
               % Check("map.0", "[0]", "", "std::pair<int const, @QString>")
               % Check("map.0.first", "11", "int")
               % Check("map.0.second", "\"11.0\"", "@QString")
               % Check("map.1", "[1]", "", "std::pair<int const, @QString>")
               % Check("map.1.first", "22", "int")
               % Check("map.1.second", "\"22.0\"", "@QString");

    QTest::newRow("StdMapStringPointer")
            << Data("#include <QPointer>\n"
                    "#include <QObject>\n"
                    "#include <QString>\n"
                    "#include <map>\n",
                    "QObject ob;\n"
                    "std::map<QString, QPointer<QObject> > map;\n"
                    "map[\"Hallo\"] = QPointer<QObject>(&ob);\n"
                    "map[\"Welt\"] = QPointer<QObject>(&ob);\n"
                    "map[\".\"] = QPointer<QObject>(&ob);\n"
                    "unused(&map);\n")
               % Check("map", "<3 items>", "std::map<@QString, @QPointer<@QObject>>")
               % Check("map.0", "[0]", "", "std::pair<@QString const, @QPointer<@QObject>>")
               % Check("map.0.first", "\".\"", "@QString")
               % Check("map.0.second", "", "@QPointer<@QObject>")
               % Check("map.2", "[2]", "", "std::pair<@QString const, @QPointer<@QObject>>")
               % Check("map.2.first", "\"Welt\"", "@QString");

    QTest::newRow("StdUniquePtr")
            << Data("#include <memory>\n" + fooData,
                    "std::unique_ptr<int> pi(new int(32));\n"
                    "std::unique_ptr<Foo> pf(new Foo);\n"
                    "unused(&pi, &pf);\n")
               % Cxx11Profile()
               % MacLibCppProfile()
               % Check("pi", Pointer("32"), "std::unique_ptr<int, std::default_delete<int> >")
               % Check("pi.data", "32", "int")
               % Check("pf", Pointer(), "std::unique_ptr<Foo, std::default_delete<Foo> >")
               % CheckType("pf.data", "Foo");

    QTest::newRow("StdSharedPtr")
            << Data("#include <memory>\n" + fooData,
                    "std::shared_ptr<int> pi(new int(32));\n"
                    "std::shared_ptr<Foo> pf(new Foo);\n"
                    "unused(&pi, &pf);\n")
               % Cxx11Profile()
               % MacLibCppProfile()
               % Check("pi", Pointer("32"), "std::shared_ptr<int>")
               % Check("pi.data", "32", "int")
               % Check("pf", Pointer(), "std::shared_ptr<Foo>")
               % CheckType("pf.data", "Foo");


    QTest::newRow("StdSet")
            << Data("#include <set>\n",
                    "std::set<int> set;\n"
                    "set.insert(11);\n"
                    "set.insert(22);\n"
                    "set.insert(33);\n"
                    "unused(&set);\n")
               % Check("set", "<3 items>", "std::set<int>");


    QTest::newRow("StdSetIterator")
            << Data("#include <set>\n",
                    "typedef std::set<int> Set;\n"
                    "Set set;\n"
                    "set.insert(11.0);\n"
                    "set.insert(22.0);\n"
                    "set.insert(33.0);\n"
                    "set.insert(44.0);\n"
                    "set.insert(55.0);\n"
                    "set.insert(66.0);\n"
                    "Set::iterator it1 = set.begin();\n"
                    "Set::iterator it2 = it1; ++it2;\n"
                    "Set::iterator it3 = it2; ++it3;\n"
                    "Set::iterator it4 = it3; ++it4;\n"
                    "Set::iterator it5 = it4; ++it5;\n"
                    "Set::iterator it6 = it5; ++it6;\n"
                    "unused(&it6);\n")
               % Check("set", "<6 items>", "Set")
               % Check("it1.value", "11", "int")
               % Check("it6.value", "66", "int");


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

               % Check("set1", "<1 items>", "std::set<@QString>")
               % Check("set1.0", "[0]", "\"22.0\"", "@QString")

               % Check("set2", "<1 items>", "std::set<@QPointer<@QObject>, "
                    "std::less<@QPointer<@QObject>>, std::allocator<@QPointer<@QObject>>>")
               % Check("ob", "", "@QObject")
               % Check("ptr", "", "@QPointer<@QObject>");


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

               % Check("s0", "<0 items>", "std::stack<int*>")

               % Check("s1", "<3 items>", "std::stack<int*>")
               % Check("s1.0", "[0]", "1", "int")
               % Check("s1.1", "[1]", "0x0", "int *")
               % Check("s1.2", "[2]", "2", "int")

               % Check("s2", "<0 items>", "std::stack<int>")

               % Check("s3", "<2 items>", "std::stack<int>")
               % Check("s3.0", "[0]", "1", "int")
               % Check("s3.1", "[1]", "2", "int");


    QTest::newRow("StdStackQt")
            << Data("#include <stack>\n" + fooData,
                    "std::stack<Foo *> s0, s1;\n"
                    "std::stack<Foo> s2, s3;\n"
                    "s1.push(new Foo(1));\n"
                    "s1.push(new Foo(2));\n"
                    "s3.push(1);\n"
                    "s3.push(2);\n"
                    "unused(&s0, &s1, &s2, &s3);\n")
               % CoreProfile()
               % Check("s0", "<0 items>", "std::stack<Foo*>")
               % Check("s1", "<2 items>", "std::stack<Foo*>")
               % Check("s1.0", "[0]", "", "Foo")
               % Check("s1.0.a", "1", "int")
               % Check("s1.1", "[1]", "", "Foo")
               % Check("s1.1.a", "2", "int")
               % Check("s2", "<0 items>", "std::stack<Foo>")
               % Check("s3", "<2 items>", "std::stack<Foo>")
               % Check("s3.0", "[0]", "", "Foo")
               % Check("s3.0.a", "1", "int")
               % Check("s3.1", "[1]", "", "Foo")
               % Check("s3.1.a", "2", "int");


    QTest::newRow("StdString1")
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
               % Check("str0", "\"\"", "std::string")
               % Check("wstr0", "\"\"", "std::wstring")
               % Check("str", "\"bdebdee\"", "std::string")
               % Check("wstr", "\"eeee\"", "std::wstring");

    QTest::newRow("StdString2")
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
               % CoreProfile()
               % Check("l0", "<0 items>", "@QList<std::string>")
               % Check("l", "<2 items>", "@QList<std::string>")
               % Check("str", "\"foo\"", "std::string")
               % Check("v", "<2 items>", "std::vector<std::string>")
               % Check("v.0", "[0]", "\"foo\"", "std::string");


    QTest::newRow("StdVector")
            << Data("#include <vector>\n",
                    "std::vector<double> v0, v1;\n"
                    "v1.push_back(1);\n"
                    "v1.push_back(0);\n"
                    "v1.push_back(2);\n"

                    "std::vector<int *> v2, v3;\n"
                    "v3.push_back(new int(1));\n"
                    "v3.push_back(0);\n"
                    "v3.push_back(new int(2));\n"

                    "std::vector<int> v4;\n"
                    "v4.push_back(1);\n"
                    "v4.push_back(2);\n"
                    "v4.push_back(3);\n"
                    "v4.push_back(4);\n"

                    "unused(&v0, &v1, &v2, &v3, &v4);\n")

               % Check("v0", "<0 items>", "std::vector<double>")
               % Check("v1", "<3 items>", "std::vector<double>")
               % Check("v1.0", "[0]", "1", "double")
               % Check("v1.1", "[1]", "0", "double")
               % Check("v1.2", "[2]", "2", "double")

               % Check("v2", "<0 items>", "std::vector<int*>")
               % Check("v3", "<3 items>", "std::vector<int*>")
               % Check("v3.0", "[0]", "1", "int")
               % Check("v3.1", "[1]", "0x0", "int *")
               % Check("v3.2", "[2]", "2", "int")

               % Check("v4", "<4 items>", "std::vector<int>")
               % Check("v4.0", "[0]", "1", "int")
               % Check("v4.3", "[3]", "4", "int");


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

               % CoreProfile()
               % Check("v1", "<3 items>", "std::vector<Foo*>")
               % Check("v1.0", "[0]", "", "Foo")
               % Check("v1.0.a", "1", "int")
               % Check("v1.1", "[1]", "0x0", "Foo *")
               % Check("v1.2", "[2]", "", "Foo")
               % Check("v1.2.a", "2", "int")

               % Check("v2", "<4 items>", "std::vector<Foo>")
               % Check("v2.0", "[0]", "", "Foo")
               % Check("v2.1.a", "2", "int")
               % Check("v2.3", "[3]", "", "Foo");


    QTest::newRow("StdVectorBool")
            << Data("#include <vector>\n",
                    "std::vector<bool> v;\n"
                    "v.push_back(true);\n"
                    "v.push_back(false);\n"
                    "v.push_back(false);\n"
                    "v.push_back(true);\n"
                    "v.push_back(false);\n"
                    "std::vector<bool> v1(65, true);\n"
                    "std::vector<bool> v2(65);\n"
                    "unused(&v1, &v2);\n"
                    "unused(&v);\n")
               % Check("v", "<5 items>", "std::vector<bool>").setForGdbOnly()
               // Known issue: Clang produces "std::vector<std::allocator<bool>>
               % Check("v", "<5 items>", "std::vector<std::allocator<bool>>").setForLldbOnly()
               % Check("v.0", "[0]", "1", "bool")
               % Check("v.1", "[1]", "0", "bool")
               % Check("v.2", "[2]", "0", "bool")
               % Check("v.3", "[3]", "1", "bool")
               % Check("v.4", "[4]", "0", "bool")
               % Check("v1", "<65 items>", "std::vector<bool>").setForGdbOnly()
               % Check("v1", "<65 items>", "std::vector<std::allocator<bool>>").setForLldbOnly()
               % Check("v1.0", "[0]", "1", "bool")
               % Check("v1.64", "[64]", "1", "bool")
               % Check("v2", "<65 items>", "std::vector<bool>").setForGdbOnly()
               % Check("v2", "<65 items>", "std::vector<std::allocator<bool>>").setForLldbOnly()
               % Check("v2.0", "[0]", "0", "bool")
               % Check("v2.64", "[64]", "0", "bool");


    QTest::newRow("StdVector6")
            << Data("#include <vector>\n"
                    "#include <list>\n",
                    "std::vector<std::list<int> *> vector;\n"
                    "std::list<int> list;\n"
                    "vector.push_back(new std::list<int>(list));\n"
                    "vector.push_back(0);\n"
                    "list.push_back(45);\n"
                    "vector.push_back(new std::list<int>(list));\n"
                    "vector.push_back(0);\n"
                    "unused(&vector);\n")
               % Check("list", "<1 items>", "std::list<int>")
               % Check("list.0", "[0]", "45", "int")
               % Check("vector", "<4 items>", "std::vector<std::list<int>*>")
               % Check("vector.0", "[0]", "<0 items>", "std::list<int>")
               % Check("vector.2", "[2]", "<1 items>", "std::list<int>")
               % Check("vector.2.0", "[0]", "45", "int")
               % Check("vector.3", "[3]", "0x0", "std::list<int> *");

    QTest::newRow("StdStream")
            << Data("#include <istream>\n"
                    "#include <fstream>\n",
                    "using namespace std;\n"
                    "ifstream is0, is;\n"
                    "#ifdef Q_OS_WIN\n"
                    "        is.open(\"C:\\\\Program Files\\\\Windows NT\\\\Accessories\\\\wordpad.exe\");\n"
                    "#else\n"
                    "        is.open(\"/etc/passwd\");\n"
                    "#endif\n"
                    "bool ok = is.good();\n"
                    "unused(&ok, &is, &is0);\n")
               % Check("is", "", "std::ifstream")
               % Check("ok", "true", "bool");


    QTest::newRow("StdUnorderedMap")
            << Data("#include <unordered_map>\n"
                    "#include <string>\n",

                    "std::unordered_map<unsigned int, unsigned int> map1;\n"
                    "map1[11] = 1;\n"
                    "map1[22] = 2;\n"

                    "std::unordered_map<std::string, float> map2;\n"
                    "map2[\"11.0\"] = 11.0;\n"
                    "map2[\"22.0\"] = 22.0;\n"

                    "unused(&map1);\n")

               % Cxx11Profile()

               % Check("map1", "<2 items>", "std::unordered_map<unsigned int, unsigned int>")
               % Check("map1.0", "[0] 22", "2", "unsigned int")
               % Check("map1.1", "[1] 11", "1", "unsigned int")

               % Check("map2", "<2 items>", "std::unordered_map<std::string, float>")
               //% Check("map2.0", "[0]", "", "std::pair<std:string const, float>")
               % Check("map2.0.first", "\"22.0\"", "std::string")
               % Check("map2.0.second", "22", "float")
               //% Check("map2.1", "[1]", "", "std::pair<std::string const, float>")
               % Check("map2.1.first", "\"11.0\"", "std::string")
               % Check("map2.1.second", "11", "float");


    QTest::newRow("StdUnorderedSet")
            << Data("#include <unordered_set>\n",
                    "std::unordered_set<int> set1;\n"
                    "set1.insert(11);\n"
                    "set1.insert(22);\n"
                    "set1.insert(33);\n"
                    "unused(&set1);\n")
               % Cxx11Profile()
               % Check("set1", "<3 items>", "std::unordered_set<int>")
               % Check("set1.0", "[0]", "33", "int")
               % Check("set1.1", "[1]", "22", "int")
               % Check("set1.2", "[2]", "11", "int");


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
               % GuiProfile()
               % Check("i1", "", "@QStandardItem")
               % Check("i11", "", "@QStandardItem")
               % Check("i2", "", "@QStandardItem")
               % Check("m", "", "@QStandardItemModel")
               % Check("mi", "\"1\"", "@QModelIndex");

    QTest::newRow("QStackInt")
            << Data("#include <QStack>\n",
                    "QStack<int> s;\n"
                    "s.append(1);\n"
                    "s.append(2);\n"
                    "unused(&s);\n")
               % Check("s", "<2 items>", "@QStack<int>")
               % Check("s.0", "[0]", "1", "int")
               % Check("s.1", "[1]", "2", "int");

    QTest::newRow("QStackBig")
            << Data("#include <QStack>\n",
                    "QStack<int> s;\n"
                    "for (int i = 0; i != 10000; ++i)\n"
                    "    s.append(i);\n"
                    "unused(&s);\n")
               % CoreProfile()
               % Check("s", "<10000 items>", "@QStack<int>")
               % Check("s.0", "[0]", "0", "int")
               % Check("s.1999", "[1999]", "1999", "int");

    QTest::newRow("QStackFooPointer")
            << Data("#include <QStack>\n" + fooData,
                    "QStack<Foo *> s;\n"
                    "s.append(new Foo(1));\n"
                    "s.append(0);\n"
                    "s.append(new Foo(2));\n"
                    "unused(&s);\n")
               % CoreProfile()
               % Check("s", "<3 items>", "@QStack<Foo*>")
               % Check("s.0", "[0]", "", "Foo")
               % Check("s.0.a", "1", "int")
               % Check("s.1", "[1]", "0x0", "Foo *")
               % Check("s.2", "[2]", "", "Foo")
               % Check("s.2.a", "2", "int");

    QTest::newRow("QStackFoo")
            << Data("#include <QStack>\n" + fooData,
                    "QStack<Foo> s;\n"
                    "s.append(1);\n"
                    "s.append(2);\n"
                    "s.append(3);\n"
                    "s.append(4);\n")
               % CoreProfile()
               % Check("s", "<4 items>", "@QStack<Foo>")
               % Check("s.0", "[0]", "", "Foo")
               % Check("s.0.a", "1", "int")
               % Check("s.3", "[3]", "", "Foo")
               % Check("s.3.a", "4", "int");

    QTest::newRow("QStackBool")
            << Data("#include <QStack>",
                    "QStack<bool> s;\n"
                    "s.append(true);\n"
                    "s.append(false);\n")
               % CoreProfile()
               % Check("s", "<2 items>", "@QStack<bool>")
               % Check("s.0", "[0]", "1", "bool")  // 1 -> true is done on display
               % Check("s.1", "[1]", "0", "bool");

    QTest::newRow("QTimeZone")
            << Data("#include <QTimeZone>\n",
                    "QTimeZone tz0;\n"
                    "QTimeZone tz1(\"UTC+05:00\");\n"
                    "unused(&tz0, &tz1);\n")
               % CoreProfile()
               % QtVersion(50200)
               % Check("tz0", "(null)", "@QTimeZone")
               % Check("tz1", "\"UTC+05:00\"", "@QTimeZone");


    QTest::newRow("QUrl")
            << Data("#include <QUrl>",
                    "QUrl url0;\n"
                    "QUrl url1 = QUrl::fromEncoded(\"http://foo@qt-project.org:10/have_fun\");\n"
                    "unused(&url0, &url1);\n")
               % CoreProfile()
               % Check("url0", "<invalid>", "@QUrl")
               % Check("url1", UnsubstitutedValue("\"http://foo@qt-project.org:10/have_fun\""), "@QUrl")
               % Check("url1.port", "10", "int")
               % Check("url1.scheme", "\"http\"", "@QString")
               % Check("url1.userName", "\"foo\"", "@QString")
               % Check("url1.password", "\"\"", "@QString")
               % Check("url1.host", "\"qt-project.org\"", "@QString")
               % Check("url1.path", "\"/have_fun\"", "@QString")
               % Check("url1.query", "\"\"", Type4("@QByteArray"))
               % Check("url1.query", "\"\"", Type5("@QString"))
               % Check("url1.fragment", "\"\"", "@QString");


    QTest::newRow("QStringQuotes")
            << Data("#include <QString>\n",
                    "QString str1(\"Hello Qt\");\n"
                    // --> Value: \"Hello Qt\"
                    "QString str2(\"Hello\\nQt\");\n"
                    // --> Value: \\"\"Hello\nQt"" (double quote not expected)
                    "QString str3(\"Hello\\rQt\");\n"
                    // --> Value: ""HelloQt"" (double quote and missing \r not expected)
                    "QString str4(\"Hello\\tQt\");\n"
                    "unused(&str1, &str2, &str3, &str3);\n")
               // --> Value: "Hello\9Qt" (expected \t instead of \9)
               % CoreProfile()
               % Check("str1", "\"Hello Qt\"", "@QString")
               % Check("str2", "\"Hello\nQt\"", "@QString")
               % Check("str3", "\"Hello\rQt\"", "@QString")
               % Check("str4", "\"Hello\tQt\"", "@QString");

    QTest::newRow("QString0")
            << Data("#include <QByteArray>\n",
                    "QByteArray str = \"Hello\";\n"
                    "str.prepend(\"Prefix: \");\n"
                    "unused(&str);\n")
               % CoreProfile()
               % Check("str", "\"Prefix: Hello\"", "@QByteArray");

    QByteArray expected1 = "\"AAA";
    expected1.append(char('\t'));
    expected1.append(char('\r'));
    expected1.append(char('\n'));
    expected1.append(char(0));
    expected1.append(char(1));
    expected1.append("BBB\"");

    QTest::newRow("QString1")
            << Data("#include <QByteArray>\n",
                    "QByteArray str = \"AAA\";\n"
                    "str += '\\t';\n"
                    "str += '\\r';\n"
                    "str += '\\n';\n"
                    "str += char(0);\n"
                    "str += char(1);\n"
                    "str += \"BBB\";\n"
                    "unused(&str);\n")
               % CoreProfile()
               % Check("str", expected1, "@QByteArray");

    QTest::newRow("QString2")
            << Data("#include <QString>\n",
                    "QChar data[] = { 'H', 'e', 'l', 'l', 'o' };\n"
                    "QString str1 = QString::fromRawData(data, 4);\n"
                    "QString str2 = QString::fromRawData(data + 1, 4);\n"
                    "unused(&data, &str1, &str2);\n")
               % CoreProfile()
               % Check("str1", "\"Hell\"", "@QString")
               % Check("str2", "\"ello\"", "@QString");

    QTest::newRow("QString3")
            << Data("#include <QString>\n"
                    "void stringRefTest(const QString &refstring) { BREAK; unused(&refstring); }\n",
                    "stringRefTest(QString(\"Ref String Test\"));\n")
               % CoreProfile()
               % Check("refstring", "\"Ref String Test\"", "@QString &");

    QTest::newRow("QString4")
            << Data("#include <QString>\n",
                    "QString str = \"Hello \";\n"
                    "QString string(\"String Test\");\n"
                    "QString *pstring = new QString(\"Pointer String Test\");\n"
                    "unused(&str, &string, &pstring);\n")
               % CoreProfile()
               % Check("pstring", "\"Pointer String Test\"", "@QString")
               % Check("str", "\"Hello \"", "@QString")
               % Check("string", "\"String Test\"", "@QString");

    QTest::newRow("QStringRef1")
            << Data("#include <QStringRef>\n",
                    "QString str = \"Hello\";\n"
                    "QStringRef ref1(&str, 1, 2);\n"
                    "QStringRef ref2;\n"
                    "unused(&ref1, &ref2);\n")
               % CoreProfile()
               % Check("ref1", "\"el\"", "@QStringRef")
               % Check("ref2", "(null)", "@QStringRef");

    QTest::newRow("QStringList")
            << Data("#include <QStringList>\n",
                    "QStringList l;\n"
                    "l << \"Hello \";\n"
                    "l << \" big, \";\n"
                    "l.takeFirst();\n"
                    "l << \" World \";\n")
               % CoreProfile()
               % Check("l", "<2 items>", "@QStringList")
               % Check("l.0", "[0]", "\" big, \"", "@QString")
               % Check("l.1", "[1]", "\" World \"", "@QString");

    QChar oUmlaut = QLatin1Char((char)0xf6);
    QTest::newRow("String")
            << Data("#include <QString>",
                    "const wchar_t *w = L\"aöa\";\n"
                    "QString u;\n"
                    "if (sizeof(wchar_t) == 4)\n"
                    "    u = QString::fromUcs4((uint *)w);\n"
                    "else\n"
                    "    u = QString::fromUtf16((ushort *)w);\n"
                    "unused(&w, &u);\n")
               % CoreProfile()
               % Check("u", QString::fromLatin1("\"a%1a\"").arg(oUmlaut), "@QString")
               % CheckType("w", "w", "wchar_t *");

        // All: Select UTF-8 in "Change Format for Type" in L&W context menu");
        // Windows: Select UTF-16 in "Change Format for Type" in L&W context menu");
        // Other: Select UCS-6 in "Change Format for Type" in L&W context menu");


    // These tests should result in properly displayed umlauts in the
    // Locals&Watchers view. It is only support on gdb with Python");
    QTest::newRow("CharPointers")
            << Data("const char *s = \"aöa\";\n"
                    "const char *t = \"a\\xc3\\xb6\";\n"
                    "const unsigned char uu[] = { 'a', 153 /* ö Latin1 */, 'a' };\n"
                    "const unsigned char *u = uu;\n"
                    "const wchar_t *w = L\"aöa\";\n"
                    "unused(&s, &t, &uu, &u, &w);\n")
               % CheckType("u", "unsigned char *")
               % CheckType("uu", "unsigned char [3]")
               % CheckType("s", "char *")
               % CheckType("t", "char *")
               % CheckType("w", "wchar_t *");

        // All: Select UTF-8 in "Change Format for Type" in L&W context menu");
        // Windows: Select UTF-16 in "Change Format for Type" in L&W context menu");
        // Other: Select UCS-6 in "Change Format for Type" in L&W context menu");

    QTest::newRow("CharArrays")
            << Data("const char s[] = \"aöa\";\n"
                    "const char t[] = \"aöax\";\n"
                    "const wchar_t w[] = L\"aöa\";\n"
                    "unused(&s, &t, &w);\n")
               % CheckType("s", "char [5]")
               % CheckType("t", "char [6]")
               % CheckType("w", "wchar_t [4]");

    QTest::newRow("Text")
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
               % GuiProfile()
               % CheckType("doc", "@QTextDocument")
               % Check("tc", "4", "@QTextCursor")
               % Check("pos", "4", "int")
               % Check("anc", "1", "int");

    QTest::newRow("QThread1")
            << Data("#include <QThread>\n"
                    "struct Thread : QThread\n"
                    "{\n"
                    "    void run()\n"
                    "    {\n"
                    "        if (m_id == 3)\n"
                    "            BREAK;\n"
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
               % CoreProfile()
               % CheckType("this", "Thread")
               % Check("this.@1", "[@QThread]", "\"This is thread #3\"", "@QThread")
               % Check("this.@1.@1", "[@QObject]", "\"This is thread #3\"", "@QObject");

    QTest::newRow("QVariant0")
            << Data("#include <QVariant>\n",
                    "QVariant value;\n"
                    "QVariant::Type t = QVariant::String;\n"
                    "value = QVariant(t, (void*)0);\n"
                    "*(QString*)value.data() = QString(\"Some string\");\n")
               % CoreProfile()
               % Check("value", "\"Some string\"", "@QVariant (QString)");

    QTest::newRow("QVariant1")
            << Data("#include <QVariant>\n",
                    "QVariant value;\n"
                    "QVariant::Type t = QVariant::String;\n"
                    "value = QVariant(t, (void*)0);\n"
                    "*(QString*)value.data() = QString(\"Some string\");\n")
               % CoreProfile()
               % Check("value", "\"Some string\"", "@QVariant (QString)");

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
                    "#include <QQuaternion>\n",
                    "QApplication app(argc, argv);\n"
                    "QRect r(100, 200, 300, 400);\n"
                    "QRectF rf(100.5, 200.5, 300.5, 400.5);\n"
                    "QUrl url = QUrl::fromEncoded(\"http://foo@qt-project.org:10/have_fun\");\n"
                    "QVariant var;                                  // Type 0, invalid\n"
                    "QVariant var1(true);                           // 1, bool\n"
                    "QVariant var2(2);                              // 2, int\n"
                    "QVariant var3(3u);                             // 3, uint\n"
                    "QVariant var4(qlonglong(4));                   // 4, qlonglong\n"
                    "QVariant var5(qulonglong(5));                  // 5, qulonglong\n"
                    "QVariant var6(double(6.0));                    // 6, double\n"
                    "QVariant var7(QChar(7));                       // 7, QChar\n"
                    "QVariant var8 = QVariantMap();                 // 8, QVariantMap\n"
                    "QVariant var9 = QVariantList();                // 9, QVariantList\n"
                    "QVariant var10 = QString(\"Hello 10\");        // 10, QString\n"
                    "QVariant var11 = QStringList() << \"Hello\" << \"World\"; // 11, QStringList\n"
                    "QVariant var12 = QByteArray(\"array\");        // 12 QByteArray\n"
                    "QVariant var13 = QBitArray();                  // 13 QBitArray\n"
                    "QVariant var14 = QDate();                      // 14 QDate\n"
                    "QVariant var15 = QTime();                      // 15 QTime\n"
                    "QVariant var16 = QDateTime();                  // 16 QDateTime\n"
                    "QVariant var17 = url;                          // 17 QUrl\n"
                    "QVariant var18 = QLocale(\"en_US\");           // 18 QLocale\n"
                    "QVariant var19(r);                             // 19 QRect\n"
                    "QVariant var20(rf);                            // 20 QRectF\n"
                    "QVariant var21 = QSize();                      // 21 QSize\n"
                    "QVariant var22 = QSizeF();                     // 22 QSizeF\n"
                    "QVariant var23 = QLine();                      // 23 QLine\n"
                    "QVariant var24 = QLineF();                     // 24 QLineF\n"
                    "QVariant var25 = QPoint();                     // 25 QPoint\n"
                    "QVariant var26 = QPointF();                    // 26 QPointF\n"
                    "QVariant var27 = QRegExp();                    // 27 QRegExp\n"
                    "QVariant var28 = QVariantHash();               // 28 QVariantHash\n"
                    "QVariant var31 = QVariant::fromValue<void *>(&r);         // 31 void *\n"
                    "QVariant var32 = QVariant::fromValue<long>(32);           // 32 long\n"
                    "QVariant var33 = QVariant::fromValue<short>(33);          // 33 short\n"
                    "QVariant var34 = QVariant::fromValue<char>(34);           // 34 char\n"
                    "QVariant var35 = QVariant::fromValue<unsigned long>(35);  // 35 unsigned long\n"
                    "QVariant var36 = QVariant::fromValue<unsigned short>(36); // 36 unsigned short\n"
                    "QVariant var37 = QVariant::fromValue<unsigned char>(37);  // 37 unsigned char\n"
                    "QVariant var38 = QVariant::fromValue<float>(38);          // 38 float\n"
                    "QVariant var64 = QFont();                      // 64 QFont\n"
                    "QVariant var65 = QPixmap();                    // 65 QPixmap\n"
                    "QVariant var66 = QBrush();                     // 66 QBrush\n"
                    "QVariant var67 = QColor();                     // 67 QColor\n"
                    "QVariant var68 = QPalette();                   // 68 QPalette\n"
                    "QVariant var69 = QIcon();                      // 69 QIcon\n"
                    "QVariant var70 = QImage();                     // 70 QImage\n"
                    "QVariant var71 = QPolygon();                   // 71 QPolygon\n"
                    "QVariant var72 = QRegion();                    // 72 QRegion\n"
                    "QVariant var73 = QBitmap();                    // 73 QBitmap\n"
                    "QVariant var74 = QCursor();                    // 74 QCursor\n"
                    "QVariant var75 = QKeySequence();               // 75 QKeySequence\n"
                    "QVariant var76 = QPen();                       // 76 QPen\n"
                    "QVariant var77 = QTextLength();                // 77 QTextLength\n"
                    "QVariant var78 = QTextFormat();                // 78 QTextFormat\n"
                    "QVariant var80 = QTransform();                 // 80 QTransform\n"
                    "QVariant var81 = QMatrix4x4();                 // 81 QMatrix4x4\n"
                    "QVariant var82 = QVector2D();                  // 82 QVector2D\n"
                    "QVariant var83 = QVector3D();                  // 83 QVector3D\n"
                    "QVariant var84 = QVector4D();                  // 84 QVector4D\n"
                    "QVariant var85 = QQuaternion();                // 85 QQuaternion\n"
                    "QVariant var86 = QPolygonF();                  // 86 QPolygonF\n"
                    "unused(&var, &var1, &var2, &var3, &var4, &var5, &var6);\n"
                    "unused(&var7, &var8, &var9, &var10, &var11, &var19, &var20);\n"
                    "unused(&var12, &var13, &var14, &var15, &var16, &var17);\n"
                    "unused(&var21, &var22, &var23, &var24, &var25, &var26);\n"
                    "unused(&var27, &var28, &var32, &var33, &var34, &var35);\n"
                    "unused(&var36, &var37, &var38, &var64, &var65, &var66);\n"
                    "unused(&var67, &var68, &var69, &var70, &var71, &var72);\n"
                    "unused(&var73, &var74, &var75, &var76, &var77, &var78);\n"
                    "unused(&var80, &var81, &var82, &var83, &var84, &var18);\n"
                    "unused(&var85, &var86);\n"
                    )
               % GuiProfile()
               % Check("var", "(invalid)", "@QVariant (invalid)")
               % Check("var1", "true", "@QVariant (bool)")
               % Check("var2", "2", "@QVariant (int)")
               % Check("var3", "3", "@QVariant (uint)")
               % Check("var4", "4", "@QVariant (qlonglong)")
               % Check("var5", "5", "@QVariant (qulonglong)")
               % Check("var6", "6.0", "@QVariant (double)")
               % Check("var7", "7", "@QVariant (QChar)")
               % Check("var8", "<0 items>", "@QVariant (QVariantMap)")
               % Check("var9", "<0 items>", "@QVariant (QVariantList)")
               % Check("var10", "\"Hello 10\"", "@QVariant (QString)")
               % Check("var11", "<2 items>", "@QVariant (QStringList)")
               % Check("var11.1", "[1]", "\"World\"", "@QString")
               % Check("var12", "\"array\"", "@QVariant (QByteArray)")
               % Check("var13", "", "@QVariant (QBitArray)")
               % Check("var14", "(invalid)", "@QVariant (QDate)")
               % Check("var15", "(invalid)", "@QVariant (QTime)")
               % Check("var16", "(invalid)", "@QVariant (QDateTime)")
               % Check("var17", UnsubstitutedValue("\"http://foo@qt-project.org:10/have_fun\""), "@QVariant (QUrl)")
               % Check("var17.port", "10", "int")
               % Check("var18", "\"en_US\"", "@QVariant (QLocale)")
               % Check("var19", "300x400+100+200", "@QVariant (QRect)")
               % Check("var20", "300.5x400.5+100.5+200.5", "@QVariant (QRectF)")
               % Check("var21", "(-1, -1)", "@QVariant (QSize)")
               % Check("var22", "(-1.0, -1.0)", "@QVariant (QSizeF)")
               % Check("var23", "", "@QVariant (QLine)")
               % Check("var24", "", "@QVariant (QLineF)")
               % Check("var25", "(0, 0)", "@QVariant (QPoint)")
               % Check("var26", "(0.0, 0.0)", "@QVariant (QPointF)")
               % Check("var27", "\"\"", "@QVariant (QRegExp)")
               % Check("var28", "<0 items>", "@QVariant (QVariantHash)")
               % CheckType("var31", "@QVariant (void *)")
               % Check("var32", "32", "@QVariant (long)")
               % Check("var33", "33", "@QVariant (short)")
               % Check("var34", "34", "@QVariant (char)")
               % Check("var35", "35", "@QVariant (unsigned long)")
               % Check("var36", "36", "@QVariant (unsigned short)")
               % Check("var37", "37", "@QVariant (unsigned char)")
               % Check("var38", FloatValue("38.0"), "@QVariant (float)")
               % Check("var64", "", "@QVariant (QFont)")
               % Check("var65", "(invalid)", "@QVariant (QPixmap)")
               % Check("var66", "", "@QVariant (QBrush)")
               % Check("var67", "", "@QVariant (QColor)")
               % Check("var68", "", "@QVariant (QPalette)")
               % Check("var69", "", "@QVariant (QIcon)")
               % Check("var70", "(invalid)", "@QVariant (QImage)")
               % Check("var71", "", "@QVariant (QPolygon)")
               //% Check("var72", "", "@QVariant (QRegion)")    FIXME
               % Check("var73", "", "@QVariant (QBitmap)")
               % Check("var74", "", "@QVariant (QCursor)")
               % Check("var75", "", "@QVariant (QKeySequence)")
               % Check("var76", "", "@QVariant (QPen)")
               % Check("var77", "", "@QVariant (QTextLength)")
               % Check("var78", "", "@QVariant (QTextFormat)")
               % Check("var80", "", "@QVariant (QTransform)")
               % Check("var81", "", "@QVariant (QMatrix4x4)")
               % Check("var82", "", "@QVariant (QVector2D)")
               % Check("var83", "", "@QVariant (QVector3D)")
               % Check("var84", "", "@QVariant (QVector4D)")
               % Check("var85", "", "@QVariant (QQuaternion)")
               % Check("var86", "", "@QVariant (QPolygonF)");


    QTest::newRow("QVariant3")
            << Data("#include <QVariant>\n",
                    "QVariant var;\n"
                    "unused(&var);\n")
               % CoreProfile()
               % Check("var", "(invalid)", "@QVariant (invalid)");

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
               % NetworkProfile()
               % UseDebugImage()
               % Check("ha", "\"127.0.0.1\"", "@QHostAddress")
               % Check("ha.a", "2130706433", "@quint32")
               % Check("ha.ipString", "\"127.0.0.1\"", "@QString")
               % Check("ha.isParsed", "true", "bool")
               % Check("ha.protocol", "@QAbstractSocket::IPv4Protocol (0)",
                       "@QAbstractSocket::NetworkLayerProtocol")
               % Check("ha.scopeId", "\"\"", "@QString")
               % Check("ha1", "\"127.0.0.1\"", "@QHostAddress")
               % Check("ha1.a", "2130706433", "@quint32")
               % Check("ha1.ipString", "\"127.0.0.1\"", "@QString")
               % Check("ha1.isParsed", "true", "bool")
               % Check("ha1.protocol", "@QAbstractSocket::IPv4Protocol (0)",
                       "@QAbstractSocket::NetworkLayerProtocol")
               % Check("ha1.scopeId", "\"\"", "@QString")
               % Check("var", "", "@QVariant (@QHostAddress)")
               % Check("var.data", "\"127.0.0.1\"", "@QHostAddress");

    QTest::newRow("QVariant5")
            << Data("#include <QMap>\n"
                    "#include <QStringList>\n"
                    "#include <QVariant>\n"
                    // This checks user defined types in QVariants\n";
                    "typedef QMap<uint, QStringList> MyType;\n"
                    "Q_DECLARE_METATYPE(QList<int>)\n"
                    "Q_DECLARE_METATYPE(QStringList)\n"
                    "Q_DECLARE_METATYPE(MyType)\n",
                    "MyType my;\n"
                    "my[1] = (QStringList() << \"Hello\");\n"
                    "my[3] = (QStringList() << \"World\");\n"
                    "QVariant var;\n"
                    "var.setValue(my);\n"
                    "int t = QMetaType::type(\"MyType\");\n"
                    "const char *s = QMetaType::typeName(t);\n"
                    "BREAK;\n"
                    "unused(&var, &t, &s);\n")
               % CoreProfile()
               % Check("my", "<2 items>", "MyType")
               % Check("my.0", "[0]", "", "@QMapNode<unsigned int, @QStringList>")
               % Check("my.0.key", "1", "unsigned int")
               % Check("my.0.value", "<1 items>", "@QStringList")
               % Check("my.0.value.0", "[0]", "\"Hello\"", "@QString")
               % Check("my.1", "[1]", "", "@QMapNode<unsigned int, @QStringList>")
               % Check("my.1.key", "3", "unsigned int")
               % Check("my.1.value", "<1 items>", "@QStringList")
               % Check("my.1.value.0", "[0]", "\"World\"", "@QString")
               % CheckType("var", "@QVariant (MyType)")
               % Check("var.data", "<2 items>", "MyType")
               % Check("var.data.0", "[0]", "", "@QMapNode<unsigned int, @QStringList>")
               % Check("var.data.0.key", "1", "unsigned int")
               % Check("var.data.0.value", "<1 items>", "@QStringList")
               % Check("var.data.0.value.0", "[0]", "\"Hello\"", "@QString")
               % Check("var.data.1", "[1]", "", "@QMapNode<unsigned int, @QStringList>")
               % Check("var.data.1.key", "3", "unsigned int")
               % Check("var.data.1.value", "<1 items>", "@QStringList")
               % Check("var.data.1.value.0", "[0]", "\"World\"", "@QString");

    QTest::newRow("QVariant6")
            << Data("#include <QList>\n"
                    "#include <QVariant>\n"
                    "Q_DECLARE_METATYPE(QList<int>)\n",
                    "QList<int> list;\n"
                    "list << 1 << 2 << 3;\n"
                    "QVariant variant = qVariantFromValue(list);\n"
                    "unused(&list, &variant);\n")
               % CoreProfile()
               % Check("list", "<3 items>", "@QList<int>")
               % Check("list.0", "[0]", "1", "int")
               % Check("list.1", "[1]", "2", "int")
               % Check("list.2", "[2]", "3", "int")
               % Check("variant", "", "@QVariant (@QList<int>)")
               % Check("variant.data", "<3 items>", "@QList<int>")
               % Check("variant.data.0", "[0]", "1", "int")
               % Check("variant.data.1", "[1]", "2", "int")
               % Check("variant.data.2", "[2]", "3", "int");

    QTest::newRow("QVariantList")
            << Data("#include <QVariantList>\n",
                    "QVariantList vl;\n"
                    "vl.append(QVariant(1));\n"
                    "vl.append(QVariant(2));\n"
                    "vl.append(QVariant(\"Some String\"));\n"
                    "vl.append(QVariant(21));\n"
                    "vl.append(QVariant(22));\n"
                    "vl.append(QVariant(\"2Some String\"));\n")
         % CoreProfile()
         % Check("vl", "<6 items>", "@QVariantList")
         % CheckType("vl.0", "[0]", "@QVariant (int)")
         % CheckType("vl.2", "[2]", "@QVariant (QString)");

    QTest::newRow("QVariantMap")
            << Data("#include <QVariantMap>\n",
                    "QVariantMap vm;\n"
                    "vm[\"a\"] = QVariant(1);\n"
                    "vm[\"b\"] = QVariant(2);\n"
                    "vm[\"c\"] = QVariant(\"Some String\");\n"
                    "vm[\"d\"] = QVariant(21);\n"
                    "vm[\"e\"] = QVariant(22);\n"
                    "vm[\"f\"] = QVariant(\"2Some String\");\n")
         % CoreProfile()
         % Check("vm", "<6 items>", "@QVariantMap")
         % Check("vm.0", "[0]", "", "@QMapNode<@QString, @QVariant>")
         % Check("vm.0.key", "\"a\"", "@QString")
         % Check("vm.0.value", "1", "@QVariant (int)")
         % Check("vm.5", "[5]", "", "@QMapNode<@QString, @QVariant>")
         % Check("vm.5.key", "\"f\"", "@QString")
         % Check("vm.5.value", "\"2Some String\"", "@QVariant (QString)");

    QTest::newRow("QVariantHash1")
            << Data("#include <QVariant>\n",
                    "QVariantHash h;\n"
                    "h[\"one\"] = \"vone\";\n"
                    "QVariant v = h;\n"
                    "unused(&v);\n")
               % CoreProfile()
               % Check("v", "<1 items>", "@QVariant (QVariantHash)")
               % Check("v.0", "[0]", "", "@QHashNode<@QString, @QVariant>")
               % Check("v.0.key", "\"one\"", "@QString")
               % Check("v.0.value", "\"vone\"", "@QVariant (QString)");

    QTest::newRow("QVariantMap1")
            << Data("#include <QVariant>\n",
                    "QVariantMap h;\n"
                    "h[\"one\"] = \"vone\";\n"
                    "QVariant v = h;\n"
                    "unused(&v);\n")
               % CoreProfile()
               % Check("v", "<1 items>", "@QVariant (QVariantMap)")
               % Check("v.0", "[0]", "", "@QMapNode<@QString, @QVariant>")
               % Check("v.0.key", "\"one\"", "@QString")
               % Check("v.0.value", "\"vone\"", "@QVariant (QString)");

    QTest::newRow("QVariantList1")
            << Data("#include <QVariant>\n",
                    "QVariantList h;\n"
                    "h.append(\"one\");\n"
                    "QVariant v = h;\n"
                    "unused(&v);\n")
               % CoreProfile()
               % Check("v", "<1 items>", "@QVariant (QVariantList)")
               % Check("v.0", "[0]", "\"one\"", "@QVariant (QString)");

    QTest::newRow("QVectorIntBig")
            << Data("#include <QVector>\n",
                   "QVector<int> vec(10000);\n"
                   "for (int i = 0; i != vec.size(); ++i)\n"
                   "     vec[i] = i * i;\n")
         % CoreProfile()
         % Check("vec", "<10000 items>", "@QVector<int>")
         % Check("vec.0", "[0]", "0", "int")
         % Check("vec.1999", "[1999]", "3996001", "int");

    QTest::newRow("QVectorFoo")
            << Data("#include <QVector>\n" + fooData,
                    "QVector<Foo> vec;\n"
                    "vec.append(1);\n"
                    "vec.append(2);\n")
         % CoreProfile()
         % Check("vec", "<2 items>", "@QVector<Foo>")
         % Check("vec.0", "[0]", "", "Foo")
         % Check("vec.0.a", "1", "int")
         % Check("vec.1", "[1]", "", "Foo")
         % Check("vec.1.a", "2", "int");

    QTest::newRow("QVectorFooTypedef")
            << Data("#include <QVector>\n" + fooData +
                    "typedef QVector<Foo> FooVector;\n",
                    "FooVector vec;\n"
                    "vec.append(1);\n"
                    "vec.append(2);\n")
         % CoreProfile()
         % Check("vec", "<2 items>", "FooVector")
         % Check("vec.0", "[0]", "", "Foo")
         % Check("vec.0.a", "1", "int")
         % Check("vec.1", "[1]", "", "Foo")
         % Check("vec.1.a", "2", "int");


    // This tests the display of a vector of pointers to custom structs");
    QTest::newRow("QVectorFooStar")
            << Data("#include <QVector>\n" + fooData,
                    "QVector<Foo *> vec;\n"
                    "vec.append(new Foo(1));\n"
                    "vec.append(0);\n"
                    "vec.append(new Foo(5));\n")
            % CoreProfile()
            % Check("vec", "<3 items>", "@QVector<Foo*>")
            % CheckType("vec.0", "[0]", "Foo")
            % Check("vec.0.a", "1", "int")
            % Check("vec.1", "[1]", "0x0", "Foo *")
            % CheckType("vec.2", "[2]", "Foo")
            % Check("vec.2.a", "5", "int");


    QTest::newRow("QVectorBool")
            << Data("#include <QVector>\n",
                    "QVector<bool> vec;\n"
                    "vec.append(true);\n"
                    "vec.append(false);\n")
            % CoreProfile()
            % Check("vec", "<2 items>", "@QVector<bool>")
            % Check("vec.0", "[0]", "1", "bool")
            % Check("vec.1", "[1]", "0", "bool");


//    QTest::newRow("QVectorListInt")
//                             << Data(
//        "QVector<QList<int> > vec;
//        "QVector<QList<int> > *pv = &vec;
//         % Check("pv", "QVector<QList<int>>");
//         % Check("vec <0 items> QVector<QList<int>>");
//        // Continue");
//        "vec.append(QList<int>() << 1);
//        "vec.append(QList<int>() << 2 << 3);
//         % Check("pv", "QVector<QList<int>>");
//         % Check("pv.0 <1 items> QList<int>");
//         % Check("pv.0.0", "1", "int");
//         % Check("pv.1 <2 items> QList<int>");
//         % Check("pv.1.0", "2", "int");
//         % Check("pv.1.1", "3", "int");
//         % Check("vec <2 items> QVector<QList<int>>");
//         % Check("vec.0 <1 items> QList<int>");
//         % Check("vec.0.0", "1", "int");
//         % Check("vec.1 <2 items> QList<int>");
//         % Check("vec.1.0", "2", "int");
//         % Check("vec.1.1", "3", "int");

//namespace noargs {

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

//        "QList<Goo> list2;
//        "list2.append(Goo("Hello", 1));
//        "list2.append(Goo("World", 2));

//         % Check("i", "1", "int");
//         % Check("k", "3", "int");
//         % Check("list <2 items> noargs::GooList");
//         % Check("list.0", "noargs::Goo");
//         % Check("list.0.n_", "1", "int");
//         % Check("list.0.str_ "Hello" QString");
//         % Check("list.1", "noargs::Goo");
//         % Check("list.1.n_", "2", "int");
//         % Check("list.1.str_ "World" QString");
//         % Check("list2 <2 items> QList<noargs::Goo>");
//         % Check("list2.0", "noargs::Goo");
//         % Check("list2.0.n_", "1", "int");
//         % Check("list2.0.str_ "Hello" QString");
//         % Check("list2.1", "noargs::Goo");
//         % Check("list2.1.n_", "2", "int");
//         % Check("list2.1.str_ "World" QString");


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
//         % CheckType("anon namespc::nested::(anonymous namespace)::MyAnon");
//         % Check("bar", "namespc::nested::MyBar");
//         % CheckType("baz namespc::nested::(anonymous namespace)::baz::MyBaz");
//         % Check("foo", "namespc::nested::MyFoo");
//        // Continue");
//        // step into the doit() functions

//        baz.doit(1);\n"
//        anon.doit(1);\n"
//        foo.doit(1);\n"
//        bar.doit(1);\n"
//        unused();\n"
//    }


    QTest::newRow("GccExtensions")
            << Data(
                   "char v[8] = { 1, 2 };\n"
                   "char w __attribute__ ((vector_size (8))) = { 1, 2 };\n"
                   "int y[2] = { 1, 2 };\n"
                   "int z __attribute__ ((vector_size (8))) = { 1, 2 };\n"
                   "unused(&v, &w, &y, &z);\n")
               % Check("v.0", "[0]", "1", "char")
               % Check("v.1", "[1]", "2", "char")
               % Check("w.0", "[0]", "1", "char")
               % Check("w.1", "[1]", "2", "char")
               % Check("y.0", "[0]", "1", "int")
               % Check("y.1", "[1]", "2", "int")
               % Check("z.0", "[0]", "1", "int")
               % Check("z.1", "[1]", "2", "int");


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
               % CoreProfile()
               % Check("u64", "18446744073709551615", "@quint64")
               % Check("s64", "9223372036854775807", "@qint64")
               % Check("u32", "4294967295", "@quint32")
               % Check("s32", "2147483647", "@qint32")
               % Check("u64s", "0", "@quint64")
               % Check("s64s", "-9223372036854775808", "@qint64")
               % Check("u32s", "0", "@quint32")
               % Check("s32s", "-2147483648", "@qint32");

    QTest::newRow("Array1")
            << Data("double d[3][3];\n"
                    "for (int i = 0; i != 3; ++i)\n"
                    "    for (int j = 0; j != 3; ++j)\n"
                    "        d[i][j] = i + j;\n"
                    "unused(&d);\n")
               % Check("d", Pointer(), "double [3][3]")
               % Check("d.0", "[0]", Pointer(), "double [3]")
               % Check("d.0.0", "[0]", "0", "double")
               % Check("d.0.2", "[2]", "2", "double")
               % Check("d.2", "[2]", Pointer(), "double [3]");

    QTest::newRow("Array2")
            << Data("char c[20] = { 0 };\n"
                    "c[0] = 'a';\n"
                    "c[1] = 'b';\n"
                    "c[2] = 'c';\n"
                    "c[3] = 'd';\n"
                    "c[4] = 0;\n"
                    "unused(&c);\n")
               % Check("c", "\"abcd" + QByteArray(16, 0) + '"', "char [20]")
               % Check("c.0", "[0]", "97", "char")
               % Check("c.3", "[3]", "100", "char");

    QTest::newRow("Array3")
            << Data("#include <QString>\n",
                    "QString b[20];\n"
                    "b[0] = \"a\";\n"
                    "b[1] = \"b\";\n"
                    "b[2] = \"c\";\n"
                    "b[3] = \"d\";\n")
               % CheckType("b", "@QString [20]")
               % Check("b.0", "[0]", "\"a\"", "@QString")
               % Check("b.3", "[3]", "\"d\"", "@QString")
               % Check("b.4", "[4]", "\"\"", "@QString")
               % Check("b.19", "[19]", "\"\"", "@QString");

    QTest::newRow("Array4")
            << Data("#include <QByteArray>\n",
                    "QByteArray b[20];\n"
                    "b[0] = \"a\";\n"
                    "b[1] = \"b\";\n"
                    "b[2] = \"c\";\n"
                    "b[3] = \"d\";\n")
               % CheckType("b", "@QByteArray [20]")
               % Check("b.0", "[0]", "\"a\"", "@QByteArray")
               % Check("b.3", "[3]", "\"d\"", "@QByteArray")
               % Check("b.4", "[4]", "\"\"", "@QByteArray")
               % Check("b.19", "[19]", "\"\"", "@QByteArray");

    QTest::newRow("Array5")
            << Data(fooData,
                    "Foo foo[10];\n"
                    "for (int i = 0; i < 5; ++i)\n"
                    "    foo[i].a = i;\n")
               % CheckType("foo", "Foo [10]")
               % Check("foo.0", "[0]", "", "Foo")
               % Check("foo.9", "[9]", "", "Foo");


    QTest::newRow("Bitfields")
            << Data("struct S\n"
                    "{\n"
                    "    S() : x(0), y(0), c(0), b(0), f(0), d(0), i(0) {}\n"
                    "    unsigned int x : 1;\n"
                    "    unsigned int y : 1;\n"
                    "    bool c : 1;\n"
                    "    bool b;\n"
                    "    float f;\n"
                    "    double d;\n"
                    "    int i;\n"
                    "} s;\n"
                    "unused(&s);\n")
               % Check("s", "", "S")
               % Check("s.b", "false", "bool")
               % Check("s.c", "false", "bool")
               % Check("s.d", "0", "double")
               % Check("s.f", "0", "float")
               % Check("s.i", "0", "int")
               % Check("s.x", "0", "unsigned int")
               % Check("s.y", "0", "unsigned int");


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
               % CoreProfile()
               % Check("func", "", "Function")
               % Check("func.min", "0", "double")
               % Check("func.var", "\"x\"", "@QByteArray")
               % Check("func", "", "Function")
               % Check("func.f", "\"cos(x)\"", "@QByteArray")
               % Check("func.max", "7", "double");

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
//         % Check("c", "basic::Color");
//         % Check("c.a", "4", "int");
//         % Check("c.b", "3", "int");
//         % Check("c.g", "2", "int");
//         % Check("c.r", "1", "int");

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
                % CoreProfile()
                % Check("j", "1000", "ns::vl")
                % Check("k", "1000", "ns::verylong")
                % Check("t1", "0", "myType1")
                % Check("t2", "0", "myType2");

    QTest::newRow("Struct")
            << Data(fooData, "Foo f(3);\n"
                    "unused(&f);\n")
               % Check("f", "", "Foo")
               % Check("f.a", "3", "int")
               % Check("f.b", "2", "int");

    QTest::newRow("Union")
            << Data("union U { int a; int b; };", "U u;\n"
                    "unused(&u);\n")
               % Check("u", "", "U")
               % CheckType("u.a", "int")
               % CheckType("u.b", "int");

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
//         % CheckType("s char *");
//         % Check("u "" QString");
//         % CheckType("w wchar_t *");


    QTest::newRow("Pointer")
            << Data(fooData,
                    "Foo *f = new Foo();\n"
                    "unused(&f);\n")
               % Check("f", Pointer(), "Foo")
               % Check("f.a", "0", "int")
               % Check("f.b", "2", "int");

    QTest::newRow("PointerTypedef")
            << Data("typedef void *VoidPtr;\n"
                    "typedef const void *CVoidPtr;\n"
                    "struct A {};\n",
                    "A a;\n"
                    "VoidPtr p = &a;\n"
                    "CVoidPtr cp = &a;\n"
                    "unused(&a, &p, &cp);\n")
               % Check("a", "", "A")
               % Check("cp", Pointer(), "CVoidPtr")
               % Check("p", Pointer(), "VoidPtr");

    QTest::newRow("Reference1")
            << Data("int a = 43;\n"
                    "const int &b = a;\n"
                    "typedef int &Ref;\n"
                    "const int c = 44;\n"
                    "const Ref d = a;\n"
                    "unused(&a, &b, &c, &d);\n")
               % Check("a", "43", "int")
               % Check("b", "43", "int &")
               % Check("c", "44", "int")
               % Check("d", "43", "Ref");

    QTest::newRow("Reference2")
            << Data("#include <QString>\n"
                    "QString fooxx() { return \"bababa\"; }\n",
                    "QString a = \"hello\";\n"
                    "const QString &b = fooxx();\n"
                    "typedef QString &Ref;\n"
                    "const QString c = \"world\";\n"
                    "const Ref d = a;\n"
                    "unused(&a, &b, &c, &d);\n")
               % CoreProfile()
               % Check("a", "\"hello\"", "@QString")
               % Check("b", "\"bababa\"", "@QString &")
               % Check("c", "\"world\"", "@QString")
               % Check("d", "\"hello\"", "Ref");

    QTest::newRow("Reference3")
            << Data("#include <QString>\n",
                    "QString a = QLatin1String(\"hello\");\n"
                    "const QString &b = a;\n"
                    "typedef QString &Ref;\n"
                    "const Ref d = const_cast<Ref>(a);\n"
                    "unused(&a, &b, &d);\n")
               % CoreProfile()
               % Check("a", "\"hello\"", "@QString")
               % Check("b", "\"hello\"", "@QString &")
               % Check("d", "\"hello\"", "Ref");

    QTest::newRow("DynamicReference")
            << Data("struct BaseClass { virtual ~BaseClass() {} };\n"
                    "struct DerivedClass : BaseClass {};\n",
                    "DerivedClass d;\n"
                    "BaseClass *b1 = &d;\n"
                    "BaseClass &b2 = d;\n"
                    "unused(&d, &b1, &b2);\n")
               % CheckType("b1", "DerivedClass") // autoderef
               % CheckType("b2", "DerivedClass &");

    QTest::newRow("LongEvaluation1")
            << Data("#include <QDateTime>",
                    "QDateTime time = QDateTime::currentDateTime();\n"
                    "const int N = 10000;\n"
                    "QDateTime bigv[N];\n"
                    "for (int i = 0; i < 10000; ++i) {\n"
                    "    bigv[i] = time;\n"
                    "    time = time.addDays(1);\n"
                    "}\n"
                    "unused(&bigv[10]);\n")
             % Check("N", "10000", "int")
             % CheckType("bigv", "@QDateTime [10000]")
             % CheckType("bigv.0", "[0]", "@QDateTime")
             % CheckType("bigv.9999", "[9999]", "@QDateTime");

    QTest::newRow("LongEvaluation2")
            << Data("const int N = 10000;\n"
                    "int bigv[N];\n"
                    "for (int i = 0; i < 10000; ++i)\n"
                    "    bigv[i] = i;\n"
                    "unused(&bigv[10]);\n")
             % Check("N", "10000", "int")
             % CheckType("bigv", "int [10000]")
             % Check("bigv.0", "[0]", "0", "int")
             % Check("bigv.9999", "[9999]", "9999", "int");

//    QTest::newRow("Fork")
//                  << Data(
//        "QProcess proc;\n"
//        "proc.start("/bin/ls");\n"
//        "proc.waitForFinished();\n"
//        "QByteArray ba = proc.readAllStandardError();\n"
//        "ba.append('x');\n"
//         % Check("ba "x" QByteArray");
//         % Check("proc  QProcess");


    QTest::newRow("FunctionPointer")
            << Data("int testFunctionPointerHelper(int x) { return x; }\n"
                    "typedef int (*func_t)(int);\n",
                    "func_t f = testFunctionPointerHelper;\n"
                    "int a = f(43);\n"
                    "unused(&a);\n")
             % CheckType("f", "func_t");


    QByteArray dataClass = "struct Class\n"
                           "{\n"
                           "    Class() : a(34) {}\n"
                           "    int testFunctionPointerHelper(int x) { return x; }\n"
                           "    int a;\n"
                           "};\n";

    QTest::newRow("MemberFunctionPointer")
            << Data(dataClass  + "Class x;\n"
                    "typedef int (Class::*func_t)(int);\n",
                    "func_t f = &Class::testFunctionPointerHelper;\n"
                    "int a = (x.*f)(43);\n"
                    "unused(&a);\n")
             % CheckType("f", "func_t");


    QTest::newRow("MemberPointer")
            << Data(dataClass  + "Class x;\n"
                    "typedef int (Class::*member_t);\n",
                    "member_t m = &Class::a;\n"
                    "int a = x.*m;\n"
                    "unused(&a);\n")
             % CheckType("m", "member_t");


  QTest::newRow("PassByReference")
           << Data(fooData +
                   "void testPassByReference(Foo &f) {\n"
                   "   BREAK;\n"
                   "}\n",
                   "Foo f(12);\n"
                   "testPassByReference(f);\n")
               % CheckType("f", "Foo &")
               % Check("f.a", "12", "int");


    QTest::newRow("BigInt")
            << Data("#include <QString>\n"
                    "#include <limits>\n",
                    "qint64 a = Q_INT64_C(0xF020304050607080);\n"
                    "quint64 b = Q_UINT64_C(0xF020304050607080);\n"
                    "quint64 c = std::numeric_limits<quint64>::max() - quint64(1);\n"
                    "qint64 d = c;\n"
                    "QString dummy;\n"
                    "unused(&a, &b, &c, &d, &dummy);\n")
               % Check("a", "-1143861252567568256", "@qint64")
               % Check("b", "17302882821141983360", "@quint64")
               % Check("c", "18446744073709551614", "@quint64")
               % Check("d", "-2", "@qint64");

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
               % CoreProfile()
               % Check("n", "3.5", "double")
               % Check("n@1", "\"2\"", "@QString")
               % Check("n@2", "1", "int");


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
               % Cxx11Profile()
               % Check("y1", "", "X")
               % Check("y2", "", "X")
               % Check("y3", "", "X");

    QTest::newRow("RValueReferenceLldb")
            << Data(rvalueData)
               % LldbEngine
               % Check("x1", "", "X &&")
               % Check("x2", "", "X &&")
               % Check("x3", "", "X &&");

    QTest::newRow("RValueReferenceGdb")
            << Data(rvalueData)
               % GdbEngine
               % GccVersion(0, 40704)
               % Check("x1", "", "X &")
               % Check("x2", "", "X &")
               % Check("x3", "", "X &");

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
               % Profile("QMAKE_CXXFLAGS += -msse2")
               % Check("sseA", "", "__m128")
               % Check("sseB", "", "__m128");


    QTest::newRow("BoostOptional1")
            << Data("#include <boost/optional.hpp>\n",
                    "boost::optional<int> i0, i1;\n"
                    "i1 = 1;\n"
                    "unused(&i0, &i1);\n")
             % BoostProfile()
             % Check("i0", "<uninitialized>", "boost::optional<int>")
             % Check("i1", "1", "boost::optional<int>");

    QTest::newRow("BoostOptional2")
            << Data("#include <QStringList>\n"
                    "#include <boost/optional.hpp>\n",
                    "boost::optional<QStringList> sl0, sl;\n"
                    "sl = (QStringList() << \"xxx\" << \"yyy\");\n"
                    "sl.get().append(\"zzz\");\n"
                    "unused(&sl);\n")
             % BoostProfile()
             % Check("sl", "<3 items>", "boost::optional<@QStringList>");

    QTest::newRow("BoostSharedPtr")
            << Data("#include <QStringList>\n"
                    "#include <boost/shared_ptr.hpp>\n",
                    "boost::shared_ptr<int> s;\n"
                    "boost::shared_ptr<int> i(new int(43));\n"
                    "boost::shared_ptr<int> j = i;\n"
                    "boost::shared_ptr<QStringList> sl(new QStringList(QStringList() << \"HUH!\"));\n"
                    "unused(&s, &i, &j, &sl);\n")
             % BoostProfile()
             % Check("s", "(null)", "boost::shared_ptr<int>")
             % Check("i", "43", "boost::shared_ptr<int>")
             % Check("j", "43", "boost::shared_ptr<int>")
             % Check("sl", "", " boost::shared_ptr<@QStringList>")
             % Check("sl.data", "<1 items>", "@QStringList");

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
             % BoostProfile()
             % Check("d0", "Tue Nov 29 2005", "boost::gregorian::date")
             % Check("d1", "Thu Dec 29 2005", "boost::gregorian::date")
             % Check("d2", "Sun Jan 29 2006", "boost::gregorian::date")
             % Check("d3", "Tue Feb 28 2006", "boost::gregorian::date")
             % Check("d4", "Fri Mar 31 2006", "boost::gregorian::date")
             % Check("d5", "Wed Nov 30 2005", "boost::gregorian::date");

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
             % BoostProfile()
             % Check("d1", "01:00:00", "boost::posix_time::time_duration")
             % Check("d2", "00:01:00", "boost::posix_time::time_duration")
             % Check("d3", "00:00:01", "boost::posix_time::time_duration");

    QTest::newRow("BoostBimap")
            << Data("#include <boost/bimap.hpp>\n",
                    "typedef boost::bimap<int, int> B;\n"
                    "B b;\n"
                    "b.left.insert(B::left_value_type(1, 2));\n"
                    "B::left_const_iterator it = b.left.begin();\n"
                    "int l = it->first;\n"
                    "int r = it->second;\n"
                    "unused(&l, &r);\n")
             % BoostProfile()
             % Check("b", "<1 items>", "B");

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
             % BoostProfile()
             % Check("p1", "Thu Jan 10 01:00:00 2002", "boost::posix_time::ptime")
             % Check("p2", "Thu Jan 10 00:00:00 2002", "boost::posix_time::ptime")
             % Check("p3", "Thu Jan 1 00:00:00 1970", "boost::posix_time::ptime");



//    // This tests qdump__KRBase in share/qtcreator/debugger/qttypes.py which uses
//    // a static typeflag to dispatch to subclasses");

//    QTest::newRow("KR")
//          << Data(
//            "namespace kr {\n"
//\n"
//            "   // FIXME: put in namespace kr, adjust qdump__KRBase in dumpers/qttypes.py\n"
//            "   struct KRBase\n"
//            "   {\n"
//            "       enum Type { TYPE_A, TYPE_B } type;\n"
//            "       KRBase(Type _type) : type(_type) {}\n"
//            "   };\n"
//\n"
//            "   struct KRA : KRBase { int x; int y; KRA():KRBase(TYPE_A), x(1), y(32) {} };\n"
//            "   struct KRB : KRBase { KRB():KRBase(TYPE_B) {} };\n"

//            "/} // namespace kr\n"

//        "KRBase *ptr1 = new KRA;\n"
//        "KRBase *ptr2 = new KRB;\n"
//        "ptr2 = new KRB;\n"
//         % Check("ptr1", "KRBase");
//         % Check("ptr1.type KRBase::TYPE_A (0) KRBase::Type");
//         % Check("ptr2", "KRBase");
//         % Check("ptr2.type KRBase::TYPE_B (1) KRBase::Type");


#if 0
    QTest::newRow("Eigen")
         << Data("#include <Eigen/Core>",
                "using namespace Eigen;\n"
                "Vector3d test = Vector3d::Zero();\n"
                "Matrix3d myMatrix = Matrix3d::Constant(5);\n"
                "MatrixXd myDynamicMatrix(30, 10);\n"

                "myDynamicMatrix(0, 0) = 0;\n"
                "myDynamicMatrix(1, 0) = 1;\n"
                "myDynamicMatrix(2, 0) = 2;\n"

                "Matrix<double, 12, 15, ColMajor> colMajorMatrix;\n"
                "Matrix<double, 12, 15, RowMajor> rowMajorMatrix;\n"

                "int k = 0;\n"
                "for (int i = 0; i != 12; ++i) {\n"
                "    for (int j = 0; j != 15; ++j) {\n"
                "        colMajorMatrix(i, j) = k;\n"
                "        rowMajorMatrix(i, j) = k;\n"
                "        ++k;\n"
                "    }\n"
                "}\n")
            % EigenProfile()
            % CheckType("myMatrix", "Eigen::Matrix3d");
#endif


    // https://bugreports.qt-project.org/browse/QTCREATORBUG-3611
    QTest::newRow("Bug3611")
        << Data("typedef unsigned char byte;\n"
                "byte f = '2';\n"
                "int *x = (int*)&f;\n")
         % Check("f", "'2'", "byte").setForLldbOnly()
         % Check("f", "50 '2'", "byte").setForGdbOnly();


    // https://bugreports.qt-project.org/browse/QTCREATORBUG-4904
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
         % CoreProfile()
         % Check("map", "<2 items>", "@QMap<int, CustomStruct>")
         % CheckType("map.0", "[0]", "@QMapNode<int, CustomStruct>")
         % Check("map.0.key", "-1", "int")
         % CheckType("map.0.value", "CustomStruct")
         % Check("map.0.value.dvalue", FloatValue("3.14"), "double")
         % Check("map.0.value.id", "-1", "int");


#if 0
      // https://bugreports.qt-project.org/browse/QTCREATORBUG-5106
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
            % Check(?)
#endif


    // https://bugreports.qt-project.org/browse/QTCREATORBUG-5184

    // Note: The report there shows type field "QUrl &" instead of QUrl");
    // It's unclear how this can happen. It should never have been like
    // that with a stock 7.2 and any version of Creator");
    QTest::newRow("Bug5184")
        << Data("#include <QUrl>\n"
                "#include <QNetworkRequest>\n"
                "void helper(const QUrl &url)\n"
                "{\n"
                "    QNetworkRequest request(url);\n"
                "    QList<QByteArray> raw = request.rawHeaderList();\n"
                "    BREAK;\n"
                "    unused(&url);\n"
                "}\n",
               " QUrl url(QString(\"http://127.0.0.1/\"));\n"
               " helper(url);\n")
           % NetworkProfile()
           % Check("raw", "<0 items>", "@QList<@QByteArray>")
           % CheckType("request", "@QNetworkRequest")
           % Check("url", "\"http://127.0.0.1/\"", "@QUrl &");


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
            % CheckType("obj", "Circle");


    // https://bugreports.qt-project.org/browse/QTCREATORBUG-5799
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
             % CheckType("a1", "S1 [10]")
             % CheckType("a2", "Array")
             % CheckType("s2", "S2")
             % CheckType("s2.@1", "[S1]", "S1")
             % Check("s2.@1.m1", "5", "int")
             % CheckType("s2.@1.m2", "int")
             % CheckType("s4", "S4")
             % CheckType("s4.@1", "[S3]", "S3")
             % Check("s4.@1.m1", "5", "int")
             % CheckType("s4.@1.m2", "int");


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
         % Check("m", "<2 items>", "map_t")
         % Check("m.0.first", "\"one\"", "std::string")
         % Check("m.0.second", "<3 items>", "std::list<std::string>")
         % Check("m.0.second.0", "[0]", "\"a\"", "std::string")
         % Check("m.0.second.1", "[1]", "\"b\"", "std::string")
         % Check("m.0.second.2", "[2]", "\"c\"", "std::string")
         % Check("m.1.first", "\"two\"", "std::string")
         % Check("m.1.second", "<3 items>", "std::list<std::string>")
         % Check("m.1.second.0", "[0]", "\"1\"", "std::string")
         % Check("m.1.second.1", "[1]", "\"2\"", "std::string")
         % Check("m.1.second.2", "[2]", "\"3\"", "std::string")
         % CheckType("it", "std::map<std::string, std::list<std::string> >::const_iterator")
         % Check("it.first", "\"one\"", "std::string")
         % Check("it.second", "<3 items>", "std::list<std::string>");


    // https://bugreports.qt-project.org/browse/QTCREATORBUG-6465
    QTest::newRow("Bug6465")
        << Data("typedef char Foo[20];\n"
                "Foo foo = \"foo\";\n"
                "char bar[20] = \"baz\";\n")
        % CheckType("bar", "char[20]");


#ifndef Q_OS_WIN
    // https://bugreports.qt-project.org/browse/QTCREATORBUG-6857
    QTest::newRow("Bug6857")
        << Data("#include <QFile>\n"
                "#include <QString>\n"
                "struct MyFile : public QFile {\n"
                "    MyFile(const QString &fileName)\n"
                "        : QFile(fileName) {}\n"
                "};\n",
                "MyFile file(\"/tmp/tt\");\n"
                "file.setObjectName(\"A file\");\n")
         % CoreProfile()
         % QtVersion(50000)
         % Check("file", "\"A file\"", "MyFile")
         % Check("file.@1", "[@QFile]", "\"/tmp/tt\"", "@QFile");
        // FIXME: The classname in the iname is sub-optimal.
         //% Check("file.@1.[QFileDevice]", "[@QFileDevice]", "\"A file\"", "@QFileDevice");
         //% Check("file.@1.@1", "[QFile]", "\"A file\"", "@QObject");
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
         % Check("pfile", "\"Another file\"", "MyFile")
         % Check("pfile.@1", "\"/tmp/tt\"", "@QFile")
         % Check("pfile.@1.@1", "\"Another file\"", "@QObject");
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
//         % Check("obj.[QObject].properties", "<2", "items>");
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
               % Check("b.@1.a", "a", "21", "int")
               % Check("b.b", "b", "42", "int");


    QTest::newRow("valist")
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
               % Check("format", "\"abc\"", "char *")
               % Check("i", "1", "int")
               % Check("f", "2", "double");


    QByteArray inheritanceData =
        "struct Empty {};\n"
        "struct Data { Data() : a(42) {} int a; };\n"
        "struct VEmpty {};\n"
        "struct VData { VData() : v(42) {} int v; };\n"
        "struct S1 : Empty, Data, virtual VEmpty, virtual VData\n"
        "    { S1() : i1(1) {} int i1; };\n"
        "struct S2 : Empty, Data, virtual VEmpty, virtual VData\n"
        "    { S2() : i2(1) {} int i2; };\n"
        "struct Combined : S1, S2 { Combined() : c(1) {} int c; };\n";

    QTest::newRow("inheritance")
            << Data(inheritanceData,
                    "Combined c;\n"
                    "c.S1::a = 42;\n"
                    "c.S2::a = 43;\n"
                    "c.S1::v = 44;\n"
                    "c.S2::v = 45;\n"
                    "unused(&c.S2::v);\n")
                % DebuggerEngine(~LldbEngine)
                % Check("c.c", "1", "int")
                % Check("c.@1.@2.a", "42", "int")
                % Check("c.@1.@4.v", "45", "int")
                % Check("c.@2.@2.a", "43", "int")
                % Check("c.@2.@4.v", "45", "int");

    // FIXME: Virtual inheritance doesn't work with LLDB 300
    QTest::newRow("inheritanceLldb")
            << Data(inheritanceData,
                    "Combined c;\n"
                    "c.S1::a = 42;\n"
                    "c.S2::a = 43;\n"
                    "c.S1::v = 44;\n"
                    "c.S2::v = 45;\n"
                    "unused(&c.S2::v);\n")
                % LldbEngine
                % Check("c.c", "1", "int")
                % Check("c.@1.@1.a", "42", "int")
                //% Check("c.@1.@4.v", "45", "int")
                % Check("c.@2.@1.a", "43", "int");
                //% Check("c.@2.@4.v", "45", "int");


    QTest::newRow("gdb13393")
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
               % GdbEngine
               % GdbVersion(70500)
               % BoostProfile()
               % Check("d", "", "Derived")
               % Check("d.@1", "[Base]", "", "Base")
               % Check("d.b", "2", "int")
               % Check("ptr", "", "Derived")
               % Check("ptr.@1", "[Base]", "", "Base")
               % Check("ptr.@1.a", "1", "int")
               % Check("ptrConst", "", "Derived")
               % Check("ptrConst.@1", "[Base]", "", "Base")
               % Check("ptrConst.b", "2", "int")
               % Check("ptrToPtr", "", "Derived")
               //% CheckType("ptrToPtr.[vptr]", " ")
               % Check("ptrToPtr.@1.a", "1", "int")
               % Check("ref", "", "Derived &")
               //% CheckType("ref.[vptr]", "")
               % Check("ref.@1.a", "1", "int")
               % Check("refConst", "", "Derived &")
               //% CheckType("refConst.[vptr]", "")
               % Check("refConst.@1.a", "1", "int")
               % Check("s", "", "S")
               % Check("s.ptr", "", "Derived")
               % Check("s.ptrConst", "", "Derived")
               % Check("s.ref", "", "Derived &")
               % Check("s.refConst", "", "Derived &")
               % Check("sharedPtr", "1", "int");


    // http://sourceware.org/bugzilla/show_bug.cgi?id=10586. fsf/MI errors out
    // on -var-list-children on an anonymous union. mac/MI was fixed in 2006");
    // The proposed fix has been reported to crash gdb steered from eclipse");
    // http://sourceware.org/ml/gdb-patches/2011-12/msg00420.html
    QTest::newRow("gdb10586miGdb")
            << Data("struct Test {\n"
                    "    struct { int a; float b; };\n"
                    "    struct { int c; float d; };\n"
                    "} v = {{1, 2}, {3, 4}};\n"
                    "unused(&v);\n")
               % GdbEngine
               % Check("v", "", "Test")
               % Check("v.a", "1", "int");

    QTest::newRow("gdb10586miLldb")
            << Data("struct Test {\n"
                    "    struct { int a; float b; };\n"
                    "    struct { int c; float d; };\n"
                    "} v = {{1, 2}, {3, 4}};\n"
                    "unused(&v);\n")
               % LldbEngine
               % Check("v", "", "Test")
               % Check("v.#1.a", "1", "int");

    QTest::newRow("gdb10586eclipseGdb")
            << Data("struct { int x; struct { int a; }; struct { int b; }; } v = {1, {2}, {3}};\n"
                    "struct S { int x, y; } n = {10, 20};\n"
                    "unused(&v, &n);\n")
               % GdbEngine
               % Check("v", "", "{...}")
               % Check("n", "", "S")
               % Check("v.a", "2", "int")
               % Check("v.b", "3", "int")
               % Check("v.x", "1", "int")
               % Check("n.x", "10", "int")
               % Check("n.y", "20", "int");

    QTest::newRow("gdb10586eclipseLldb")
            << Data("struct { int x; struct { int a; }; struct { int b; }; } v = {1, {2}, {3}};\n"
                    "struct S { int x, y; } n = {10, 20};\n"
                    "unused(&v, &n);\n")
               % LldbEngine
               % Check("v", "", Pattern("<anonymous .*>"))
               % Check("n", "", "S")
               % Check("v.#1.a", "2", "int")
               % Check("v.#2.b", "3", "int")
               % Check("v.x", "1", "int")
               % Check("n.x", "10", "int")
               % Check("n.y", "20", "int");

    QTest::newRow("stdint")
            << Data("#include <stdint.h>\n",
                    "uint8_t u8 = 64;\n"
                    "int8_t s8 = 65;\n"
                    "uint16_t u16 = 66;\n"
                    "int16_t s16 = 67;\n"
                    "uint32_t u32 = 68;\n"
                    "int32_t s32 = 69;\n"
                    "unused(&u8, &s8, &u16, &s16, &u32, &s32);\n")
               % Check("u8", "64", "uint8_t")
               % Check("s8", "65", "int8_t")
               % Check("u16", "66", "uint16_t")
               % Check("s16", "67", "int16_t")
               % Check("u32", "68", "uint32_t")
               % Check("s32", "69", "int32_t");

    QTest::newRow("stdlist2")
            << Data("#include <list>\n"
                    "struct Base { virtual ~Base() {} };\n"
                    "template<class T>\n"
                    "struct Derived : public std::list<T>, Base {};\n",
                    "Derived<int> l;\n"
                    "l.push_back(1);\n"
                    "l.push_back(2);\n")
               % Check("l.@1.0", "[0]", "1", "int")
               % Check("l.@1.1", "[1]", "2", "int");

}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    tst_Dumpers test;
    return QTest::qExec(&test, argc, argv);
}

#include "tst_dumpers.moc"
