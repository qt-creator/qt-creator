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

#include "maemotoolchain.h"

#include "maemoglobal.h"
#include "maemomanager.h"
#include "qt4projectmanagerconstants.h"
#include "qtversionmanager.h"

#include <projectexplorer/gccparser.h>
#include <projectexplorer/headerpath.h>
#include <projectexplorer/toolchainmanager.h>
#include <utils/environment.h>

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtGui/QLabel>
#include <QtGui/QVBoxLayout>

namespace Qt4ProjectManager {
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

bool MaemoToolChain::isValid() const
{
    return GccToolChain::isValid() && m_qtVersionId >= 0 && m_targetAbi.isValid();
}

void MaemoToolChain::addToEnvironment(Utils::Environment &env) const
{
    QtVersion *v = QtVersionManager::instance()->version(m_qtVersionId);
    const QString maddeRoot = MaemoGlobal::maddeRoot(v);
    env.prependOrSetPath(QDir::toNativeSeparators(QString("%1/bin").arg(maddeRoot)));
    env.prependOrSetPath(QDir::toNativeSeparators(QString("%1/bin")
                                                  .arg(MaemoGlobal::targetRoot(v))));

    // put this into environment to make pkg-config stuff work
    env.prependOrSet(QLatin1String("SYSROOT_DIR"), QDir::toNativeSeparators(sysroot()));
    env.prependOrSetPath(QDir::toNativeSeparators(QString("%1/madbin")
        .arg(maddeRoot)));
    env.prependOrSetPath(QDir::toNativeSeparators(QString("%1/madlib")
        .arg(maddeRoot)));
    env.prependOrSet(QLatin1String("PERL5LIB"),
        QDir::toNativeSeparators(QString("%1/madlib/perl5").arg(maddeRoot)));

    const QString manglePathsKey = QLatin1String("GCCWRAPPER_PATHMANGLE");
    if (!env.hasKey(manglePathsKey)) {
        const QStringList pathsToMangle = QStringList() << QLatin1String("/lib")
            << QLatin1String("/opt") << QLatin1String("/usr");
        env.set(manglePathsKey, QString());
        foreach (const QString &path, pathsToMangle)
            env.appendOrSet(manglePathsKey, path, QLatin1String(":"));
    }
}

QString MaemoToolChain::sysroot() const
{
    QtVersion *v = QtVersionManager::instance()->version(m_qtVersionId);
    if (!v)
        return QString();

    if (m_sysroot.isEmpty()) {
        QFile file(QDir::cleanPath(MaemoGlobal::targetRoot(v)) + QLatin1String("/information"));
        if (file.exists() && file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream stream(&file);
            while (!stream.atEnd()) {
                const QString &line = stream.readLine().trimmed();
                const QStringList &list = line.split(QLatin1Char(' '));
                if (list.count() > 1 && list.at(0) == QLatin1String("sysroot"))
                    m_sysroot = MaemoGlobal::maddeRoot(v) + QLatin1String("/sysroots/") + list.at(1);
            }
        }
    }
    return m_sysroot;
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
        updateId();
        return;
    }

    QtVersion *version = QtVersionManager::instance()->version(id);
    Q_ASSERT(version);
    ProjectExplorer::Abi::OSFlavor flavour = ProjectExplorer::Abi::HarmattanLinuxFlavor;
    if (MaemoGlobal::isValidMaemo5QtVersion(version))
        flavour = ProjectExplorer::Abi::MaemoLinuxFlavor;
    else if (MaemoGlobal::isValidHarmattanQtVersion(version))
        flavour = ProjectExplorer::Abi::HarmattanLinuxFlavor;
    else if (MaemoGlobal::isValidMeegoQtVersion(version))
        flavour = ProjectExplorer::Abi::MeegoLinuxFlavor;
    else
        return;

    m_qtVersionId = id;

    Q_ASSERT(version->qtAbis().count() == 1);
    m_targetAbi = version->qtAbis().at(0);

    updateId();
    setDisplayName(MaemoToolChainFactory::tr("Maemo GCC for %1").arg(version->displayName()));
}

int MaemoToolChain::qtVersionId() const
{
    return m_qtVersionId;
}

void MaemoToolChain::updateId()
{
    setId(QString::fromLatin1("%1:%2").arg(Constants::MAEMO_TOOLCHAIN_ID).arg(m_qtVersionId));
}

// --------------------------------------------------------------------------
// ToolChainConfigWidget
// --------------------------------------------------------------------------

MaemoToolChainConfigWidget::MaemoToolChainConfigWidget(MaemoToolChain *tc) :
    ProjectExplorer::ToolChainConfigWidget(tc)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    QLabel *label = new QLabel;
    QtVersion *v = QtVersionManager::instance()->version(tc->qtVersionId());
    Q_ASSERT(v);
    label->setText(tr("MADDE Root: %1<br>Target Root: %2")
                   .arg(MaemoGlobal::maddeRoot(v))
                   .arg(MaemoGlobal::targetRoot(v)));
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
// ToolChainFactory
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
    QList<ProjectExplorer::ToolChain *> result;

    QtVersionManager *vm = QtVersionManager::instance();
    connect(vm, SIGNAL(qtVersionsChanged(QList<int>)),
            this, SLOT(handleQtVersionChanges(QList<int>)));

    QList<int> versionList;
    foreach (QtVersion *v, vm->versions())
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
    QtVersionManager *vm = QtVersionManager::instance();
    QList<ProjectExplorer::ToolChain *> result;

    foreach (int i, changes) {
        QtVersion *v = vm->version(i);
        if (!v) {
            // remove ToolChain:
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
        } else if (v->supportsTargetId(Constants::MAEMO5_DEVICE_TARGET_ID)
                   || v->supportsTargetId(Constants::HARMATTAN_DEVICE_TARGET_ID)
                   || v->supportsTargetId(Constants::MEEGO_DEVICE_TARGET_ID)) {
            // add ToolChain:
            MaemoToolChain *mTc = new MaemoToolChain(true);
            mTc->setQtVersionId(i);
            QString target = "Maemo 5";
            if (v->supportsTargetId(Constants::HARMATTAN_DEVICE_TARGET_ID))
                target = "Maemo 6";
            else if (v->supportsTargetId(Constants::MEEGO_DEVICE_TARGET_ID))
                target = "Meego";
            mTc->setDisplayName(tr("%1 GCC (%2)").arg(target).arg(MaemoGlobal::maddeRoot(v)));
            mTc->setCompilerPath(MaemoGlobal::targetRoot(v) + QLatin1String("/bin/gcc"));
            result.append(mTc);
        }
    }
    return result;
}

} // namespace Internal
} // namespace Qt4ProjectManager
