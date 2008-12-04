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

#include "gitversioncontrol.h"
#include "gitclient.h"

namespace Git {
namespace Internal {

GitVersionControl::GitVersionControl(GitClient *client) :
    m_enabled(true),
    m_client(client)
{
}

QString GitVersionControl::name() const
{
    return QLatin1String("git");
}

bool GitVersionControl::isEnabled() const
{
    return m_enabled;
}

void GitVersionControl::setEnabled(bool enabled)
{
    if (m_enabled != enabled) {
        m_enabled = enabled;
        emit enabledChanged(m_enabled);
    }
}

bool GitVersionControl::supportsOperation(Operation operation) const
{
    bool rc = false;
    switch (operation) {
    case AddOperation:
    case DeleteOperation:
    case OpenOperation:
        break;
    }
    return rc;
}

bool GitVersionControl::vcsOpen(const QString & /*fileName*/)
{
    return false;
}

bool GitVersionControl::vcsAdd(const QString & /*fileName*/)
{
    return false;
}

bool GitVersionControl::vcsDelete(const QString & /*fileName*/)
{
    // TODO: implement using 'git rm'.
    return false;
}

bool GitVersionControl::managesDirectory(const QString &directory) const
{
    return !GitClient::findRepositoryForDirectory(directory).isEmpty();

}

QString GitVersionControl::findTopLevelForDirectory(const QString &directory) const
{
    return GitClient::findRepositoryForDirectory(directory);
}

} // Internal
} // Git
