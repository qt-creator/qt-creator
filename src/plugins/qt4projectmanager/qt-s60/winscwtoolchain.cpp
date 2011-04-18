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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "winscwtoolchain.h"

#include "qt4projectmanager/qt4projectmanagerconstants.h"
#include "qtversionmanager.h"

#include "ui_winscwtoolchainconfigwidget.h"
#include "winscwparser.h"

#include <projectexplorer/abi.h>
#include <projectexplorer/headerpath.h>
#include <utils/environment.h>

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QStringList>

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

static QString winscwRoot(const QString &path)
{
    if (path.isEmpty())
        return QString();

    QDir dir(path);
    dir.cdUp();
    dir.cdUp();
    dir.cdUp();
    dir.cd("Symbian_Support");
    return dir.absolutePath();
}

static QString toNativePath(const QStringList &list)
{
    return QDir::toNativeSeparators(list.join(QString(QLatin1Char(';'))));
}

static QStringList fromNativePath(const QString &list)
{
    QString tmp = QDir::fromNativeSeparators(list);
    return tmp.split(';');
}

static QStringList detectIncludesFor(const QString path)
{
    QString root = winscwRoot(path);
    QStringList result;
    for (int i = 0; WINSCW_DEFAULT_SYSTEM_INCLUDES[i] != 0; ++i) {
        QDir dir(root + QLatin1String(WINSCW_DEFAULT_SYSTEM_INCLUDES[i]));
        if (dir.exists())
            result.append(dir.absolutePath());
    }
    return result;
}

static QStringList detectLibrariesFor(const QString path)
{
    QString root = winscwRoot(path);
    QStringList result;
    for (int i = 0; WINSCW_DEFAULT_SYSTEM_LIBRARIES[i] != 0; ++i) {
        QDir dir(root + QLatin1String(WINSCW_DEFAULT_SYSTEM_LIBRARIES[i]));
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

QString WinscwToolChain::typeName() const
{
    return WinscwToolChainFactory::tr("WINSCW");
}

ProjectExplorer::Abi WinscwToolChain::targetAbi() const
{
    return ProjectExplorer::Abi(ProjectExplorer::Abi::ArmArchitecture, ProjectExplorer::Abi::SymbianOS,
                                ProjectExplorer::Abi::SymbianEmulatorFlavor,
                                ProjectExplorer::Abi::ElfFormat, 32);
}

bool WinscwToolChain::isValid() const
{
    if (m_compilerPath.isEmpty())
        return false;

    QFileInfo fi(m_compilerPath);
    return fi.exists() && fi.isExecutable();
}

QByteArray WinscwToolChain::predefinedMacros() const
{
    return QByteArray("#define __SYMBIAN32__\n");
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
    env.prependOrSetPath(QFileInfo(m_compilerPath).absolutePath());
}

QString WinscwToolChain::makeCommand() const
{
#if defined Q_OS_WIN
    return QLatin1String("make.exe");
#else
    return QLatin1String("make");
#endif
}

QString WinscwToolChain::debuggerCommand() const
{
    return QString();
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
    result.insert(QLatin1String(winscwCompilerPathKeyC), m_compilerPath);
    const QString semicolon = QString(QLatin1Char(';'));
    result.insert(QLatin1String(winscwSystemIncludePathKeyC), m_systemIncludePathes.join(semicolon));
    result.insert(QLatin1String(winscwSystemLibraryPathKeyC), m_systemLibraryPathes.join(semicolon));
    return result;
}

bool WinscwToolChain::fromMap(const QVariantMap &data)
{
    if (!ToolChain::fromMap(data))
        return false;
    m_compilerPath = data.value(QLatin1String(winscwCompilerPathKeyC)).toString();
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

void WinscwToolChain::setCompilerPath(const QString &path)
{
    if (m_compilerPath == path)
        return;

    m_compilerPath = path;
    updateId(); // Will trigger topolChainUpdated()!
}

QString WinscwToolChain::compilerPath() const
{
    return m_compilerPath;
}

void WinscwToolChain::updateId()
{
    setId(QString::fromLatin1("%1:%2").arg(Constants::WINSCW_TOOLCHAIN_ID).arg(m_compilerPath));
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

void WinscwToolChainConfigWidget::apply()
{
    WinscwToolChain *tc = static_cast<WinscwToolChain *>(toolChain());
    Q_ASSERT(tc);
    tc->setCompilerPath(m_ui->compilerPath->path());
    tc->setSystemIncludePathes(fromNativePath(m_ui->includeEdit->text()));
    tc->setSystemLibraryPathes(fromNativePath(m_ui->libraryEdit->text()));
}

void WinscwToolChainConfigWidget::discard()
{
    WinscwToolChain *tc = static_cast<WinscwToolChain *>(toolChain());
    Q_ASSERT(tc);
    m_ui->compilerPath->setPath(tc->compilerPath());
    m_ui->includeEdit->setText(toNativePath(tc->systemIncludePathes()));
    m_ui->libraryEdit->setText(toNativePath(tc->systemLibraryPathes()));
}

bool WinscwToolChainConfigWidget::isDirty() const
{
    WinscwToolChain *tc = static_cast<WinscwToolChain *>(toolChain());
    Q_ASSERT(tc);
    return tc->compilerPath() != m_ui->compilerPath->path()
            || tc->systemIncludePathes() != fromNativePath(m_ui->includeEdit->text())
            || tc->systemLibraryPathes() != fromNativePath(m_ui->libraryEdit->text());
}

void WinscwToolChainConfigWidget::handleCompilerPathUpdate()
{
    QString path = m_ui->compilerPath->path();
    if (path.isEmpty())
        return;
    QFileInfo fi(path);
    if (!fi.exists())
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
        const QString path = QtVersionManager::instance()->popPendingMwcUpdate();
        if (path.isNull())
            break;

        QFileInfo fi(path + QLatin1String("/x86Build/Symbian_Tools/Command_Line_Tools/mwwinrc.exe"));
        if (fi.exists() && fi.isExecutable()) {
            WinscwToolChain *tc = new WinscwToolChain(false);
            tc->setCompilerPath(fi.absoluteFilePath());
            tc->setDisplayName(tr("WINSCW from Qt version"));
            result.append(tc);
        }
    }

    QString cc = Utils::Environment::systemEnvironment().searchInPath(QLatin1String("mwwinrc"));
    if (!cc.isEmpty()) {
        WinscwToolChain *tc = new WinscwToolChain(true);
        tc->setCompilerPath(cc);
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
