/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/
#ifndef LINUXDEVICECONFIGURATION_H
#define LINUXDEVICECONFIGURATION_H

#include "remotelinux_export.h"

#include <QSharedPointer>
#include <QString>
#include <QStringList>
#include <QVariantHash>
#include <QWizard>

QT_BEGIN_NAMESPACE
class QDialog;
class QSettings;
QT_END_NAMESPACE

namespace Utils {
class SshConnectionParameters;
}

namespace RemoteLinux {
class LinuxDeviceConfigurations;
class PortList;

namespace Internal {
class LinuxDeviceConfigurationPrivate;
} // namespace Internal

class REMOTELINUX_EXPORT LinuxDeviceConfiguration
{
    friend class LinuxDeviceConfigurations;
public:
    typedef QSharedPointer<LinuxDeviceConfiguration> Ptr;
    typedef QSharedPointer<const LinuxDeviceConfiguration> ConstPtr;

    typedef quint64 Id;

    enum DeviceType { Hardware, Emulator };
    enum Origin { ManuallyAdded, AutoDetected };

    ~LinuxDeviceConfiguration();

    PortList freePorts() const;
    Utils::SshConnectionParameters sshParameters() const;
    QString displayName() const;
    QString osType() const;
    DeviceType deviceType() const;
    Id internalId() const;
    bool isDefault() const;
    bool isAutoDetected() const;
    QVariantHash attributes() const;
    QVariant attribute(const QString &name) const;

    void setSshParameters(const Utils::SshConnectionParameters &sshParameters);
    void setFreePorts(const PortList &freePorts);
    void setAttribute(const QString &name, const QVariant &value);

    static QString defaultPrivateKeyFilePath();
    static QString defaultPublicKeyFilePath();

    static const Id InvalidId;

    static Ptr create(const QString &name, const QString &osType, DeviceType deviceType,
        const PortList &freePorts, const Utils::SshConnectionParameters &sshParams,
        const QVariantHash &attributes = QVariantHash(), Origin origin = ManuallyAdded);
private:
    LinuxDeviceConfiguration(const QString &name, const QString &osType, DeviceType deviceType,
        const PortList &freePorts, const Utils::SshConnectionParameters &sshParams,
        const QVariantHash &attributes, Origin origin);
    LinuxDeviceConfiguration(const QSettings &settings, Id &nextId);
    LinuxDeviceConfiguration(const ConstPtr &other);

    LinuxDeviceConfiguration(const LinuxDeviceConfiguration &);
    LinuxDeviceConfiguration &operator=(const LinuxDeviceConfiguration &);

    static Ptr create(const QSettings &settings, Id &nextId);
    static Ptr create(const ConstPtr &other);

    void setDisplayName(const QString &name);
    void setInternalId(Id id);
    void setDefault(bool isDefault);
    void save(QSettings &settings) const;

    Internal::LinuxDeviceConfigurationPrivate *d;
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

public:
    virtual LinuxDeviceConfiguration::Ptr deviceConfiguration() = 0;

protected:
    ILinuxDeviceConfigurationWizard(QWidget *parent) : QWizard(parent) {}
};


/*!
 \class RemoteLinux::LinuxDeviceConfigurationWidget : public QWidget

 \brief Provides an interface for the widget configuring a LinuxDeviceConfiguration

 A class implementing this interface will display a widget in the configuration
 options page "Linux Device", in the "Device configuration" tab.
 It's used to configure a particular device, the default widget is empty.
*/
class REMOTELINUX_EXPORT ILinuxDeviceConfigurationWidget : public QWidget
{
    Q_OBJECT

public:
    ILinuxDeviceConfigurationWidget(const LinuxDeviceConfiguration::Ptr &deviceConfig,
                                    QWidget *parent = 0);

signals:
    void defaultSshKeyFilePathChanged(const QString &path);

protected:
    LinuxDeviceConfiguration::Ptr deviceConfiguration() const;

private:
    LinuxDeviceConfiguration::Ptr m_deviceConfiguration;
};


/*!
  \class RemoteLinux::ILinuxDeviceConfiguration factory.

  \brief Provides an interface for classes providing services related to certain type of Linux devices.

  The main service is a wizard providing the device configuration itself.

  The factory objects have to be added to the global object pool via
  \c ExtensionSystem::PluginManager::addObject().
  \sa ExtensionSystem::PluginManager::addObject()
*/
class REMOTELINUX_EXPORT ILinuxDeviceConfigurationFactory : public QObject
{
    Q_OBJECT

public:
    /*!
      A short, one-line description of what kind of device this factory supports.
    */
    virtual QString displayName() const = 0;

    /*!
      A wizard that can create the types of device configuration this factory supports.
    */
    virtual ILinuxDeviceConfigurationWizard *createWizard(QWidget *parent = 0) const = 0;

    /*!
      A widget that can configure the device this factory supports.
    */
    virtual ILinuxDeviceConfigurationWidget *createWidget(
            const LinuxDeviceConfiguration::Ptr &deviceConfig,
            QWidget *parent = 0) const = 0;

    /*!
      Returns true iff this factory supports the given device type.
    */
    virtual bool supportsOsType(const QString &osType) const = 0;

    /*!
      Returns a human-readable string for the given OS type, if this factory supports that type.
    */
    virtual QString displayNameForOsType(const QString &osType) const = 0;

    /*!
      Returns a list of ids representing actions that can be run on device configurations
      that this factory supports. These actions will be available in the "Devices"
      options page.
    */
    virtual QStringList supportedDeviceActionIds() const = 0;

    /*!
      A human-readable string for the given id. Will be displayed on a button which, when clicked,
      will start the respective action.
    */
    virtual QString displayNameForActionId(const QString &actionId) const = 0;

    /*!
      True iff the user should be allowed to edit the device configurations created by this
      factory. Returns true by default. Override if your factory creates fixed configurations
      for which later editing makes no sense.
    */
    bool isUserEditable() const { return true; }

    /*!
      Produces a dialog implementing the respective action. The dialog is supposed to be
      modal, so implementers must make sure to make it interruptible as to not needlessly
      block the UI.
    */
    virtual QDialog *createDeviceAction(const QString &actionId,
        const LinuxDeviceConfiguration::ConstPtr &deviceConfig, QWidget *parent = 0) const = 0;

protected:
    ILinuxDeviceConfigurationFactory(QObject *parent) : QObject(parent) {}
};

} // namespace RemoteLinux

#endif // LINUXDEVICECONFIGURATION_H
