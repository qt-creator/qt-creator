/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
****************************************************************************/

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

