/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef IFILEFACTORY_H
#define IFILEFACTORY_H

#include "core_global.h"

#include <QtCore/QObject>

QT_BEGIN_NAMESPACE
class QStringList;
QT_END_NAMESPACE

namespace Core {

class IFile;

class CORE_EXPORT IFileFactory : public QObject
{
    Q_OBJECT
public:
    IFileFactory(QObject *parent = 0) : QObject(parent) {}
    virtual ~IFileFactory() {}

    virtual QStringList mimeTypes() const = 0;

    virtual QString kind() const = 0;
    virtual Core::IFile *open(const QString &fileName) = 0;
};

} // namespace Core

#endif // IFILEFACTORY_H
