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

#include "widgethost.h"
#include "formresizer.h"
#include "widgethostconstants.h"

#include <QDesignerFormWindowInterface>
#include <QDesignerFormWindowCursorInterface>

#include <QPalette>
#include <QLayout>
#include <QFrame>
#include <QResizeEvent>
#include <QDebug>

using namespace SharedTools;

// ---------- WidgetHost
WidgetHost::WidgetHost(QWidget *parent, QDesignerFormWindowInterface *formWindow) :
    QScrollArea(parent),
    m_formWindow(0),
    m_formResizer(new Internal::FormResizer)
{
    setWidget(m_formResizer);
    // Re-set flag (gets cleared by QScrollArea): Make the resize grip of a mainwindow form find the resizer as resizable window.
    m_formResizer->setWindowFlags(m_formResizer->windowFlags() | Qt::SubWindow);
    setFormWindow(formWindow);
}

WidgetHost::~WidgetHost()
{
    if (m_formWindow)
        delete m_formWindow;
}

void WidgetHost::setFormWindow(QDesignerFormWindowInterface *fw)
{
    m_formWindow = fw;
    if (!fw)
        return;

    m_formResizer->setFormWindow(fw);

    setBackgroundRole(QPalette::Base);
    m_formWindow->setAutoFillBackground(true);
    m_formWindow->setBackgroundRole(QPalette::Background);

    connect(m_formResizer, SIGNAL(formWindowSizeChanged(QRect,QRect)),
            this, SLOT(fwSizeWasChanged(QRect,QRect)));
}

QSize WidgetHost::formWindowSize() const
{
    if (!m_formWindow || !m_formWindow->mainContainer())
        return QSize();
    return m_formWindow->mainContainer()->size();
}

void WidgetHost::fwSizeWasChanged(const QRect &, const QRect &)
{
    // newGeo is the mouse coordinates, thus moving the Right will actually emit wrong height
    emit formWindowSizeChanged(formWindowSize().width(), formWindowSize().height());
}

void WidgetHost::updateFormWindowSelectionHandles(bool active)
{
    Internal::SelectionHandleState state = Internal::SelectionHandleOff;
    const QDesignerFormWindowCursorInterface *cursor = m_formWindow->cursor();
    if (cursor->isWidgetSelected(m_formWindow->mainContainer()))
        state = active ? Internal::SelectionHandleActive :  Internal::SelectionHandleInactive;

    m_formResizer->setState(state);
}

QWidget *WidgetHost::integrationContainer() const
{
    return m_formResizer;
}
