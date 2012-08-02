/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
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
#ifndef MAEMOAPPLICATIONRUNNERHELPERACTIONS_H
#define MAEMOAPPLICATIONRUNNERHELPERACTIONS_H

#include <projectexplorer/devicesupport/deviceapplicationrunner.h>

#include <QList>

namespace Utils { class FileName; }

namespace Madde {
namespace Internal {
class MaemoMountSpecification;
class MaemoRemoteMounter;

class MaemoPreRunAction : public ProjectExplorer::DeviceApplicationHelperAction
{
    Q_OBJECT
public:
    MaemoPreRunAction(const ProjectExplorer::IDevice::ConstPtr &device,
            const Utils::FileName &maddeRoot, const QList<MaemoMountSpecification> &mountSpecs,
            QObject *parent = 0);

    MaemoRemoteMounter *mounter() const { return m_mounter; }

private slots:
    void handleMounted();
    void handleError(const QString &message);

private:
    void start();
    void stop();

    void setFinished(bool success);

    MaemoRemoteMounter * const m_mounter;
    bool m_isRunning;
};

class MaemoPostRunAction : public ProjectExplorer::DeviceApplicationHelperAction
{
    Q_OBJECT
public:
    MaemoPostRunAction(MaemoRemoteMounter *mounter, QObject *parent = 0);

private slots:
    void handleUnmounted();
    void handleError(const QString &message);

private:
    void start();
    void stop();

    void setFinished(bool success);

    MaemoRemoteMounter * const m_mounter;
    bool m_isRunning;
};

} // namespace Internal
} // namespace Madde

#endif // MAEMOAPPLICATIONRUNNERHELPERACTIONS_H
