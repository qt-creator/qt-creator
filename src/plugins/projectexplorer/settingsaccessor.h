/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef SETTINGSACCESSOR_H
#define SETTINGSACCESSOR_H

#include <utils/fileutils.h>

#include <QVariantMap>

namespace Utils { class PersistentSettingsWriter; }

namespace ProjectExplorer {

class Project;

namespace Internal { class VersionUpgrader; }

class SettingsAccessorPrivate;

class SettingsAccessor
{
public:
    SettingsAccessor(Project *project);
    virtual ~SettingsAccessor();

    Project *project() const;

    QVariantMap restoreSettings(QWidget *parent) const;
    bool saveSettings(const QVariantMap &data, QWidget *parent) const;

    static QVariantMap setVersionInMap(const QVariantMap &data, int version);
    static int versionFromMap(const QVariantMap &data);
    static int originalVersionFromMap(const QVariantMap &data);
    static QVariantMap setOriginalVersionInMap(const QVariantMap &data, int version);

    void addVersionUpgrader(Internal::VersionUpgrader *handler); // Takes ownership of the handler!
private:
    QStringList findSettingsFiles(const QString &suffix) const;
    static QByteArray creatorId();
    QString defaultFileName(const QString &suffix) const;
    int currentVersion() const;
    void backupUserFile() const;

    // The relevant data from the settings currently in use.
    class SettingsData
    {
    public:
        SettingsData() {}
        SettingsData(const QVariantMap &map) : m_map(map) {}

        void clear();
        bool isValid() const;
        Utils::FileName fileName() const { return m_fileName; }

        QVariantMap m_map;
        Utils::FileName m_fileName;
    };

    void upgradeSettings(SettingsData &data, int toVersion) const;

    SettingsData readUserSettings(QWidget *parent) const;
    SettingsData readSharedSettings(QWidget *parent) const;
    SettingsData findBestSettings(const QStringList &candidates) const;
    SettingsData mergeSettings(const SettingsData &user, const SettingsData &shared) const;

    bool readFile(SettingsData *settings) const;
    bool writeFile(const SettingsData *settings, QWidget *parent) const;

    QByteArray environmentIdFromMap(const QVariantMap &data) const;

    int m_firstVersion;
    int m_lastVersion;
    QString m_userSuffix;
    QString m_sharedSuffix;

    Project *m_project;

    SettingsAccessorPrivate *d;

    friend class SettingsAccessorPrivate;
};

namespace Internal {
class UserFileAccessor : public SettingsAccessor
{
public:
    UserFileAccessor(Project *project);
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // SETTINGSACCESSOR_H
