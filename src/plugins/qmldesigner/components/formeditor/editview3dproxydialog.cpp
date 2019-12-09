/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "editview3dproxydialog.h"
#include "formeditorview.h"

#include <nodeinstanceview.h>

#include <coreplugin/icore.h>

#include <utils/hostosinfo.h>

#include <QApplication>
#include <QMouseEvent>
#include <QStyle>
#include <QWindow>

namespace QmlDesigner {

const int borderOffset = 8;

static int titleBarHeight() {
    return QApplication::style()->pixelMetric(QStyle::PM_TitleBarHeight);
}

EditView3DProxyDialog::EditView3DProxyDialog(FormEditorView *view) :
    QDialog(Core::ICore::dialogParent())
    ,m_formEditorView(view)
{
    setFocusPolicy(Qt::ClickFocus);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    if (Utils::HostOsInfo::isMacHost()) {
        setWindowFlag(Qt::Tool);
        setAttribute(Qt::WA_MacAlwaysShowToolWindow);
    }

    resize(1024, 768);
}

void EditView3DProxyDialog::invalidate()
{
    if (nodeInstanceView() && isVisible())
        nodeInstanceView()->show3DView(adjustedRect());
}

void EditView3DProxyDialog::moveEvent(QMoveEvent *event)
{
    if (nodeInstanceView())
        nodeInstanceView()->move3DView(pos() + QPoint(borderOffset, titleBarHeight() + 2 * borderOffset));

    QDialog::moveEvent(event);
}

void EditView3DProxyDialog::closeEvent(QCloseEvent *event)
{
    if (m_formEditorView) {
        m_formEditorView->toggle3DViewEnabled(false);
        m_formEditorView->setupOption3DAction();
    }

    nodeInstanceView()->hide3DView();

    QDialog::closeEvent(event);
}

void EditView3DProxyDialog::hideEvent(QHideEvent *event)
{
    if (m_formEditorView) {
        m_formEditorView->toggle3DViewEnabled(false);
        m_formEditorView->setupOption3DAction();
    }

    nodeInstanceView()->hide3DView();

    QDialog::hideEvent(event);
}

void EditView3DProxyDialog::focusOutEvent(QFocusEvent *event)
{
    if (isVisible())
        showView();

    QDialog::focusOutEvent(event);
}

void EditView3DProxyDialog::focusInEvent(QFocusEvent *event)
{
    showView();

    QDialog::focusInEvent(event);
}

void EditView3DProxyDialog::resizeEvent(QResizeEvent *event)
{
    if (nodeInstanceView())
        nodeInstanceView()->show3DView(adjustedRect());

    QDialog::resizeEvent(event);
}

bool EditView3DProxyDialog::event(QEvent *event)
{
    if (event->type() == QEvent::WindowActivate) {
        showView();
    } else if (event->type() == QEvent::NonClientAreaMouseButtonPress) {
        auto mouseMoveEvent = static_cast<QMouseEvent *>(event);
        if (mouseMoveEvent->buttons() & Qt::LeftButton)
            hideView();
    } else if (event->type() == QEvent::NonClientAreaMouseButtonRelease) {
        auto mouseMoveEvent = static_cast<QMouseEvent *>(event);
        if (mouseMoveEvent->buttons() & Qt::LeftButton)
            showView();
    }

    return QDialog::event(event);
}

QRect EditView3DProxyDialog::adjustedRect() const
{
    return QRect(pos(), size()).adjusted(borderOffset,
                                         titleBarHeight() + 2 * borderOffset,
                                         -borderOffset, titleBarHeight());
}

NodeInstanceView *EditView3DProxyDialog::nodeInstanceView() const
{
    if (m_formEditorView)
        return m_formEditorView->nodeInstanceView();

    return nullptr;
}

void EditView3DProxyDialog::showView()
{
    if (nodeInstanceView())
        nodeInstanceView()->show3DView(adjustedRect());
}

void EditView3DProxyDialog::hideView()
{
    if (nodeInstanceView())
        nodeInstanceView()->hide3DView();
}

} //QmlDesigner
