/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Creator.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
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
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef MAEMODEVICECONFIGURATIONS_H
#define MAEMODEVICECONFIGURATIONS_H

#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QtGlobal>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace Qt4ProjectManager {
namespace Internal {

class MaemoDeviceConfigurations
{
public:
    class DeviceConfig
    {
    public:
        enum DeviceType { Physical, Simulator };
        DeviceConfig(const QString &name);
        DeviceConfig(const QSettings &settings);
        void save(QSettings &settings) const;
        QString name;
        DeviceType type;
        QString host;
        int port;
        QString uname;
        QString pwd;
        int timeout;
    };

    static MaemoDeviceConfigurations &instance();
    QList<DeviceConfig> devConfigs() const { return m_devConfigs; }
    void setDevConfigs(const QList<DeviceConfig> &devConfigs);

private:
    MaemoDeviceConfigurations();
    MaemoDeviceConfigurations(const MaemoDeviceConfigurations &);
    MaemoDeviceConfigurations& operator=(const MaemoDeviceConfigurations &);
    void load();
    void save();

    QList<DeviceConfig> m_devConfigs;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // MAEMODEVICECONFIGURATIONS_H
