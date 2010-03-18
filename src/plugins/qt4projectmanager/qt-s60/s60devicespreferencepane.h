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

#include <coreplugin/dialogs/ioptionspage.h>

#include <QtCore/QPointer>
#include <QtGui/QWidget>

namespace Qt4ProjectManager {
namespace Internal {
class S60Devices;

namespace Ui {    
    class S60DevicesPreferencePane;
}

class S60DevicesWidget : public QWidget
{
    Q_OBJECT
public:
    S60DevicesWidget(QWidget *parent, S60Devices *devices);
    ~S60DevicesWidget();

private slots:
    void updateDevices();

private:
    void updateDevicesList();
    void setErrorLabel(const QString&);
    void clearErrorLabel();

    Ui::S60DevicesPreferencePane *m_ui;
    S60Devices *m_devices;
};

class S60DevicesPreferencePane : public Core::IOptionsPage {
    Q_OBJECT
public:
    S60DevicesPreferencePane(S60Devices *devices, QObject *parent = 0);
    ~S60DevicesPreferencePane();

    QString id() const;
    QString displayName() const;
    QString category() const;
    QString displayCategory() const;

    QWidget *createPage(QWidget *parent);
    void apply();
    void finish();

private:
    QPointer<S60DevicesWidget> m_widget;
    S60Devices *m_devices;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // S60DEVICESPREFERENCEPANE_H
