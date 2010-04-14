/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef S60DEVICESPREFERENCEPANE_H
#define S60DEVICESPREFERENCEPANE_H

#include "s60devices.h"
#include <coreplugin/dialogs/ioptionspage.h>

#include <QtCore/QPointer>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE
class QStandardItem;
class QAbstractButton;
class QModelIndex;
QT_END_NAMESPACE

namespace Qt4ProjectManager {
namespace Internal {
class AutoDetectS60Devices;
class GnuPocS60Devices;
class S60DevicesModel;

namespace Ui {    
    class S60DevicesPreferencePane;
}

// Base pane listing Symbian SDKs with "Change Qt" version functionality.
class S60DevicesBaseWidget : public QWidget
{
    Q_OBJECT
public:
    typedef QList<S60Devices::Device> DeviceList;

    enum Flags { ShowAddButton = 0x1, ShowRemoveButton = 0x2,
                 ShowChangeQtButton =0x4, ShowRefreshButton = 0x8,
                 DeviceDefaultCheckable = 0x10 };

    virtual ~S60DevicesBaseWidget();

    DeviceList devices() const;
    void setDevices(const DeviceList &dl, const QString &errorString = QString());

protected:
    explicit S60DevicesBaseWidget(unsigned flags, QWidget *parent = 0);

    void setErrorLabel(const QString&);
    void clearErrorLabel();

    QString promptDirectory(const QString &title);
    void appendDevice(const S60Devices::Device &d);
    int deviceCount() const;

    QStandardItem *currentItem() const;

private slots:
    void currentChanged(const QModelIndex &current, const QModelIndex &previous);
    void changeQtVersion();
    void removeDevice();
    virtual void addDevice() {} // Default does nothing
    virtual void refresh() {} // Default does nothing

private:
    Ui::S60DevicesPreferencePane *m_ui;
    S60DevicesModel *m_model;
};

// Widget for autodetected SDK's showing a refresh button.
class AutoDetectS60DevicesWidget : public S60DevicesBaseWidget
{
    Q_OBJECT
public:
    explicit AutoDetectS60DevicesWidget(QWidget *parent,
                                        AutoDetectS60Devices *devices,
                                        bool changeQtVersionEnabled);

private slots:
    virtual void refresh();

private:
    AutoDetectS60Devices *m_devices;
};

// Widget for manually configured SDK's showing a add/remove buttons.
class GnuPocS60DevicesWidget : public S60DevicesBaseWidget
{
    Q_OBJECT
public:
    explicit GnuPocS60DevicesWidget(QWidget *parent = 0);

private slots:
    virtual void addDevice();
};

// Options Pane.
class S60DevicesPreferencePane : public Core::IOptionsPage
{
    Q_OBJECT
public:
    S60DevicesPreferencePane(S60Devices *devices, QObject *parent = 0);
    ~S60DevicesPreferencePane();

    QString id() const;
    QString displayName() const;
    QString category() const;
    QString displayCategory() const;
    QIcon categoryIcon() const;

    QWidget *createPage(QWidget *parent);
    void apply();
    void finish();

private:
    S60DevicesBaseWidget *createWidget(QWidget *parent) const;

    QPointer<S60DevicesBaseWidget> m_widget;
    S60Devices *m_devices;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // S60DEVICESPREFERENCEPANE_H
