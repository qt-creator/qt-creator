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

#include "projectwelcomepage.h"
#include "projectwelcomepagewidget.h"

namespace ProjectExplorer {
namespace Internal {

ProjectWelcomePage::ProjectWelcomePage()
    : m_page(0)
{
}

QWidget *ProjectWelcomePage::page()
{
    if (!m_page) {
        m_page = new ProjectWelcomePageWidget;

        // Forward signals
        connect(m_page, SIGNAL(requestProject(QString)), this, SIGNAL(requestProject(QString)));
        connect(m_page, SIGNAL(requestSession(QString)), this, SIGNAL(requestSession(QString)));
        connect(m_page, SIGNAL(manageSessions()), this, SIGNAL(manageSessions()));

        m_page->updateWelcomePage(m_welcomePageData);
    }
    return m_page;
}

void ProjectWelcomePage::setWelcomePageData(const ProjectWelcomePageWidget::WelcomePageData &welcomePageData)
{
    m_welcomePageData = welcomePageData;

    if (m_page)
        m_page->updateWelcomePage(welcomePageData);
}

} // namespace Internal
} // namespace ProjectExplorer
