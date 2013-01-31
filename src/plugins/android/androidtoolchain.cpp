/**************************************************************************
**
** Copyright (c) 2013 BogDan Vatra <bog_dan_ro@yahoo.com>
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

#include "androidtoolchain.h"
#include "androidconstants.h"
#include "androidconfigurations.h"
#include "androidqtversion.h"


#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtversionmanager.h>

#include <projectexplorer/target.h>
#include <projectexplorer/toolchainmanager.h>
#include <projectexplorer/projectexplorer.h>
#include <qtsupport/qtkitinformation.h>

#include <utils/environment.h>
#include <utils/hostosinfo.h>

#include <QDir>
#include <QFormLayout>
#include <QLabel>
#include <QVBoxLayout>

namespace Android {
namespace Internal {

using namespace ProjectExplorer;
using namespace Utils;

static const char ANDROID_QT_VERSION_KEY[] = "Qt4ProjectManager.Android.QtVersion";


AndroidToolChain::AndroidToolChain(bool autodetected) :
    GccToolChain(QLatin1String(Constants::ANDROID_TOOLCHAIN_ID), autodetected),
    m_qtVersionId(-1)
{}

AndroidToolChain::AndroidToolChain(const AndroidToolChain &tc) :
    GccToolChain(tc),
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

void AndroidToolChain::addToEnvironment(Environment &env) const
{

// TODO this vars should be configurable in projects -> build tab
// TODO invalidate all .pro files !!!

    QString ndkHost;
    switch (HostOsInfo::hostOs()) {
    case HostOsInfo::HostOsLinux:
        ndkHost = QLatin1String("linux-x86");
        break;
    case HostOsInfo::HostOsWindows:
        ndkHost = QLatin1String("windows");
        break;
    case HostOsInfo::HostOsMac:
        ndkHost = QLatin1String("darwin-x86");
        break;
    default:
        break;
    }
    env.set(QLatin1String("ANDROID_NDK_HOST"), ndkHost);
    env.set(QLatin1String("ANDROID_NDK_TOOLCHAIN_PREFIX"), AndroidConfigurations::toolchainPrefix(targetAbi().architecture()));
    env.set(QLatin1String("ANDROID_NDK_TOOLS_PREFIX"), AndroidConfigurations::toolsPrefix(targetAbi().architecture()));
    env.set(QLatin1String("ANDROID_NDK_TOOLCHAIN_VERSION"), AndroidConfigurations::instance().config().ndkToolchainVersion);
}

bool AndroidToolChain::operator ==(const ToolChain &tc) const
{
    if (!ToolChain::operator ==(tc))
        return false;

    const AndroidToolChain *tcPtr = static_cast<const AndroidToolChain *>(&tc);
    return m_qtVersionId == tcPtr->m_qtVersionId;
}

ToolChainConfigWidget *AndroidToolChain::configurationWidget()
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

QList<FileName> AndroidToolChain::suggestedMkspecList() const
{
    return QList<FileName>()<< FileName::fromString(QLatin1String("android-g++"));
}

QString AndroidToolChain::makeCommand(const Utils::Environment &env) const
{
    QString make = HostOsInfo::isWindowsHost()
            ? QLatin1String("ma-make.exe") : QLatin1String("make");
    QString tmp = env.searchInPath(make);
    return tmp.isEmpty() ? make : tmp;
}

void AndroidToolChain::setQtVersionId(int id)
{
    if (id < 0) {
        setTargetAbi(Abi());
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
    setDisplayName(AndroidToolChainFactory::tr("Android GCC for %1").arg(version->displayName()));
}

int AndroidToolChain::qtVersionId() const
{
    return m_qtVersionId;
}

QList<Abi> AndroidToolChain::detectSupportedAbis() const
{
    if (m_qtVersionId < 0)
        return QList<Abi>();

    AndroidQtVersion *aqv = dynamic_cast<AndroidQtVersion *>(QtSupport::QtVersionManager::instance()->version(m_qtVersionId));
    if (!aqv)
        return QList<Abi>();

    return aqv->qtAbis();
}

// --------------------------------------------------------------------------
// ToolChainConfigWidget
// --------------------------------------------------------------------------

AndroidToolChainConfigWidget::AndroidToolChainConfigWidget(AndroidToolChain *tc) :
   ToolChainConfigWidget(tc)
{
    QLabel *label = new QLabel(AndroidConfigurations::instance().config().ndkLocation.toUserOutput());
    m_mainLayout->addRow(tr("NDK Root:"), label);
}

// --------------------------------------------------------------------------
// ToolChainFactory
// --------------------------------------------------------------------------

AndroidToolChainFactory::AndroidToolChainFactory() :
    ToolChainFactory()
{ }

QString AndroidToolChainFactory::displayName() const
{
    return tr("Android GCC");
}

QString AndroidToolChainFactory::id() const
{
    return QLatin1String(Constants::ANDROID_TOOLCHAIN_ID);
}

QList<ToolChain *> AndroidToolChainFactory::autoDetect()
{
    QList<ToolChain *> result;

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

ToolChain *AndroidToolChainFactory::restore(const QVariantMap &data)
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
    ToolChainManager *tcm = ToolChainManager::instance();
    QList<ToolChain *> tcList = createToolChainList(changes);
    foreach (ToolChain *tc, tcList)
        tcm->registerToolChain(tc);
}

QList<ToolChain *> AndroidToolChainFactory::createToolChainList(const QList<int> &changes)
{
    ToolChainManager *tcm = ToolChainManager::instance();
    QtSupport::QtVersionManager *vm = QtSupport::QtVersionManager::instance();
    QList<ToolChain *> result;

    foreach (int i, changes) {
        QtSupport::BaseQtVersion *v = vm->version(i);
        QList<ToolChain *> toRemove;
        foreach (ToolChain *tc, tcm->toolChains()) {
            if (tc->id() != QLatin1String(Constants::ANDROID_TOOLCHAIN_ID))
                continue;
            AndroidToolChain *aTc = static_cast<AndroidToolChain *>(tc);
            if (aTc->qtVersionId() == i)
                toRemove.append(aTc);
        }
        foreach (ToolChain *tc, toRemove)
            tcm->deregisterToolChain(tc);

        const AndroidQtVersion * const aqv = dynamic_cast<AndroidQtVersion *>(v);
        if (!aqv || !aqv->isValid())
            continue;

        AndroidToolChain *aTc = new AndroidToolChain(true);
        aTc->setQtVersionId(i);
        aTc->setDisplayName(tr("Android GCC (%1-%2)")
                            .arg(Abi::toString(aTc->targetAbi().architecture()))
                            .arg(AndroidConfigurations::instance().config().ndkToolchainVersion));
        aTc->setCompilerCommand(AndroidConfigurations::instance().gccPath(aTc->targetAbi().architecture()));
        result.append(aTc);
    }
    return result;
}

} // namespace Internal
} // namespace Android
