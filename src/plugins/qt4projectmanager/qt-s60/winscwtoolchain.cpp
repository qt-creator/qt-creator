/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "winscwtoolchain.h"

#include "qt4projectmanager/qt4projectmanagerconstants.h"

#include "ui_winscwtoolchainconfigwidget.h"
#include "winscwparser.h"

#include <projectexplorer/abi.h>
#include <projectexplorer/headerpath.h>
#include <utils/environment.h>
#include <qtsupport/qtversionmanager.h>

#include <QDir>
#include <QFileInfo>
#include <QStringList>

namespace Qt4ProjectManager {
namespace Internal {

static const char winscwCompilerPathKeyC[] = "Qt4ProjectManager.Winscw.CompilerPath";
static const char winscwSystemIncludePathKeyC[] = "Qt4ProjectManager.Winscw.IncludePath";
static const char winscwSystemLibraryPathKeyC[] = "Qt4ProjectManager.Winscw.LibraryPath";

static const char *const WINSCW_DEFAULT_SYSTEM_INCLUDES[] = {
    "/MSL/MSL_C/MSL_Common/Include",
    "/MSL/MSL_C/MSL_Win32/Include",
    "/MSL/MSL_CMSL_X86",
    "/MSL/MSL_C++/MSL_Common/Include",
    "/MSL/MSL_Extras/MSL_Common/Include",
    "/MSL/MSL_Extras/MSL_Win32/Include",
    "/Win32-x86 Support/Headers/Win32 SDK",
    0
};

static const char *const WINSCW_DEFAULT_SYSTEM_LIBRARIES[] = {
    "/Win32-x86 Support/Libraries/Win32 SDK",
    "/Runtime/Runtime_x86/Runtime_Win32/Libs",
    0
};

static Utils::FileName winscwRoot(const Utils::FileName &pathIn)
{
    Utils::FileName path = pathIn;
    if (path.isEmpty())
        return Utils::FileName();

    path = path.parentDir();
    path = path.parentDir();
    path = path.parentDir();
    path.appendPath(QLatin1String("Symbian_Support"));
    return path;
}

static QString toNativePath(const QStringList &list)
{
    return QDir::toNativeSeparators(list.join(QString(QLatin1Char(';'))));
}

static QStringList fromNativePath(const QString &list)
{
    return QDir::fromNativeSeparators(list).split(QLatin1Char(';'));
}

static QStringList detectIncludesFor(const Utils::FileName &path)
{
    Utils::FileName root = winscwRoot(path);
    QStringList result;
    for (int i = 0; WINSCW_DEFAULT_SYSTEM_INCLUDES[i] != 0; ++i) {
        QDir dir(root.toString() + QLatin1String(WINSCW_DEFAULT_SYSTEM_INCLUDES[i]));
        if (dir.exists())
            result.append(dir.absolutePath());
    }
    return result;
}

static QStringList detectLibrariesFor(const Utils::FileName &path)
{
    Utils::FileName root = winscwRoot(path);
    QStringList result;
    for (int i = 0; WINSCW_DEFAULT_SYSTEM_LIBRARIES[i] != 0; ++i) {
        QDir dir(root.toString() + QLatin1String(WINSCW_DEFAULT_SYSTEM_LIBRARIES[i]));
        if (dir.exists())
            result.append(dir.absolutePath());
    }
    return result;
}

// --------------------------------------------------------------------------
// WinscwToolChain
// --------------------------------------------------------------------------

WinscwToolChain::WinscwToolChain(bool autodetected) :
    ProjectExplorer::ToolChain(QLatin1String(Constants::WINSCW_TOOLCHAIN_ID), autodetected)
{ }

WinscwToolChain::WinscwToolChain(const WinscwToolChain &tc) :
    ProjectExplorer::ToolChain(tc),
    m_systemIncludePathes(tc.m_systemIncludePathes),
    m_systemLibraryPathes(tc.m_systemLibraryPathes),
    m_compilerPath(tc.m_compilerPath)
{ }

WinscwToolChain::~WinscwToolChain()
{ }

QString WinscwToolChain::type() const
{
    return QLatin1String("winscw");
}

QString WinscwToolChain::typeDisplayName() const
{
    return WinscwToolChainFactory::tr("WINSCW");
}

ProjectExplorer::Abi WinscwToolChain::targetAbi() const
{
    return ProjectExplorer::Abi(ProjectExplorer::Abi::ArmArchitecture, ProjectExplorer::Abi::SymbianOS,
                                ProjectExplorer::Abi::SymbianEmulatorFlavor,
                                ProjectExplorer::Abi::ElfFormat, 32);
}

QString WinscwToolChain::legacyId() const
{
    return QLatin1String(Constants::WINSCW_TOOLCHAIN_ID) + QLatin1Char(':')  + m_compilerPath.toString();
}

bool WinscwToolChain::isValid() const
{
    if (m_compilerPath.isEmpty())
        return false;

    QFileInfo fi(m_compilerPath.toFileInfo());
    return fi.exists() && fi.isExecutable();
}

QByteArray WinscwToolChain::predefinedMacros(const QStringList &cxxflags) const
{
    Q_UNUSED(cxxflags);
    return QByteArray("#define __SYMBIAN32__\n");
}

ProjectExplorer::ToolChain::CompilerFlags WinscwToolChain::compilerFlags(const QStringList &cxxflags) const
{
    Q_UNUSED(cxxflags);
    return NO_FLAGS;
}

QList<ProjectExplorer::HeaderPath> WinscwToolChain::systemHeaderPaths() const
{
    QList<ProjectExplorer::HeaderPath> result;
    foreach (const QString &value, m_systemIncludePathes)
        result.append(ProjectExplorer::HeaderPath(value, ProjectExplorer::HeaderPath::GlobalHeaderPath));
    return result;
}

void WinscwToolChain::addToEnvironment(Utils::Environment &env) const
{
    if (!isValid())
        return;

    env.set(QLatin1String("MWCSYM2INCLUDES"), toNativePath(m_systemIncludePathes));
    env.set(QLatin1String("MWSYM2LIBRARIES"), toNativePath(m_systemLibraryPathes));
    env.set(QLatin1String("MWSYM2LIBRARYFILES"),
            QLatin1String("MSL_All_MSE_Symbian_D.lib;gdi32.lib;user32.lib;kernel32.lib"));
    env.prependOrSetPath(m_compilerPath.toString());
}

QString WinscwToolChain::makeCommand() const
{
#if defined Q_OS_WIN
    return QLatin1String("make.exe");
#else
    return QLatin1String("make");
#endif
}

Utils::FileName WinscwToolChain::debuggerCommand() const
{
    return Utils::FileName();
}

QString WinscwToolChain::defaultMakeTarget() const
{
    return QLatin1String("winscw");
}

ProjectExplorer::IOutputParser *WinscwToolChain::outputParser() const
{
    return new WinscwParser;
}

bool WinscwToolChain::operator ==(const ProjectExplorer::ToolChain &tc) const
{
    if (!ToolChain::operator ==(tc))
        return false;

    const WinscwToolChain *tcPtr = dynamic_cast<const WinscwToolChain *>(&tc);
    Q_ASSERT(tcPtr);
    return m_compilerPath == tcPtr->m_compilerPath
            && m_systemIncludePathes == tcPtr->m_systemIncludePathes
            && m_systemLibraryPathes == tcPtr->m_systemLibraryPathes;
}

ProjectExplorer::ToolChainConfigWidget *WinscwToolChain::configurationWidget()
{
    return new WinscwToolChainConfigWidget(this);
}

ProjectExplorer::ToolChain *WinscwToolChain::clone() const
{
    return new WinscwToolChain(*this);
}

QVariantMap WinscwToolChain::toMap() const
{
    QVariantMap result = ToolChain::toMap();
    result.insert(QLatin1String(winscwCompilerPathKeyC), m_compilerPath.toString());
    const QString semicolon = QString(QLatin1Char(';'));
    result.insert(QLatin1String(winscwSystemIncludePathKeyC), m_systemIncludePathes.join(semicolon));
    result.insert(QLatin1String(winscwSystemLibraryPathKeyC), m_systemLibraryPathes.join(semicolon));
    return result;
}

bool WinscwToolChain::fromMap(const QVariantMap &data)
{
    if (!ToolChain::fromMap(data))
        return false;
    m_compilerPath = Utils::FileName::fromString(data.value(QLatin1String(winscwCompilerPathKeyC)).toString());
    const QChar semicolon = QLatin1Char(';');
    m_systemIncludePathes = data.value(QLatin1String(winscwSystemIncludePathKeyC)).toString().split(semicolon);
    m_systemLibraryPathes = data.value(QLatin1String(winscwSystemLibraryPathKeyC)).toString().split(semicolon);
    return isValid();
}

void WinscwToolChain::setSystemIncludePathes(const QStringList &pathes)
{
    if (m_systemIncludePathes == pathes)
        return;
    m_systemIncludePathes = pathes;
    toolChainUpdated();
}

QStringList WinscwToolChain::systemIncludePathes() const
{
    return m_systemIncludePathes;
}

void WinscwToolChain::setSystemLibraryPathes(const QStringList &pathes)
{
    if (m_systemLibraryPathes == pathes)
        return;
    m_systemLibraryPathes = pathes;
    toolChainUpdated();
}

QStringList WinscwToolChain::systemLibraryPathes() const
{
    return m_systemLibraryPathes;
}

void WinscwToolChain::setCompilerCommand(const Utils::FileName &path)
{
    if (m_compilerPath == path)
        return;

    m_compilerPath = path;
    toolChainUpdated();
}

Utils::FileName WinscwToolChain::compilerCommand() const
{
    return m_compilerPath;
}

// --------------------------------------------------------------------------
// ToolChainConfigWidget
// --------------------------------------------------------------------------

WinscwToolChainConfigWidget::WinscwToolChainConfigWidget(WinscwToolChain *tc) :
    ProjectExplorer::ToolChainConfigWidget(tc),
    m_ui(new Ui::WinscwToolChainConfigWidget)
{
    m_ui->setupUi(this);

    m_ui->compilerPath->setExpectedKind(Utils::PathChooser::ExistingCommand);
    connect(m_ui->compilerPath, SIGNAL(changed(QString)),
            this, SLOT(handleCompilerPathUpdate()));
    connect(m_ui->includeEdit, SIGNAL(textChanged(QString)), this, SLOT(makeDirty()));
    connect(m_ui->libraryEdit, SIGNAL(textChanged(QString)), this, SLOT(makeDirty()));

    discard();
}

WinscwToolChainConfigWidget::~WinscwToolChainConfigWidget()
{
    delete m_ui;
}

void WinscwToolChainConfigWidget::apply()
{
    WinscwToolChain *tc = static_cast<WinscwToolChain *>(toolChain());
    Q_ASSERT(tc);
    tc->setCompilerCommand(m_ui->compilerPath->fileName());
    tc->setSystemIncludePathes(fromNativePath(m_ui->includeEdit->text()));
    tc->setSystemLibraryPathes(fromNativePath(m_ui->libraryEdit->text()));
}

void WinscwToolChainConfigWidget::discard()
{
    WinscwToolChain *tc = static_cast<WinscwToolChain *>(toolChain());
    Q_ASSERT(tc);
    m_ui->compilerPath->setFileName(tc->compilerCommand());
    m_ui->includeEdit->setText(toNativePath(tc->systemIncludePathes()));
    m_ui->libraryEdit->setText(toNativePath(tc->systemLibraryPathes()));
}

bool WinscwToolChainConfigWidget::isDirty() const
{
    WinscwToolChain *tc = static_cast<WinscwToolChain *>(toolChain());
    Q_ASSERT(tc);
    return tc->compilerCommand() != m_ui->compilerPath->fileName()
            || tc->systemIncludePathes() != fromNativePath(m_ui->includeEdit->text())
            || tc->systemLibraryPathes() != fromNativePath(m_ui->libraryEdit->text());
}

void WinscwToolChainConfigWidget::handleCompilerPathUpdate()
{
    Utils::FileName path = m_ui->compilerPath->fileName();
    if (path.isEmpty())
        return;
    if (!path.toFileInfo().exists())
        return;
    m_ui->includeEdit->setText(toNativePath(detectIncludesFor(path)));
    m_ui->libraryEdit->setText(toNativePath(detectLibrariesFor(path)));
}

void WinscwToolChainConfigWidget::makeDirty()
{
    emit dirty(toolChain());
}

// --------------------------------------------------------------------------
// ToolChainFactory
// --------------------------------------------------------------------------

WinscwToolChainFactory::WinscwToolChainFactory() :
    ProjectExplorer::ToolChainFactory()
{ }

QString WinscwToolChainFactory::displayName() const
{
    return tr("WINSCW");
}

QString WinscwToolChainFactory::id() const
{
    return QLatin1String(Constants::WINSCW_TOOLCHAIN_ID);
}

QList<ProjectExplorer::ToolChain *> WinscwToolChainFactory::autoDetect()
{
    QList<ProjectExplorer::ToolChain *> result;

    // Compatibility to pre-2.2:
    while (true) {
        const QString path = QtSupport::QtVersionManager::instance()->popPendingMwcUpdate();
        if (path.isNull())
            break;

        QFileInfo fi(path + QLatin1String("/x86Build/Symbian_Tools/Command_Line_Tools/mwwinrc.exe"));
        if (fi.exists() && fi.isExecutable()) {
            WinscwToolChain *tc = new WinscwToolChain(false);
            tc->setCompilerCommand(Utils::FileName(fi));
            tc->setDisplayName(tr("WINSCW from Qt version"));
            result.append(tc);
        }
    }

    Utils::FileName cc = Utils::FileName::fromString(Utils::Environment::systemEnvironment().searchInPath(QLatin1String("mwwinrc")));
    if (!cc.isEmpty()) {
        WinscwToolChain *tc = new WinscwToolChain(true);
        tc->setCompilerCommand(cc);
        tc->setSystemIncludePathes(detectIncludesFor(cc));
        tc->setSystemLibraryPathes(detectLibrariesFor(cc));
        result.append(tc);
    }
    return result;
}

bool WinscwToolChainFactory::canCreate()
{
    return true;
}

ProjectExplorer::ToolChain *WinscwToolChainFactory::create()
{
    return new WinscwToolChain(false);
}

bool WinscwToolChainFactory::canRestore(const QVariantMap &data)
{
    return idFromMap(data).startsWith(QLatin1String(Constants::WINSCW_TOOLCHAIN_ID));
}

ProjectExplorer::ToolChain *WinscwToolChainFactory::restore(const QVariantMap &data)
{
    WinscwToolChain *tc = new WinscwToolChain(false);
    if (tc->fromMap(data))
        return tc;

    delete tc;
    return 0;
}

} // namespace Internal
} // namespace Qt4ProjectManager
