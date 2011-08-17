/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef QTVERSIONFACTORY_H
#define QTVERSIONFACTORY_H

#include "qtsupport_global.h"

#include <QtCore/QObject>
#include <QtCore/QVariantMap>

QT_FORWARD_DECLARE_CLASS(QSettings)
QT_FORWARD_DECLARE_CLASS(ProFileEvaluator)

namespace QtSupport {

class BaseQtVersion;

class QTSUPPORT_EXPORT QtVersionFactory : public QObject
{
    Q_OBJECT
public:
    explicit QtVersionFactory(QObject *parent = 0);
    ~QtVersionFactory();

    virtual bool canRestore(const QString &type) = 0;
    virtual BaseQtVersion *restore(const QString &type, const QVariantMap &data) = 0;

    /// factories with higher priority are asked first to identify
    /// a qtversion, the priority of the desktop factory is 0 and
    /// the desktop factory claims to handle all paths
    virtual int priority() const = 0;
    virtual BaseQtVersion *create(const QString &qmakePath, ProFileEvaluator *evaluator, bool isAutoDetected = false, const QString &autoDetectionSource = QString()) = 0;

    static BaseQtVersion *createQtVersionFromQMakePath(const QString &qmakePath, bool isAutoDetected = false, const QString &autoDetectionSource = QString());
    static BaseQtVersion *createQtVersionFromLegacySettings(const QString &qmakePath, int id, QSettings *s);
};

}
#endif // QTVERSIONFACTORY_H
