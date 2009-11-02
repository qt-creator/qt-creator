/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Brian McGillion
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

#include "mercurialcontrol.h"
#include "mercurialclient.h"

#include <QtCore/QFileInfo>

using namespace Mercurial::Internal;

MercurialControl::MercurialControl(MercurialClient *client)
        :   mercurialClient(client),
            mercurialEnabled(true)
{
}

QString MercurialControl::name() const
{
    return tr("Mercurial");
}

bool MercurialControl::isEnabled() const
{
    return mercurialEnabled;
}

void MercurialControl::setEnabled(bool enabled)
{
    if (mercurialEnabled != enabled) {
        mercurialEnabled = enabled;
        emit enabledChanged(mercurialEnabled);
    }
}

bool MercurialControl::managesDirectory(const QString &directory) const
{
    QFileInfo dir(directory);
    return !mercurialClient->findTopLevelForFile(dir).isEmpty();
}

QString MercurialControl::findTopLevelForDirectory(const QString &directory) const
{
    QFileInfo dir(directory);
    return mercurialClient->findTopLevelForFile(dir);
}

bool MercurialControl::supportsOperation(Operation operation) const
{
    bool supported = true;

    switch (operation) {
    case Core::IVersionControl::AddOperation:
    case Core::IVersionControl::DeleteOperation:
        break;
    case Core::IVersionControl::OpenOperation:
    default:
        supported = false;
        break;
    }

    return supported;
}

bool MercurialControl::vcsOpen(const QString &filename)
{
    Q_UNUSED(filename)
    return true;
}

bool MercurialControl::vcsAdd(const QString &filename)
{
    return mercurialClient->add(filename);
}

bool MercurialControl::vcsDelete(const QString &filename)
{
    return mercurialClient->remove(filename);
}

bool MercurialControl::sccManaged(const QString &filename)
{
    return mercurialClient->manifestSync(filename);
}
