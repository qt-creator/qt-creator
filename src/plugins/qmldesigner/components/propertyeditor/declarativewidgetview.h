/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef DECLARATIVEWIDGETVIEW_H
#define DECLARATIVEWIDGETVIEW_H

#include <QWidget>
#include <QUrl>

QT_BEGIN_NAMESPACE
class QDeclarativeEngine;
class QDeclarativeContext;
class QDeclarativeError;
QT_END_NAMESPACE

namespace QmlDesigner {

class DeclarativeWidgetViewPrivate;

class DeclarativeWidgetView : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(QUrl source READ source WRITE setSource DESIGNABLE true)
public:
    explicit DeclarativeWidgetView(QWidget *parent = 0);

    virtual ~DeclarativeWidgetView();

    QUrl source() const;
    void setSource(const QUrl&);

    QDeclarativeEngine* engine();
    QDeclarativeContext* rootContext();

    QWidget *rootWidget() const;

    enum Status { Null, Ready, Loading, Error };
    Status status() const;

signals:
    void statusChanged(DeclarativeWidgetView::Status);

protected:
    virtual void setRootWidget(QWidget *);

private Q_SLOTS:
    void continueExecute();

private:
     friend class DeclarativeWidgetViewPrivate;
     DeclarativeWidgetViewPrivate *d;

};

} //QmlDesigner

#endif // DECLARATIVEWIDGETVIEW_H
