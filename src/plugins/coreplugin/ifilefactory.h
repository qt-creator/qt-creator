/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
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

    virtual QString id() const = 0;
    virtual QString displayName() const = 0;

    virtual Core::IFile *open(const QString &fileName) = 0;
};

} // namespace Core

#endif // IFILEFACTORY_H
