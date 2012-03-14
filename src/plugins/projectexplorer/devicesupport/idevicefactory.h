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
#ifndef IDEVICEFACTORY_H
#define IDEVICEFACTORY_H

#include "idevice.h"
#include <projectexplorer/projectexplorer_export.h>

#include <QObject>
#include <QStringList>
#include <QVariantMap>

QT_BEGIN_NAMESPACE
class QDialog;
class QWidget;
QT_END_NAMESPACE

namespace ProjectExplorer {
class IDeviceWidget;
class IDeviceWizard;

/*!
  \class ProjectExplorer::IDeviceFactory

  \brief Provides an interface for classes providing services related to certain type of device.

  The factory objects have to be added to the global object pool via
  \c ExtensionSystem::PluginManager::addObject().
  \sa ExtensionSystem::PluginManager::addObject()
*/
class PROJECTEXPLORER_EXPORT IDeviceFactory : public QObject
{
    Q_OBJECT

public:
    /*!
      A short, one-line description of what kind of device this factory supports.
    */
    virtual QString displayName() const = 0;

    /*!
      A wizard that can create the types of device this factory supports.
    */
    virtual IDeviceWizard *createWizard(QWidget *parent = 0) const = 0;

    /*!
      Loads a device from a serialized state. The device must be of a matching type.
    */
    virtual IDevice::Ptr loadDevice(const QVariantMap &map) const = 0;

    /*!
      A widget that can configure the device this factory supports.
    */
    virtual IDeviceWidget *createWidget(const IDevice::Ptr &device, QWidget *parent = 0) const = 0;

    /*!
      Returns true iff this factory supports the given device type.
    */
    virtual bool supportsDeviceType(const QString &type) const = 0;

    /*!
      Returns a human-readable string for the given device type, if this factory supports that type.
    */
    virtual QString displayNameForDeviceType(const QString &type) const = 0;

    /*!
      Returns a list of ids representing actions that can be run on devices
      that this factory supports. These actions will be available in the "Devices"
      options page.
    */
    virtual QStringList supportedDeviceActionIds() const = 0;

    /*!
      A human-readable string for the given id. Will be displayed on a button which, when clicked,
      starts the respective action.
    */
    virtual QString displayNameForActionId(const QString &actionId) const = 0;

    /*!
      True iff the user should be allowed to edit the devices created by this
      factory. Returns true by default. Override if your factory creates fixed configurations
      for which later editing makes no sense.
    */
    virtual bool isUserEditable() const { return true; }

    /*!
      Produces a dialog implementing the respective action. The dialog is supposed to be
      modal, so implementers must make sure to make it interruptible as to not needlessly
      block the UI.
    */
    virtual QDialog *createDeviceAction(const QString &actionId, const IDevice::ConstPtr &device,
        QWidget *parent = 0) const = 0;

protected:
    IDeviceFactory(QObject *parent) : QObject(parent) { }
};

} // namespace ProjectExplorer

#endif // IDEVICEFACTORY_H
