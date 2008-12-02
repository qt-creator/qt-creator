/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/
***************************************************************************/
#include "gitsubmiteditorwidget.h"
#include "commitdata.h"

namespace Git {
namespace Internal {

GitSubmitEditorWidget::GitSubmitEditorWidget(QWidget *parent) :
    Core::Utils::SubmitEditorWidget(parent),
    m_gitSubmitPanel(new QWidget)
{
    m_gitSubmitPanelUi.setupUi(m_gitSubmitPanel);
    insertTopWidget(m_gitSubmitPanel);
}

void GitSubmitEditorWidget::setPanelInfo(const GitSubmitEditorPanelInfo &info)
{
    m_gitSubmitPanelUi.repositoryLabel->setText(info.repository);
    m_gitSubmitPanelUi.descriptionLabel->setText(info.description);
    m_gitSubmitPanelUi.branchLabel->setText(info.branch);
}

GitSubmitEditorPanelData GitSubmitEditorWidget::panelData() const
{
    GitSubmitEditorPanelData rc;
    rc.author = m_gitSubmitPanelUi.authorLineEdit->text();
    rc.email = m_gitSubmitPanelUi.emailLineEdit->text();
    return rc;
};

void GitSubmitEditorWidget::setPanelData(const  GitSubmitEditorPanelData &data)
{
    m_gitSubmitPanelUi.authorLineEdit->setText(data.author);
    m_gitSubmitPanelUi.emailLineEdit->setText(data.email);
}

}
}
