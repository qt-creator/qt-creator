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

#ifndef BASEVIEW_H
#define BASEVIEW_H

#include "core_global.h"
#include "iview.h"
#include <QtCore/QPointer>

namespace Core {

class CORE_EXPORT BaseView : public IView
{
    Q_OBJECT

public:
    BaseView(QObject *parent = 0);
    ~BaseView();

    QList<int> context() const;
    QWidget *widget();
    const char *uniqueViewName() const;
    IView::ViewPosition defaultPosition() const;

    void setUniqueViewName(const char *name);
    QWidget *setWidget(QWidget *widget);
    void setContext(const QList<int> &context);
    void setDefaultPosition(IView::ViewPosition position);

private:
    const char *m_viewName;
    QPointer<QWidget> m_widget;
    QList<int> m_context;
    IView::ViewPosition m_defaultPosition;
};

} // namespace Core

#endif // BASEVIEW_H
