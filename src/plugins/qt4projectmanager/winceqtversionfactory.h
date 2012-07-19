/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#ifndef WINCEQTVERSIONFACTORY_H
#define WINCEQTVERSIONFACTORY_H

#include <qtsupport/qtversionfactory.h>

namespace Qt4ProjectManager {
namespace Internal {

class WinCeQtVersionFactory : public QtSupport::QtVersionFactory
{
public:
    explicit WinCeQtVersionFactory(QObject *parent = 0);
    ~WinCeQtVersionFactory();

    virtual bool canRestore(const QString &type);
    virtual QtSupport::BaseQtVersion *restore(const QString &type, const QVariantMap &data);

    virtual int priority() const;

    virtual QtSupport::BaseQtVersion *create(const Utils::FileName &qmakePath, ProFileEvaluator *evaluator,
                                             bool isAutoDetected = false, const QString &autoDetectionSource = QString());

};

} // Internal
} // Qt4ProjectManager

#endif // WINCEQTVERSIONFACTORY_H
