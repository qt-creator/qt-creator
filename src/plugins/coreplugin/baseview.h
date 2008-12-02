/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#ifndef BASEVIEW_H
#define BASEVIEW_H

#include "core_global.h"
#include "iview.h"
#include <QtCore/QPointer>

namespace Core {

class CORE_EXPORT BaseView
  : public IView
{
    Q_OBJECT

public:
    BaseView(QObject *parent = 0);
    BaseView(const char *name, QWidget *widget, const QList<int> &context, IView::ViewPosition position, QObject *parent = 0);
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
