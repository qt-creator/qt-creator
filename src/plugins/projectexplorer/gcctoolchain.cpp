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

#include "gcctoolchain.h"
#include "gcctoolchainfactories.h"
#include "gccparser.h"
#include "linuxiccparser.h"
#include "headerpath.h"
#include "projectexplorerconstants.h"

#include <utils/environment.h>
#include <utils/synchronousprocess.h>
#include <utils/qtcassert.h>
#include <utils/pathchooser.h>

#include <QtCore/QBuffer>
#include <QtCore/QCoreApplication>
#include <QtCore/QFileInfo>
#include <QtCore/QProcess>
#include <QtCore/QScopedPointer>

#include <QtGui/QFormLayout>
#include <QtGui/QLabel>
#include <QtGui/QCheckBox>

namespace ProjectExplorer {

// --------------------------------------------------------------------------
// Helpers:
// --------------------------------------------------------------------------

static const char compilerPathKeyC[] = "ProjectExplorer.GccToolChain.Path";
static const char force32bitKeyC[] = "ProjectExplorer.GccToolChain.Force32Bit";
static const char debuggerCommandKeyC[] = "ProjectExplorer.GccToolChain.Debugger";

static QByteArray runGcc(const QString &gcc, const QStringList &arguments, const QStringList &env)
{
    if (gcc.isEmpty() || !QFileInfo(gcc).isExecutable())
        return QByteArray();

    QProcess cpp;
    cpp.setEnvironment(env);
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
    return cpp.readAllStandardOutput();
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

static ProjectExplorer::Abi guessGccAbi(const QString &m)
{
    QString machine = m.toLower();
    if (machine.isEmpty())
        return ProjectExplorer::Abi();

    QStringList parts = machine.split(QRegExp("[ /-]"));

    ProjectExplorer::Abi::Architecture arch = ProjectExplorer::Abi::UnknownArchitecture;
    ProjectExplorer::Abi::OS os = ProjectExplorer::Abi::UnknownOS;
    ProjectExplorer::Abi::OSFlavor flavor = ProjectExplorer::Abi::UnknownFlavor;
    ProjectExplorer::Abi::BinaryFormat format = ProjectExplorer::Abi::UnknownFormat;
    int width = 0;
    int unknownCount = 0;

    foreach (const QString &p, parts) {
        if (p == QLatin1String("unknown") || p == QLatin1String("pc") || p == QLatin1String("none")
            || p == QLatin1String("gnu") || p == QLatin1String("86_64")) {
            continue;
        } else if (p == QLatin1String("i386") || p == QLatin1String("i486") || p == QLatin1String("i586")
                   || p == QLatin1String("i686") || p == QLatin1String("x86")) {
            arch = ProjectExplorer::Abi::X86Architecture;
            width = 32;
        } else if (p == QLatin1String("arm")) {
            arch = ProjectExplorer::Abi::ArmArchitecture;
            width = 32;
        } else if (p == QLatin1String("x86_64")) {
            arch = ProjectExplorer::Abi::X86Architecture;
            width = 64;
        } else if (p == QLatin1String("w64")) {
            width = 64;
        } else if (p == QLatin1String("linux")) {
            os = ProjectExplorer::Abi::LinuxOS;
            flavor = ProjectExplorer::Abi::GenericLinuxFlavor;
            format = ProjectExplorer::Abi::ElfFormat;
        } else if (p == QLatin1String("symbianelf")) {
            os = ProjectExplorer::Abi::SymbianOS;
            flavor = ProjectExplorer::Abi::SymbianDeviceFlavor;
            format = ProjectExplorer::Abi::ElfFormat;
            width = 32;
        } else if (p == QLatin1String("mingw32")) {
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
        } else if (p == QLatin1String("gnueabi")) {
            format = ProjectExplorer::Abi::ElfFormat;
        } else {
            ++unknownCount;
        }
    }

    if (unknownCount == parts.count())
        return ProjectExplorer::Abi();
    return ProjectExplorer::Abi(arch, os, flavor, format, width);
}

static ProjectExplorer::Abi guessGccAbi(const QString &path, const QStringList &env)
{
    QStringList arguments(QLatin1String("-dumpmachine"));
    QString machine = QString::fromLocal8Bit(runGcc(path, arguments, env)).trimmed();
    return guessGccAbi(machine);
}

// --------------------------------------------------------------------------
// GccToolChain
// --------------------------------------------------------------------------

GccToolChain::GccToolChain(bool autodetect) :
    ToolChain(QLatin1String(Constants::GCC_TOOLCHAIN_ID), autodetect),
    m_forcedTo32Bit(false),
    m_supports64Bit(false)
{ }

GccToolChain::GccToolChain(const QString &id, bool autodetect) :
    ToolChain(id, autodetect),
    m_forcedTo32Bit(false),
    m_supports64Bit(false)
{ }

GccToolChain::GccToolChain(const GccToolChain &tc) :
    ToolChain(tc),
    m_compilerPath(tc.compilerPath()),
    m_debuggerCommand(tc.debuggerCommand()),
    m_forcedTo32Bit(tc.m_forcedTo32Bit),
    m_supports64Bit(tc.m_supports64Bit),
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
    setId(QString::fromLatin1("%1:%2.%3").arg(i).arg(m_compilerPath).arg(m_forcedTo32Bit));
}

QString GccToolChain::typeName() const
{
    return Internal::GccToolChainFactory::tr("GCC");
}

Abi GccToolChain::targetAbi() const
{
    if (!m_targetAbi.isValid()) {
        Utils::Environment env = Utils::Environment::systemEnvironment();
        addToEnvironment(env);
        m_targetAbi = guessGccAbi(m_compilerPath, env.toStringList());
        m_supports64Bit = (m_targetAbi.wordWidth() == 64);
        if (m_targetAbi.wordWidth() == 64 && m_forcedTo32Bit)
            m_targetAbi = Abi(m_targetAbi.architecture(), m_targetAbi.os(), m_targetAbi.osFlavor(),
                              m_targetAbi.binaryFormat(), 32);

        if (displayName() == typeName())
            setDisplayName(defaultDisplayName());
    }
    return m_targetAbi;
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
}

void GccToolChain::setDebuggerCommand(const QString &d)
{
    m_debuggerCommand = d;
}

QString GccToolChain::debuggerCommand() const
{
    return m_debuggerCommand;
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

    if (displayName() == defaultDisplayName())
        setDisplayName(typeName());
    m_compilerPath = path;
    m_targetAbi = Abi();
    updateId();

    if (m_compilerPath.isEmpty())
        return;

    targetAbi(); // update ABI information (and default display name)
}

QString GccToolChain::compilerPath() const
{
    return m_compilerPath;
}

bool GccToolChain::isForcedTo32Bit() const
{
    return m_forcedTo32Bit;
}
void GccToolChain::forceTo32Bit(bool f)
{
    if (f == m_forcedTo32Bit)
        return;

    if (displayName() == defaultDisplayName())
        setDisplayName(typeName());

    m_forcedTo32Bit = f;
    m_targetAbi = Abi(); // Invalidate ABI.
    updateId();

    targetAbi();
}

bool GccToolChain::supports64Bit() const
{
    return m_supports64Bit;
}

ToolChain *GccToolChain::clone() const
{
    return new GccToolChain(*this);
}

QVariantMap GccToolChain::toMap() const
{
    QVariantMap data = ToolChain::toMap();
    data.insert(QLatin1String(compilerPathKeyC), m_compilerPath);
    data.insert(QLatin1String(force32bitKeyC), m_forcedTo32Bit);
    data.insert(QLatin1String(debuggerCommandKeyC), m_debuggerCommand);
    return data;
}

bool GccToolChain::fromMap(const QVariantMap &data)
{
    if (!ToolChain::fromMap(data))
        return false;

    m_compilerPath = data.value(QLatin1String(compilerPathKeyC)).toString();
    m_forcedTo32Bit = data.value(QLatin1String(force32bitKeyC)).toBool();
    m_debuggerCommand = data.value(QLatin1String(debuggerCommandKeyC)).toString();
    updateId();
    return true;
}

bool GccToolChain::operator ==(const ToolChain &other) const
{
    if (!ToolChain::operator ==(other))
        return false;

    const GccToolChain *gccTc = static_cast<const GccToolChain *>(&other);
    return m_compilerPath == gccTc->m_compilerPath && m_forcedTo32Bit == gccTc->m_forcedTo32Bit
            && m_debuggerCommand == gccTc->m_debuggerCommand;
}

ToolChainConfigWidget *GccToolChain::configurationWidget()
{
    return new Internal::GccToolChainConfigWidget(this);
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

// Used by the ToolChainManager to restore user-generated ToolChains
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
    QString debuggerPath; // Find the first debugger
    foreach (const QString &debugger, debuggers) {
        debuggerPath = systemEnvironment.searchInPath(debugger);
        if (!debuggerPath.isEmpty())
            break;
    }

    // Create 64bit
    QScopedPointer<GccToolChain> tc(createToolChain(true));
    if (tc.isNull())
        return result;

    tc->setCompilerPath(compilerPath);
    tc->setDebuggerCommand(debuggerPath);
    const ProjectExplorer::Abi abi = tc->targetAbi();
    if (abi.isValid() && abi == requiredAbi)
        result.append(tc.take());

    if (abi.wordWidth() != 64)
        return result;

    // Create 32bit
    tc.reset(createToolChain(true));
    QTC_ASSERT(!tc.isNull(), return result; ); // worked once, so should work again:-)

    tc->forceTo32Bit(true);
    tc->setCompilerPath(compilerPath);
    tc->setDebuggerCommand(debuggerPath);
    if (tc->targetAbi().isValid())
        result.append(tc.take());

    return result;
}

// --------------------------------------------------------------------------
// GccToolChainConfigWidget
// --------------------------------------------------------------------------

Internal::GccToolChainConfigWidget::GccToolChainConfigWidget(GccToolChain *tc) :
    ToolChainConfigWidget(tc),
    m_compilerPath(new Utils::PathChooser),
    m_force32BitCheckBox(new QCheckBox)
{
    Q_ASSERT(tc);

    const QStringList gnuVersionArgs = QStringList(QLatin1String("--version"));
    m_compilerPath->setExpectedKind(Utils::PathChooser::ExistingCommand);
    m_compilerPath->setCommandVersionArguments(gnuVersionArgs);
    connect(m_compilerPath, SIGNAL(changed(QString)), this, SLOT(handlePathChange()));

    QFormLayout *layout = new QFormLayout(this);
    layout->addRow(tr("&Compiler path:"), m_compilerPath);
    layout->addRow(tr("&Force 32bit compilation:"), m_force32BitCheckBox);

    connect(m_force32BitCheckBox, SIGNAL(toggled(bool)), this, SLOT(handle32BitChange()));

    addDebuggerCommandControls(layout, gnuVersionArgs);
    addErrorLabel(layout);

    setFromToolchain();
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
    tc->forceTo32Bit(m_force32BitCheckBox->isChecked());
    tc->setCompilerPath(path);
    tc->setDisplayName(displayName); // reset display name
    tc->setDebuggerCommand(debuggerCommand());
}

void Internal::GccToolChainConfigWidget::setFromToolchain()
{
    GccToolChain *tc = static_cast<GccToolChain *>(toolChain());
    Q_ASSERT(tc);
    m_compilerPath->setPath(tc->compilerPath());
    m_force32BitCheckBox->setChecked(tc->isForcedTo32Bit());
    m_force32BitCheckBox->setEnabled(tc->supports64Bit());
    setDebuggerCommand(tc->debuggerCommand());
}

bool Internal::GccToolChainConfigWidget::isDirty() const
{
    GccToolChain *tc = static_cast<GccToolChain *>(toolChain());
    Q_ASSERT(tc);
    return m_compilerPath->path() != tc->compilerPath()
            || m_force32BitCheckBox->isChecked() != tc->isForcedTo32Bit();
}

void Internal::GccToolChainConfigWidget::handlePathChange()
{
    QString path = m_compilerPath->path();
    if (!QFileInfo(path).isExecutable()) {
        m_force32BitCheckBox->setEnabled(false);
        m_force32BitCheckBox->setChecked(true);
    } else {
        ProjectExplorer::Abi abi = guessGccAbi(path, Utils::Environment::systemEnvironment().toStringList());
        m_force32BitCheckBox->setEnabled(abi.wordWidth() == 64);
        m_force32BitCheckBox->setChecked(abi.wordWidth() == 32);
    }
    emit dirty(toolChain());
}

void Internal::GccToolChainConfigWidget::handle32BitChange()
{
    emit dirty(toolChain());
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
    return autoDetectToolchains(QLatin1String("gcc"), QStringList(), Abi::hostAbi());
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
    QTest::addColumn<QString>("abi");

    QTest::newRow("invalid input")
            << QString::fromLatin1("Some text")
            << QString::fromLatin1("unknown-unknown-unknown-unknown-unknown");
    QTest::newRow("empty input")
            << QString::fromLatin1("")
            << QString::fromLatin1("unknown-unknown-unknown-unknown-unknown");
    QTest::newRow("broken input")
            << QString::fromLatin1("arm-none-foo-gnueabi")
            << QString::fromLatin1("arm-unknown-unknown-elf-32bit");

    QTest::newRow("Maemo 1")
            << QString::fromLatin1("arm-none-linux-gnueabi")
            << QString::fromLatin1("arm-linux-generic-elf-32bit");
    QTest::newRow("Linux 1")
            << QString::fromLatin1("i686-linux-gnu")
            << QString::fromLatin1("x86-linux-generic-elf-32bit");
    QTest::newRow("Linux 2")
            << QString::fromLatin1("i486-linux-gnu")
            << QString::fromLatin1("x86-linux-generic-elf-32bit");
    QTest::newRow("Linux 3")
            << QString::fromLatin1("x86_64-linux-gnu")
            << QString::fromLatin1("x86-linux-generic-elf-64bit");
    QTest::newRow("Mingw 1")
            << QString::fromLatin1("i686-w64-mingw32")
            << QString::fromLatin1("x86-windows-msys-pe-64bit");
    QTest::newRow("Mingw 2")
            << QString::fromLatin1("mingw32")
            << QString::fromLatin1("x86-windows-msys-pe-32bit");
    QTest::newRow("Mac 1")
            << QString::fromLatin1("i686-apple-darwin10")
            << QString::fromLatin1("x86-macos-generic-mach_o-64bit");
    QTest::newRow("Intel 1")
            << QString::fromLatin1("86_64 x86_64 GNU/Linux")
            << QString::fromLatin1("x86-linux-generic-elf-64bit");
    QTest::newRow("Symbian 1")
            << QString::fromLatin1("arm-none-symbianelf")
            << QString::fromLatin1("arm-symbian-device-elf-32bit");
}

void ProjectExplorerPlugin::testGccAbiGuessing()
{
    QFETCH(QString, input);
    QFETCH(QString, abi);

    ProjectExplorer::Abi a = guessGccAbi(input);
    QCOMPARE(a.toString(), abi);
}

} // namespace ProjectExplorer

#endif
