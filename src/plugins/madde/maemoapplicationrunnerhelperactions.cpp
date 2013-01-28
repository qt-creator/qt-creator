/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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
#include "maemoapplicationrunnerhelperactions.h"

#include "maemomountspecification.h"
#include "maemoremotemounter.h"

#include <utils/qtcassert.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace Madde {
namespace Internal {

MaemoPreRunAction::MaemoPreRunAction(const IDevice::ConstPtr &device, const FileName &maddeRoot,
        const QList<MaemoMountSpecification> &mountSpecs, QObject *parent)
    : DeviceApplicationHelperAction(parent),
      m_mounter(new MaemoRemoteMounter(this)),
      m_isRunning(false)
{
    m_mounter->setParameters(device, maddeRoot);
    foreach (const MaemoMountSpecification &m, mountSpecs)
        m_mounter->addMountSpecification(m, false);
}

void MaemoPreRunAction::handleMounted()
{
    QTC_ASSERT(m_isRunning, return);

    setFinished(true);
}

void MaemoPreRunAction::handleError(const QString &message)
{
    if (!m_isRunning)
        return;

    emit reportError(message);
    setFinished(false);
}

void MaemoPreRunAction::start()
{
    QTC_ASSERT(!m_isRunning, return);

    m_isRunning = true;
    if (!m_mounter->hasValidMountSpecifications()) {
        setFinished(true);
        return;
    }

    connect(m_mounter, SIGNAL(debugOutput(QString)), SIGNAL(reportProgress(QString)));
    connect(m_mounter, SIGNAL(reportProgress(QString)), SIGNAL(reportProgress(QString)));
    connect(m_mounter, SIGNAL(mounted()), SLOT(handleMounted()));
    connect(m_mounter, SIGNAL(error(QString)), SLOT(handleError(QString)));
    m_mounter->mount();
}

void MaemoPreRunAction::stop()
{
    QTC_ASSERT(m_isRunning, return);

    if (m_mounter->hasValidMountSpecifications())
        m_mounter->stop();
    setFinished(false);
}

void MaemoPreRunAction::setFinished(bool success)
{
    QTC_ASSERT(m_isRunning, return);

    m_mounter->disconnect(this);
    m_isRunning = false;
    emit finished(success);
}

MaemoPostRunAction::MaemoPostRunAction(MaemoRemoteMounter *mounter, QObject *parent)
        : DeviceApplicationHelperAction(parent), m_mounter(mounter), m_isRunning(false)
{
}

void MaemoPostRunAction::handleUnmounted()
{
    QTC_ASSERT(m_isRunning, return);

    setFinished(true);
}

void MaemoPostRunAction::handleError(const QString &message)
{
    if (!m_isRunning)
        return;

    emit reportError(message);
    setFinished(false);
}

void MaemoPostRunAction::start()
{
    QTC_ASSERT(!m_isRunning, return);

    m_isRunning = true;
    if (!m_mounter->hasValidMountSpecifications()) {
        setFinished(true);
        return;
    }

    connect(m_mounter, SIGNAL(debugOutput(QString)), SIGNAL(reportProgress(QString)));
    connect(m_mounter, SIGNAL(reportProgress(QString)), SIGNAL(reportProgress(QString)));
    connect(m_mounter, SIGNAL(unmounted()), SLOT(handleUnmounted()));
    connect(m_mounter, SIGNAL(error(QString)), SLOT(handleError(QString)));
    m_mounter->unmount();
}

void MaemoPostRunAction::stop()
{
    QTC_ASSERT(m_isRunning, return);

    m_mounter->stop();
    setFinished(false);
}

void MaemoPostRunAction::setFinished(bool success)
{
    QTC_ASSERT(m_isRunning, return);

    if (m_mounter->hasValidMountSpecifications()) {
        m_mounter->disconnect(this);
        m_isRunning = false;
    }
    emit finished(success);
}

} // namespace Internal
} // namespace Madde
