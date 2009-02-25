/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

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
