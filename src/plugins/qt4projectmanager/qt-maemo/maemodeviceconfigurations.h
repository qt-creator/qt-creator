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
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QtGlobal>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace Qt4ProjectManager {
namespace Internal {

class MaemoDeviceConfigurations : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(MaemoDeviceConfigurations)
public:
    class DeviceConfig
    {
    public:
        enum DeviceType { Physical, Simulator };
        enum AuthType { Password, Key };
        DeviceConfig();
        DeviceConfig(const QString &name);
        DeviceConfig(const QSettings &settings);
        void save(QSettings &settings) const;
        bool isValid() const;
        QString name;
        DeviceType type;
        QString host;
        int port;
        QString uname;
        AuthType authentication;
        QString pwd;
        QString keyFile;
        int timeout;
        quint64 internalId;

    private:
        static const quint64 InvalidId = 0;
    };

    class DevConfNameMatcher
    {
    public:
        DevConfNameMatcher(const QString &name) : m_name(name) {}
        bool operator()(const MaemoDeviceConfigurations::DeviceConfig &devConfig)
        {
            return devConfig.name == m_name;
        }
    private:
        const QString m_name;
    };

    static MaemoDeviceConfigurations &instance(QObject *parent = 0);
    QList<DeviceConfig> devConfigs() const { return m_devConfigs; }
    void setDevConfigs(const QList<DeviceConfig> &devConfigs);
    DeviceConfig find(const QString &name) const;
    DeviceConfig find(int id) const;

signals:
    void updated();

private:
    MaemoDeviceConfigurations(QObject *parent);
    void load();
    void save();

    static MaemoDeviceConfigurations *m_instance;
    QList<DeviceConfig> m_devConfigs;
    quint64 m_nextId;
    friend class MaemoDeviceConfigurations::DeviceConfig;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // MAEMODEVICECONFIGURATIONS_H
