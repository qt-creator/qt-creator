/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#include "quick2propertyeditorview.h"

#include "propertyeditorvalue.h"
#include "fileresourcesmodel.h"
#include "gradientmodel.h"
#include "qmlanchorbindingproxy.h"

#include <QVBoxLayout>

namespace QmlDesigner {

void Quick2PropertyEditorView::execute()
{
    m_view.setSource(m_source);

    if (!m_source.isEmpty()) {
        m_view.setSource(m_source);
        connect(&m_view, SIGNAL(statusChanged(QQuickView::Status)), this, SLOT(continueExecute()));
    }
}

Quick2PropertyEditorView::Quick2PropertyEditorView(QWidget *parent) :
    QWidget(parent)
{
    m_containerWidget = createWindowContainer(&m_view);

    QVBoxLayout *layout = new QVBoxLayout(this);
    setLayout(layout);
    layout->addWidget(m_containerWidget);
    layout->setMargin(0);
    m_view.setResizeMode(QQuickView::SizeRootObjectToView);
}

QUrl Quick2PropertyEditorView::source() const
{
    return m_source;
}

void Quick2PropertyEditorView::setSource(const QUrl& url)
{
    m_source = url;
    execute();
}

QQmlEngine* Quick2PropertyEditorView::engine()
{
   return m_view.engine();
}

QQmlContext* Quick2PropertyEditorView::rootContext()
{
   return engine()->rootContext();
}

Quick2PropertyEditorView::Status Quick2PropertyEditorView::status() const
{
    return Quick2PropertyEditorView::Status(m_view.status());
}


void Quick2PropertyEditorView::continueExecute()
{
    disconnect(&m_view, SIGNAL(statusChanged(QQuickView::Status)), this, SLOT(continueExecute()));

    if (!m_view.errors().isEmpty()) {
        QList<QQmlError> errorList = m_view.errors();
        foreach (const QQmlError &error, errorList) {
            qWarning() << error;
        }
        emit statusChanged(status());
        return;
    }

    emit statusChanged(status());
}

void Quick2PropertyEditorView::registerQmlTypes()
{
    static bool declarativeTypesRegistered = false;
    if (!declarativeTypesRegistered) {
        declarativeTypesRegistered = true;
        PropertyEditorValue::registerDeclarativeTypes();
        FileResourcesModel::registerDeclarativeType();
        GradientModel::registerDeclarativeType();
        Internal::QmlAnchorBindingProxy::registerDeclarativeType();
    }
}

} //QmlDesigner
