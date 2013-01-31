/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "declarativewidgetview.h"

#include <qdeclarative.h>
#include <QDeclarativeItem>
#include <QDeclarativeEngine>
#include <QDeclarativeContext>
#include <QPointer>

namespace QmlDesigner {

void DeclarativeWidgetView::execute()
{
    if (m_root)
        delete m_root.data();

    if (m_component)
        delete m_component.data();

    if (!m_source.isEmpty()) {
        m_component = new QDeclarativeComponent(&m_engine, m_source, this);
        if (!m_component->isLoading())
            continueExecute();
        else
            connect(m_component.data(), SIGNAL(statusChanged(QDeclarativeComponent::Status)), this, SLOT(continueExecute()));
    }
}

DeclarativeWidgetView::DeclarativeWidgetView(QWidget *parent) :
    QWidget(parent)
{
}

QUrl DeclarativeWidgetView::source() const
{
    return m_source;
}

void DeclarativeWidgetView::setSource(const QUrl& url)
{
    m_source = url;
    execute();
}

QDeclarativeEngine* DeclarativeWidgetView::engine()
{
   return &m_engine;
}

QWidget *DeclarativeWidgetView::rootWidget() const
{
    return m_root.data();
}

QDeclarativeContext* DeclarativeWidgetView::rootContext()
{
   return m_engine.rootContext();
}

DeclarativeWidgetView::Status DeclarativeWidgetView::status() const
{
    if (!m_component)
        return DeclarativeWidgetView::Null;

    return DeclarativeWidgetView::Status(m_component->status());
}


void DeclarativeWidgetView::continueExecute()
{

    disconnect(m_component.data(), SIGNAL(statusChanged(QDeclarativeComponent::Status)), this, SLOT(continueExecute()));

    if (m_component->isError()) {
        QList<QDeclarativeError> errorList = m_component->errors();
        foreach (const QDeclarativeError &error, errorList) {
            qWarning() << error;
        }
        emit statusChanged(status());
        return;
    }

    QObject *obj = m_component->create();

    if (m_component->isError()) {
        QList<QDeclarativeError> errorList = m_component->errors();
        foreach (const QDeclarativeError &error, errorList) {
            qWarning() << error;
        }
        emit statusChanged(status());
        return;
    }

    setRootWidget(qobject_cast<QWidget *>(obj));
    emit statusChanged(status());
}

void DeclarativeWidgetView::setRootWidget(QWidget *widget)
{
    if (m_root.data() == widget)
        return;

    window()->setAttribute(Qt::WA_OpaquePaintEvent, false);
    window()->setAttribute(Qt::WA_NoSystemBackground, false);
    widget->setParent(this);
    if (isVisible())
        widget->setVisible(true);
    resize(widget->size());
    m_root.reset(widget);

    if (m_root) {
        QSize initialSize = m_root->size();
        if (initialSize != size())
            resize(initialSize);
    }
}

} //QmlDesigner
