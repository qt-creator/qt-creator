// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "stackedutilitypanelcontroller.h"

#include <QStackedWidget>


namespace QmlDesigner {

StackedUtilityPanelController::StackedUtilityPanelController(QObject* parent):
        UtilityPanelController(parent),
        m_stackedWidget(new QStackedWidget)
{
    m_stackedWidget->setLineWidth(0);
    m_stackedWidget->setMidLineWidth(0);
    m_stackedWidget->setFrameStyle(QFrame::NoFrame);
}

void StackedUtilityPanelController::show(DesignDocument* DesignDocument)
{
    if (!DesignDocument)
        return;

    QWidget* page = stackedPageWidget(DesignDocument);

    if (!m_stackedWidget->children().contains(page))
        m_stackedWidget->addWidget(page);

    m_stackedWidget->setCurrentWidget(page);
    page->show();
}

void StackedUtilityPanelController::close(DesignDocument* DesignDocument)
{
    QWidget* page = stackedPageWidget(DesignDocument);

    if (m_stackedWidget->children().contains(page)) {
        page->hide();
        m_stackedWidget->removeWidget(page);
    }
}

QWidget* StackedUtilityPanelController::contentWidget() const
{
    return m_stackedWidget;
}

}  // namespace QmlDesigner

