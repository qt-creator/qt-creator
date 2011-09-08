/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "gcctoolchain.h"
#include "clangparser.h"
#include "gcctoolchainfactories.h"
#include "gccparser.h"
#include "linuxiccparser.h"
#include "projectexplorerconstants.h"
#include "toolchainmanager.h"

#include <utils/environment.h>
#include <utils/synchronousprocess.h>
#include <utils/qtcassert.h>
#include <utils/pathchooser.h>

#include <QtCore/QBuffer>
#include <QtCore/QCoreApplication>
#include <QtCore/QFileInfo>
#include <QtCore/QProcess>
#include <QtCore/QScopedPointer>

#include <QtGui/QComboBox>
#include <QtGui/QFormLayout>
#include <QtGui/QLabel>

namespace ProjectExplorer {

// --------------------------------------------------------------------------
// Helpers:
// --------------------------------------------------------------------------

static const char compilerPathKeyC[] = "ProjectExplorer.GccToolChain.Path";
static const char targetAbiKeyC[] = "ProjectExplorer.GccToolChain.TargetAbi";
static const char supportedAbisKeyC[] = "ProjectExplorer.GccToolChain.SupportedAbis";
static const char debuggerCommandKeyC[] = "ProjectExplorer.GccToolChain.Debugger";

static QByteArray runGcc(const QString &gcc, const QStringList &arguments, const QStringList &env)
{
    if (gcc.isEmpty() || !QFileInfo(gcc).isExecutable())
        return QByteArray();

    QProcess cpp;
    // Force locale: This function is used only to detect settings inside the tool chain, so this is save.
    QStringList environment(env);
    environment.append(QLatin1String("LC_ALL"));
    environment.append(QLatin1String("C"));

    cpp.setEnvironment(environment);
    cpp.start(gcc, arguments);
    if (!cpp.waitForStarted()) {
        qWarning("%s: Cannot start '%s': %s", Q_FUNC_INFO, qPrintable(gcc),
            qPrintable(cpp.errorString()));
        return QByteArray();
    }
    cpp.closeWriteChannel();
    if (!cpp.waitForFinished()) {
        Utils::SynchronousProcess::stopProcess(cpp);
        qWarning("%s: Timeout running '%s'.", Q_FUNC_INFO, qPrintable(gcc));
        return QByteArray();
    }
    if (cpp.exitStatus() != QProcess::NormalExit) {
        qWarning("%s: '%s' crashed.", Q_FUNC_INFO, qPrintable(gcc));
        return QByteArray();
    }

    return cpp.readAllStandardOutput() + '\n' + cpp.readAllStandardError();
}

static QByteArray gccPredefinedMacros(const QString &gcc, const QStringList &env)
{
    QStringList arguments;
    arguments << QLatin1String("-xc++")
              << QLatin1String("-E")
              << QLatin1String("-dM")
              << QLatin1String("-");

    QByteArray predefinedMacros = runGcc(gcc, arguments, env);
#ifdef Q_OS_MAC
    // Turn off flag indicating Apple's blocks support
    const QByteArray blocksDefine("#define __BLOCKS__ 1");
    const QByteArray blocksUndefine("#undef __BLOCKS__");
    const int idx = predefinedMacros.indexOf(blocksDefine);
    if (idx != -1) {
        predefinedMacros.replace(idx, blocksDefine.length(), blocksUndefine);
    }

    // Define __strong and __weak (used for Apple's GC extension of C) to be empty
    predefinedMacros.append("#define __strong\n");
    predefinedMacros.append("#define __weak\n");
#endif // Q_OS_MAC
    return predefinedMacros;
}

static QList<HeaderPath> gccHeaderPathes(const QString &gcc, const QStringList &env)
{
    QList<HeaderPath> systemHeaderPaths;
    QStringList arguments;
    arguments << QLatin1String("-xc++")
              << QLatin1String("-E")
              << QLatin1String("-v")
              << QLatin1String("-");

    QByteArray line;
    QByteArray data = runGcc(gcc, arguments, env);
    QBuffer cpp(&data);
    cpp.open(QIODevice::ReadOnly);
    while (cpp.canReadLine()) {
        line = cpp.readLine();
        if (line.startsWith("#include"))
            break;
    }

    if (!line.isEmpty() && line.startsWith("#include")) {
        HeaderPath::Kind kind = HeaderPath::UserHeaderPath;
        while (cpp.canReadLine()) {
            line = cpp.readLine();
            if (line.startsWith("#include")) {
                kind = HeaderPath::GlobalHeaderPath;
            } else if (! line.isEmpty() && QChar(line.at(0)).isSpace()) {
                HeaderPath::Kind thisHeaderKind = kind;

                line = line.trimmed();

                const int index = line.indexOf(" (framework directory)");
                if (index != -1) {
                    line.truncate(index);
                    thisHeaderKind = HeaderPath::FrameworkHeaderPath;
                }

                systemHeaderPaths.append(HeaderPath(QFile::decodeName(line), thisHeaderKind));
            } else if (line.startsWith("End of search list.")) {
                break;
            } else {
                qWarning() << "ignore line:" << line;
            }
        }
    }
    return systemHeaderPaths;
}

static QList<ProjectExplorer::Abi> guessGccAbi(const QString &m)
{
    QList<ProjectExplorer::Abi> abiList;

    QString machine = m.toLower();
    if (machine.isEmpty())
        return abiList;

    QStringList parts = machine.split(QRegExp("[ /-]"));

    ProjectExplorer::Abi::Architecture arch = ProjectExplorer::Abi::UnknownArchitecture;
    ProjectExplorer::Abi::OS os = ProjectExplorer::Abi::UnknownOS;
    ProjectExplorer::Abi::OSFlavor flavor = ProjectExplorer::Abi::UnknownFlavor;
    ProjectExplorer::Abi::BinaryFormat format = ProjectExplorer::Abi::UnknownFormat;
    int width = 0;
    int unknownCount = 0;

    foreach (const QString &p, parts) {
        if (p == QLatin1String("unknown") || p == QLatin1String("pc") || p == QLatin1String("none")
            || p == QLatin1String("gnu") || p == QLatin1String("uclibc")
            || p == QLatin1String("86_64") || p == QLatin1String("redhat") || p == QLatin1String("gnueabi")) {
            continue;
        } else if (p == QLatin1String("i386") || p == QLatin1String("i486") || p == QLatin1String("i586")
                   || p == QLatin1String("i686") || p == QLatin1String("x86")) {
            arch = ProjectExplorer::Abi::X86Architecture;
            width = 32;
        } else if (p.startsWith(QLatin1String("arm"))) {
            arch = ProjectExplorer::Abi::ArmArchitecture;
            width = 32;
        } else if (p == QLatin1String("mipsel")) {
            arch = ProjectExplorer::Abi::MipsArchitecture;
            width = 32;
        } else if (p == QLatin1String("x86_64")) {
            arch = ProjectExplorer::Abi::X86Architecture;
            width = 64;
        } else if (p == QLatin1String("powerpc")) {
            arch = ProjectExplorer::Abi::PowerPCArchitecture;
        } else if (p == QLatin1String("w64")) {
            width = 64;
        } else if (p == QLatin1String("linux") || p == QLatin1String("linux6e")) {
            os = ProjectExplorer::Abi::LinuxOS;
            if (flavor == Abi::UnknownFlavor)
                flavor = ProjectExplorer::Abi::GenericLinuxFlavor;
            format = ProjectExplorer::Abi::ElfFormat;
        } else if (p.startsWith("freebsd")) {
            os = ProjectExplorer::Abi::BsdOS;
            if (flavor == Abi::UnknownFlavor)
                flavor = ProjectExplorer::Abi::FreeBsdFlavor;
            format = ProjectExplorer::Abi::ElfFormat;
        } else if (p == QLatin1String("meego")) {
            os = ProjectExplorer::Abi::LinuxOS;
            flavor = ProjectExplorer::Abi::MeegoLinuxFlavor;
            format = ProjectExplorer::Abi::ElfFormat;
        } else if (p == QLatin1String("symbianelf")) {
            os = ProjectExplorer::Abi::SymbianOS;
            flavor = ProjectExplorer::Abi::SymbianDeviceFlavor;
            format = ProjectExplorer::Abi::ElfFormat;
            width = 32;
        } else if (p == QLatin1String("mingw32") || p == QLatin1String("win32")) {
            arch = ProjectExplorer::Abi::X86Architecture;
            os = ProjectExplorer::Abi::WindowsOS;
            flavor = ProjectExplorer::Abi::WindowsMSysFlavor;
            format = ProjectExplorer::Abi::PEFormat;
            if (width == 0)
                width = 32;
        } else if (p == QLatin1String("apple")) {
            os = ProjectExplorer::Abi::MacOS;
            flavor = ProjectExplorer::Abi::GenericMacFlavor;
            format = ProjectExplorer::Abi::MachOFormat;
        } else if (p == QLatin1String("darwin10")) {
            width = 64;
        } else if (p == QLatin1String("darwin9")) {
            width = 32;
        } else if (p == QLatin1String("gnueabi")) {
            format = ProjectExplorer::Abi::ElfFormat;
        } else {
            ++unknownCount;
        }
    }

    if (unknownCount == parts.count())
        return abiList;

    if (os == Abi::MacOS && arch != Abi::ArmArchitecture) {
        // Apple does PPC and x86!
        abiList << ProjectExplorer::Abi(arch, os, flavor, format, width);
        abiList << ProjectExplorer::Abi(arch, os, flavor, format, width == 64 ? 32 : 64);
        abiList << ProjectExplorer::Abi(arch == Abi::X86Architecture ? Abi::PowerPCArchitecture : Abi::X86Architecture, os, flavor, format, width);
        abiList << ProjectExplorer::Abi(arch == Abi::X86Architecture ? Abi::PowerPCArchitecture : Abi::X86Architecture, os, flavor, format, width == 64 ? 32 : 64);
    } else if (width == 64) {
        abiList << ProjectExplorer::Abi(arch, os, flavor, format, width);
        abiList << ProjectExplorer::Abi(arch, os, flavor, format, 32);
    } else {
        abiList << ProjectExplorer::Abi(arch, os, flavor, format, width);
    }
    return abiList;
}

static QList<ProjectExplorer::Abi> guessGccAbi(const QString &path, const QStringList &env)
{
    QStringList arguments(QLatin1String("-dumpmachine"));
    QString machine = QString::fromLocal8Bit(runGcc(path, arguments, env)).trimmed();
    return guessGccAbi(machine);
}

static QString gccVersion(const QString &path, const QStringList &env)
{
    QStringList arguments(QLatin1String("-dumpversion"));
    return QString::fromLocal8Bit(runGcc(path, arguments, env)).trimmed();
}

// --------------------------------------------------------------------------
// GccToolChain
// --------------------------------------------------------------------------

GccToolChain::GccToolChain(bool autodetect) :
    ToolChain(QLatin1String(Constants::GCC_TOOLCHAIN_ID), autodetect)
{ }

GccToolChain::GccToolChain(const QString &id, bool autodetect) :
    ToolChain(id, autodetect)
{ }

GccToolChain::GccToolChain(const GccToolChain &tc) :
    ToolChain(tc),
    m_compilerPath(tc.compilerPath()),
    m_debuggerCommand(tc.debuggerCommand()),
    m_targetAbi(tc.m_targetAbi)
{
    setCompilerPath(tc.m_compilerPath);
}

QString GccToolChain::defaultDisplayName() const
{
    if (!m_targetAbi.isValid())
        return typeName();
    return QString::fromLatin1("%1 (%2 %3)").arg(typeName(),
                                                 ProjectExplorer::Abi::toString(m_targetAbi.architecture()),
                                                 ProjectExplorer::Abi::toString(m_targetAbi.wordWidth()));
}

void GccToolChain::updateId()
{
    QString i = id();
    i = i.left(i.indexOf(QLatin1Char(':')));
    setId(QString::fromLatin1("%1:%2.%3.%4")
          .arg(i).arg(m_compilerPath)
          .arg(m_targetAbi.toString()).arg(m_debuggerCommand));
}

QString GccToolChain::typeName() const
{
    return Internal::GccToolChainFactory::tr("GCC");
}

Abi GccToolChain::targetAbi() const
{
    return m_targetAbi;
}

QString GccToolChain::version() const
{
    if (m_version.isEmpty())
        m_version = detectVersion();
    return m_version;
}

void GccToolChain::setTargetAbi(const Abi &abi)
{
    if (abi == m_targetAbi)
        return;

    updateSupportedAbis();
    m_targetAbi = abi;
    toolChainUpdated();
}

QList<Abi> GccToolChain::supportedAbis() const
{
    updateSupportedAbis();
    return m_supportedAbis;
}

bool GccToolChain::isValid() const
{
    return !m_compilerPath.isNull();
}

QByteArray GccToolChain::predefinedMacros() const
{
    if (m_predefinedMacros.isEmpty()) {
        // Using a clean environment breaks ccache/distcc/etc.
        Utils::Environment env = Utils::Environment::systemEnvironment();
        addToEnvironment(env);
        m_predefinedMacros = gccPredefinedMacros(m_compilerPath, env.toStringList());
    }
    return m_predefinedMacros;
}

QList<HeaderPath> GccToolChain::systemHeaderPaths() const
{
    if (m_headerPathes.isEmpty()) {
        // Using a clean environment breaks ccache/distcc/etc.
        Utils::Environment env = Utils::Environment::systemEnvironment();
        addToEnvironment(env);
        m_headerPathes = gccHeaderPathes(m_compilerPath, env.toStringList());
    }
    return m_headerPathes;
}

void GccToolChain::addToEnvironment(Utils::Environment &env) const
{
    if (!m_compilerPath.isEmpty())
        env.prependOrSetPath(QFileInfo(m_compilerPath).absolutePath());
    env.set(QLatin1String("LANG"), QLatin1String("C"));
}

void GccToolChain::setDebuggerCommand(const QString &d)
{
    if (m_debuggerCommand == d)
        return;
    m_debuggerCommand = d;
    updateId();
    toolChainUpdated();
}

QString GccToolChain::debuggerCommand() const
{
    return m_debuggerCommand;
}

QString GccToolChain::mkspec() const
{
    Abi abi = targetAbi();
    if (abi.os() == Abi::MacOS) {
        QString v = version();
        // prefer versioned g++ on mac. This is required to enable building for older Mac OS versions
        if (v.startsWith(QLatin1String("4.0")))
            return QLatin1String("macx-g++40");
        if (v.startsWith(QLatin1String("4.2")))
            return QLatin1String("macx-g++42");
        return QLatin1String("macx-g++");
    }

    QList<Abi> gccAbiList = Abi::abisOfBinary(m_compilerPath);
    Abi gccAbi;
    if (!gccAbiList.isEmpty())
        gccAbi  = gccAbiList.first();
    if (!gccAbi.isNull()
            && (gccAbi.architecture() != abi.architecture()
                || gccAbi.os() != abi.os()
                || gccAbi.osFlavor() != abi.osFlavor())) {
        // Note: This can fail:-(
        return QString(); // this is a cross-compiler, leave the mkspec alone!
    }
    if (abi.os() == Abi::LinuxOS) {
        if (abi.osFlavor() != Abi::GenericLinuxFlavor)
            return QString(); // most likely not a desktop, so leave the mkspec alone.
        if (abi.wordWidth() == gccAbi.wordWidth())
            return QLatin1String("linux-g++"); // no need to explicitly set the word width
        return QLatin1String("linux-g++-") + QString::number(m_targetAbi.wordWidth());
    }
    if (abi.os() == Abi::BsdOS && abi.osFlavor() == Abi::FreeBsdFlavor)
        return QLatin1String("freebsd-g++");
    return QString();
}

QString GccToolChain::makeCommand() const
{
    return QLatin1String("make");
}

IOutputParser *GccToolChain::outputParser() const
{
    return new GccParser;
}

void GccToolChain::setCompilerPath(const QString &path)
{
    if (path == m_compilerPath)
        return;

    bool resetDisplayName = displayName() == defaultDisplayName();

    m_compilerPath = path;
    m_supportedAbis.clear();

    Abi currentAbi = m_targetAbi;

    m_targetAbi = Abi();
    if (!m_compilerPath.isEmpty()) {
        updateSupportedAbis();
        if (!m_supportedAbis.isEmpty()) {
            if (m_supportedAbis.contains(currentAbi))
                m_targetAbi = currentAbi;
            else
                m_targetAbi = m_supportedAbis.at(0);
        }

        if (resetDisplayName)
            setDisplayName(defaultDisplayName());
    }
    updateId(); // Will trigger toolChainUpdated()!
}

QString GccToolChain::compilerPath() const
{
    return m_compilerPath;
}

ToolChain *GccToolChain::clone() const
{
    return new GccToolChain(*this);
}

QVariantMap GccToolChain::toMap() const
{
    QVariantMap data = ToolChain::toMap();
    data.insert(QLatin1String(compilerPathKeyC), m_compilerPath);
    data.insert(QLatin1String(targetAbiKeyC), m_targetAbi.toString());
    QStringList abiList;
    foreach (const ProjectExplorer::Abi &a, m_supportedAbis)
        abiList.append(a.toString());
    data.insert(QLatin1String(supportedAbisKeyC), abiList);
    data.insert(QLatin1String(debuggerCommandKeyC), m_debuggerCommand);
    return data;
}

bool GccToolChain::fromMap(const QVariantMap &data)
{
    if (!ToolChain::fromMap(data))
        return false;

    m_compilerPath = data.value(QLatin1String(compilerPathKeyC)).toString();
    m_targetAbi = Abi(data.value(QLatin1String(targetAbiKeyC)).toString());
    QStringList abiList = data.value(QLatin1String(supportedAbisKeyC)).toStringList();
    m_supportedAbis.clear();
    foreach (const QString &a, abiList) {
        ProjectExplorer::Abi abi(a);
        if (!abi.isValid())
            continue;
        m_supportedAbis.append(abi);
    }
    m_debuggerCommand = data.value(QLatin1String(debuggerCommandKeyC)).toString();
    updateId();
    return true;
}

bool GccToolChain::operator ==(const ToolChain &other) const
{
    if (!ToolChain::operator ==(other))
        return false;

    const GccToolChain *gccTc = static_cast<const GccToolChain *>(&other);
    return m_compilerPath == gccTc->m_compilerPath && m_targetAbi == gccTc->m_targetAbi
            && m_debuggerCommand == gccTc->m_debuggerCommand;
}

ToolChainConfigWidget *GccToolChain::configurationWidget()
{
    return new Internal::GccToolChainConfigWidget(this);
}

void GccToolChain::updateSupportedAbis() const
{
    if (m_supportedAbis.isEmpty())
        m_supportedAbis = detectSupportedAbis();
}

QList<Abi> GccToolChain::detectSupportedAbis() const
{
    Utils::Environment env = Utils::Environment::systemEnvironment();
    addToEnvironment(env);
    return guessGccAbi(m_compilerPath, env.toStringList());
}

QString GccToolChain::detectVersion() const
{
    Utils::Environment env = Utils::Environment::systemEnvironment();
    addToEnvironment(env);
    return gccVersion(m_compilerPath, env.toStringList());
}

// --------------------------------------------------------------------------
// GccToolChainFactory
// --------------------------------------------------------------------------

QString Internal::GccToolChainFactory::displayName() const
{
    return tr("GCC");
}

QString Internal::GccToolChainFactory::id() const
{
    return QLatin1String(Constants::GCC_TOOLCHAIN_ID);
}

bool Internal::GccToolChainFactory::canCreate()
{
    return true;
}

ToolChain *Internal::GccToolChainFactory::create()
{
    return createToolChain(false);
}

QList<ToolChain *> Internal::GccToolChainFactory::autoDetect()
{
    QStringList debuggers;
#ifdef Q_OS_MAC
    // Fixme Prefer lldb once it is implemented: debuggers.push_back(QLatin1String("lldb"));
#endif
    debuggers.push_back(QLatin1String("gdb"));
    return autoDetectToolchains(QLatin1String("g++"), debuggers, Abi::hostAbi());
}

// Used by the ToolChainManager to restore user-generated tool chains
bool Internal::GccToolChainFactory::canRestore(const QVariantMap &data)
{
    return idFromMap(data).startsWith(QLatin1String(Constants::GCC_TOOLCHAIN_ID) + QLatin1Char(':'));
}

ToolChain *Internal::GccToolChainFactory::restore(const QVariantMap &data)
{
    GccToolChain *tc = new GccToolChain(false);
    if (tc->fromMap(data))
        return tc;

    delete tc;
    return 0;
}

GccToolChain *Internal::GccToolChainFactory::createToolChain(bool autoDetect)
{
    return new GccToolChain(autoDetect);
}

QList<ToolChain *> Internal::GccToolChainFactory::autoDetectToolchains(const QString &compiler,
                                                                       const QStringList &debuggers,
                                                                       const Abi &requiredAbi)
{
    QList<ToolChain *> result;

    const Utils::Environment systemEnvironment = Utils::Environment::systemEnvironment();
    const QString compilerPath = systemEnvironment.searchInPath(compiler);
    if (compilerPath.isEmpty())
        return result;

    QList<Abi> abiList = guessGccAbi(compilerPath, systemEnvironment.toStringList());
    if (!abiList.contains(requiredAbi)) {
        if (requiredAbi.wordWidth() != 64
                || !abiList.contains(Abi(requiredAbi.architecture(), requiredAbi.os(), requiredAbi.osFlavor(),
                                         requiredAbi.binaryFormat(), 32)))
            return result;
    }

    QString debuggerPath = ToolChainManager::instance()->defaultDebugger(requiredAbi); // Find the first debugger
    if (debuggerPath.isEmpty()) {
        foreach (const QString &debugger, debuggers) {
            debuggerPath = systemEnvironment.searchInPath(debugger);
            if (!debuggerPath.isEmpty())
                break;
        }
    }

    foreach (const Abi &abi, abiList) {
        QScopedPointer<GccToolChain> tc(createToolChain(true));
        if (tc.isNull())
            return result;

        tc->setCompilerPath(compilerPath);
        tc->setDebuggerCommand(debuggerPath);
        tc->setTargetAbi(abi);
        tc->setDisplayName(tc->defaultDisplayName()); // reset displayname

        result.append(tc.take());
    }

    return result;
}

// --------------------------------------------------------------------------
// GccToolChainConfigWidget
// --------------------------------------------------------------------------

Internal::GccToolChainConfigWidget::GccToolChainConfigWidget(GccToolChain *tc) :
    ToolChainConfigWidget(tc),
    m_compilerPath(new Utils::PathChooser),
    m_abiWidget(new AbiWidget),
    m_isReadOnly(false)
{
    Q_ASSERT(tc);

    QFormLayout *layout = new QFormLayout(this);

    const QStringList gnuVersionArgs = QStringList(QLatin1String("--version"));
    m_compilerPath->setExpectedKind(Utils::PathChooser::ExistingCommand);
    m_compilerPath->setCommandVersionArguments(gnuVersionArgs);
    layout->addRow(tr("&Compiler path:"), m_compilerPath);
    layout->addRow(tr("&ABI:"), m_abiWidget);
    m_abiWidget->setEnabled(false);

    addDebuggerCommandControls(layout, gnuVersionArgs);
    addErrorLabel(layout);

    setFromToolchain();

    connect(m_compilerPath, SIGNAL(changed(QString)), this, SLOT(handlePathChange()));
    connect(m_abiWidget, SIGNAL(abiChanged()), this, SLOT(handleAbiChange()));
}

void Internal::GccToolChainConfigWidget::apply()
{
    if (toolChain()->isAutoDetected())
        return;

    GccToolChain *tc = static_cast<GccToolChain *>(toolChain());
    Q_ASSERT(tc);
    QString displayName = tc->displayName();
    QString path = m_compilerPath->path();
    if (path.isEmpty())
        path = m_compilerPath->rawPath();
    tc->setCompilerPath(path);
    tc->setTargetAbi(m_abiWidget->currentAbi());
    tc->setDisplayName(displayName); // reset display name
    tc->setDebuggerCommand(debuggerCommand());
    m_autoDebuggerCommand = QLatin1String("<manually set>");
}

void Internal::GccToolChainConfigWidget::setFromToolchain()
{
    // subwidgets are not yet connected!
    bool blocked = blockSignals(true);
    GccToolChain *tc = static_cast<GccToolChain *>(toolChain());
    m_compilerPath->setPath(tc->compilerPath());
    m_abiWidget->setAbis(tc->supportedAbis(), tc->targetAbi());
    if (!m_isReadOnly && !m_compilerPath->path().isEmpty())
        m_abiWidget->setEnabled(true);
    setDebuggerCommand(tc->debuggerCommand());
    blockSignals(blocked);
}

bool Internal::GccToolChainConfigWidget::isDirty() const
{
    GccToolChain *tc = static_cast<GccToolChain *>(toolChain());
    Q_ASSERT(tc);
    return m_compilerPath->path() != tc->compilerPath()
            || m_abiWidget->currentAbi() != tc->targetAbi();
}

void Internal::GccToolChainConfigWidget::makeReadOnly()
{
    m_compilerPath->setEnabled(false);
    m_abiWidget->setEnabled(false);
    m_isReadOnly = true;
    ToolChainConfigWidget::makeReadOnly();
}

void Internal::GccToolChainConfigWidget::handlePathChange()
{
    QString path = m_compilerPath->path();
    QList<Abi> abiList;
    bool haveCompiler = false;
    if (!path.isEmpty()) {
        QFileInfo fi(path);
        haveCompiler = fi.isExecutable() && fi.isFile();
    }
    if (haveCompiler)
        abiList = guessGccAbi(path, Utils::Environment::systemEnvironment().toStringList());
    m_abiWidget->setEnabled(haveCompiler);
    Abi currentAbi = m_abiWidget->currentAbi();
    m_abiWidget->setAbis(abiList, abiList.contains(currentAbi) ? currentAbi : Abi());
    handleAbiChange();
}

void Internal::GccToolChainConfigWidget::handleAbiChange()
{
    if (m_autoDebuggerCommand == debuggerCommand()) {
        ProjectExplorer::Abi abi = m_abiWidget->currentAbi();
        m_autoDebuggerCommand = ToolChainManager::instance()->defaultDebugger(abi);
        setDebuggerCommand(m_autoDebuggerCommand);
    }
    emit dirty(toolChain());
}

// --------------------------------------------------------------------------
// ClangToolChain
// --------------------------------------------------------------------------

ClangToolChain::ClangToolChain(bool autodetect) :
    GccToolChain(QLatin1String(Constants::CLANG_TOOLCHAIN_ID), autodetect)
{ }

QString ClangToolChain::typeName() const
{
    return Internal::ClangToolChainFactory::tr("Clang");
}

QString ClangToolChain::makeCommand() const
{
#if defined(Q_OS_WIN)
    return QLatin1String("mingw32-make.exe");
#else
    return QLatin1String("make");
#endif
}

QString ClangToolChain::mkspec() const
{
    Abi abi = targetAbi();
    if (abi.os() == Abi::MacOS)
        return QLatin1String("unsupported/macx-clang");
    else if (abi.os() == Abi::LinuxOS)
        return QLatin1String("unsupported/linux-clang");
    return QString(); // Note: Not supported by Qt yet, so default to the mkspec the Qt was build with
}

IOutputParser *ClangToolChain::outputParser() const
{
    return new ClangParser;
}

ToolChain *ClangToolChain::clone() const
{
    return new ClangToolChain(*this);
}

// --------------------------------------------------------------------------
// ClangToolChainFactory
// --------------------------------------------------------------------------

QString Internal::ClangToolChainFactory::displayName() const
{
    return tr("Clang");
}

QString Internal::ClangToolChainFactory::id() const
{
    return QLatin1String(Constants::CLANG_TOOLCHAIN_ID);
}

QList<ToolChain *> Internal::ClangToolChainFactory::autoDetect()
{
    Abi ha = Abi::hostAbi();
    return autoDetectToolchains(QLatin1String("clang++"), QStringList(), ha);
}

bool Internal::ClangToolChainFactory::canCreate()
{
    return true;
}

ToolChain *Internal::ClangToolChainFactory::create()
{
    return createToolChain(false);
}

bool Internal::ClangToolChainFactory::canRestore(const QVariantMap &data)
{
    return idFromMap(data).startsWith(QLatin1String(Constants::CLANG_TOOLCHAIN_ID) + QLatin1Char(':'));
}

ToolChain *Internal::ClangToolChainFactory::restore(const QVariantMap &data)
{
    ClangToolChain *tc = new ClangToolChain(false);
    if (tc->fromMap(data))
        return tc;

    delete tc;
    return 0;
}

GccToolChain *Internal::ClangToolChainFactory::createToolChain(bool autoDetect)
{
    return new ClangToolChain(autoDetect);
}

// --------------------------------------------------------------------------
// MingwToolChain
// --------------------------------------------------------------------------

MingwToolChain::MingwToolChain(bool autodetect) :
    GccToolChain(QLatin1String(Constants::MINGW_TOOLCHAIN_ID), autodetect)
{ }

QString MingwToolChain::typeName() const
{
    return Internal::MingwToolChainFactory::tr("MinGW");
}

QString MingwToolChain::mkspec() const
{
    return QLatin1String("win32-g++");
}

QString MingwToolChain::makeCommand() const
{
    return QLatin1String("mingw32-make.exe");
}

ToolChain *MingwToolChain::clone() const
{
    return new MingwToolChain(*this);
}

// --------------------------------------------------------------------------
// MingwToolChainFactory
// --------------------------------------------------------------------------

QString Internal::MingwToolChainFactory::displayName() const
{
    return tr("MinGW");
}

QString Internal::MingwToolChainFactory::id() const
{
    return QLatin1String(Constants::MINGW_TOOLCHAIN_ID);
}

QList<ToolChain *> Internal::MingwToolChainFactory::autoDetect()
{
    // Compatibility to pre-2.2:
    // All Mingw toolchains that exist so far are either installed by the SDK itself (in
    // which case they most likely have debuggers set up) or were created when updating
    // from a previous Qt version. Add debugger in that case.
    foreach (ToolChain *tc, ToolChainManager::instance()->toolChains()) {
        if (tc->debuggerCommand().isEmpty() && tc->id().startsWith(QLatin1String(Constants::MINGW_TOOLCHAIN_ID)))
            static_cast<MingwToolChain *>(tc)
                ->setDebuggerCommand(ToolChainManager::instance()->defaultDebugger(tc->targetAbi()));
    }

    Abi ha = Abi::hostAbi();
    return autoDetectToolchains(QLatin1String("g++"), QStringList(),
                                Abi(ha.architecture(), Abi::WindowsOS, Abi::WindowsMSysFlavor, Abi::PEFormat, ha.wordWidth()));
}

bool Internal::MingwToolChainFactory::canCreate()
{
    return true;
}

ToolChain *Internal::MingwToolChainFactory::create()
{
    return createToolChain(false);
}

bool Internal::MingwToolChainFactory::canRestore(const QVariantMap &data)
{
    return idFromMap(data).startsWith(QLatin1String(Constants::MINGW_TOOLCHAIN_ID) + QLatin1Char(':'));
}

ToolChain *Internal::MingwToolChainFactory::restore(const QVariantMap &data)
{
    MingwToolChain *tc = new MingwToolChain(false);
    if (tc->fromMap(data))
        return tc;

    delete tc;
    return 0;
}

GccToolChain *Internal::MingwToolChainFactory::createToolChain(bool autoDetect)
{
    return new MingwToolChain(autoDetect);
}

// --------------------------------------------------------------------------
// LinuxIccToolChain
// --------------------------------------------------------------------------

LinuxIccToolChain::LinuxIccToolChain(bool autodetect) :
    GccToolChain(QLatin1String(Constants::LINUXICC_TOOLCHAIN_ID), autodetect)
{ }

QString LinuxIccToolChain::typeName() const
{
    return Internal::LinuxIccToolChainFactory::tr("Linux ICC");
}

IOutputParser *LinuxIccToolChain::outputParser() const
{
    return new LinuxIccParser;
}

QString LinuxIccToolChain::mkspec() const
{
    return QLatin1String("linux-icc-") + QString::number(targetAbi().wordWidth());
}

ToolChain *LinuxIccToolChain::clone() const
{
    return new LinuxIccToolChain(*this);
}

// --------------------------------------------------------------------------
// LinuxIccToolChainFactory
// --------------------------------------------------------------------------

QString Internal::LinuxIccToolChainFactory::displayName() const
{
    return tr("Linux ICC");
}

QString Internal::LinuxIccToolChainFactory::id() const
{
    return QLatin1String(Constants::LINUXICC_TOOLCHAIN_ID);
}

QList<ToolChain *> Internal::LinuxIccToolChainFactory::autoDetect()
{
    return autoDetectToolchains(QLatin1String("icpc"),
                                QStringList(QLatin1String("gdb")),
                                Abi::hostAbi());
}

ToolChain *Internal::LinuxIccToolChainFactory::create()
{
    return createToolChain(false);
}

bool Internal::LinuxIccToolChainFactory::canRestore(const QVariantMap &data)
{
    return idFromMap(data).startsWith(QLatin1String(Constants::LINUXICC_TOOLCHAIN_ID) + QLatin1Char(':'));
}

ToolChain *Internal::LinuxIccToolChainFactory::restore(const QVariantMap &data)
{
    LinuxIccToolChain *tc = new LinuxIccToolChain(false);
    if (tc->fromMap(data))
        return tc;

    delete tc;
    return 0;
}

GccToolChain *Internal::LinuxIccToolChainFactory::createToolChain(bool autoDetect)
{
    return new LinuxIccToolChain(autoDetect);
}

} // namespace ProjectExplorer

// Unit tests:

#ifdef WITH_TESTS
#   include "projectexplorer.h"

#   include <QTest>
#   include <QtCore/QUrl>

namespace ProjectExplorer {
void ProjectExplorerPlugin::testGccAbiGuessing_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<QStringList>("abiList");

    QTest::newRow("invalid input")
            << QString::fromLatin1("Some text")
            << (QStringList());
    QTest::newRow("empty input")
            << QString::fromLatin1("")
            << (QStringList());
    QTest::newRow("broken input")
            << QString::fromLatin1("arm-none-foo-gnueabi")
            << (QStringList() << QLatin1String("arm-unknown-unknown-unknown-32bit"));
    QTest::newRow("totally broken input")
            << QString::fromLatin1("foo-bar-foo")
            << (QStringList());

    QTest::newRow("Maemo 1")
            << QString::fromLatin1("arm-none-linux-gnueabi")
            << (QStringList() << QLatin1String("arm-linux-generic-elf-32bit"));
    QTest::newRow("Linux 1")
            << QString::fromLatin1("i686-linux-gnu")
            << (QStringList() << QLatin1String("x86-linux-generic-elf-32bit"));
    QTest::newRow("Linux 2")
            << QString::fromLatin1("i486-linux-gnu")
            << (QStringList() << QLatin1String("x86-linux-generic-elf-32bit"));
    QTest::newRow("Linux 3")
            << QString::fromLatin1("x86_64-linux-gnu")
            << (QStringList() << QLatin1String("x86-linux-generic-elf-64bit")
                              << QLatin1String("x86-linux-generic-elf-32bit"));
    QTest::newRow("Linux 4")
            << QString::fromLatin1("mipsel-linux-uclibc")
            << (QStringList() << QLatin1String("mips-linux-generic-elf-32bit"));
    QTest::newRow("Linux 5") // from QTCREATORBUG-4690
            << QString::fromLatin1("x86_64-redhat-linux6E")
            << (QStringList() << QLatin1String("x86-linux-generic-elf-64bit")
                              << QLatin1String("x86-linux-generic-elf-32bit"));
    QTest::newRow("Linux 6") // from QTCREATORBUG-4690
            << QString::fromLatin1("x86_64-redhat-linux")
            << (QStringList() << QLatin1String("x86-linux-generic-elf-64bit")
                              << QLatin1String("x86-linux-generic-elf-32bit"));
    QTest::newRow("Linux 7") // Meego
                << QString::fromLatin1("armv5tel-meego-linux-gnueabi")
                << (QStringList() << QLatin1String("arm-linux-meego-elf-32bit"));
    QTest::newRow("Linux 8")
                << QString::fromLatin1("armv5tl-montavista-linux-gnueabi")
                << (QStringList() << QLatin1String("arm-linux-generic-elf-32bit"));
    QTest::newRow("Linux 9")
                << QString::fromLatin1("arm-angstrom-linux-gnueabi")
                << (QStringList() << QLatin1String("arm-linux-generic-elf-32bit"));

    QTest::newRow("Mingw 1")
            << QString::fromLatin1("i686-w64-mingw32")
            << (QStringList() << QLatin1String("x86-windows-msys-pe-64bit")
                              << QLatin1String("x86-windows-msys-pe-32bit"));
    QTest::newRow("Mingw 2")
            << QString::fromLatin1("mingw32")
            << (QStringList() << QLatin1String("x86-windows-msys-pe-32bit"));
    QTest::newRow("Clang 1: windows")
            << QString::fromLatin1("x86_64-pc-win32")
            << (QStringList() << QLatin1String("x86-windows-msys-pe-64bit")
                              << QLatin1String("x86-windows-msys-pe-32bit"));
    QTest::newRow("Clang 1: linux")
            << QString::fromLatin1("x86_64-unknown-linux-gnu")
            << (QStringList() << QLatin1String("x86-linux-generic-elf-64bit")
                              << QLatin1String("x86-linux-generic-elf-32bit"));
    QTest::newRow("Mac 1")
            << QString::fromLatin1("i686-apple-darwin10")
            << (QStringList() << QLatin1String("x86-macos-generic-mach_o-64bit")
                              << QLatin1String("x86-macos-generic-mach_o-32bit")
                              << QLatin1String("ppc-macos-generic-mach_o-64bit")
                              << QLatin1String("ppc-macos-generic-mach_o-32bit"));
    QTest::newRow("Mac 2")
            << QString::fromLatin1("powerpc-apple-darwin10")
            << (QStringList() << QLatin1String("ppc-macos-generic-mach_o-64bit")
                              << QLatin1String("ppc-macos-generic-mach_o-32bit")
                              << QLatin1String("x86-macos-generic-mach_o-64bit")
                              << QLatin1String("x86-macos-generic-mach_o-32bit"));
    QTest::newRow("Mac 3")
            << QString::fromLatin1("i686-apple-darwin9")
            << (QStringList() << QLatin1String("x86-macos-generic-mach_o-32bit")
                              << QLatin1String("x86-macos-generic-mach_o-64bit")
                              << QLatin1String("ppc-macos-generic-mach_o-32bit")
                              << QLatin1String("ppc-macos-generic-mach_o-64bit"));
    QTest::newRow("Mac IOS")
            << QString::fromLatin1("arm-apple-darwin9")
            << (QStringList() << QLatin1String("arm-macos-generic-mach_o-32bit"));
    QTest::newRow("Intel 1")
            << QString::fromLatin1("86_64 x86_64 GNU/Linux")
            << (QStringList() << QLatin1String("x86-linux-generic-elf-64bit")
                              << QLatin1String("x86-linux-generic-elf-32bit"));
    QTest::newRow("Symbian 1")
            << QString::fromLatin1("arm-none-symbianelf")
            << (QStringList() << QLatin1String("arm-symbian-device-elf-32bit"));
    QTest::newRow("FreeBSD 1")
            << QString::fromLatin1("i386-portbld-freebsd9.0")
            << (QStringList() << QLatin1String("x86-bsd-freebsd-elf-32bit"));
    QTest::newRow("FreeBSD 2")
            << QString::fromLatin1("i386-undermydesk-freebsd")
            << (QStringList() << QLatin1String("x86-bsd-freebsd-elf-32bit"));
}

void ProjectExplorerPlugin::testGccAbiGuessing()
{
    QFETCH(QString, input);
    QFETCH(QStringList, abiList);

    QList<ProjectExplorer::Abi> al = guessGccAbi(input);
    QCOMPARE(al.count(), abiList.count());
    for (int i = 0; i < al.count(); ++i) {
        QCOMPARE(al.at(i).toString(), abiList.at(i));
    }
}

} // namespace ProjectExplorer

#endif
