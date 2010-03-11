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

#ifndef PROJECTMANAGER_USERFILEACCESSOR_H
#define PROJECTMANAGER_USERFILEACCESSOR_H

#include <QtCore/QVariantMap>

namespace ProjectExplorer {

class Project;

class UserFileVersionHandler
{
public:
    UserFileVersionHandler();
    virtual ~UserFileVersionHandler();

    // The user file version this handler accepts for input.
    virtual int userFileVersion() const = 0;
    virtual QString displayUserFileVersion() const = 0;
    // Update from userFileVersion() to userFileVersion() + 1
    virtual QVariantMap update(Project *project, const QVariantMap &map) = 0;

protected:
    typedef QPair<QLatin1String,QLatin1String> Change;
    QVariantMap renameKeys(const QList<Change> &changes, QVariantMap map);
};

class UserFileAccessor
{
public:
    UserFileAccessor();
    ~UserFileAccessor();
    QVariantMap restoreSettings(Project * project);
    bool saveSettings(Project * project, const QVariantMap &map);

    int latestUserFileVersion() const;

    QString mapIdTo(const QString &id, int version);

private:
    // Takes ownership of the handler!
    void addVersionHandler(UserFileVersionHandler *handler);

    QMap<int, UserFileVersionHandler *> m_handlers;
    int m_firstVersion;
    int m_lastVersion;
};

} // namespace ProjectExplorer

#endif // PROJECTMANAGER_USERFILEACCESSOR_H
