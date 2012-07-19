/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 BogDan Vatra <bog_dan_ro@yahoo.com>
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#include "androidtoolchain.h"
#include "androidconstants.h"
#include "androidconfigurations.h"
#include "androidmanager.h"
#include "androidqtversion.h"

#include "qt4projectmanager/qt4projectmanagerconstants.h"

#include <projectexplorer/target.h>
#include <projectexplorer/toolchainmanager.h>
#include <projectexplorer/projectexplorer.h>
#include <qt4projectmanager/qt4project.h>
#include <qtsupport/qtprofileinformation.h>
#include <qtsupport/qtversionmanager.h>

#include <utils/environment.h>

#include <QDir>
#include <QLabel>
#include <QVBoxLayout>

namespace Android {
namespace Internal {

using namespace Qt4ProjectManager;

static const char ANDROID_QT_VERSION_KEY[] = "Qt4ProjectManager.Android.QtVersion";


AndroidToolChain::AndroidToolChain(bool autodetected) :
    ProjectExplorer::GccToolChain(QLatin1String(Constants::ANDROID_TOOLCHAIN_ID), autodetected),
    m_qtVersionId(-1)
{}

AndroidToolChain::AndroidToolChain(const AndroidToolChain &tc) :
    ProjectExplorer::GccToolChain(tc),
    m_qtVersionId(tc.m_qtVersionId)
{ }

AndroidToolChain::~AndroidToolChain()
{ }

QString AndroidToolChain::type() const
{
    return QLatin1String("androidgcc");
}

QString AndroidToolChain::typeDisplayName() const
{
    return AndroidToolChainFactory::tr("Android GCC");
}

bool AndroidToolChain::isValid() const
{
    return GccToolChain::isValid() && m_qtVersionId >= 0 && targetAbi().isValid();
}

void AndroidToolChain::addToEnvironment(Utils::Environment &env) const
{

// TODO this vars should be configurable in projects -> build tab
// TODO invalidate all .pro files !!!

    Qt4Project *qt4pro = qobject_cast<Qt4Project *>(ProjectExplorer::ProjectExplorerPlugin::instance()->currentProject());
    if (!qt4pro || !qt4pro->activeTarget()
            || QtSupport::QtProfileInformation::qtVersion(qt4pro->activeTarget()->profile())->type() != QLatin1String(Constants::ANDROIDQT))
        return;

    QString ndk_host = QLatin1String(
#if defined(Q_OS_LINUX)
        "linux-x86"
#elif defined(Q_OS_WIN)
        "windows"
#elif defined(Q_OS_MAC)
        "darwin-x86"
#endif
    );

    // this env vars are used by qmake mkspecs to generate makefiles (check QTDIR/mkspecs/android-g++/qmake.conf for more info)
    env.set(QLatin1String("ANDROID_NDK_HOST"), ndk_host);
    env.set(QLatin1String("ANDROID_NDK_ROOT"), AndroidConfigurations::instance().config().ndkLocation.toUserOutput());
    env.set(QLatin1String("ANDROID_NDK_TOOLCHAIN_PREFIX"), AndroidConfigurations::toolchainPrefix(targetAbi().architecture()));
    env.set(QLatin1String("ANDROID_NDK_TOOLS_PREFIX"), AndroidConfigurations::toolsPrefix(targetAbi().architecture()));
    env.set(QLatin1String("ANDROID_NDK_TOOLCHAIN_VERSION"), AndroidConfigurations::instance().config().ndkToolchainVersion);
    env.set(QLatin1String("ANDROID_NDK_PLATFORM"),
            AndroidConfigurations::instance().bestMatch(AndroidManager::targetSDK(qt4pro->activeTarget())));
}

bool AndroidToolChain::operator ==(const ProjectExplorer::ToolChain &tc) const
{
    if (!ToolChain::operator ==(tc))
        return false;

    const AndroidToolChain *tcPtr = static_cast<const AndroidToolChain *>(&tc);
    return m_qtVersionId == tcPtr->m_qtVersionId;
}

ProjectExplorer::ToolChainConfigWidget *AndroidToolChain::configurationWidget()
{
    return new AndroidToolChainConfigWidget(this);
}

QVariantMap AndroidToolChain::toMap() const
{
    QVariantMap result = GccToolChain::toMap();
    result.insert(QLatin1String(ANDROID_QT_VERSION_KEY), m_qtVersionId);
    return result;
}

bool AndroidToolChain::fromMap(const QVariantMap &data)
{
    if (!GccToolChain::fromMap(data))
        return false;

    m_qtVersionId = data.value(QLatin1String(ANDROID_QT_VERSION_KEY), -1).toInt();

    return isValid();
}

QList<Utils::FileName> AndroidToolChain::suggestedMkspecList() const
{
    return QList<Utils::FileName>()<< Utils::FileName::fromString(QLatin1String("android-g++"));
}

QString AndroidToolChain::makeCommand() const
{
#if defined(Q_OS_WIN)
    return QLatin1String("ma-make.exe");
#else
    return QLatin1String("make");
#endif
}

void AndroidToolChain::setQtVersionId(int id)
{
    if (id < 0) {
        setTargetAbi(ProjectExplorer::Abi());
        m_qtVersionId = -1;
        toolChainUpdated();
        return;
    }

    QtSupport::BaseQtVersion *version = QtSupport::QtVersionManager::instance()->version(id);
    Q_ASSERT(version);
    m_qtVersionId = id;

    Q_ASSERT(version->qtAbis().count() == 1);
    setTargetAbi(version->qtAbis().at(0));

    toolChainUpdated();
    setDisplayName(AndroidToolChainFactory::tr("Android Gcc for %1").arg(version->displayName()));
}

int AndroidToolChain::qtVersionId() const
{
    return m_qtVersionId;
}

QList<ProjectExplorer::Abi> AndroidToolChain::detectSupportedAbis() const
{
    if (m_qtVersionId < 0)
        return QList<ProjectExplorer::Abi>();

    AndroidQtVersion *aqv = dynamic_cast<AndroidQtVersion *>(QtSupport::QtVersionManager::instance()->version(m_qtVersionId));
    if (!aqv)
        return QList<ProjectExplorer::Abi>();

    return aqv->qtAbis();
}

// --------------------------------------------------------------------------
// ToolChainConfigWidget
// --------------------------------------------------------------------------

AndroidToolChainConfigWidget::AndroidToolChainConfigWidget(AndroidToolChain *tc) :
   ProjectExplorer::ToolChainConfigWidget(tc)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    QLabel *label = new QLabel;
    QtSupport::BaseQtVersion *v = QtSupport::QtVersionManager::instance()->version(tc->qtVersionId());
    Q_ASSERT(v);
    label->setText(tr("NDK Root: %1").arg(AndroidConfigurations::instance().config().ndkLocation.toUserOutput()));
    layout->addWidget(label);
}

void AndroidToolChainConfigWidget::apply()
{
    // nothing to do!
}

void AndroidToolChainConfigWidget::discard()
{
    // nothing to do!
}

bool AndroidToolChainConfigWidget::isDirty() const
{
    return false;
}

// --------------------------------------------------------------------------
// ToolChainFactory
// --------------------------------------------------------------------------

AndroidToolChainFactory::AndroidToolChainFactory() :
    ProjectExplorer::ToolChainFactory()
{ }

QString AndroidToolChainFactory::displayName() const
{
    return tr("Android GCC");
}

QString AndroidToolChainFactory::id() const
{
    return QLatin1String(Constants::ANDROID_TOOLCHAIN_ID);
}

QList<ProjectExplorer::ToolChain *> AndroidToolChainFactory::autoDetect()
{
    QList<ProjectExplorer::ToolChain *> result;

    QtSupport::QtVersionManager *vm = QtSupport::QtVersionManager::instance();
    connect(vm, SIGNAL(qtVersionsChanged(QList<int>,QList<int>,QList<int>)),
            this, SLOT(handleQtVersionChanges(QList<int>,QList<int>,QList<int>)));

    QList<int> versionList;
    foreach (QtSupport::BaseQtVersion *v, vm->versions())
        versionList.append(v->uniqueId());

    return createToolChainList(versionList);
}

bool AndroidToolChainFactory::canRestore(const QVariantMap &data)
{
    return idFromMap(data).startsWith(QLatin1String(Constants::ANDROID_TOOLCHAIN_ID) + QLatin1Char(':'));
}

ProjectExplorer::ToolChain *AndroidToolChainFactory::restore(const QVariantMap &data)
{
    AndroidToolChain *tc = new AndroidToolChain(false);
    if (tc->fromMap(data))
        return tc;

    delete tc;
    return 0;
}

void AndroidToolChainFactory::handleQtVersionChanges(const QList<int> &added, const QList<int> &removed, const QList<int> &changed)
{
    QList<int> changes;
    changes << added << removed << changed;
    ProjectExplorer::ToolChainManager *tcm = ProjectExplorer::ToolChainManager::instance();
    QList<ProjectExplorer::ToolChain *> tcList = createToolChainList(changes);
    foreach (ProjectExplorer::ToolChain *tc, tcList)
        tcm->registerToolChain(tc);
}

QList<ProjectExplorer::ToolChain *> AndroidToolChainFactory::createToolChainList(const QList<int> &changes)
{
    ProjectExplorer::ToolChainManager *tcm = ProjectExplorer::ToolChainManager::instance();
    QtSupport::QtVersionManager *vm = QtSupport::QtVersionManager::instance();
    QList<ProjectExplorer::ToolChain *> result;

    foreach (int i, changes) {
        QtSupport::BaseQtVersion *v = vm->version(i);
        QList<ProjectExplorer::ToolChain *> toRemove;
        foreach (ProjectExplorer::ToolChain *tc, tcm->toolChains()) {
            if (tc->id() != QLatin1String(Constants::ANDROID_TOOLCHAIN_ID))
                continue;
            AndroidToolChain *aTc = static_cast<AndroidToolChain *>(tc);
            if (aTc->qtVersionId() == i)
                toRemove.append(aTc);
        }
        foreach (ProjectExplorer::ToolChain *tc, toRemove)
            tcm->deregisterToolChain(tc);

        const AndroidQtVersion * const aqv = dynamic_cast<AndroidQtVersion *>(v);
        if (!aqv || !aqv->isValid())
            continue;

        AndroidToolChain *aTc = new AndroidToolChain(true);
        aTc->setQtVersionId(i);
        aTc->setDisplayName(tr("Android GCC (%1-%2)")
                            .arg(ProjectExplorer::Abi::toString(aTc->targetAbi().architecture()))
                            .arg(AndroidConfigurations::instance().config().ndkToolchainVersion));
        aTc->setCompilerCommand(AndroidConfigurations::instance().gccPath(aTc->targetAbi().architecture()));
        result.append(aTc);
    }
    return result;
}

} // namespace Internal
} // namespace Android
