/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
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
