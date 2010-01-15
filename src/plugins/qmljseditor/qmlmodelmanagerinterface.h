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

#ifndef QMLMODELMANAGERINTERFACE_H
#define QMLMODELMANAGERINTERFACE_H

#include "qmljseditor_global.h"

#include <qmljs/qmldocument.h>
#include <qmljs/qmltypesystem.h>

#include <QObject>
#include <QStringList>
#include <QSharedPointer>

namespace Qml {
class Snapshot;
}

namespace QmlJSEditor {

class QMLJSEDITOR_EXPORT QmlModelManagerInterface: public QObject
{
    Q_OBJECT

public:
    QmlModelManagerInterface(QObject *parent = 0);
    virtual ~QmlModelManagerInterface();

    virtual Qml::Snapshot snapshot() const = 0;
    virtual void updateSourceFiles(const QStringList &files) = 0;

signals:
    void documentUpdated(Qml::QmlDocument::Ptr doc);
};

} // namespace QmlJSEditor

#endif // QMLMODELMANAGERINTERFACE_H
