/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef SERIALDEVICELISTER_H
#define SERIALDEVICELISTER_H

#include <QtCore/QAbstractEventDispatcher>
#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QString>

//#ifdef Q_OS_WIN32
//#include <windows.h>
//#include <dbt.h>
//#endif

namespace Qt4ProjectManager {
namespace Internal {

class SerialDeviceLister : public QObject
{
    Q_OBJECT
public:

    struct SerialDevice {
        QString portName;
        QString friendlyName;
    };

    SerialDeviceLister(QObject *parent = 0);
    ~SerialDeviceLister();
    QList<SerialDevice> serialDevices() const;
    QString friendlyNameForPort(const QString &port) const;

public slots:
    void update();

signals:
    void updated();

private:
    void updateSilently() const;

    mutable bool m_initialized;
    mutable QList<SerialDevice> m_devices;
};

} // Internal
} // Qt4ProjectManager

#endif // SERIALDEVICELISTER_H
