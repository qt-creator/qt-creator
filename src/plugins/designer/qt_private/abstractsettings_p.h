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

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of Qt Designer.  This header
// file may change from version to version without notice, or even be removed.
//
// We mean it.
//

#ifndef ABSTRACTSETTINGS_P_H
#define ABSTRACTSETTINGS_P_H

#include <sdk_global.h>

#include <QVariant>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

class QString;

/*!
 To be implemented by IDEs that want to control the way designer retrieves/stores its settings.
 */
class QDESIGNER_SDK_EXPORT QDesignerSettingsInterface
{
public:
    virtual ~QDesignerSettingsInterface() {}

    virtual void beginGroup(const QString &prefix) = 0;
    virtual void endGroup() = 0;

    virtual bool contains(const QString &key) const = 0;
    virtual void setValue(const QString &key, const QVariant &value) = 0;
    virtual QVariant value(const QString &key, const QVariant &defaultValue = QVariant()) const = 0;
    virtual void remove(const QString &key) = 0;
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // ABSTRACTSETTINGS_P_H
