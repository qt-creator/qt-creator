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

#ifndef STATUSBARWIDGET_H
#define STATUSBARWIDGET_H

#include "core_global.h"
#include "icontext.h"
#include <QPointer>

namespace Core {

class CORE_EXPORT StatusBarWidget : public IContext
{
    Q_OBJECT

public:
    enum StatusBarPosition { First=0, Second=1, Third=2, Last=Third };

    StatusBarWidget(QObject *parent = 0);
    ~StatusBarWidget();

    StatusBarWidget::StatusBarPosition position() const;
    void setPosition(StatusBarWidget::StatusBarPosition position);

private:
    StatusBarWidget::StatusBarPosition m_defaultPosition;
};

} // namespace Core

#endif // STATUSBARWIDGET_H
