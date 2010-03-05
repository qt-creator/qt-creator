/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include <QtGui/QStackedWidget>

#include "designdocumentcontroller.h"
#include "stackedutilitypanelcontroller.h"

namespace QmlDesigner {

StackedUtilityPanelController::StackedUtilityPanelController(QObject* parent):
        UtilityPanelController(parent),
        m_stackedWidget(new QStackedWidget)
{
    m_stackedWidget->setLineWidth(0);
    m_stackedWidget->setMidLineWidth(0);
    m_stackedWidget->setFrameStyle(QFrame::NoFrame);
}

void StackedUtilityPanelController::show(DesignDocumentController* designDocumentController)
{
    if (!designDocumentController)
        return;

    QWidget* page = stackedPageWidget(designDocumentController);

    if (!m_stackedWidget->children().contains(page))
        m_stackedWidget->addWidget(page);

    m_stackedWidget->setCurrentWidget(page);
    page->show();
}

void StackedUtilityPanelController::close(DesignDocumentController* designDocumentController)
{
    QWidget* page = stackedPageWidget(designDocumentController);

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

