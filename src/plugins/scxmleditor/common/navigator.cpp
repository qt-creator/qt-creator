// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "graphicsscene.h"
#include "graphicsview.h"
#include "navigator.h"
#include "navigatorgraphicsview.h"
#include "navigatorslider.h"
#include "scxmleditortr.h"
#include "sizegrip.h"

#include <utils/utilsicons.h>

#include <QLabel>
#include <QResizeEvent>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>

using namespace ScxmlEditor::Common;

Navigator::Navigator(QWidget *parent)
    : MovableFrame(parent)
{
    createUi();
    connect(m_closeButton, &QToolButton::clicked, this, &Navigator::hideFrame);
}

void Navigator::setCurrentView(GraphicsView *view)
{
    if (m_currentView) {
        m_currentView->disconnect(m_navigatorView);
        m_navigatorView->disconnect(m_currentView);
        m_currentView->disconnect(m_navigatorSlider);
        m_navigatorSlider->disconnect(m_currentView);
    }

    m_currentView = view;

    if (m_currentView) {
        connect(m_currentView.data(), &GraphicsView::viewChanged, m_navigatorView, &NavigatorGraphicsView::setMainViewPolygon);
        connect(m_currentView.data(), &GraphicsView::zoomPercentChanged, m_navigatorSlider, &NavigatorSlider::setSliderValue);

        connect(m_navigatorSlider, &NavigatorSlider::valueChanged, m_currentView.data(), &GraphicsView::zoomTo);
        connect(m_navigatorView, &NavigatorGraphicsView::moveMainViewTo, m_currentView.data(), &GraphicsView::moveToPoint);
        connect(m_navigatorView, &NavigatorGraphicsView::zoomIn, m_currentView.data(), &GraphicsView::zoomIn);
        connect(m_navigatorView, &NavigatorGraphicsView::zoomOut, m_currentView.data(), &GraphicsView::zoomOut);
    }
}

void Navigator::setCurrentScene(ScxmlEditor::PluginInterface::GraphicsScene *scene)
{
    m_navigatorView->setGraphicsScene(scene);
}

void Navigator::resizeEvent(QResizeEvent *e)
{
    MovableFrame::resizeEvent(e);
    m_sizeGrip->move(e->size().width() - m_sizeGrip->width(), e->size().height() - m_sizeGrip->height());
}

void Navigator::createUi()
{
    auto titleLabel = new QLabel(Tr::tr("Navigator"));
    titleLabel->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);

    m_closeButton = new QToolButton;
    m_closeButton->setIcon(Utils::Icons::CLOSE_TOOLBAR.icon());

    auto titleToolBar = new QToolBar;
    titleToolBar->addWidget(titleLabel);
    titleToolBar->addWidget(m_closeButton);

    m_navigatorView = new NavigatorGraphicsView;
    m_navigatorSlider = new NavigatorSlider;

    setLayout(new QVBoxLayout);
    layout()->setSpacing(0);
    layout()->setContentsMargins(0, 0, 0, 0);
    layout()->addWidget(titleToolBar);
    layout()->addWidget(m_navigatorView);
    layout()->addWidget(m_navigatorSlider);

    m_sizeGrip = new SizeGrip(this);
    m_sizeGrip->setGeometry(0, 0, 18, 18);

    setAutoFillBackground(true);
    setMinimumSize(300, 200);
    setGeometry(x(), y(), 400, 300);
}
