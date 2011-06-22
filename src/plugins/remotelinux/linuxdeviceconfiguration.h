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
#ifndef LINUXDEVICECONFIGURATION_H
#define LINUXDEVICECONFIGURATION_H

#include "portlist.h"
#include "remotelinux_export.h"

#include <utils/ssh/sshconnection.h>

#include <QtCore/QSharedPointer>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtGui/QWizard>

QT_BEGIN_NAMESPACE
class QDialog;
class QSettings;
QT_END_NAMESPACE


namespace RemoteLinux {
namespace Internal {
class LinuxDeviceConfigurations;
}

class REMOTELINUX_EXPORT LinuxDeviceConfiguration
{
    friend class Internal::LinuxDeviceConfigurations;
public:
    typedef QSharedPointer<LinuxDeviceConfiguration> Ptr;
    typedef QSharedPointer<const LinuxDeviceConfiguration> ConstPtr;

    typedef quint64 Id;

    static const QString Maemo5OsType;
    static const QString HarmattanOsType;
    static const QString MeeGoOsType;
    static const QString GenericLinuxOsType;

    enum DeviceType { Physical, Emulator };

    ~LinuxDeviceConfiguration();

    PortList freePorts() const { return m_freePorts; }
    Utils::SshConnectionParameters sshParameters() const { return m_sshParameters; }
    QString name() const { return m_name; }
    void setName(const QString &name) { m_name = name; }
    QString osType() const { return m_osType; }
    DeviceType type() const { return m_type; }
    Id internalId() const { return m_internalId; }
    bool isDefault() const { return m_isDefault; }

    static QString defaultPrivateKeyFilePath();
    static QString defaultPublicKeyFilePath();

    static const Id InvalidId;

    static Ptr create(const QString &name, const QString &osType, DeviceType deviceType,
        const PortList &freePorts, const Utils::SshConnectionParameters &sshParams);
private:
    LinuxDeviceConfiguration(const QString &name, const QString &osType, DeviceType deviceType,
        const PortList &freePorts, const Utils::SshConnectionParameters &sshParams);

    LinuxDeviceConfiguration(const QSettings &settings, Id &nextId);
    LinuxDeviceConfiguration(const ConstPtr &other);

    LinuxDeviceConfiguration(const LinuxDeviceConfiguration &);
    LinuxDeviceConfiguration &operator=(const LinuxDeviceConfiguration &);

    static Ptr create(const QSettings &settings, Id &nextId);
    static Ptr create(const ConstPtr &other);

    void save(QSettings &settings) const;

    Utils::SshConnectionParameters m_sshParameters;
    QString m_name;
    QString m_osType;
    DeviceType m_type;
    PortList m_freePorts;
    bool m_isDefault;
    Id m_internalId;
};


/*!
  \class RemoteLinux::ILinuxDeviceConfigurationWizard

  \brief Provides an interface for wizards creating a LinuxDeviceConfiguration

  A class implementing this interface is a wizard whose final result is
  a LinuxDeviceConfiguration object. The wizard will be started when the user chooses the
  "Add..." action from the "Linux devices" options page.
*/
class REMOTELINUX_EXPORT ILinuxDeviceConfigurationWizard : public QWizard
{
    Q_OBJECT
    Q_DISABLE_COPY(ILinuxDeviceConfigurationWizard)
public:
    virtual LinuxDeviceConfiguration::Ptr deviceConfiguration()=0;

protected:
    ILinuxDeviceConfigurationWizard(QWidget *parent) : QWizard(parent) {}
};


/*!
  \class ProjectExplorer::ILinuxDeviceConfiguration factory.

  \brief Provides an interface for classes providing services related to certain type of Linux devices.

  The main service is a wizard providing the device configuration itself.

  The factory objects have to be added to the global object pool via
  \c ExtensionSystem::PluginManager::addObject().
  \sa ExtensionSystem::PluginManager::addObject()
*/
class REMOTELINUX_EXPORT ILinuxDeviceConfigurationFactory : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(ILinuxDeviceConfigurationFactory)
public:
    /*!
      A short, one-line description of what kind of device this factory supports.
    */
    virtual QString displayName() const=0;

    /*!
      A wizard that can create the types of device configuration this factory supports.
    */
    virtual ILinuxDeviceConfigurationWizard *createWizard(QWidget *parent = 0) const=0;


    /*!
      Returns true iff this factory supports the given device type.
    */
    virtual bool supportsOsType(const QString &osType) const=0;

    /*!
      Returns a human-readable string for the given OS type, if this factory supports that type.
    */
    virtual QString displayNameForOsType(const QString &osType) const=0;

    /*!
      Returns a list of ids representing actions that can be run on device configurations
      that this factory supports. These actions will be available in the "Linux Devices"
      options page.
    */
    virtual QStringList supportedDeviceActionIds() const=0;

    /*!
      A human-readable string for the given id. Will be displayed on a button which, when clicked,
      will start the respective action.
    */
    virtual QString displayNameForActionId(const QString &actionId) const=0;


    /*!
      Produces a dialog implementing the respective action. The dialog is supposed to be
      modal, so implementers must make sure to make it interruptible as to not needlessly
      block the UI.
    */
    virtual QDialog *createDeviceAction(const QString &actionId,
        const LinuxDeviceConfiguration::ConstPtr &deviceConfig, QWidget *parent = 0) const=0;

protected:
    ILinuxDeviceConfigurationFactory(QObject *parent) : QObject(parent) {}
};

} // namespace RemoteLinux

#endif // LINUXDEVICECONFIGURATION_H
