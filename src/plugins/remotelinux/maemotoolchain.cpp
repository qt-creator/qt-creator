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

#include "maemotoolchain.h"

#include "maemoglobal.h"
#include "maemoqtversion.h"

#include <projectexplorer/gccparser.h>
#include <projectexplorer/headerpath.h>
#include <projectexplorer/toolchainmanager.h>
#include <qt4projectmanager/qt4projectmanagerconstants.h>
#include <utils/environment.h>
#include <qtsupport/qtversionmanager.h>

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtGui/QLabel>
#include <QtGui/QVBoxLayout>

using namespace Qt4ProjectManager;

namespace RemoteLinux {
namespace Internal {

static const char *const MAEMO_QT_VERSION_KEY = "Qt4ProjectManager.Maemo.QtVersion";

// --------------------------------------------------------------------------
// MaemoToolChain
// --------------------------------------------------------------------------

MaemoToolChain::MaemoToolChain(bool autodetected) :
    ProjectExplorer::GccToolChain(QLatin1String(Constants::MAEMO_TOOLCHAIN_ID), autodetected),
    m_qtVersionId(-1)
{
    updateId();
}

MaemoToolChain::MaemoToolChain(const MaemoToolChain &tc) :
    ProjectExplorer::GccToolChain(tc),
    m_qtVersionId(tc.m_qtVersionId)
{ }

MaemoToolChain::~MaemoToolChain()
{ }

QString MaemoToolChain::typeName() const
{
    return MaemoToolChainFactory::tr("Maemo GCC");
}

ProjectExplorer::Abi MaemoToolChain::targetAbi() const
{
    return m_targetAbi;
}

QString MaemoToolChain::mkspec() const
{
    return QString(); // always use default
}

bool MaemoToolChain::isValid() const
{
    return GccToolChain::isValid() && m_qtVersionId >= 0 && m_targetAbi.isValid();
}

bool MaemoToolChain::canClone() const
{
    return false;
}

void MaemoToolChain::addToEnvironment(Utils::Environment &env) const
{
    const QString manglePathsKey = QLatin1String("GCCWRAPPER_PATHMANGLE");
    if (!env.hasKey(manglePathsKey)) {
        const QStringList pathsToMangle = QStringList() << QLatin1String("/lib")
            << QLatin1String("/opt") << QLatin1String("/usr");
        env.set(manglePathsKey, QString());
        foreach (const QString &path, pathsToMangle)
            env.appendOrSet(manglePathsKey, path, QLatin1String(":"));
    }
}

bool MaemoToolChain::operator ==(const ProjectExplorer::ToolChain &tc) const
{
    if (!ToolChain::operator ==(tc))
        return false;

    const MaemoToolChain *tcPtr = static_cast<const MaemoToolChain *>(&tc);
    return m_qtVersionId == tcPtr->m_qtVersionId;
}

ProjectExplorer::ToolChainConfigWidget *MaemoToolChain::configurationWidget()
{
    return new MaemoToolChainConfigWidget(this);
}

QVariantMap MaemoToolChain::toMap() const
{
    QVariantMap result = GccToolChain::toMap();
    result.insert(QLatin1String(MAEMO_QT_VERSION_KEY), m_qtVersionId);
    return result;
}

bool MaemoToolChain::fromMap(const QVariantMap &data)
{
    if (!GccToolChain::fromMap(data))
        return false;

    m_qtVersionId = data.value(QLatin1String(MAEMO_QT_VERSION_KEY), -1).toInt();

    return isValid();
}

void MaemoToolChain::setQtVersionId(int id)
{
    if (id < 0) {
        m_targetAbi = ProjectExplorer::Abi();
        m_qtVersionId = -1;
        updateId(); // Will trigger toolChainUpdated()!
        return;
    }

    MaemoQtVersion *version = dynamic_cast<MaemoQtVersion *>(QtSupport::QtVersionManager::instance()->version(id));
    Q_ASSERT(version);
    if (!version->isValid())
        return;
    Q_ASSERT(version->qtAbis().count() == 1);

    m_qtVersionId = id;
    m_targetAbi = version->qtAbis().at(0);

    updateId(); // Will trigger toolChainUpdated()!
    setDisplayName(MaemoToolChainFactory::tr("Maemo GCC for %1").arg(version->displayName()));
}

int MaemoToolChain::qtVersionId() const
{
    return m_qtVersionId;
}

void MaemoToolChain::updateId()
{
    setId(QString::fromLatin1("%1:%2.%3").arg(Constants::MAEMO_TOOLCHAIN_ID)
          .arg(m_qtVersionId).arg(debuggerCommand()));
}

// --------------------------------------------------------------------------
// MaemoToolChainConfigWidget
// --------------------------------------------------------------------------

MaemoToolChainConfigWidget::MaemoToolChainConfigWidget(MaemoToolChain *tc) :
    ProjectExplorer::ToolChainConfigWidget(tc)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    QLabel *label = new QLabel;
    QtSupport::BaseQtVersion *v = QtSupport::QtVersionManager::instance()->version(tc->qtVersionId());
    Q_ASSERT(v);
    label->setText(tr("<html><head/><body><table>"
                      "<tr><td>Path to MADDE:</td><td>%1</td></tr>"
                      "<tr><td>Path to MADDE target:</td><td>%2</td></tr>"
                      "<tr><td>Debugger:</td/><td>%3</td></tr></body></html>")
                   .arg(QDir::toNativeSeparators(MaemoGlobal::maddeRoot(v->qmakeCommand())),
                        QDir::toNativeSeparators(MaemoGlobal::targetRoot(v->qmakeCommand())),
                        QDir::toNativeSeparators(tc->debuggerCommand())));
    layout->addWidget(label);
}

void MaemoToolChainConfigWidget::apply()
{
    // nothing to do!
}

void MaemoToolChainConfigWidget::discard()
{
    // nothing to do!
}

bool MaemoToolChainConfigWidget::isDirty() const
{
    return false;
}

// --------------------------------------------------------------------------
// MaemoToolChainFactory
// --------------------------------------------------------------------------

MaemoToolChainFactory::MaemoToolChainFactory() :
    ProjectExplorer::ToolChainFactory()
{ }

QString MaemoToolChainFactory::displayName() const
{
    return tr("Maemo GCC");
}

QString MaemoToolChainFactory::id() const
{
    return QLatin1String(Constants::MAEMO_TOOLCHAIN_ID);
}

QList<ProjectExplorer::ToolChain *> MaemoToolChainFactory::autoDetect()
{
    QtSupport::QtVersionManager *vm = QtSupport::QtVersionManager::instance();
    connect(vm, SIGNAL(qtVersionsChanged(QList<int>)),
            this, SLOT(handleQtVersionChanges(QList<int>)));

    QList<int> versionList;
    foreach (QtSupport::BaseQtVersion *v, vm->versions())
        versionList.append(v->uniqueId());

    return createToolChainList(versionList);
}

void MaemoToolChainFactory::handleQtVersionChanges(const QList<int> &changes)
{
    ProjectExplorer::ToolChainManager *tcm = ProjectExplorer::ToolChainManager::instance();
    QList<ProjectExplorer::ToolChain *> tcList = createToolChainList(changes);
    foreach (ProjectExplorer::ToolChain *tc, tcList)
        tcm->registerToolChain(tc);
}

QList<ProjectExplorer::ToolChain *> MaemoToolChainFactory::createToolChainList(const QList<int> &changes)
{
    ProjectExplorer::ToolChainManager *tcm = ProjectExplorer::ToolChainManager::instance();
    QtSupport::QtVersionManager *vm = QtSupport::QtVersionManager::instance();
    QList<ProjectExplorer::ToolChain *> result;

    foreach (int i, changes) {
        QtSupport::BaseQtVersion *v = vm->version(i);
        // remove tool chain (on removal, change or addition:
        QList<ProjectExplorer::ToolChain *> toRemove;
        foreach (ProjectExplorer::ToolChain *tc, tcm->toolChains()) {
            if (!tc->id().startsWith(QLatin1String(Constants::MAEMO_TOOLCHAIN_ID)))
                continue;
            MaemoToolChain *mTc = static_cast<MaemoToolChain *>(tc);
            if (mTc->qtVersionId() == i)
                toRemove.append(mTc);
        }
        foreach (ProjectExplorer::ToolChain *tc, toRemove)
            tcm->deregisterToolChain(tc);

        const MaemoQtVersion * const mqv = dynamic_cast<MaemoQtVersion *>(v);
        if (!mqv || !mqv->isValid())
            continue;

        // (Re-)add toolchain:
        // add tool chain:
        MaemoToolChain *mTc = new MaemoToolChain(true);
        mTc->setQtVersionId(i);
        QString target = "Maemo 5";
        if (v->supportsTargetId(Constants::HARMATTAN_DEVICE_TARGET_ID))
            target = "Maemo 6";
        else if (v->supportsTargetId(Constants::MEEGO_DEVICE_TARGET_ID))
            target = "Meego";
        mTc->setDisplayName(tr("%1 GCC (%2)").arg(target).arg(MaemoGlobal::maddeRoot(mqv->qmakeCommand())));
        mTc->setCompilerPath(MaemoGlobal::targetRoot(mqv->qmakeCommand()) + QLatin1String("/bin/gcc"));
        mTc->setDebuggerCommand(ProjectExplorer::ToolChainManager::instance()->defaultDebugger(mqv->qtAbis().at(0)));
        if (mTc->debuggerCommand().isEmpty())
            mTc->setDebuggerCommand(MaemoGlobal::targetRoot(mqv->qmakeCommand()) + QLatin1String("/bin/gdb"));
        result.append(mTc);
    }
    return result;
}

} // namespace Internal
} // namespace RemoteLinux
