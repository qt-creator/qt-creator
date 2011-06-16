/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
#ifndef MAEMORUNCONFIGURATION_H
#define MAEMORUNCONFIGURATION_H

#include "remotelinuxrunconfiguration.h"

namespace RemoteLinux {
namespace Internal {
class AbstractQt4MaemoTarget;
class MaemoRemoteMountsModel;

class MaemoRunConfiguration : public RemoteLinuxRunConfiguration
{
    Q_OBJECT
    Q_DISABLE_COPY(MaemoRunConfiguration)
public:
    MaemoRunConfiguration(AbstractQt4MaemoTarget *parent, const QString &proFilePath);
    MaemoRunConfiguration(AbstractQt4MaemoTarget *parent, MaemoRunConfiguration *source);

    QVariantMap toMap() const;
    bool fromMap(const QVariantMap &map);
    bool isEnabled() const;
    QWidget *createConfigurationWidget();
    QString commandPrefix() const;
    PortList freePorts() const;
    DebuggingType debuggingType() const;

    Internal::MaemoRemoteMountsModel *remoteMounts() const { return m_remoteMounts; }
    bool hasEnoughFreePorts(const QString &mode) const;
    QString localDirToMountForRemoteGdb() const;
    QString remoteProjectSourcesMountPoint() const;

    static const QString Id;

signals:
    void remoteMountsChanged();

private slots:
    void handleRemoteMountsChanged();

private:
    void init();
    const AbstractQt4MaemoTarget *maemoTarget() const;

    MaemoRemoteMountsModel *m_remoteMounts;
};

} // namespace Internal
} // namespace RemoteLinux

#endif // MAEMORUNCONFIGURATION_H
