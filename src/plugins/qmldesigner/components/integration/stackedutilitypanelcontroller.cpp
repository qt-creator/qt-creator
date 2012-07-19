/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#include <QStackedWidget>

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

