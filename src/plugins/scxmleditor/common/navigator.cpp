/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "navigator.h"
#include "graphicsscene.h"
#include "graphicsview.h"
#include "sizegrip.h"

#include <QResizeEvent>

using namespace ScxmlEditor::Common;

Navigator::Navigator(QWidget *parent)
    : MovableFrame(parent)
{
    m_ui.setupUi(this);
    connect(m_ui.m_closeButton, &QToolButton::clicked, this, &Navigator::hideFrame);
    m_sizeGrip = new SizeGrip(this);
    m_sizeGrip->setGeometry(0, 0, 18, 18);
}

void Navigator::setCurrentView(GraphicsView *view)
{
    if (m_currentView) {
        m_currentView->disconnect(m_ui.m_navigatorView);
        m_ui.m_navigatorView->disconnect(m_currentView);
        m_currentView->disconnect(m_ui.m_navigatorSlider);
        m_ui.m_navigatorSlider->disconnect(m_currentView);
    }

    m_currentView = view;

    if (m_currentView) {
        connect(m_currentView.data(), &GraphicsView::viewChanged, m_ui.m_navigatorView, &NavigatorGraphicsView::setMainViewPolygon);
        connect(m_currentView.data(), &GraphicsView::zoomPercentChanged, m_ui.m_navigatorSlider, &NavigatorSlider::setSliderValue);

        connect(m_ui.m_navigatorSlider, &NavigatorSlider::valueChanged, m_currentView.data(), &GraphicsView::zoomTo);
        connect(m_ui.m_navigatorView, &NavigatorGraphicsView::moveMainViewTo, m_currentView.data(), &GraphicsView::moveToPoint);
        connect(m_ui.m_navigatorView, &NavigatorGraphicsView::zoomIn, m_currentView.data(), &GraphicsView::zoomIn);
        connect(m_ui.m_navigatorView, &NavigatorGraphicsView::zoomOut, m_currentView.data(), &GraphicsView::zoomOut);
    }
}

void Navigator::setCurrentScene(ScxmlEditor::PluginInterface::GraphicsScene *scene)
{
    m_ui.m_navigatorView->setGraphicsScene(scene);
}

void Navigator::resizeEvent(QResizeEvent *e)
{
    MovableFrame::resizeEvent(e);
    m_sizeGrip->move(e->size().width() - m_sizeGrip->width(), e->size().height() - m_sizeGrip->height());
}
