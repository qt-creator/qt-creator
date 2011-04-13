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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef IMODE_H
#define IMODE_H

#include "icontext.h"

#include <coreplugin/core_global.h>

QT_BEGIN_NAMESPACE
class QIcon;
QT_END_NAMESPACE

namespace Core {

class CORE_EXPORT IMode : public IContext
{
    Q_OBJECT
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled)
public:
    IMode(QObject *parent = 0);
    virtual ~IMode() {}

    virtual QString displayName() const = 0;
    virtual QIcon icon() const = 0;
    virtual int priority() const = 0;
    virtual QString id() const = 0;
    virtual QString type() const = 0;

    void setEnabled(bool enabled);
    bool isEnabled() const;

signals:
    void enabledStateChanged(bool enabled);

private:
    bool m_isEnabled;
};

} // namespace Core

#endif // IMODE_H
