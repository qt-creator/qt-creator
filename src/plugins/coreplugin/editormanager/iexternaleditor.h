/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef IEXTERNALEDITOR_H
#define IEXTERNALEDITOR_H

#include <coreplugin/core_global.h>

#include <QObject>

namespace Core {

class Id;

class CORE_EXPORT IExternalEditor : public QObject
{
    Q_OBJECT

public:
    explicit IExternalEditor(QObject *parent = 0) : QObject(parent) {}

    virtual QStringList mimeTypes() const = 0;
    virtual Id id() const = 0;
    virtual QString displayName() const = 0;
    virtual bool startEditor(const QString &fileName, QString *errorMessage) = 0;
};

} // namespace Core

#endif // IEXTERNALEDITOR_H
