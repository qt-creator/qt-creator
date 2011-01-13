/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#include <coreplugin/ssh/sshconnection.h>

#include <QtCore/QAbstractListModel>
#include <QtCore/QList>
#include <QtCore/QPair>
#include <QtCore/QSharedPointer>
#include <QtCore/QString>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace Qt4ProjectManager {
namespace Internal {

class MaemoPortList
{
    typedef QPair<int, int> Range;
public:
    void addPort(int port) { addRange(port, port); }
    void addRange(int startPort, int endPort) {
        m_ranges << Range(startPort, endPort);
    }
    bool hasMore() const { return !m_ranges.isEmpty(); }
    int count() const {
        int n = 0;
        foreach (const Range &r, m_ranges)
            n += r.second - r.first + 1;
        return n;
    }
    int getNext() {
        Q_ASSERT(!m_ranges.isEmpty());
        Range &firstRange = m_ranges.first();
        const int next = firstRange.first++;
        if (firstRange.first > firstRange.second)
            m_ranges.removeFirst();
        return next;
    }
    QString toString() const
    {
        QString stringRep;
        foreach (const Range &range, m_ranges) {
            stringRep += QString::number(range.first);
            if (range.second != range.first)
                stringRep += QLatin1Char('-') + QString::number(range.second);
            stringRep += QLatin1Char(',');
        }
        if (!stringRep.isEmpty())
            stringRep.remove(stringRep.length() - 1, 1); // Trailing comma.
        return stringRep;
    }

private:
    QList<Range> m_ranges;
};

class MaemoDeviceConfig
{
    friend class MaemoDeviceConfigurations;
public:
    typedef QSharedPointer<const MaemoDeviceConfig> ConstPtr;
    typedef quint64 Id;
    enum DeviceType { Physical, Simulator };

    MaemoPortList freePorts() const;
    Core::SshConnectionParameters sshParameters() const { return m_sshParameters; }
    QString name() const { return m_name; }
    DeviceType type() const { return m_type; }
    QString portsSpec() const { return m_portsSpec; }
    bool isDefault() const { return m_isDefault; }
    Id internalId() const { return m_internalId; }
    static QString portsRegExpr();

    static const Id InvalidId = 0;

private:
    typedef QSharedPointer<MaemoDeviceConfig> Ptr;

    MaemoDeviceConfig(const QString &name, DeviceType type, Id &nextId);
    MaemoDeviceConfig(const QSettings &settings, Id &nextId);
    MaemoDeviceConfig(const ConstPtr &other);

    MaemoDeviceConfig(const MaemoDeviceConfig &);
    MaemoDeviceConfig &operator=(const MaemoDeviceConfig &);

    static Ptr create(const QString &name, DeviceType type, Id &nextId);
    static Ptr create(const QSettings &settings, Id &nextId);
    static Ptr create(const ConstPtr &other);

    void save(QSettings &settings) const;
    int defaultSshPort(DeviceType type) const;
    QString defaultPortsSpec(DeviceType type) const;
    QString defaultHost(DeviceType type) const;

    Core::SshConnectionParameters m_sshParameters;
    QString m_name;
    DeviceType m_type;
    QString m_portsSpec;
    bool m_isDefault;
    Id m_internalId;
};


class MaemoDeviceConfigurations : public QAbstractListModel
{
    Q_OBJECT
    Q_DISABLE_COPY(MaemoDeviceConfigurations)
public:
    static MaemoDeviceConfigurations *instance(QObject *parent = 0);

    static void replaceInstance(const MaemoDeviceConfigurations *other);
    static MaemoDeviceConfigurations *cloneInstance();

    MaemoDeviceConfig::ConstPtr deviceAt(int i) const;
    MaemoDeviceConfig::ConstPtr find(MaemoDeviceConfig::Id id) const;
    MaemoDeviceConfig::ConstPtr defaultDeviceConfig() const;
    bool hasConfig(const QString &name) const;
    int indexForInternalId(MaemoDeviceConfig::Id internalId) const;
    MaemoDeviceConfig::Id internalId(MaemoDeviceConfig::ConstPtr devConf) const;

    void setDefaultSshKeyFilePath(const QString &path) { m_defaultSshKeyFilePath = path; }
    QString defaultSshKeyFilePath() const { return m_defaultSshKeyFilePath; }

    void addConfiguration(const QString &name,
        MaemoDeviceConfig::DeviceType type);
    void removeConfiguration(int i);
    void setConfigurationName(int i, const QString &name);
    void setDeviceType(int i, const MaemoDeviceConfig::DeviceType type);
    void setSshParameters(int i, const Core::SshConnectionParameters &params);
    void setPortsSpec(int i, const QString &portsSpec);
    void setDefaultDevice(int index);

    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex &index,
        int role = Qt::DisplayRole) const;

signals:
    void updated();

private:
    MaemoDeviceConfigurations(QObject *parent);
    void load();
    void save();
    void initShadowDevConfs();
    static void copy(const MaemoDeviceConfigurations *source,
        MaemoDeviceConfigurations *target, bool deep);
    void setupShadowDevConf(int i);

    static MaemoDeviceConfigurations *m_instance;
    MaemoDeviceConfig::Id m_nextId;
    QList<MaemoDeviceConfig::Ptr> m_devConfigs;
    QList<MaemoDeviceConfig::Ptr> m_shadowDevConfigs;
    QString m_defaultSshKeyFilePath;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // MAEMODEVICECONFIGURATIONS_H
