/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "msvctoolchain.h"
#include "msvcparser.h"
#include "projectexplorerconstants.h"
#include "headerpath.h"

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorersettings.h>

#include <utils/qtcprocess.h>
#include <utils/qtcassert.h>
#include <utils/synchronousprocess.h>
#ifdef Q_OS_WIN
#    include <utils/winutils.h>
#endif

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QSettings>
#include <QtCore/QUrl>
#include <QtCore/QTemporaryFile>
#include <QtGui/QLabel>
#include <QtGui/QFormLayout>
#include <QtGui/QDesktopServices>

static const char debuggerCommandKeyC[] = "ProjectExplorer.MsvcToolChain.Debugger";

enum { debug = 0 };

namespace ProjectExplorer {
namespace Internal {

// --------------------------------------------------------------------------
// Helpers:
// --------------------------------------------------------------------------

static QString platformName(MsvcToolChain::Platform t)
{
    switch (t) {
    case MsvcToolChain::s32:
        return QLatin1String(" (x86)");
    case MsvcToolChain::s64:
        return QLatin1String(" (x64)");
    case MsvcToolChain::ia64:
        return QLatin1String(" (ia64)");
    case MsvcToolChain::amd64:
        return QLatin1String(" (amd64)");
    }
    return QString();
}

static Abi findAbiOfMsvc(MsvcToolChain::Type type, MsvcToolChain::Platform platform, const QString &version)
{
    Abi::Architecture arch = Abi::X86Architecture;
    Abi::OSFlavor flavor = Abi::UnknownFlavor;
    int wordWidth = 64;

    switch (platform)
    {
    case ProjectExplorer::Internal::MsvcToolChain::s32:
        wordWidth = 32;
        break;
    case ProjectExplorer::Internal::MsvcToolChain::ia64:
        arch = Abi::ItaniumArchitecture;
        break;
    case ProjectExplorer::Internal::MsvcToolChain::s64:
    case ProjectExplorer::Internal::MsvcToolChain::amd64:
        break;
    };

    QString msvcVersionString = version;
    if (type == MsvcToolChain::WindowsSDK) {
        if (version.startsWith(QLatin1String("7.")))
            msvcVersionString = QLatin1String("10.0");
        else if (version.startsWith(QLatin1String("6.1"))
                 || (version.startsWith(QLatin1String("6.0")) && version != QLatin1String("6.0")))
            // The 6.0 SDK is shipping MSVC2005, Starting at 6.0a it is MSVC2008.
            msvcVersionString = QLatin1String("9.0");
        else
            msvcVersionString = QLatin1String("8.0");
    }
    if (msvcVersionString.startsWith(QLatin1String("10.")))
        flavor = Abi::WindowsMsvc2010Flavor;
    else if (msvcVersionString.startsWith(QLatin1String("9.")))
        flavor = Abi::WindowsMsvc2008Flavor;
    else
        flavor = Abi::WindowsMsvc2005Flavor;

    return Abi(arch, Abi::WindowsOS, flavor, Abi::PEFormat, wordWidth);
}

static QString generateDisplayName(const QString &name,
                                   MsvcToolChain::Type t,
                                   MsvcToolChain::Platform p)
{
    if (t == MsvcToolChain::WindowsSDK) {
        QString sdkName = name;
        sdkName += platformName(p);
        return sdkName;
    }
    // Comes as "9.0" from the registry
    QString vcName = QLatin1String("Microsoft Visual C++ Compiler ");
    vcName += name;
    vcName += platformName(p);
    return vcName;
}

static QByteArray msvcCompilationFile()
{
    static const char* macros[] = {"_ATL_VER", "_CHAR_UNSIGNED", "__CLR_VER",
                                   "__cplusplus_cli", "__COUNTER__", "__cplusplus",
                                   "_CPPLIB_VER", "_CPPRTTI", "_CPPUNWIND",
                                   "_DEBUG", "_DLL", "__FUNCDNAME__",
                                   "__FUNCSIG__","__FUNCTION__","_INTEGRAL_MAX_BITS",
                                   "_M_ALPHA","_M_CEE","_M_CEE_PURE",
                                   "_M_CEE_SAFE","_M_IX86","_M_IA64",
                                   "_M_IX86_FP","_M_MPPC","_M_MRX000",
                                   "_M_PPC","_M_X64","_MANAGED",
                                   "_MFC_VER","_MSC_BUILD", /* "_MSC_EXTENSIONS", */
                                   "_MSC_FULL_VER","_MSC_VER","__MSVC_RUNTIME_CHECKS",
                                   "_MT", "_NATIVE_WCHAR_T_DEFINED", "_OPENMP",
                                   "_VC_NODEFAULTLIB", "_WCHAR_T_DEFINED", "_WIN32",
                                   "_WIN32_WCE", "_WIN64", "_Wp64", "__DATE__",
                                   "__DATE__", "__TIME__", "__TIMESTAMP__",
                                   0};
    QByteArray file = "#define __PPOUT__(x) V##x=x\n\n";
    for (int i = 0; macros[i] != 0; ++i) {
        const QByteArray macro(macros[i]);
        file += "#if defined(" + macro + ")\n__PPOUT__("
                + macro + ")\n#endif\n";
    }
    file += "\nvoid main(){}\n\n";
    return file;
}

// Run MSVC 'cl' compiler to obtain #defines.
static QByteArray msvcPredefinedMacros(const Utils::Environment &env)
{
    QByteArray predefinedMacros = "#define __MSVCRT__\n"
                      "#define __w64\n"
                      "#define __int64 long long\n"
                      "#define __int32 long\n"
                      "#define __int16 short\n"
                      "#define __int8 char\n"
                      "#define __ptr32\n"
                      "#define __ptr64\n";

    QString tmpFilePath;
    {
        // QTemporaryFile is buggy and will not unlock the file for cl.exe
        QTemporaryFile tmpFile(QDir::tempPath()+"/envtestXXXXXX.cpp");
        tmpFile.setAutoRemove(false);
        if (!tmpFile.open())
            return predefinedMacros;
        tmpFilePath = QFileInfo(tmpFile).canonicalFilePath();
        tmpFile.write(msvcCompilationFile());
        tmpFile.close();
    }
    QProcess cpp;
    cpp.setEnvironment(env.toStringList());
    cpp.setWorkingDirectory(QDir::tempPath());
    QStringList arguments;
    const QString binary = env.searchInPath(QLatin1String("cl.exe"));
    if (binary.isEmpty()) {
        qWarning("%s: The compiler binary cl.exe could not be found in the path.", Q_FUNC_INFO);
        return predefinedMacros;
    }

    arguments << QLatin1String("/EP") << QDir::toNativeSeparators(tmpFilePath);
    cpp.start(binary, arguments);
    if (!cpp.waitForStarted()) {
        qWarning("%s: Cannot start '%s': %s", Q_FUNC_INFO, qPrintable(binary),
            qPrintable(cpp.errorString()));
        return predefinedMacros;
    }
    cpp.closeWriteChannel();
    if (!cpp.waitForFinished()) {
        Utils::SynchronousProcess::stopProcess(cpp);
        qWarning("%s: Timeout running '%s'.", Q_FUNC_INFO, qPrintable(binary));
        return predefinedMacros;
    }
    if (cpp.exitStatus() != QProcess::NormalExit) {
        qWarning("%s: '%s' crashed.", Q_FUNC_INFO, qPrintable(binary));
        return predefinedMacros;
    }

    const QList<QByteArray> output = cpp.readAllStandardOutput().split('\n');
    foreach (const QByteArray& line, output) {
        if (line.startsWith('V')) {
            QList<QByteArray> split = line.split('=');
            const QByteArray key = split.at(0).mid(1);
            QByteArray value = split.at(1);
            if (!value.isEmpty()) {
                value.chop(1); //remove '\n'
            }
            predefinedMacros += "#define ";
            predefinedMacros += key;
            predefinedMacros += ' ';
            predefinedMacros += value;
            predefinedMacros += '\n';
        }
    }
    QFile::remove(tmpFilePath);
    if (debug)
        qDebug() << "msvcPredefinedMacros" << predefinedMacros;
    return predefinedMacros;
}

// Windows: Expand the delayed evaluation references returned by the
// SDK setup scripts: "PATH=!Path!;foo". Some values might expand
// to empty and should not be added
static QString winExpandDelayedEnvReferences(QString in, const Utils::Environment &env)
{
    const QChar exclamationMark = QLatin1Char('!');
    for (int pos = 0; pos < in.size(); ) {
        // Replace "!REF!" by its value in process environment
        pos = in.indexOf(exclamationMark, pos);
        if (pos == -1)
            break;
        const int nextPos = in.indexOf(exclamationMark, pos + 1);
        if (nextPos == -1)
            break;
        const QString var = in.mid(pos + 1, nextPos - pos - 1);
        const QString replacement = env.value(var.toUpper());
        in.replace(pos, nextPos + 1 - pos, replacement);
        pos += replacement.size();
    }
    return in;
}

static Utils::Environment msvcReadEnvironmentSetting(const QString &varsBat,
                                                     const QString &args,
                                                     const Utils::Environment &env)
{
    // Run the setup script and extract the variables
    Utils::Environment result = env;
    if (!QFileInfo(varsBat).exists())
        return result;

    const QString tempOutputFileName = QDir::tempPath() + QLatin1String("\\qtcreator-msvc-environment.txt");
    QTemporaryFile tf(QDir::tempPath() + "\\XXXXXX.bat");
    tf.setAutoRemove(true);
    if (!tf.open())
        return result;

    const QString filename = tf.fileName();

    QByteArray call = "call ";
    call += Utils::QtcProcess::quoteArg(varsBat).toLocal8Bit();
    if (!args.isEmpty()) {
        call += ' ';
        call += args.toLocal8Bit();
    }
    call += "\r\n";
    tf.write(call);
    const QByteArray redirect = "set > " + Utils::QtcProcess::quoteArg(
                QDir::toNativeSeparators(tempOutputFileName)).toLocal8Bit() + "\r\n";
    tf.write(redirect);
    tf.flush();
    tf.waitForBytesWritten(30000);

    Utils::QtcProcess run;
    run.setEnvironment(env);
    const QString cmdPath = QString::fromLocal8Bit(qgetenv("COMSPEC"));
    // Windows SDK setup scripts require command line switches for environment expansion.
    QString cmdArguments = QLatin1String(" /E:ON /V:ON /c \"");
    cmdArguments += QDir::toNativeSeparators(filename);
    cmdArguments += QLatin1Char('"');
    run.setCommand(cmdPath, cmdArguments);
    if (debug)
        qDebug() << "msvcReadEnvironmentSetting: " << call << cmdPath << cmdArguments
                 << " Env: " << env.size();
    run.start();

    if (!run.waitForStarted()) {
        qWarning("%s: Unable to run '%s': %s", Q_FUNC_INFO, qPrintable(varsBat),
            qPrintable(run.errorString()));
        return result;
    }
    if (!run.waitForFinished()) {
        qWarning("%s: Timeout running '%s'", Q_FUNC_INFO, qPrintable(varsBat));
        Utils::SynchronousProcess::stopProcess(run);
        return result;
    }
    tf.close();

    QFile varsFile(tempOutputFileName);
    if (!varsFile.open(QIODevice::ReadOnly|QIODevice::Text))
        return result;

    QRegExp regexp(QLatin1String("(\\w*)=(.*)"));
    while (!varsFile.atEnd()) {
        const QString line = QString::fromLocal8Bit(varsFile.readLine()).trimmed();
        if (regexp.exactMatch(line)) {
            const QString varName = regexp.cap(1);
            const QString expandedValue = winExpandDelayedEnvReferences(regexp.cap(2), env);
            if (!expandedValue.isEmpty())
                result.set(varName, expandedValue);
        }
    }
    varsFile.close();
    varsFile.remove();
    if (debug) {
        const QStringList newVars = result.toStringList();
        const QStringList oldVars = env.toStringList();
        QDebug nsp = qDebug().nospace();
        foreach (const QString &n, newVars) {
            if (!oldVars.contains(n))
                nsp << n << '\n';
        }
    }
    return result;
}

// --------------------------------------------------------------------------
// MsvcToolChain
// --------------------------------------------------------------------------

MsvcToolChain::MsvcToolChain(const QString &name, const Abi &abi,
                             const QString &varsBat, const QString &varsBatArg, bool autodetect) :
    ToolChain(QLatin1String(Constants::MSVC_TOOLCHAIN_ID), autodetect),
    m_varsBat(varsBat),
    m_varsBatArg(varsBatArg),
    m_lastEnvironment(Utils::Environment::systemEnvironment()),
    m_abi(abi)
{
    Q_ASSERT(!name.isEmpty());
    Q_ASSERT(!m_varsBat.isEmpty());
    Q_ASSERT(QFileInfo(m_varsBat).exists());
    Q_ASSERT(abi.os() == Abi::WindowsOS);
    Q_ASSERT(abi.binaryFormat() == Abi::PEFormat);
    Q_ASSERT(abi.osFlavor() != Abi::WindowsMSysFlavor);

    setId(QString::fromLatin1("%1:%2.%3").arg(Constants::MSVC_TOOLCHAIN_ID).arg(m_varsBat)
            .arg(m_varsBatArg));

    setDisplayName(name);
}

QString MsvcToolChain::typeName() const
{
    return MsvcToolChainFactory::tr("MSVC");
}

Abi MsvcToolChain::targetAbi() const
{
    return m_abi;
}

bool MsvcToolChain::isValid() const
{
    return !m_varsBat.isEmpty();
}

QByteArray MsvcToolChain::predefinedMacros() const
{
    if (m_predefinedMacros.isEmpty()) {
        Utils::Environment env(m_lastEnvironment);
        addToEnvironment(env);
        m_predefinedMacros = msvcPredefinedMacros(env);
    }
    return m_predefinedMacros;
}

QList<HeaderPath> MsvcToolChain::systemHeaderPaths() const
{
    if (m_headerPaths.isEmpty()) {
        Utils::Environment env(m_lastEnvironment);
        addToEnvironment(env);
        foreach (const QString &path, env.value("INCLUDE").split(QLatin1Char(';')))
            m_headerPaths.append(HeaderPath(path, HeaderPath::GlobalHeaderPath));
    }
    return m_headerPaths;
}

void MsvcToolChain::addToEnvironment(Utils::Environment &env) const
{
    // We cache the full environment (incoming + modifications by setup script).
    if (!m_resultEnvironment.size() || env != m_lastEnvironment) {
        if (debug)
            qDebug() << "addToEnvironment: " << displayName();
        m_lastEnvironment = env;
        m_resultEnvironment = msvcReadEnvironmentSetting(m_varsBat, m_varsBatArg, env);
    }
    env = m_resultEnvironment;
}

QString MsvcToolChain::makeCommand() const
{
    if (ProjectExplorerPlugin::instance()->projectExplorerSettings().useJom) {
        // We want jom! Try to find it.
        const QString jom = QLatin1String("jom.exe");
        const QFileInfo installedJom = QFileInfo(QCoreApplication::applicationDirPath()
                                                 + QLatin1Char('/') + jom);
        if (installedJom.isFile() && installedJom.isExecutable()) {
            return installedJom.absoluteFilePath();
        } else {
            return jom;
        }
    }
    return QLatin1String("nmake.exe");
}

void MsvcToolChain::setDebuggerCommand(const QString &d)
{
    m_debuggerCommand = d;
}

QString MsvcToolChain::debuggerCommand() const
{
    return m_debuggerCommand;
}

QVariantMap MsvcToolChain::toMap() const
{
    QVariantMap data = ToolChain::toMap();
    data.insert(QLatin1String(debuggerCommandKeyC), m_debuggerCommand);
    return data;
}

bool MsvcToolChain::fromMap(const QVariantMap &data)
{
    if (!ToolChain::fromMap(data))
        return false;

    m_debuggerCommand= data.value(QLatin1String(debuggerCommandKeyC)).toString();
    return true;
}

IOutputParser *MsvcToolChain::outputParser() const
{
    return new MsvcParser;
}

ToolChainConfigWidget *MsvcToolChain::configurationWidget()
{
    return new MsvcToolChainConfigWidget(this);
}

bool MsvcToolChain::canClone() const
{
    return true;
}

ToolChain *MsvcToolChain::clone() const
{
    return new MsvcToolChain(*this);
}

// --------------------------------------------------------------------------
// MsvcDebuggerConfigLabel
// --------------------------------------------------------------------------

static const char dgbToolsDownloadLink32C[] = "http://www.microsoft.com/whdc/devtools/debugging/installx86.Mspx";
static const char dgbToolsDownloadLink64C[] = "http://www.microsoft.com/whdc/devtools/debugging/install64bit.Mspx";

QString MsvcDebuggerConfigLabel::labelText()
{
#ifdef Q_OS_WIN
    const bool is64bit = Utils::winIs64BitSystem();
#else
    const bool is64bit = false;
#endif
    const QString link = is64bit ? QLatin1String(dgbToolsDownloadLink64C) : QLatin1String(dgbToolsDownloadLink32C);
    //: Label text for path configuration. %2 is "x-bit version".
    return tr(
    "<html><body><p>Specify the path to the "
    "<a href=\"%1\">Windows Console Debugger executable</a>"
    " (%2) here.</p>"
    "</body></html>").arg(link, (is64bit ? tr("64-bit version")
                                         : tr("32-bit version")));
}

MsvcDebuggerConfigLabel::MsvcDebuggerConfigLabel(QWidget *parent) :
        QLabel(labelText(), parent)
{
    connect(this, SIGNAL(linkActivated(QString)), this, SLOT(slotLinkActivated(QString)));
    setTextInteractionFlags(Qt::TextBrowserInteraction);
}

void MsvcDebuggerConfigLabel::slotLinkActivated(const QString &link)
{
    QDesktopServices::openUrl(QUrl(link));
}

// --------------------------------------------------------------------------
// MsvcToolChainConfigWidget
// --------------------------------------------------------------------------

MsvcToolChainConfigWidget::MsvcToolChainConfigWidget(ToolChain *tc) :
    ToolChainConfigWidget(tc)
{
    QFormLayout *formLayout = new QFormLayout(this);
    formLayout->addRow(new QLabel(tc->displayName()));
    formLayout->addRow(new MsvcDebuggerConfigLabel);
    addDebuggerCommandControls(formLayout, QStringList(QLatin1String("-version")));
    addDebuggerAutoDetection(this, SLOT(autoDetectDebugger()));
    addErrorLabel(formLayout);
    setFromToolChain();
}

void MsvcToolChainConfigWidget::apply()
{
    MsvcToolChain *tc = static_cast<MsvcToolChain *>(toolChain());
    QTC_ASSERT(tc, return; );
    tc->setDebuggerCommand(debuggerCommand());
}

void MsvcToolChainConfigWidget::setFromToolChain()
{
    MsvcToolChain *tc = static_cast<MsvcToolChain *>(toolChain());
    QTC_ASSERT(tc, return);
    setDebuggerCommand(tc->debuggerCommand());
}

bool MsvcToolChainConfigWidget::isDirty() const
{
    MsvcToolChain *tc = static_cast<MsvcToolChain *>(toolChain());
    QTC_ASSERT(tc, return false);
    return debuggerCommand() != tc->debuggerCommand();
}

void MsvcToolChainConfigWidget::autoDetectDebugger()
{
    QStringList directories;
    const QString cdbExecutable = MsvcToolChain::autoDetectCdbDebugger(&directories);
    if (cdbExecutable.isEmpty()) {
        const QString msg = tr("The CDB debugger could not be found in %1").arg(directories.join(QLatin1String(", ")));
        setErrorMessage(msg);
    } else {
        clearErrorMessage();
        if (cdbExecutable != debuggerCommand()) {
            setDebuggerCommand(cdbExecutable);
            emitDirty();
        }
    }
}

// --------------------------------------------------------------------------
// MsvcToolChainFactory
// --------------------------------------------------------------------------

QString MsvcToolChainFactory::displayName() const
{
    return tr("MSVC");
}

QString MsvcToolChainFactory::id() const
{
    return QLatin1String(Constants::MSVC_TOOLCHAIN_ID);
}

QList<ToolChain *> MsvcToolChainFactory::autoDetect()
{
    QList<ToolChain *> results;

#ifdef Q_OS_WIN
    // 1) Installed SDKs preferred over standalone Visual studio
    const QSettings sdkRegistry("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Microsoft SDKs\\Windows",
                                QSettings::NativeFormat);
    const QString defaultSdkPath = sdkRegistry.value(QLatin1String("CurrentInstallFolder")).toString();
    if (!defaultSdkPath.isEmpty()) {
        foreach (const QString &sdkKey, sdkRegistry.childGroups()) {
            const QString name = sdkRegistry.value(sdkKey + QLatin1String("/ProductName")).toString();
            const QString version = sdkRegistry.value(sdkKey + QLatin1String("/ProductVersion")).toString();
            const QString folder = sdkRegistry.value(sdkKey + QLatin1String("/InstallationFolder")).toString();
            if (folder.isEmpty())
                continue;

            const QString sdkVcVarsBat = folder + QLatin1String("bin\\SetEnv.cmd");
            if (!QFileInfo(sdkVcVarsBat).exists())
                continue;
            QList<ToolChain *> tmp;

            tmp.append(new MsvcToolChain(generateDisplayName(name, MsvcToolChain::WindowsSDK, MsvcToolChain::s32),
                                         findAbiOfMsvc(MsvcToolChain::WindowsSDK, MsvcToolChain::s32, version),
                                         sdkVcVarsBat, QLatin1String("/x86"), true));
#ifdef Q_OS_WIN64
            // Add all platforms
            tmp.append(new MsvcToolChain(generateDisplayName(name, MsvcToolChain::WindowsSDK, MsvcToolChain::s64),
                                         findAbiOfMsvc(MsvcToolChain::WindowsSDK, MsvcToolChain::s64, version),
                                         sdkVcVarsBat, QLatin1String("/x64"), true));
            tmp.append(new MsvcToolChain(generateDisplayName(name, MsvcToolChain::WindowsSDK, MsvcToolChain::ia64),
                                         findAbiOfMsvc(MsvcToolChain::WindowsSDK, MsvcToolChain::ia64, version),
                                         sdkVcVarsBat, QLatin1String("/ia64"), true));
#endif
            // Make sure the default is front.
            if (folder == defaultSdkPath)
                results = tmp + results;
            else
                results += tmp;
        } // foreach
    }

    // 2) Installed MSVCs
    const QSettings vsRegistry(
#ifdef Q_OS_WIN64
                QLatin1String("HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\Microsoft\\VisualStudio\\SxS\\VC7"),
#else
                QLatin1String("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\VisualStudio\\SxS\\VC7"),
#endif
                QSettings::NativeFormat);
    foreach (const QString &vsName, vsRegistry.allKeys()) {
        // Scan for version major.minor
        const int dotPos = vsName.indexOf(QLatin1Char('.'));
        if (dotPos == -1)
            continue;

        const QString path = vsRegistry.value(vsName).toString();
        const int version = vsName.left(dotPos).toInt();
        // Check existence of various install scripts
        const QString vcvars32bat = path + QLatin1String("bin\\vcvars32.bat");
        if (QFileInfo(vcvars32bat).isFile())
            results.append(new MsvcToolChain(generateDisplayName(vsName, MsvcToolChain::VS, MsvcToolChain::s32),
                                             findAbiOfMsvc(MsvcToolChain::VS, MsvcToolChain::s32, vsName),
                                             vcvars32bat, QString(), true));
        if (version >= 10) {
            // Just one common file
            const QString vcvarsAllbat = path + QLatin1String("vcvarsall.bat");
            if (QFileInfo(vcvarsAllbat).isFile()) {
                results.append(new MsvcToolChain(generateDisplayName(vsName, MsvcToolChain::VS, MsvcToolChain::s32),
                                                 findAbiOfMsvc(MsvcToolChain::VS, MsvcToolChain::s32, vsName),
                                                 vcvarsAllbat, QLatin1String("x86"), true));
                results.append(new MsvcToolChain(generateDisplayName(vsName, MsvcToolChain::VS, MsvcToolChain::amd64),
                                                 findAbiOfMsvc(MsvcToolChain::VS, MsvcToolChain::amd64, vsName),
                                                 vcvarsAllbat, QLatin1String("amd64"), true));
                results.append(new MsvcToolChain(generateDisplayName(vsName, MsvcToolChain::VS, MsvcToolChain::s64),
                                                 findAbiOfMsvc(MsvcToolChain::VS, MsvcToolChain::s64, vsName),
                                                 vcvarsAllbat, QLatin1String("x64"), true));
                results.append(new MsvcToolChain(generateDisplayName(vsName, MsvcToolChain::VS, MsvcToolChain::ia64),
                                                 findAbiOfMsvc(MsvcToolChain::VS, MsvcToolChain::ia64, vsName),
                                                 vcvarsAllbat, QLatin1String("ia64"), true));
            } else {
                qWarning("Unable to find MSVC setup script %s in version %d", qPrintable(vcvarsAllbat), version);
            }
        } else {
            // Amd 64 is the preferred 64bit platform
            const QString vcvarsAmd64bat = path + QLatin1String("bin\\amd64\\vcvarsamd64.bat");
            if (QFileInfo(vcvarsAmd64bat).isFile())
                results.append(new MsvcToolChain(generateDisplayName(vsName, MsvcToolChain::VS, MsvcToolChain::amd64),
                                                 findAbiOfMsvc(MsvcToolChain::VS, MsvcToolChain::amd64, vsName),
                                                 vcvarsAmd64bat, QString(), true));
            const QString vcvarsAmd64bat2 = path + QLatin1String("bin\\vcvarsx86_amd64.bat");
            if (QFileInfo(vcvarsAmd64bat2).isFile())
                results.append(new MsvcToolChain(generateDisplayName(vsName, MsvcToolChain::VS, MsvcToolChain::amd64),
                                                 findAbiOfMsvc(MsvcToolChain::VS, MsvcToolChain::amd64, vsName),
                                                 vcvarsAmd64bat2, QString(), true));
            const QString vcvars64bat = path + QLatin1String("bin\\vcvars64.bat");
            if (QFileInfo(vcvars64bat).isFile())
                results.append(new MsvcToolChain(generateDisplayName(vsName, MsvcToolChain::VS, MsvcToolChain::s64),
                                                 findAbiOfMsvc(MsvcToolChain::VS, MsvcToolChain::s64, vsName),
                                                 vcvars64bat, QString(), true));
            const QString vcvarsIA64bat = path + QLatin1String("bin\\vcvarsx86_ia64.bat");
            if (QFileInfo(vcvarsIA64bat).isFile())
                results.append(new MsvcToolChain(generateDisplayName(vsName, MsvcToolChain::VS, MsvcToolChain::ia64),
                                                 findAbiOfMsvc(MsvcToolChain::VS, MsvcToolChain::ia64, vsName),
                                                 vcvarsIA64bat, QString(), true));
        }
    }
#endif
    if (!results.isEmpty()) { // Detect debugger
        const QString cdbDebugger = MsvcToolChain::autoDetectCdbDebugger();
        if (!cdbDebugger.isEmpty()) {
            foreach (ToolChain *tc, results)
                static_cast<MsvcToolChain *>(tc)->setDebuggerCommand(cdbDebugger);
        }
    }
    return results;
}

// Check the CDB executable and accumulate the list of checked paths
// for reporting.
static QString checkCdbExecutable(const QString &programDir, const QString &postfix,
                                  QStringList *checkedDirectories = 0)
{
    QString executable = programDir;
    executable += QLatin1String("/Debugging Tools For Windows");
    executable += postfix;
    if (checkedDirectories)
        checkedDirectories->push_back(QDir::toNativeSeparators(executable));
    executable += QLatin1String("/cdb.exe");
    const QFileInfo fi(executable);
    return fi.isFile() && fi.isExecutable() ? fi.absoluteFilePath() : QString();
}

QString MsvcToolChain::autoDetectCdbDebugger(QStringList *checkedDirectories /* = 0 */)
{
    // Look for $ProgramFiles/"Debugging Tools For Windows <bit-idy>/cdb.exe" and its
    // " (x86)", " (x64)" variations.
    static const char *postFixes[] = {"", " (x64)", " 64-bit", " (x86)", " (x32)" };

    if (checkedDirectories)
        checkedDirectories->clear();

    const QString programDir = QString::fromLocal8Bit(qgetenv("ProgramFiles"));
    if (programDir.isEmpty())
        return QString();

    // Try the post fixes
    QString outPath;
    for (unsigned i = 0; i < sizeof(postFixes)/sizeof(const char*); i++) {
        outPath = checkCdbExecutable(programDir, QLatin1String(postFixes[i]), checkedDirectories);
        if (!outPath.isEmpty())
            return outPath;
    }
    // A 32bit-compile running on a 64bit system sees the 64 bit installation
    // as "$ProgramFiles (x64)/Debugging Tools..." and (untested), a 64 bit-
    // compile running on a 64bit system sees the 32 bit installation as
    // "$ProgramFiles (x86)/Debugging Tools..." (assuming this works at all)
#ifdef Q_OS_WIN64
    outPath = checkCdbExecutable(programDir + QLatin1String(" (x32)"), QString(), checkedDirectories);
    if (!outPath.isEmpty())
        return QString();
#else
    // A 32bit process on 64 bit sees "ProgramFiles\Debg.. (x64)"
    if (programDir.endsWith(QLatin1String(" (x86)"))) {
        outPath = checkCdbExecutable(programDir.left(programDir.size() - 6),
                                     QLatin1String(" (x64)"), checkedDirectories);

        if (!outPath.isEmpty())
            return QString();
    }
#endif
    return QString();
}

} // namespace Internal
} // namespace ProjectExplorer
