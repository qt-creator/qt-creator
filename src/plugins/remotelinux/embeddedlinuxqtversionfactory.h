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

#ifndef EMBEDDEDLINUXQTVERSIONFACTORY_H
#define EMBEDDEDLINUXQTVERSIONFACTORY_H

#include <qtsupport/qtversionfactory.h>

namespace RemoteLinux {
namespace Internal {

class EmbeddedLinuxQtVersionFactory : public QtSupport::QtVersionFactory
{
public:
    explicit EmbeddedLinuxQtVersionFactory(QObject *parent = 0);
    ~EmbeddedLinuxQtVersionFactory();

    bool canRestore(const QString &type);
    QtSupport::BaseQtVersion *restore(const QString &type, const QVariantMap &data);

    int priority() const;
    QtSupport::BaseQtVersion *create(const Utils::FileName &qmakePath, ProFileEvaluator *evaluator,
                                     bool isAutoDetected = false,
                                     const QString &autoDetectionSource = QString());
};

} // Internal
} // RemoteLinux

#endif // EMBEDDEDLINUXQTVERSIONFACTORY_H
