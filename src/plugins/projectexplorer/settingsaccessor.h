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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef PROJECTMANAGER_USERFILEACCESSOR_H
#define PROJECTMANAGER_USERFILEACCESSOR_H

#include <QtCore/QVariantMap>


namespace ProjectExplorer {

class Project;

namespace Internal {
class UserFileVersionHandler;
}

/*
  The SettingsAcessor

  The SettingsAcessor currently persists a user-specific file and a shared file. By default
  settings are user-specific and the shared ones are identified through the key
  SHARED_SETTINGS_KEYS_KEY in the map. The corresponding value for this key must be a
  QStringList containing the keys from this particular map that should be shared. As an aid,
  if the QStringList contains a unique item ALL_SETTINGS_KEYS_KEY all keys from the map
  are considered shared.

  While we are very strict regarding versions from the user settings (we create backups for
  old versions, always try to find the correct file, etc), this is not the same for the shared
  files. When considering many usability aspects and issues like having them in repositories
  (and unintenional overwrites when pulling), simply passing them around, matching their
  version against the user version, it seems that the more simplistic approach would be the
  best (at least for now).

  The idea is that shared settings are never removed from the user settings file, they work
  as a backup when the different "synchronization" issues arise. We then make sure that
  whenever settings are restored we start with the user ones and eventually merge the shared
  ones into the final map, replacing the corresponding ones in the user map (so the shared
  ones take precedences). An analoguous behavior happens when saving the settings.

  The main benefit of this approach is that we always try to use the shared settings. When
  possible we run the version update handlers and make the shared settings compatible with
  the user settings (or vice-versa). From that on it's transparent for handlers to update
  the settings to the current Creator version (if it's the case) since it's just one map.

 */
class SettingsAccessor
{
public:
    ~SettingsAccessor();

    static SettingsAccessor *instance();

    QVariantMap restoreSettings(Project *project) const;
    bool saveSettings(const Project *project, const QVariantMap &map) const;

private:
    SettingsAccessor();

    // Takes ownership of the handler!
    void addVersionHandler(Internal::UserFileVersionHandler *handler);

    // The relevant data from the settings currently in use.
    class SettingsData
    {
    public:
        SettingsData() : m_version(-1), m_usingBackup(false) {}
        SettingsData(const QVariantMap &map) : m_version(-1), m_usingBackup(false), m_map(map) {}

        int m_version;
        bool m_usingBackup;
        QVariantMap m_map;
        QString m_fileName;
    };

    // The entity which actually reads/writes to the settings file.
    class FileAccessor
    {
    public:
        FileAccessor(const QByteArray &id,
                     const QString &defaultSuffix,
                     const QString &environmentSuffix,
                     bool envSpecific,
                     bool versionStrict);

        bool readFile(Project *project, SettingsData *settings) const;
        bool writeFile(const Project *project, const SettingsData *settings) const;

    private:
        void assignSuffix(const QString &defaultSuffix, const QString &environmentSuffix);
        QString assembleFileName(const Project *project) const;
        bool findNewestCompatibleSetting(SettingsData *settings) const;

        QByteArray m_id;
        QString m_suffix;
        bool m_environmentSpecific;
        bool m_versionStrict;
    };

    static bool verifyEnvironmentId(const QString &id);

    QMap<int, Internal::UserFileVersionHandler *> m_handlers;
    int m_firstVersion;
    int m_lastVersion;
    const FileAccessor m_userFileAcessor;
    const FileAccessor m_sharedFileAcessor;
};

} // namespace ProjectExplorer

#endif // PROJECTMANAGER_USERFILEACCESSOR_H
