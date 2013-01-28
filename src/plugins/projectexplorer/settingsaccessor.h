/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#ifndef PROJECTMANAGER_USERFILEACCESSOR_H
#define PROJECTMANAGER_USERFILEACCESSOR_H

#include <utils/fileutils.h>
#include <utils/persistentsettings.h>

namespace ProjectExplorer {

class Project;

namespace Internal {
class UserFileVersionHandler;
}

class SettingsAccessor
{
public:
    SettingsAccessor(Project *project);
    ~SettingsAccessor();

    Project *project() const;

    QVariantMap restoreSettings() const;
    bool saveSettings(const QVariantMap &map) const;

private:

    // Takes ownership of the handler!
    void addVersionHandler(Internal::UserFileVersionHandler *handler);

    // The relevant data from the settings currently in use.
    class SettingsData
    {
    public:
        SettingsData() : m_version(-1), m_usingBackup(false) {}
        SettingsData(const QVariantMap &map) : m_version(-1), m_usingBackup(false), m_map(map) {}

        void clear();
        bool isValid() const;

        int m_version;
        bool m_usingBackup;
        QVariantMap m_map;
        Utils::FileName m_fileName;
    };

    // The entity which actually reads/writes to the settings file.
    class FileAccessor
    {
    public:
        FileAccessor(const QByteArray &id,
                     const QString &defaultSuffix,
                     const QString &environmentSuffix,
                     bool envSpecific,
                     bool versionStrict,
                     SettingsAccessor *accessor);
        ~FileAccessor();

        bool readFile(SettingsData *settings) const;
        bool writeFile(const SettingsData *settings) const;

        QString suffix() const { return m_suffix; }
        QByteArray id() const { return m_id; }

    private:
        void assignSuffix(const QString &defaultSuffix, const QString &environmentSuffix);
        QString assembleFileName(const Project *project) const;
        bool findNewestCompatibleSetting(SettingsData *settings) const;

        QByteArray m_id;
        QString m_suffix;
        bool m_environmentSpecific;
        bool m_versionStrict;
        SettingsAccessor *m_accessor;
        mutable Utils::PersistentSettingsWriter *m_writer;
    };

    static bool verifyEnvironmentId(const QString &id);

    QMap<int, Internal::UserFileVersionHandler *> m_handlers;
    int m_firstVersion;
    int m_lastVersion;
    const FileAccessor m_userFileAcessor;
    const FileAccessor m_sharedFileAcessor;

    Project *m_project;
};

} // namespace ProjectExplorer

#endif // PROJECTMANAGER_USERFILEACCESSOR_H
