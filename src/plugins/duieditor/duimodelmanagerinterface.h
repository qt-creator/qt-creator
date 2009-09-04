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

#ifndef DUIMODELMANAGERINTERFACE_H
#define DUIMODELMANAGERINTERFACE_H

#include <QObject>
#include <QStringList>

#include <duieditor/duieditor_global.h>

namespace DuiEditor {

class Snapshot;

class DUIEDITOR_EXPORT DuiModelManagerInterface: public QObject
{
    Q_OBJECT

public:
    DuiModelManagerInterface(QObject *parent = 0);
    virtual ~DuiModelManagerInterface();

    virtual Snapshot snapshot() const = 0;
    virtual void updateSourceFiles(const QStringList &files) = 0;
};

}

#endif // DUIMODELMANAGERINTERFACE_H
