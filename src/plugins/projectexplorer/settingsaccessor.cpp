/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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

#include "settingsaccessor.h"

#include "buildconfiguration.h"
#include "project.h"
#include "projectexplorer.h"
#include "projectexplorersettings.h"
#include "projectexplorerconstants.h"
#include "target.h"
#include "profile.h"
#include "profilemanager.h"

#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/persistentsettings.h>

#include <QApplication>
#include <QDebug>
#include <QFile>
#include <QMainWindow>
#include <QMessageBox>

namespace ProjectExplorer {
namespace Internal {

// -------------------------------------------------------------------------
// UserFileVersionHandler
// -------------------------------------------------------------------------
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

UserFileVersionHandler::UserFileVersionHandler()
{
}

UserFileVersionHandler::~UserFileVersionHandler()
{
}

/**
 * Performs a simple renaming of the listed keys in \a changes recursively on \a map.
 */
QVariantMap UserFileVersionHandler::renameKeys(const QList<Change> &changes, QVariantMap map)
{
    foreach (const Change &change, changes) {
        QVariantMap::iterator oldSetting = map.find(change.first);
        if (oldSetting != map.end()) {
            map.insert(change.second, oldSetting.value());
            map.erase(oldSetting);
        }
    }

    QVariantMap::iterator i = map.begin();
    while (i != map.end()) {
        QVariant v = i.value();
        if (v.type() == QVariant::Map)
            i.value() = renameKeys(changes, v.toMap());

        ++i;
    }

    return map;
}

} // Internal
} // ProjectExplorer


using namespace ProjectExplorer;
using namespace Internal;
using Utils::PersistentSettingsReader;
using Utils::PersistentSettingsWriter;

namespace {

const char VERSION_KEY[] = "ProjectExplorer.Project.Updater.FileVersion";
const char ENVIRONMENT_ID_KEY[] = "ProjectExplorer.Project.Updater.EnvironmentId";
const char USER_STICKY_KEYS_KEY[] = "ProjectExplorer.Project.UserStickyKeys";

const char SHARED_SETTINGS[] = "SharedSettings";

// Version 0 is used in Qt Creator 1.3.x and
// (in a slighly different flavour) post 1.3 master.
class Version0Handler : public UserFileVersionHandler
{
public:
    int userFileVersion() const
    {
        return 0;
    }

    QString displayUserFileVersion() const
    {
        return QLatin1String("1.3");
    }

    QVariantMap update(Project *project, const QVariantMap &map);

private:
    QVariantMap convertBuildConfigurations(Project *project, const QVariantMap &map);
    QVariantMap convertRunConfigurations(Project *project, const QVariantMap &map);
    QVariantMap convertBuildSteps(Project *project, const QVariantMap &map);
};

// Version 1 is used in master post Qt Creator 1.3.x.
// It was never used in any official release but is required for the
// transition to later versions (which introduce support for targets).
class Version1Handler : public UserFileVersionHandler
{
public:
    int userFileVersion() const
    {
        return 1;
    }

    QString displayUserFileVersion() const
    {
        return QLatin1String("1.3+git");
    }

    QVariantMap update(Project *project, const QVariantMap &map);

private:
    struct TargetDescription
    {
        TargetDescription(QString tid, QString dn) :
            id(tid),
            displayName(dn)
        {
        }

        TargetDescription(const TargetDescription &td) :
            id(td.id),
            displayName(td.displayName)
        {
        }

        QString id;
        QString displayName;
    };
};

// Version 2 is used in master post Qt Creator 2.0 alpha.
class Version2Handler : public UserFileVersionHandler
{
public:
    int userFileVersion() const
    {
        return 2;
    }

    QString displayUserFileVersion() const
    {
        return QLatin1String("2.0-alpha+git");
    }

    QVariantMap update(Project *project, const QVariantMap &map);
};

// Version 3 reflect the move of symbian signing from run to build step.
class Version3Handler : public UserFileVersionHandler
{
public:
    int userFileVersion() const
    {
        return 3;
    }

    QString displayUserFileVersion() const
    {
        return QLatin1String("2.0-alpha2+git");
    }

    QVariantMap update(Project *project, const QVariantMap &map);
};

// Version 4 reflects the introduction of deploy steps
class Version4Handler : public UserFileVersionHandler
{
public:
    int userFileVersion() const
    {
        return 4;
    }

    QString displayUserFileVersion() const
    {
        return QLatin1String("2.1pre1");
    }

    QVariantMap update(Project *project, const QVariantMap &map);
};

// Version 5 reflects the introduction of new deploy steps for Symbian/Maemo
class Version5Handler : public UserFileVersionHandler
{
public:
    int userFileVersion() const
    {
        return 5;
    }

    QString displayUserFileVersion() const
    {
        return QLatin1String("2.1pre2");
    }

    QVariantMap update(Project *project, const QVariantMap &map);
};

// Version 6 reflects the introduction of new deploy steps for Symbian/Maemo
class Version6Handler : public UserFileVersionHandler
{
public:
    int userFileVersion() const
    {
        return 6;
    }

    QString displayUserFileVersion() const
    {
        return QLatin1String("2.1pre3");
    }

    QVariantMap update(Project *project, const QVariantMap &map);
};

// Version 7 reflects the introduction of new deploy configuration for Symbian
class Version7Handler : public UserFileVersionHandler
{
public:
    int userFileVersion() const
    {
        return 7;
    }

    QString displayUserFileVersion() const
    {
        return QLatin1String("2.1pre4");
    }

    QVariantMap update(Project *project, const QVariantMap &map);
};

// Version 8 reflects the change of environment variable expansion rules,
// turning some env variables into expandos, the change of argument quoting rules,
// and the change of VariableManager's expansion syntax.
class Version8Handler : public UserFileVersionHandler
{
public:
    int userFileVersion() const
    {
        return 8;
    }

    QString displayUserFileVersion() const
    {
        // pre5 because we renamed 2.2 to 2.1 later, so people already have 2.2pre4 files
        return QLatin1String("2.2pre5");
    }

    QVariantMap update(Project *project, const QVariantMap &map);
};

// Version 9 reflects the refactoring of the Maemo deploy step.
class Version9Handler : public UserFileVersionHandler
{
public:
    int userFileVersion() const
    {
        return 9;
    }

    QString displayUserFileVersion() const
    {
        return QLatin1String("2.3pre1");
    }

    QVariantMap update(Project *project, const QVariantMap &map);
};

// Version 10 introduces disabling buildsteps, and handles upgrading custom process steps
class Version10Handler : public UserFileVersionHandler
{
public:
    int userFileVersion() const
    {
        return 10;
    }

    QString displayUserFileVersion() const
    {
        return QLatin1String("2.5pre1");
    }

    QVariantMap update(Project *project, const QVariantMap &map);
};

// Version 10 introduces disabling buildsteps, and handles upgrading custom process steps
class Version11Handler : public UserFileVersionHandler
{
public:
    Version11Handler();
    ~Version11Handler();

    int userFileVersion() const
    {
        return 11;
    }

    QString displayUserFileVersion() const
    {
        return QLatin1String("2.6pre1");
    }

    QVariantMap update(Project *project, const QVariantMap &map);

private:
    void addBuildConfiguration(const QString &origTarget, Profile *p,
                        bool targetActive,
                        const QVariantMap &bc, bool bcActive);
    void addOtherConfiguration(const QString &origTarget,
                               const QList<QVariantMap> &dcs, int activeDc,
                               const QList<QVariantMap> &rcs, int activeRc);

    void parseQtversionFile();
    void parseToolChainFile();

    class ToolChainExtraData {
    public:
        explicit ToolChainExtraData(const QString &mks = QString(), const QString &d = QString()) :
            m_mkspec(mks), m_debugger(d)
        { }

        QString m_mkspec;
        QString m_debugger;
    };

    QHash<QString, ToolChainExtraData> m_toolChainExtras;
    QHash<int, QString> m_qtVersionExtras;

    QHash<Profile *, QVariantMap> m_targets;
};

} // namespace

//
// Helper functions:
//

QT_BEGIN_NAMESPACE

class HandlerNode
{
public:
    QSet<QString> strings;
    QHash<QString, HandlerNode> children;
};

Q_DECLARE_TYPEINFO(HandlerNode, Q_MOVABLE_TYPE);

QT_END_NAMESPACE

static HandlerNode buildHandlerNodes(const char * const **keys)
{
    HandlerNode ret;
    while (const char *rname = *(*keys)++) {
        QString name = QLatin1String(rname);
        if (name.endsWith(QLatin1Char('.'))) {
            HandlerNode sub = buildHandlerNodes(keys);
            foreach (const QString &key, name.split(QLatin1Char('|')))
                ret.children.insert(key, sub);
        } else {
            ret.strings.insert(name);
        }
    }
    return ret;
}

static QVariantMap processHandlerNodes(const HandlerNode &node, const QVariantMap &map,
                                       QVariant (*handler)(const QVariant &var))
{
    QVariantMap result;
    QMapIterator<QString, QVariant> it(map);
    while (it.hasNext()) {
        it.next();
        const QString &key = it.key();
        if (node.strings.contains(key)) {
            result.insert(key, handler(it.value()));
            goto handled;
        }
        if (it.value().type() == QVariant::Map)
            for (QHash<QString, HandlerNode>::ConstIterator subit = node.children.constBegin();
                 subit != node.children.constEnd(); ++subit)
                if (key.startsWith(subit.key())) {
                    result.insert(key, processHandlerNodes(subit.value(), it.value().toMap(), handler));
                    goto handled;
                }
        result.insert(key, it.value());
      handled: ;
    }
    return result;
}

// -------------------------------------------------------------------------
// UserFileAccessor
// -------------------------------------------------------------------------
SettingsAccessor::SettingsAccessor() :
    m_firstVersion(-1),
    m_lastVersion(-1),
    m_userFileAcessor(QByteArray("qtcUserFileName"),
                      QLatin1String(".user"),
                      QString::fromLocal8Bit(qgetenv("QTC_EXTENSION")),
                      true,
                      true),
    m_sharedFileAcessor(QByteArray("qtcSharedFileName"),
                        QLatin1String(".shared"),
                        QString::fromLocal8Bit(qgetenv("QTC_SHARED_EXTENSION")),
                        false,
                        false)
{
    addVersionHandler(new Version0Handler);
    addVersionHandler(new Version1Handler);
    addVersionHandler(new Version2Handler);
    addVersionHandler(new Version3Handler);
    addVersionHandler(new Version4Handler);
    addVersionHandler(new Version5Handler);
    addVersionHandler(new Version6Handler);
    addVersionHandler(new Version7Handler);
    addVersionHandler(new Version8Handler);
    addVersionHandler(new Version9Handler);
    addVersionHandler(new Version10Handler);
    addVersionHandler(new Version11Handler);
}

SettingsAccessor::~SettingsAccessor()
{
    qDeleteAll(m_handlers);
}

SettingsAccessor *SettingsAccessor::instance()
{
    static SettingsAccessor acessor;
    return &acessor;
}

namespace {

// It's assumed that the shared map has the same structure as the user map.
template <class Operation_T>
void synchronizeSettings(QVariantMap *userMap,
                         const QVariantMap &sharedMap,
                         Operation_T *op)
{
    QVariantMap::const_iterator it = sharedMap.begin();
    QVariantMap::const_iterator eit = sharedMap.end();
    for (; it != eit; ++it) {
        const QString &key = it.key();
        const QVariant &sharedValue = it.value();
        const QVariant &userValue = userMap->value(key);
        if (sharedValue.type() == QVariant::Map) {
            if (userValue.type() != QVariant::Map) {
                // This should happen only if the user manually changed the file in such a way.
                continue;
            }
            QVariantMap nestedUserMap = userValue.toMap();
            synchronizeSettings(&nestedUserMap,
                                sharedValue.toMap(),
                                op);
            userMap->insert(key, nestedUserMap);
        } else if (userMap->contains(key) && userValue != sharedValue) {
            op->apply(userMap, key, sharedValue);
        }
    }
}


struct MergeSharedSetting
{
    MergeSharedSetting(const QSet<QString> &sticky) : m_userSticky(sticky) {}

    void apply(QVariantMap *userMap, const QString &key, const QVariant &sharedValue)
    {
        if (!m_userSticky.contains(key))
            userMap->insert(key, sharedValue);
    }
    QSet<QString> m_userSticky;
};

// When restoring settings...
//   We check whether a .shared file exists. If so, we compare the settings in this file with
//   corresponding ones in the .user file. Whenever we identify a corresponding setting which
//   has a different value and which is not marked as sticky, we merge the .shared value into
//   the .user value.
void mergeSharedSettings(QVariantMap *userMap, const QVariantMap &sharedMap)
{
    if (sharedMap.isEmpty())
        return;

    QSet<QString> stickyKeys;
    const QVariant stickyList = userMap->take(QLatin1String(USER_STICKY_KEYS_KEY)).toList();
    if (stickyList.isValid()) {
        if (stickyList.type() != QVariant::List) {
            // File is messed up... The user probably changed something.
            return;
        }
        foreach (const QVariant &v, stickyList.toList())
            stickyKeys.insert(v.toString());
    }

    MergeSharedSetting op(stickyKeys);
    synchronizeSettings(userMap, sharedMap, &op);
}


struct TrackUserStickySetting
{
    void apply(QVariantMap *, const QString &key, const QVariant &)
    {
        m_userSticky.insert(key);
    }
    QSet<QString> m_userSticky;
};

// When saving settings...
//   If a .shared file was considered in the previous restoring step, we check whether for
//   any of the current .shared settings there's a .user one which is different. If so, this
//   means the user explicitly changed it and we mark this setting as sticky.
//   Note that settings are considered sticky only when they differ from the .shared ones.
//   Although this approach is more flexible than permanent/forever sticky settings, it has
//   the side-effect that if a particular value unintentionally becomes the same in both
//   the .user and .shared files, this setting will "unstick".
void trackUserStickySettings(QVariantMap *userMap, const QVariantMap &sharedMap)
{
    if (sharedMap.isEmpty())
        return;

    TrackUserStickySetting op;
    synchronizeSettings(userMap, sharedMap, &op);

    userMap->insert(QLatin1String(USER_STICKY_KEYS_KEY), QVariant(op.m_userSticky.toList()));
}

} // Anonymous


QVariantMap SettingsAccessor::restoreSettings(Project *project) const
{
    if (m_lastVersion < 0 || !project)
        return QVariantMap();

    SettingsData settings;
    if (!m_userFileAcessor.readFile(project, &settings))
        settings.clear(); // No user settings, but there can still be shared ones.

    if (settings.isValid()) {
        if (settings.m_version > SettingsAccessor::instance()->m_lastVersion + 1) {
            QMessageBox::information(
                Core::ICore::mainWindow(),
                QApplication::translate("ProjectExplorer::SettingsAccessor",
                                        "Using Old Project Settings File"),
                QApplication::translate("ProjectExplorer::SettingsAccessor",
                                        "<html><head/><body><p>A versioned backup of the .user "
                                        "settings file will be used, because the non-versioned "
                                        "file was created by an incompatible newer version of "
                                        "Qt Creator.</p><p>Project settings changes made since "
                                        "the last time this version of Qt Creator was used "
                                        "with this project are ignored, and changes made now "
                                        "will <b>not</b> be propagated to the newer version."
                                        "</p></body></html>"),
                QMessageBox::Ok);
        }

        // Verify environment.
        if (!verifyEnvironmentId(settings.m_map.value(QLatin1String(ENVIRONMENT_ID_KEY)).toString())) {
            // TODO tr, casing check
            QMessageBox msgBox(
                QMessageBox::Question,
                QApplication::translate("ProjectExplorer::SettingsAccessor",
                                        "Project Settings File from a different Environment?"),
                QApplication::translate("ProjectExplorer::SettingsAccessor",
                                        "Qt Creator has found a .user settings file which was "
                                        "created for another development setup, maybe "
                                        "originating from another machine.\n\n"
                                        "The .user settings files contain environment specific "
                                        "settings. They should not be copied to a different "
                                        "environment. \n\n"
                                        "Do you still want to load the settings file?"),
                QMessageBox::Yes | QMessageBox::No,
                Core::ICore::mainWindow());
            msgBox.setDefaultButton(QMessageBox::No);
            msgBox.setEscapeButton(QMessageBox::No);
            if (msgBox.exec() == QMessageBox::No)
                return QVariantMap();
        }

        // Do we need to generate a backup?
        if (settings.m_version < m_lastVersion + 1 && !settings.m_usingBackup) {
            const QString &backupFileName = settings.m_fileName
                    + QLatin1Char('.')
                    + m_handlers.value(settings.m_version)->displayUserFileVersion();
            QFile::remove(backupFileName);  // Remove because copy doesn't overwrite
            QFile::copy(settings.m_fileName, backupFileName);
        }
    }


    // Time to consider shared settings...
    SettingsData sharedSettings;
    if (m_sharedFileAcessor.readFile(project, &sharedSettings)) {
        bool useSharedSettings = true;
        if (sharedSettings.m_version != settings.m_version) {
            int baseFileVersion;
            if (sharedSettings.m_version > m_lastVersion + 1) {
                // The shared file version is newer than Creator... If we have valid user
                // settings we prompt the user whether we could try an *unsupported* update.
                // This makes sense since the merging operation will only replace shared settings
                // that perfectly match corresponding user ones. If we don't have valid user
                // settings to compare against, there's nothing we can do.
                if (!settings.isValid())
                    return QVariantMap();

                QMessageBox msgBox(
                            QMessageBox::Question,
                            QApplication::translate("ProjectExplorer::SettingsAccessor",
                                                    "Unsupported Shared Settings File"),
                            QApplication::translate("ProjectExplorer::SettingsAccessor",
                                                    "The version of your .shared file is not yet "
                                                    "supported by this Qt Creator version. "
                                                    "Only settings that are still compatible "
                                                    "will be taken into account.\n\n"
                                                    "Do you want to continue?\n\n"
                                                    "If you choose not to continue Qt Creator will "
                                                    "not try to load the .shared file."),
                            QMessageBox::Yes | QMessageBox::No,
                            Core::ICore::mainWindow());
                msgBox.setDefaultButton(QMessageBox::No);
                msgBox.setEscapeButton(QMessageBox::No);
                if (msgBox.exec() == QMessageBox::No)
                    useSharedSettings = false;
                else
                    baseFileVersion = m_lastVersion + 1;
            } else {
                baseFileVersion = qMax(settings.m_version, sharedSettings.m_version);
            }

            if (useSharedSettings) {
                // We now update the user and shared settings so they are compatible.
                for (int i = sharedSettings.m_version; i < baseFileVersion; ++i)
                    sharedSettings.m_map = m_handlers.value(i)->update(project, sharedSettings.m_map);

                if (!settings.isValid()) {
                    project->setProperty(SHARED_SETTINGS, sharedSettings.m_map);
                    return sharedSettings.m_map;
                }
                for (int i = settings.m_version; i < baseFileVersion; ++i)
                    settings.m_map = m_handlers.value(i)->update(project, settings.m_map);
                settings.m_version = baseFileVersion;
            }
        }

        if (useSharedSettings) {
            project->setProperty(SHARED_SETTINGS, sharedSettings.m_map);
            if (!settings.isValid())
                return sharedSettings.m_map;

            mergeSharedSettings(&settings.m_map, sharedSettings.m_map);
        }
    }

    if (!settings.isValid())
        return QVariantMap();

    // Update from the base version to Creator's version.
    for (int i = settings.m_version; i <= m_lastVersion; ++i)
        settings.m_map = m_handlers.value(i)->update(project, settings.m_map);

    return settings.m_map;
}

bool SettingsAccessor::saveSettings(const Project *project, const QVariantMap &map) const
{
    if (!project || map.isEmpty())
        return false;

    SettingsData settings(map);
    const QVariant &shared = project->property(SHARED_SETTINGS);
    if (shared.isValid())
        trackUserStickySettings(&settings.m_map, shared.toMap());

    return m_userFileAcessor.writeFile(project, &settings);
}

void SettingsAccessor::addVersionHandler(UserFileVersionHandler *handler)
{
    const int version(handler->userFileVersion());
    QTC_ASSERT(handler, return);
    QTC_ASSERT(version >= 0, return);
    QTC_ASSERT(!m_handlers.contains(version), return);
    QTC_ASSERT(m_handlers.isEmpty() ||
               (version == m_lastVersion + 1 || version == m_firstVersion - 1), return);

    if (m_handlers.isEmpty()) {
        m_firstVersion = version;
        m_lastVersion = version;
    } else {
        if (version < m_firstVersion)
            m_firstVersion = version;
        if (version > m_lastVersion)
            m_lastVersion = version;
    }

    m_handlers.insert(version, handler);

    // Postconditions:
    Q_ASSERT(m_lastVersion >= 0);
    Q_ASSERT(m_firstVersion >= 0);
    Q_ASSERT(m_lastVersion >= m_firstVersion);
    Q_ASSERT(m_handlers.count() == m_lastVersion - m_firstVersion + 1);
    for (int i = m_firstVersion; i < m_lastVersion; ++i)
        Q_ASSERT(m_handlers.contains(i));
}

bool SettingsAccessor::verifyEnvironmentId(const QString &id)
{
    QUuid fileEnvironmentId(id);
    if (!fileEnvironmentId.isNull()
        && fileEnvironmentId
            != ProjectExplorerPlugin::instance()->projectExplorerSettings().environmentId) {
        return false;
    }
    return true;
}

// -------------------------------------------------------------------------
// SettingsData
// -------------------------------------------------------------------------
void SettingsAccessor::SettingsData::clear()
{
    m_version = -1;
    m_usingBackup = false;
    m_map.clear();
    m_fileName.clear();
}

bool SettingsAccessor::SettingsData::isValid() const
{
    return m_version > -1 && !m_fileName.isEmpty();
}

// -------------------------------------------------------------------------
// FileAcessor
// -------------------------------------------------------------------------
SettingsAccessor::FileAccessor::FileAccessor(const QByteArray &id,
                                             const QString &defaultSuffix,
                                             const QString &environmentSuffix,
                                             bool envSpecific,
                                             bool versionStrict)
    : m_id(id)
    , m_environmentSpecific(envSpecific)
    , m_versionStrict(versionStrict)
{
    assignSuffix(defaultSuffix, environmentSuffix);
}

void SettingsAccessor::FileAccessor::assignSuffix(const QString &defaultSuffix,
                                                  const QString &environmentSuffix)
{
    if (!environmentSuffix.isEmpty()) {
        m_suffix = environmentSuffix;
        m_suffix.replace(QRegExp(QLatin1String("[^a-zA-Z0-9_.-]")), QString(QLatin1Char('_'))); // replace fishy characters:
        m_suffix.prepend(QLatin1Char('.'));
    } else {
        m_suffix = defaultSuffix;
    }
}

QString SettingsAccessor::FileAccessor::assembleFileName(const Project *project) const
{
    return project->document()->fileName() + m_suffix;
}

bool SettingsAccessor::FileAccessor::findNewestCompatibleSetting(SettingsData *settings) const
{
    const QString baseFileName = settings->m_fileName;
    const int baseVersion = settings->m_version;
    settings->m_fileName.clear();
    settings->m_version = -1;

    PersistentSettingsReader reader;
    SettingsAccessor *acessor = SettingsAccessor::instance();

    QFileInfo fileInfo(baseFileName);
    QStringList fileNameFilter(fileInfo.fileName() + QLatin1String(".*"));
    const QStringList &entryList = fileInfo.absoluteDir().entryList(fileNameFilter);
    QStringList candidates;

    // First we try to identify the newest old version with a quick check.
    foreach (const QString &file, entryList) {
        const QString &suffix = file.mid(fileInfo.fileName().length() + 1);
        const QString &candidateFileName = baseFileName + QLatin1String(".") + suffix;
        candidates.append(candidateFileName);
        for (int candidateVersion = acessor->m_lastVersion;
             candidateVersion >= acessor->m_firstVersion;
             --candidateVersion) {
            if (suffix == acessor->m_handlers.value(candidateVersion)->displayUserFileVersion()) {
                if (candidateVersion > settings->m_version) {
                    settings->m_version = candidateVersion;
                    settings->m_fileName = candidateFileName;
                }
                break;
            }
        }
    }

    if (settings->m_version != -1) {
        if (reader.load(settings->m_fileName)) {
            settings->m_map = reader.restoreValues();
            return true;
        }
        qWarning() << "Unable to load file" << settings->m_fileName;
    }

    // If we haven't identified a valid file or if it for any reason failed to load, we
    // try a more expensive check (which is actually needed to identify our own and newer
    // versions as we don't know what extensions will be assigned in the future).
    foreach (const QString &candidateFileName, candidates) {
        if (settings->m_fileName == candidateFileName)
            continue; // This one has already failed to load.
        if (reader.load(candidateFileName)) {
            settings->m_map = reader.restoreValues();
            int candidateVersion = settings->m_map.value(QLatin1String(VERSION_KEY), 0).toInt();
            if (candidateVersion == acessor->m_lastVersion + 1) {
                settings->m_version = candidateVersion;
                settings->m_fileName = candidateFileName;
                return true;
            }
        }
    }

    // Nothing really worked...
    qWarning() << "File version" << baseVersion << "too new.";

    return false;
}

bool SettingsAccessor::FileAccessor::readFile(Project *project,
                                             SettingsData *settings) const
{
    PersistentSettingsReader reader;
    settings->m_fileName = assembleFileName(project);
    if (!reader.load(settings->m_fileName))
        return false;

    settings->m_map = reader.restoreValues();

    // Get and verify file version
    settings->m_version = settings->m_map.value(QLatin1String(VERSION_KEY), 0).toInt();
    if (!m_versionStrict)
        return true;

    if (settings->m_version < SettingsAccessor::instance()->m_firstVersion) {
        qWarning() << "Version" << settings->m_version << "in" << m_suffix << "too old.";
        return false;
    }

    if (settings->m_version > SettingsAccessor::instance()->m_lastVersion + 1) {
        if (!findNewestCompatibleSetting(settings))
            return false;

        settings->m_usingBackup = true;
        project->setProperty(m_id.constData(), settings->m_fileName);
    }

    return true;
}

bool SettingsAccessor::FileAccessor::writeFile(const Project *project,
                                              const SettingsData *settings) const
{
    PersistentSettingsWriter writer;
    for (QVariantMap::const_iterator i = settings->m_map.constBegin();
         i != settings->m_map.constEnd();
         ++i) {
        writer.saveValue(i.key(), i.value());
    }

    writer.saveValue(QLatin1String(VERSION_KEY), SettingsAccessor::instance()->m_lastVersion + 1);

    if (m_environmentSpecific) {
        writer.saveValue(QLatin1String(ENVIRONMENT_ID_KEY),
                         ProjectExplorerPlugin::instance()->projectExplorerSettings().environmentId.toString());
    }

    const QString &fileName = project->property(m_id).toString();
    return writer.save(fileName.isEmpty() ? assembleFileName(project) : fileName,
                       QLatin1String("QtCreatorProject"),
                       Core::ICore::mainWindow());
}

// -------------------------------------------------------------------------
// Version0Handler
// -------------------------------------------------------------------------

QVariantMap Version0Handler::convertBuildConfigurations(Project *project, const QVariantMap &map)
{
    Q_ASSERT(project);
    QVariantMap result;

    // Find a valid Id to use:
    QString id;
    if (project->id() == Core::Id("GenericProjectManager.GenericProject")) {
        id = QLatin1String("GenericProjectManager.GenericBuildConfiguration");
    } else if (project->id() == Core::Id("CMakeProjectManager.CMakeProject")) {
        id = QLatin1String("CMakeProjectManager.CMakeBuildConfiguration");
    } else if (project->id() == Core::Id("Qt4ProjectManager.Qt4Project")) {
        result.insert(QLatin1String("Qt4ProjectManager.Qt4BuildConfiguration.NeedsV0Update"), QVariant());
        id = QLatin1String("Qt4ProjectManager.Qt4BuildConfiguration");
    } else {
        return QVariantMap(); // QmlProjects do not(/no longer) have BuildConfigurations,
                              // or we do not know how to handle this.
    }
    result.insert(QLatin1String("ProjectExplorer.ProjectConfiguration.Id"), id);

    for (QVariantMap::const_iterator i = map.constBegin(); i != map.constEnd(); ++i) {
        if (i.key() == QLatin1String("ProjectExplorer.BuildConfiguration.DisplayName")) {
            result.insert(QLatin1String("ProjectExplorer.ProjectConfiguration.DisplayName"),
                         i.value());
            continue;
        }

        if (id == QLatin1String("Qt4ProjectManager.Qt4BuildConfiguration") ||
            id.startsWith(QLatin1String("Qt4ProjectManager.Qt4BuildConfiguration."))) {
            // Qt4BuildConfiguration:
            if (i.key() == QLatin1String("QtVersionId")) {
                result.insert(QLatin1String("Qt4ProjectManager.Qt4BuildConfiguration.QtVersionId"),
                              i.value().toInt());
            } else if (i.key() == QLatin1String("ToolChain")) {
                result.insert(QLatin1String("Qt4ProjectManager.Qt4BuildConfiguration.ToolChain"),
                              i.value());
            } else if (i.key() == QLatin1String("buildConfiguration")) {
                result.insert(QLatin1String("Qt4ProjectManager.Qt4BuildConfiguration.BuildConfiguration"),
                              i.value());
            } else if (i.key() == QLatin1String("userEnvironmentChanges")) {
                result.insert(QLatin1String("Qt4ProjectManager.Qt4BuildConfiguration.UserEnvironmentChanges"),
                              i.value());
            } else if (i.key() == QLatin1String("useShadowBuild")) {
                result.insert(QLatin1String("Qt4ProjectManager.Qt4BuildConfiguration.UseShadowBuild"),
                              i.value());
            } else if (i.key() == QLatin1String("clearSystemEnvironment")) {
                result.insert(QLatin1String("Qt4ProjectManager.Qt4BuildConfiguration.ClearSystemEnvironment"),
                              i.value());
            } else if (i.key() == QLatin1String("buildDirectory")) {
                result.insert(QLatin1String("Qt4ProjectManager.Qt4BuildConfiguration.BuildDirectory"),
                              i.value());
            } else {
                qWarning() << "Unknown Qt4BuildConfiguration Key found:" << i.key() << i.value();
            }
            continue;
        } else if (id == QLatin1String("CMakeProjectManager.CMakeBuildConfiguration")) {
            if (i.key() == QLatin1String("userEnvironmentChanges")) {
                result.insert(QLatin1String("CMakeProjectManager.CMakeBuildConfiguration.UserEnvironmentChanges"),
                              i.value());
            } else if (i.key() == QLatin1String("msvcVersion")) {
                result.insert(QLatin1String("CMakeProjectManager.CMakeBuildConfiguration.MsvcVersion"),
                              i.value());
            } else if (i.key() == QLatin1String("buildDirectory")) {
                result.insert(QLatin1String("CMakeProjectManager.CMakeBuildConfiguration.BuildDirectory"),
                              i.value());
            } else {
                qWarning() << "Unknown CMakeBuildConfiguration Key found:" << i.key() << i.value();
            }
            continue;
        } else if (id == QLatin1String("GenericProjectManager.GenericBuildConfiguration")) {
            if (i.key() == QLatin1String("buildDirectory")) {
                result.insert(QLatin1String("GenericProjectManager.GenericBuildConfiguration.BuildDirectory"),
                              i.value());
            } else {
                qWarning() << "Unknown GenericBuildConfiguration Key found:" << i.key() << i.value();
            }
            continue;
        }
        qWarning() << "Unknown BuildConfiguration Key found:" << i.key() << i.value();
        qWarning() << "BuildConfiguration Id is:" << id;
    }
    return result;
}

QVariantMap Version0Handler::convertRunConfigurations(Project *project, const QVariantMap &map)
{
    Q_UNUSED(project);
    QVariantMap result;
    QString id;

    // Convert Id:
    id = map.value(QLatin1String("Id")).toString();
    if (id.isEmpty())
        id = map.value(QLatin1String("type")).toString();
    if (id.isEmpty())
        return QVariantMap();

    if (QLatin1String("Qt4ProjectManager.DeviceRunConfiguration") == id)
        id = QLatin1String("Qt4ProjectManager.S60DeviceRunConfiguration");
    if (QLatin1String("Qt4ProjectManager.EmulatorRunConfiguration") == id)
        id = QLatin1String("Qt4ProjectManager.S60EmulatorRunConfiguration");
    // no need to change the CMakeRunConfiguration, CustomExecutableRunConfiguration,
    //                       MaemoRunConfiguration or Qt4RunConfiguration

    result.insert(QLatin1String("ProjectExplorer.ProjectConfiguration.Id"), id);

    // Convert everything else:
    for (QVariantMap::const_iterator i = map.constBegin(); i != map.constEnd(); ++i) {
        if (i.key() == QLatin1String("Id") || i.key() == QLatin1String("type"))
            continue;
        if (i.key() == QLatin1String("RunConfiguration.name")) {
            result.insert(QLatin1String("ProjectExplorer.ProjectConfiguration.DisplayName"),
                         i.value());
        } else if (QLatin1String("CMakeProjectManager.CMakeRunConfiguration") == id) {
            if (i.key() == QLatin1String("CMakeRunConfiguration.Target"))
                result.insert(QLatin1String("CMakeProjectManager.CMakeRunConfiguration.Target"), i.value());
            else if (i.key() == QLatin1String("CMakeRunConfiguration.WorkingDirectory"))
                result.insert(QLatin1String("CMakeProjectManager.CMakeRunConfiguration.WorkingDirectory"), i.value());
            else if (i.key() == QLatin1String("CMakeRunConfiguration.UserWorkingDirectory"))
                result.insert(QLatin1String("CMakeProjectManager.CMakeRunConfiguration.UserWorkingDirectory"), i.value());
            else if (i.key() == QLatin1String("CMakeRunConfiguration.UseTerminal"))
                result.insert(QLatin1String("CMakeProjectManager.CMakeRunConfiguration.UseTerminal"), i.value());
            else if (i.key() == QLatin1String("CMakeRunConfiguation.Title"))
                result.insert(QLatin1String("CMakeProjectManager.CMakeRunConfiguation.Title"), i.value());
            else if (i.key() == QLatin1String("CMakeRunConfiguration.Arguments"))
                result.insert(QLatin1String("CMakeProjectManager.CMakeRunConfiguration.Arguments"), i.value());
            else if (i.key() == QLatin1String("CMakeRunConfiguration.UserEnvironmentChanges"))
                result.insert(QLatin1String("CMakeProjectManager.CMakeRunConfiguration.UserEnvironmentChanges"), i.value());
            else if (i.key() == QLatin1String("BaseEnvironmentBase"))
                result.insert(QLatin1String("CMakeProjectManager.BaseEnvironmentBase"), i.value());
            else
                qWarning() << "Unknown CMakeRunConfiguration key found:" << i.key() << i.value();
        } else if (QLatin1String("Qt4ProjectManager.S60DeviceRunConfiguration") == id) {
            if (i.key() == QLatin1String("ProFile"))
                result.insert(QLatin1String("Qt4ProjectManager.S60DeviceRunConfiguration.ProFile"), i.value());
            else if (i.key() == QLatin1String("SigningMode"))
                result.insert(QLatin1String("Qt4ProjectManager.S60DeviceRunConfiguration.SigningMode"), i.value());
            else if (i.key() == QLatin1String("CustomSignaturePath"))
                result.insert(QLatin1String("Qt4ProjectManager.S60DeviceRunConfiguration.CustomSignaturePath"), i.value());
            else if (i.key() == QLatin1String("CustomKeyPath"))
                result.insert(QLatin1String("Qt4ProjectManager.S60DeviceRunConfiguration.CustomKeyPath"), i.value());
            else if (i.key() == QLatin1String("SerialPortName"))
                result.insert(QLatin1String("Qt4ProjectManager.S60DeviceRunConfiguration.SerialPortName"), i.value());
            else if (i.key() == QLatin1String("CommunicationType"))
                result.insert(QLatin1String("Qt4ProjectManager.S60DeviceRunConfiguration.CommunicationType"), i.value());
            else if (i.key() == QLatin1String("CommandLineArguments"))
                result.insert(QLatin1String("Qt4ProjectManager.S60DeviceRunConfiguration.CommandLineArguments"), i.value());
            else
                qWarning() << "Unknown S60DeviceRunConfiguration key found:" << i.key() << i.value();
        } else if (QLatin1String("Qt4ProjectManager.S60EmulatorRunConfiguration") == id) {
            if (i.key() == QLatin1String("ProFile"))
                result.insert(QLatin1String("Qt4ProjectManager.S60EmulatorRunConfiguration.ProFile"), i.value());
            else
                qWarning() << "Unknown S60EmulatorRunConfiguration key found:" << i.key() << i.value();
        } else if (QLatin1String("Qt4ProjectManager.Qt4RunConfiguration") == id) {
            if (i.key() == QLatin1String("ProFile"))
                result.insert(QLatin1String("Qt4ProjectManager.Qt4RunConfiguration.ProFile"), i.value());
            else if (i.key() == QLatin1String("CommandLineArguments"))
                result.insert(QLatin1String("Qt4ProjectManager.Qt4RunConfiguration.CommandLineArguments"), i.value());
            else if (i.key() == QLatin1String("UserSetName"))
                result.insert(QLatin1String("Qt4ProjectManager.Qt4RunConfiguration.UserSetName"), i.value());
            else if (i.key() == QLatin1String("UseTerminal"))
                result.insert(QLatin1String("Qt4ProjectManager.Qt4RunConfiguration.UseTerminal"), i.value());
            else if (i.key() == QLatin1String("UseDyldImageSuffix"))
                result.insert(QLatin1String("Qt4ProjectManager.Qt4RunConfiguration.UseDyldImageSuffix"), i.value());
            else if (i.key() == QLatin1String("UserEnvironmentChanges"))
                result.insert(QLatin1String("Qt4ProjectManager.Qt4RunConfiguration.UserEnvironmentChanges"), i.value());
            else if (i.key() == QLatin1String("BaseEnvironmentBase"))
                result.insert(QLatin1String("Qt4ProjectManager.Qt4RunConfiguration.BaseEnvironmentBase"), i.value());
            else if (i.key() == QLatin1String("UserSetWorkingDirectory"))
                result.insert(QLatin1String("Qt4ProjectManager.Qt4RunConfiguration.UserSetWorkingDirectory"), i.value());
            else if (i.key() == QLatin1String("UserWorkingDirectory"))
                result.insert(QLatin1String("Qt4ProjectManager.Qt4RunConfiguration.UserWorkingDirectory"), i.value());
            else
                qWarning() << "Unknown Qt4RunConfiguration key found:" << i.key() << i.value();
        } else if (QLatin1String("Qt4ProjectManager.MaemoRunConfiguration") == id) {
            if (i.key() == QLatin1String("ProFile"))
                result.insert(QLatin1String("Qt4ProjectManager.MaemoRunConfiguration.ProFile"), i.value());
            else if (i.key() == QLatin1String("Arguments"))
                result.insert(QLatin1String("Qt4ProjectManager.MaemoRunConfiguration.Arguments"), i.value());
            else if (i.key() == QLatin1String("Simulator"))
                result.insert(QLatin1String("Qt4ProjectManager.MaemoRunConfiguration.Simulator"), i.value());
            else if (i.key() == QLatin1String("DeviceId"))
                result.insert(QLatin1String("Qt4ProjectManager.MaemoRunConfiguration.DeviceId"), i.value());
            else if (i.key() == QLatin1String("LastDeployed"))
                result.insert(QLatin1String("Qt4ProjectManager.MaemoRunConfiguration.LastDeployed"), i.value());
            else if (i.key() == QLatin1String("DebuggingHelpersLastDeployed"))
                result.insert(QLatin1String("Qt4ProjectManager.MaemoRunConfiguration.DebuggingHelpersLastDeployed"), i.value());
            else
                qWarning() << "Unknown MaemoRunConfiguration key found:" << i.key() << i.value();
        } else if (QLatin1String("ProjectExplorer.CustomExecutableRunConfiguration") == id) {
            if (i.key() == QLatin1String("Executable"))
                result.insert(QLatin1String("ProjectExplorer.CustomExecutableRunConfiguration.Executable"), i.value());
            else if (i.key() == QLatin1String("Arguments"))
                result.insert(QLatin1String("ProjectExplorer.CustomExecutableRunConfiguration.Arguments"), i.value());
            else if (i.key() == QLatin1String("WorkingDirectory"))
                result.insert(QLatin1String("ProjectExplorer.CustomExecutableRunConfiguration.WorkingDirectory"), i.value());
            else if (i.key() == QLatin1String("UseTerminal"))
                result.insert(QLatin1String("ProjectExplorer.CustomExecutableRunConfiguration.UseTerminal"), i.value());
            else if (i.key() == QLatin1String("UserSetName"))
                result.insert(QLatin1String("ProjectExplorer.CustomExecutableRunConfiguration.UserSetName"), i.value());
            else if (i.key() == QLatin1String("UserName"))
                result.insert(QLatin1String("ProjectExplorer.CustomExecutableRunConfiguration.UserName"), i.value());
            else if (i.key() == QLatin1String("UserEnvironmentChanges"))
                result.insert(QLatin1String("ProjectExplorer.CustomExecutableRunConfiguration.UserEnvironmentChanges"), i.value());
            else if (i.key() == QLatin1String("BaseEnvironmentBase"))
                result.insert(QLatin1String("ProjectExplorer.CustomExecutableRunConfiguration.BaseEnvironmentBase"), i.value());
            else
                qWarning() << "Unknown CustomExecutableRunConfiguration key found:" << i.key() << i.value();
        } else {
            result.insert(i.key(), i.value());
        }
    }

    return result;
}

QVariantMap Version0Handler::convertBuildSteps(Project *project, const QVariantMap &map)
{
    Q_UNUSED(project);
    QVariantMap result;

    QString id(map.value(QLatin1String("Id")).toString());
    if (QLatin1String("GenericProjectManager.MakeStep") == id)
        id = QLatin1String("GenericProjectManager.GenericMakeStep");
    if (QLatin1String("projectexplorer.processstep") == id)
        id = QLatin1String("ProjectExplorer.ProcessStep");
    if (QLatin1String("trolltech.qt4projectmanager.make") == id)
        id = QLatin1String("Qt4ProjectManager.MakeStep");
    if (QLatin1String("trolltech.qt4projectmanager.qmake") == id)
        id = QLatin1String("QtProjectManager.QMakeBuildStep");
    // No need to change the CMake MakeStep.
    result.insert(QLatin1String("ProjectExplorer.ProjectConfiguration.Id"), id);

    for (QVariantMap::const_iterator i = map.constBegin(); i != map.constEnd(); ++i) {
        if (i.key() == QLatin1String("Id"))
            continue;
        if (i.key() == QLatin1String("ProjectExplorer.BuildConfiguration.DisplayName")) {
            // skip this: Not needed.
            continue;
        }

        if (QLatin1String("GenericProjectManager.GenericMakeStep") == id) {
            if (i.key() == QLatin1String("buildTargets"))
                result.insert(QLatin1String("GenericProjectManager.GenericMakeStep.BuildTargets"), i.value());
            else if (i.key() == QLatin1String("makeArguments"))
                result.insert(QLatin1String("GenericProjectManager.GenericMakeStep.MakeArguments"), i.value());
            else if (i.key() == QLatin1String("makeCommand"))
                result.insert(QLatin1String("GenericProjectManager.GenericMakeStep.MakeCommand"), i.value());
            else
                qWarning() << "Unknown GenericMakeStep value found:" << i.key() << i.value();
            continue;
        } else if (QLatin1String("ProjectExplorer.ProcessStep") == id) {
            if (i.key() == QLatin1String("ProjectExplorer.ProcessStep.DisplayName"))
                result.insert(QLatin1String("ProjectExplorer.ProjectConfiguration.DisplayName"), i.value());
            else if (i.key() == QLatin1String("abstractProcess.command"))
                result.insert(QLatin1String("ProjectExplorer.ProcessStep.Command"), i.value());
            else if ((i.key() == QLatin1String("abstractProcess.workingDirectory") ||
                      i.key() == QLatin1String("workingDirectory")) &&
                     !i.value().toString().isEmpty())
                    result.insert(QLatin1String("ProjectExplorer.ProcessStep.WorkingDirectory"), i.value());
            else if (i.key() == QLatin1String("abstractProcess.arguments"))
                result.insert(QLatin1String("ProjectExplorer.ProcessStep.Arguments"), i.value());
            else if (i.key() == QLatin1String("abstractProcess.enabled"))
                result.insert(QLatin1String("ProjectExplorer.ProcessStep.Enabled"), i.value());
            else
                qWarning() << "Unknown ProcessStep value found:" << i.key() << i.value();
        } else if (QLatin1String("Qt4ProjectManager.MakeStep") == id) {
            if (i.key() == QLatin1String("makeargs"))
                result.insert(QLatin1String("Qt4ProjectManager.MakeStep.MakeArguments"), i.value());
            else if (i.key() == QLatin1String("makeCmd"))
                result.insert(QLatin1String("Qt4ProjectManager.MakeStep.MakeCommand"), i.value());
            else if (i.key() == QLatin1String("clean"))
                result.insert(QLatin1String("Qt4ProjectManager.MakeStep.Clean"), i.value());
            else
                qWarning() << "Unknown Qt4MakeStep value found:" << i.key() << i.value();
        } else if (QLatin1String("QtProjectManager.QMakeBuildStep") == id) {
            if (i.key() == QLatin1String("qmakeArgs"))
                result.insert(QLatin1String("QtProjectManager.QMakeBuildStep.QMakeArguments"), i.value());
            else
                qWarning() << "Unknown Qt4QMakeStep value found:" << i.key() << i.value();
        } else if (QLatin1String("CMakeProjectManager.MakeStep") == id) {
            if (i.key() == QLatin1String("buildTargets"))
                result.insert(QLatin1String("CMakeProjectManager.MakeStep.BuildTargets"), i.value());
            else if (i.key() == QLatin1String("additionalArguments"))
                result.insert(QLatin1String("CMakeProjectManager.MakeStep.AdditionalArguments"), i.value());
            else if (i.key() == QLatin1String("clean"))
                result.insert(QLatin1String("CMakeProjectManager.MakeStep.Clean"), i.value());
            else
                qWarning() << "Unknown CMakeMakeStep value found:" << i.key() << i.value();
        } else {
            result.insert(i.key(), i.value());
        }
    }
    return result;
}

QVariantMap Version0Handler::update(Project *project, const QVariantMap &map)
{
    QVariantMap result;

    // "project": section is unused, just ignore it.

    // "buildconfigurations" and "buildConfiguration-":
    QStringList bcs(map.value(QLatin1String("buildconfigurations")).toStringList());
    QString active(map.value(QLatin1String("activebuildconfiguration")).toString());

    int count(0);
    foreach (const QString &bc, bcs) {
        // convert buildconfiguration:
        QString oldBcKey(QString::fromLatin1("buildConfiguration-") + bc);
        if (bc == active)
            result.insert(QLatin1String("ProjectExplorer.Project.ActiveBuildConfiguration"), count);

        QVariantMap tmp(map.value(oldBcKey).toMap());
        QVariantMap bcMap(convertBuildConfigurations(project, tmp));
        if (bcMap.isEmpty())
            continue;

        // buildsteps
        QStringList buildSteps(map.value(oldBcKey + QLatin1String("-buildsteps")).toStringList());

        if (buildSteps.isEmpty())
            // try lowercase version, too:-(
            buildSteps = map.value(QString::fromLatin1("buildconfiguration-") + bc + QLatin1String("-buildsteps")).toStringList();
        if (buildSteps.isEmpty())
            buildSteps = map.value(QLatin1String("buildsteps")).toStringList();

        int pos(0);
        foreach (const QString &bs, buildSteps) {
            // Watch out: Capitalization differs from oldBcKey!
            const QString localKey(QLatin1String("buildconfiguration-") + bc + QString::fromLatin1("-buildstep") + QString::number(pos));
            const QString globalKey(QString::fromLatin1("buildstep") + QString::number(pos));

            QVariantMap local(map.value(localKey).toMap());
            QVariantMap global(map.value(globalKey).toMap());

            for (QVariantMap::const_iterator i = global.constBegin(); i != global.constEnd(); ++i) {
                if (!local.contains(i.key()))
                    local.insert(i.key(), i.value());
                if (i.key() == QLatin1String("ProjectExplorer.BuildConfiguration.DisplayName") &&
                    local.value(i.key()).toString().isEmpty())
                    local.insert(i.key(), i.value());
            }
            local.insert(QLatin1String("Id"), bs);

            bcMap.insert(QString::fromLatin1("ProjectExplorer.BuildConfiguration.BuildStep.") + QString::number(pos),
                         convertBuildSteps(project, local));
            ++pos;
        }
        bcMap.insert(QLatin1String("ProjectExplorer.BuildConfiguration.BuildStepsCount"), pos);

        // cleansteps
        QStringList cleanSteps(map.value(oldBcKey + QLatin1String("-cleansteps")).toStringList());
        if (cleanSteps.isEmpty())
            // try lowercase version, too:-(
            cleanSteps = map.value(QString::fromLatin1("buildconfiguration-") + bc + QLatin1String("-cleansteps")).toStringList();
        if (cleanSteps.isEmpty())
            cleanSteps = map.value(QLatin1String("cleansteps")).toStringList();

        pos = 0;
        foreach (const QString &bs, cleanSteps) {
            // Watch out: Capitalization differs from oldBcKey!
            const QString localKey(QLatin1String("buildconfiguration-") + bc + QString::fromLatin1("-cleanstep") + QString::number(pos));
            const QString globalKey(QString::fromLatin1("cleanstep") + QString::number(pos));

            QVariantMap local(map.value(localKey).toMap());
            QVariantMap global(map.value(globalKey).toMap());

            for (QVariantMap::const_iterator i = global.constBegin(); i != global.constEnd(); ++i) {
                if (!local.contains(i.key()))
                    local.insert(i.key(), i.value());
                if (i.key() == QLatin1String("ProjectExplorer.BuildConfiguration.DisplayName") &&
                    local.value(i.key()).toString().isEmpty())
                    local.insert(i.key(), i.value());
            }
            local.insert(QLatin1String("Id"), bs);
            bcMap.insert(QString::fromLatin1("ProjectExplorer.BuildConfiguration.CleanStep.") + QString::number(pos),
                         convertBuildSteps(project, local));
            ++pos;
        }
        bcMap.insert(QLatin1String("ProjectExplorer.BuildConfiguration.CleanStepsCount"), pos);

        // Merge into result set:
        result.insert(QString::fromLatin1("ProjectExplorer.Project.BuildConfiguration.") + QString::number(count), bcMap);
        ++count;
    }
    result.insert(QLatin1String("ProjectExplorer.Project.BuildConfigurationCount"), count);

    // "RunConfiguration*":
    active = map.value(QLatin1String("activeRunConfiguration")).toString();
    count = 0;
    forever {
        QString prefix(QLatin1String("RunConfiguration") + QString::number(count) + QLatin1Char('-'));
        QVariantMap rcMap;
        for (QVariantMap::const_iterator i = map.constBegin(); i != map.constEnd(); ++i) {
            if (!i.key().startsWith(prefix))
                continue;
            QString newKey(i.key().mid(prefix.size()));
            rcMap.insert(newKey, i.value());
        }
        if (rcMap.isEmpty()) {
            result.insert(QLatin1String("ProjectExplorer.Project.RunConfigurationCount"), count);
            break;
        }

        result.insert(QString::fromLatin1("ProjectExplorer.Project.RunConfiguration.") + QString::number(count),
                      convertRunConfigurations(project, rcMap));
        ++count;
    }

    // "defaultFileEncoding" (EditorSettings):
    QVariant codecVariant(map.value(QLatin1String("defaultFileEncoding")));
    if (codecVariant.isValid()) {
        QByteArray codec(codecVariant.toByteArray());
        QVariantMap editorSettingsMap;
        editorSettingsMap.insert(QLatin1String("EditorConfiguration.Codec"), codec);
        result.insert(QLatin1String("ProjectExplorer.Project.EditorSettings"),
                      editorSettingsMap);
    }

    QVariant toolchain(map.value(QLatin1String("toolChain")));
    if (toolchain.isValid()) {
        bool ok;
        int type(toolchain.toInt(&ok));
        if (!ok) {
            QString toolChainName(toolchain.toString());
            if (toolChainName == QLatin1String("gcc"))
                type = 0;
            else if (toolChainName == QLatin1String("mingw"))
                type = 2;
            else if (toolChainName == QLatin1String("msvc"))
                type = 3;
            else if (toolChainName == QLatin1String("wince"))
                type = 4;
        }
        result.insert(QLatin1String("GenericProjectManager.GenericProject.Toolchain"), type);
    }

    return result;
}

// -------------------------------------------------------------------------
// Version1Handler
// -------------------------------------------------------------------------

QVariantMap Version1Handler::update(Project *project, const QVariantMap &map)
{
    QVariantMap result;

    // The only difference between version 1 and 2 of the user file is that
    // we need to add targets.

    // Generate a list of all possible targets for the project:
    QList<TargetDescription> targets;
    if (project->id() == Core::Id("GenericProjectManager.GenericProject"))
        targets << TargetDescription(QString::fromLatin1("GenericProjectManager.GenericTarget"),
                                     QCoreApplication::translate("GenericProjectManager::GenericTarget",
                                                                 "Desktop",
                                                                 "Generic desktop target display name"));
    else if (project->id() == Core::Id("CMakeProjectManager.CMakeProject"))
        targets << TargetDescription(QString::fromLatin1("CMakeProjectManager.DefaultCMakeTarget"),
                                     QCoreApplication::translate("CMakeProjectManager::Internal::CMakeTarget",
                                                                 "Desktop",
                                                                 "CMake Default target display name"));
    else if (project->id() == Core::Id("Qt4ProjectManager.Qt4Project"))
        targets << TargetDescription(QString::fromLatin1("Qt4ProjectManager.Target.DesktopTarget"),
                                     QCoreApplication::translate("Qt4ProjectManager::Internal::Qt4Target",
                                                                 "Desktop",
                                                                 "Qt4 Desktop target display name"))
        << TargetDescription(QString::fromLatin1("Qt4ProjectManager.Target.S60EmulatorTarget"),
                                     QCoreApplication::translate("Qt4ProjectManager::Internal::Qt4Target",
                                                                 "Symbian Emulator",
                                                                 "Qt4 Symbian Emulator target display name"))
        << TargetDescription(QString::fromLatin1("Qt4ProjectManager.Target.S60DeviceTarget"),
                                     QCoreApplication::translate("Qt4ProjectManager::Internal::Qt4Target",
                                                                 "Symbian Device",
                                                                 "Qt4 Symbian Device target display name"))
        << TargetDescription(QString::fromLatin1("Qt4ProjectManager.Target.MaemoEmulatorTarget"),
                                     QCoreApplication::translate("Qt4ProjectManager::Internal::Qt4Target",
                                                                 "Maemo Emulator",
                                                                 "Qt4 Maemo Emulator target display name"))
        << TargetDescription(QString::fromLatin1("Qt4ProjectManager.Target.MaemoDeviceTarget"),
                                     QCoreApplication::translate("Qt4ProjectManager::Internal::Qt4Target",
                                                                 "Maemo Device",
                                                                 "Qt4 Maemo Device target display name"));
    else if (project->id() == Core::Id("QmlProjectManager.QmlProject"))
        targets << TargetDescription(QString::fromLatin1("QmlProjectManager.QmlTarget"),
                                     QCoreApplication::translate("QmlProjectManager::QmlTarget",
                                                                 "QML Viewer",
                                                                 "QML Viewer target display name"));
    else
        return QVariantMap(); // We do not know how to handle this.

    result.insert(QLatin1String("ProjectExplorer.Project.ActiveTarget"), 0);
    result.insert(QLatin1String("ProjectExplorer.Project.TargetCount"), targets.count());
    int pos(0);
    foreach (const TargetDescription &td, targets) {
        QVariantMap targetMap;
        // Do not set displayName or icon!
        targetMap.insert(QLatin1String("ProjectExplorer.ProjectConfiguration.Id"), td.id);

        int count = map.value(QLatin1String("ProjectExplorer.Project.BuildConfigurationCount")).toInt();
        targetMap.insert(QLatin1String("ProjectExplorer.Target.BuildConfigurationCount"), count);
        for (int i = 0; i < count; ++i) {
            QString key(QString::fromLatin1("ProjectExplorer.Project.BuildConfiguration.") + QString::number(i));
            if (map.contains(key)) {
                QVariantMap bcMap = map.value(key).toMap();
                if (!bcMap.contains(QLatin1String("Qt4ProjectManager.Qt4BuildConfiguration.UseShadowBuild")))
                    bcMap.insert(QLatin1String("Qt4ProjectManager.Qt4BuildConfiguration.UseShadowBuild"), false);
                targetMap.insert(QString::fromLatin1("ProjectExplorer.Target.BuildConfiguration.") + QString::number(i),
                                 bcMap);
            }
        }

        count = map.value(QLatin1String("ProjectExplorer.Project.RunConfigurationCount")).toInt();
        for (int i = 0; i < count; ++i) {
            QString key(QString::fromLatin1("ProjectExplorer.Project.RunConfiguration.") + QString::number(i));
            if (map.contains(key))
                targetMap.insert(QString::fromLatin1("ProjectExplorer.Target.RunConfiguration.") + QString::number(i),
                                 map.value(key));
        }

        if (map.contains(QLatin1String("ProjectExplorer.Project.ActiveBuildConfiguration")))
            targetMap.insert(QLatin1String("ProjectExplorer.Target.ActiveBuildConfiguration"),
                             map.value(QLatin1String("ProjectExplorer.Project.ActiveBuildConfiguration")));
        if (map.contains(QLatin1String("ProjectExplorer.Project.ActiveRunConfiguration")))
            targetMap.insert(QLatin1String("ProjectExplorer.Target.ActiveRunConfiguration"),
                             map.value(QLatin1String("ProjectExplorer.Project.ActiveRunConfiguration")));
        if (map.contains(QLatin1String("ProjectExplorer.Project.RunConfigurationCount")))
            targetMap.insert(QLatin1String("ProjectExplorer.Target.RunConfigurationCount"),
                             map.value(QLatin1String("ProjectExplorer.Project.RunConfigurationCount")));

        result.insert(QString::fromLatin1("ProjectExplorer.Project.Target.") + QString::number(pos), targetMap);
        ++pos;
    }

    // copy everything else:
    for (QVariantMap::const_iterator i = map.constBegin(); i != map.constEnd(); ++i) {
        if (i.key() == QLatin1String("ProjectExplorer.Project.ActiveBuildConfiguration") ||
            i.key() == QLatin1String("ProjectExplorer.Project.BuildConfigurationCount") ||
            i.key() == QLatin1String("ProjectExplorer.Project.ActiveRunConfiguration") ||
            i.key() == QLatin1String("ProjectExplorer.Project.RunConfigurationCount") ||
            i.key().startsWith(QLatin1String("ProjectExplorer.Project.BuildConfiguration.")) ||
            i.key().startsWith(QLatin1String("ProjectExplorer.Project.RunConfiguration.")))
            continue;
        result.insert(i.key(), i.value());
    }

    return result;
}

// -------------------------------------------------------------------------
// Version2Handler
// -------------------------------------------------------------------------

QVariantMap Version2Handler::update(Project *, const QVariantMap &map)
{
    QList<Change> changes;
    changes.append(qMakePair(QLatin1String("CMakeProjectManager.CMakeBuildConfiguration.UserEnvironmentChanges"),
                             QLatin1String("ProjectExplorer.BuildConfiguration.UserEnvironmentChanges")));
    changes.append(qMakePair(QLatin1String("CMakeProjectManager.CMakeBuildConfiguration.ClearSystemEnvironment"),
                             QLatin1String("ProjectExplorer.BuildConfiguration.ClearSystemEnvironment")));
    changes.append(qMakePair(QLatin1String("Qt4ProjectManager.Qt4BuildConfiguration.UserEnvironmentChanges"),
                             QLatin1String("ProjectExplorer.BuildConfiguration.UserEnvironmentChanges")));
    changes.append(qMakePair(QLatin1String("Qt4ProjectManager.Qt4BuildConfiguration.ClearSystemEnvironment"),
                             QLatin1String("ProjectExplorer.BuildConfiguration.ClearSystemEnvironment")));

    return renameKeys(changes, QVariantMap(map));
}

// -------------------------------------------------------------------------
// Version3Handler
// -------------------------------------------------------------------------

// insert the additional build step:
//<valuemap key="ProjectExplorer.BuildConfiguration.BuildStep.2" type="QVariantMap">
// <value key="ProjectExplorer.ProjectConfiguration.DisplayName" type="QString">Create sis Package</value>
// <value key="ProjectExplorer.ProjectConfiguration.Id" type="QString">Qt4ProjectManager.S60SignBuildStep</value>
// <value key="Qt4ProjectManager.MakeStep.Clean" type="bool">false</value>
// <valuelist key="Qt4ProjectManager.MakeStep.MakeArguments" type="QVariantList"/>
// <value key="Qt4ProjectManager.MakeStep.MakeCommand" type="QString"></value>
// <value key="Qt4ProjectManager.S60CreatePackageStep.Certificate" type="QString"></value>
// <value key="Qt4ProjectManager.S60CreatePackageStep.Keyfile" type="QString"></value>
// <value key="Qt4ProjectManager.S60CreatePackageStep.SignMode" type="int">0</value>
//</valuemap>

// remove the deprecated sign run settings from
//<valuemap key="ProjectExplorer.Target.RunConfiguration.0" type="QVariantMap">
// <value key="ProjectExplorer.ProjectConfiguration.DisplayName" type="QString">untitled1 on Symbian Device</value>
// <value key="ProjectExplorer.ProjectConfiguration.Id" type="QString">Qt4ProjectManager.S60DeviceRunConfiguration</value>
// <valuelist key="Qt4ProjectManager.S60DeviceRunConfiguration.CommandLineArguments" type="QVariantList"/>
// <value key="Qt4ProjectManager.S60DeviceRunConfiguration.CustomKeyPath" type="QString"></value>
// <value key="Qt4ProjectManager.S60DeviceRunConfiguration.CustomSignaturePath" type="QString"></value>
// <value key="Qt4ProjectManager.S60DeviceRunConfiguration.ProFile" type="QString">untitled1.pro</value>
// <value key="Qt4ProjectManager.S60DeviceRunConfiguration.SerialPortName" type="QString">COM3</value>
// <value key="Qt4ProjectManager.S60DeviceRunConfiguration.SigningMode" type="int">0</value>
//</valuemap>

QVariantMap Version3Handler::update(Project *, const QVariantMap &map)
{
    QVariantMap result;
    QMapIterator<QString, QVariant> it(map);
    while (it.hasNext()) {
        it.next();
        const QString &targetKey = it.key();
        // check for target info
        if (!targetKey.startsWith(QLatin1String("ProjectExplorer.Project.Target."))) {
            result.insert(targetKey, it.value());
            continue;
        }
        const QVariantMap &originalTarget = it.value().toMap();
        // check for symbian device target
        if (originalTarget.value(QLatin1String("ProjectExplorer.ProjectConfiguration.Id"))
                != QLatin1String("Qt4ProjectManager.Target.S60DeviceTarget")) {
            result.insert(targetKey, originalTarget);
            continue;
        }
        QVariantMap newTarget;
        // first iteration: search run configurations, get signing info, remove old signing keys
        QString customKeyPath;
        QString customSignaturePath;
        int signingMode = 0; // SelfSign
        QMapIterator<QString, QVariant> targetIt(originalTarget);
        while (targetIt.hasNext()) {
            targetIt.next();
            const QString &key = targetIt.key();
            if (key.startsWith(QLatin1String("ProjectExplorer.Target.BuildConfiguration."))) {
                // build configurations are handled in second iteration
                continue;
            }
            if (!key.startsWith(QLatin1String("ProjectExplorer.Target.RunConfiguration."))) {
                newTarget.insert(key, targetIt.value());
                continue;
            }
            QVariantMap runConfig = targetIt.value().toMap();
            if (runConfig.value(QLatin1String("ProjectExplorer.ProjectConfiguration.Id"))
                    != QLatin1String("Qt4ProjectManager.S60DeviceRunConfiguration")) {
                newTarget.insert(key, runConfig);
                continue;
            }
            // get signing info
            customKeyPath = runConfig.value(QLatin1String("Qt4ProjectManager.S60DeviceRunConfiguration.CustomKeyPath")).toString();
            customSignaturePath = runConfig.value(QLatin1String("Qt4ProjectManager.S60DeviceRunConfiguration.CustomSignaturePath")).toString();
            signingMode = runConfig.value(QLatin1String("Qt4ProjectManager.S60DeviceRunConfiguration.SigningMode")).toInt();
            // remove old signing keys
            runConfig.remove(QLatin1String("Qt4ProjectManager.S60DeviceRunConfiguration.CustomKeyPath"));
            runConfig.remove(QLatin1String("Qt4ProjectManager.S60DeviceRunConfiguration.CustomSignaturePath"));
            runConfig.remove(QLatin1String("Qt4ProjectManager.S60DeviceRunConfiguration.SigningMode"));
            newTarget.insert(key, runConfig);
        }

        // second iteration: add new signing build step
        targetIt.toFront();
        while (targetIt.hasNext()) {
            targetIt.next();
            const QString &key = targetIt.key();
            if (!key.startsWith(QLatin1String("ProjectExplorer.Target.BuildConfiguration."))) {
                // everything except build configs already handled
                continue;
            }
            QVariantMap buildConfig = targetIt.value().toMap();
            int stepCount = buildConfig.value(QLatin1String("ProjectExplorer.BuildConfiguration.BuildStepsCount")).toInt();
            QVariantMap signBuildStep;
            signBuildStep.insert(QLatin1String("ProjectExplorer.ProjectConfiguration.DisplayName"), QLatin1String("Create SIS package"));
            signBuildStep.insert(QLatin1String("ProjectExplorer.ProjectConfiguration.Id"), QLatin1String("Qt4ProjectManager.S60SignBuildStep"));
            signBuildStep.insert(QLatin1String("Qt4ProjectManager.MakeStep.Clean"), false);
            signBuildStep.insert(QLatin1String("Qt4ProjectManager.S60CreatePackageStep.Certificate"), customSignaturePath);
            signBuildStep.insert(QLatin1String("Qt4ProjectManager.S60CreatePackageStep.Keyfile"), customKeyPath);
            signBuildStep.insert(QLatin1String("Qt4ProjectManager.S60CreatePackageStep.SignMode"), signingMode);
            buildConfig.insert(QString::fromLatin1("ProjectExplorer.BuildConfiguration.BuildStep.%1").arg(stepCount), signBuildStep);
            buildConfig.insert(QLatin1String("ProjectExplorer.BuildConfiguration.BuildStepsCount"), stepCount + 1);
            newTarget.insert(key, buildConfig);
        }
        result.insert(targetKey, newTarget);
    }
    return result;
}


// -------------------------------------------------------------------------
// Version4Handler
// -------------------------------------------------------------------------

// Move packaging steps from build steps into deploy steps
QVariantMap Version4Handler::update(Project *, const QVariantMap &map)
{
    QVariantMap result;
    QMapIterator<QString, QVariant> it(map);
    while (it.hasNext()) {
        it.next();
        const QString &globalKey = it.key();
        // check for target info
        if (!globalKey.startsWith(QLatin1String("ProjectExplorer.Project.Target."))) {
            result.insert(globalKey, it.value());
            continue;
        }
        const QVariantMap &originalTarget = it.value().toMap();
        // check for symbian and maemo device target
        if (originalTarget.value(QLatin1String("ProjectExplorer.ProjectConfiguration.Id"))
                != QLatin1String("Qt4ProjectManager.Target.S60DeviceTarget")
            && originalTarget.value(QLatin1String("ProjectExplorer.ProjectConfiguration.Id"))
                != QLatin1String("Qt4ProjectManager.Target.MaemoDeviceTarget"))
        {
            result.insert(globalKey, originalTarget);
            continue;
        }

        QVariantMap newTarget;
        QMapIterator<QString, QVariant> targetIt(originalTarget);
        while (targetIt.hasNext()) {
            targetIt.next();
            const QString &targetKey = targetIt.key();

            if (targetKey.startsWith(QLatin1String("ProjectExplorer.Target.RunConfiguration."))) {
                const QVariantMap &runConfigMap = targetIt.value().toMap();
                const QLatin1String maemoRcId("Qt4ProjectManager.MaemoRunConfiguration");
                if (runConfigMap.value(QLatin1String("ProjectExplorer.ProjectConfiguration.Id")).toString()
                    == maemoRcId) {
                    QVariantMap newRunConfigMap;
                    for (QVariantMap::ConstIterator rcMapIt = runConfigMap.constBegin();
                         rcMapIt != runConfigMap.constEnd(); ++rcMapIt) {
                        const QLatin1String oldProFileKey(".ProFile");
                        if (rcMapIt.key() == oldProFileKey) {
                            newRunConfigMap.insert(maemoRcId + oldProFileKey,
                                rcMapIt.value());
                        } else {
                            newRunConfigMap.insert(rcMapIt.key(),
                                rcMapIt.value());
                        }
                    }
                    newTarget.insert(targetKey, newRunConfigMap);
                    continue;
                }
            }

            if (!targetKey.startsWith(QLatin1String("ProjectExplorer.Target.BuildConfiguration."))) {
                newTarget.insert(targetKey, targetIt.value());
                continue;
            }

            bool movedBs = false;
            const QVariantMap &originalBc = targetIt.value().toMap();
            QVariantMap newBc;
            QMapIterator<QString, QVariant> bcIt(originalBc);
            while(bcIt.hasNext()) {
                bcIt.next();
                const QString &bcKey = bcIt.key();
                if (!bcKey.startsWith(QLatin1String("ProjectExplorer.BuildConfiguration.BuildStep."))) {
                    newBc.insert(bcKey, bcIt.value());
                    continue;
                }

                const QVariantMap &buildStep = bcIt.value().toMap();
                if ((buildStep.value(QLatin1String("ProjectExplorer.ProjectConfiguration.Id")).toString() ==
                        QLatin1String("Qt4ProjectManager.S60SignBuildStep"))
                    || (buildStep.value(QLatin1String("ProjectExplorer.ProjectConfiguration.Id")).toString() ==
                        QLatin1String("Qt4ProjectManager.MaemoPackageCreationStep"))) {
                    movedBs = true;
                    newBc.insert(QLatin1String("ProjectExplorer.BuildConfiguration.DeployStep.0"), buildStep);
                } else {
                    newBc.insert(bcKey, buildStep);
                }
            }
            if (movedBs) {
                // adjust counts:
                newBc.insert(QLatin1String("ProjectExplorer.BuildConfiguration.DeployStepsCount"), 1);
                newBc.insert(QLatin1String("ProjectExplorer.BuildConfiguration.BuildStepsCount"),
                        newBc.value(QLatin1String("ProjectExplorer.BuildConfiguration.BuildStepsCount")).toInt() - 1);
            }
            newTarget.insert(targetKey, newBc);
        }
        result.insert(globalKey, newTarget);
    }
    return result;
}

// -------------------------------------------------------------------------
// Version5Handler
// -------------------------------------------------------------------------

// Move packaging steps from build steps into deploy steps
QVariantMap Version5Handler::update(Project *, const QVariantMap &map)
{
    QVariantMap result;
    QMapIterator<QString, QVariant> it(map);
    while (it.hasNext()) {
        it.next();
        const QString &globalKey = it.key();
        // check for target info
        if (!globalKey.startsWith(QLatin1String("ProjectExplorer.Project.Target."))) {
            result.insert(globalKey, it.value());
            continue;
        }
        const QVariantMap &originalTarget = it.value().toMap();
        // check for symbian and maemo device target
        if (originalTarget.value(QLatin1String("ProjectExplorer.ProjectConfiguration.Id"))
            != QLatin1String("Qt4ProjectManager.Target.S60DeviceTarget")
            && originalTarget.value(QLatin1String("ProjectExplorer.ProjectConfiguration.Id"))
            != QLatin1String("Qt4ProjectManager.Target.MaemoDeviceTarget")) {
            result.insert(globalKey, originalTarget);
            continue;
        }

        QVariantMap newTarget;
        QMapIterator<QString, QVariant> targetIt(originalTarget);
        while (targetIt.hasNext()) {
            targetIt.next();
            const QString &targetKey = targetIt.key();
            if (!targetKey.startsWith(QLatin1String("ProjectExplorer.Target.BuildConfiguration."))) {
                newTarget.insert(targetKey, targetIt.value());
                continue;
            }

            const QVariantMap &originalBc = targetIt.value().toMap();
            QVariantMap newBc = originalBc;
            QVariantMap newDeployStep;

            if (originalTarget.value(QLatin1String("ProjectExplorer.ProjectConfiguration.Id"))
                == QLatin1String("Qt4ProjectManager.Target.S60DeviceTarget")) {
                newDeployStep.insert(QLatin1String("ProjectExplorer.ProjectConfiguration.Id"),
                                     QLatin1String("Qt4ProjectManager.S60DeployStep"));
            } else {
                newDeployStep.insert(QLatin1String("ProjectExplorer.ProjectConfiguration.Id"),
                                     QLatin1String("Qt4ProjectManager.MaemoDeployStep"));
            }

            int deployCount = newBc.value(QLatin1String("ProjectExplorer.BuildConfiguration.DeployStepsCount"), 0).toInt();
            newBc.insert(QString::fromLatin1("ProjectExplorer.BuildConfiguration.DeployStep.") + QString::number(deployCount),
                         newDeployStep);
            newBc.insert(QLatin1String("ProjectExplorer.BuildConfiguration.DeployStepsCount"), deployCount + 1);

            newTarget.insert(targetKey, newBc);
        }
        result.insert(globalKey, newTarget);
    }
    return result;
}

// -------------------------------------------------------------------------
// Version6Handler
// -------------------------------------------------------------------------

// Introduce DeployConfiguration and BuildStepLists
QVariantMap Version6Handler::update(Project *, const QVariantMap &map)
{
    QVariantMap result;
    QMapIterator<QString, QVariant> it(map);
    while (it.hasNext()) {
        it.next();
        const QString &globalKey = it.key();
        // check for target info
        if (!globalKey.startsWith(QLatin1String("ProjectExplorer.Project.Target."))) {
            result.insert(globalKey, it.value());
            continue;
        }

        QVariantMap newDc;
        const QVariantMap &originalTarget = it.value().toMap();
        QVariantMap newTarget;
        QVariantMap deploySteps;
        QString deploymentName = QCoreApplication::translate("ProjectExplorer::UserFileHandler", "No deployment");

        QMapIterator<QString, QVariant> targetIt(originalTarget);
        while (targetIt.hasNext()) {
            targetIt.next();
            const QString &targetKey = targetIt.key();

            if (targetKey == QLatin1String("ProjectExplorer.ProjectConfiguration.Id")) {
                if (targetIt.value().toString() == QLatin1String("Qt4ProjectManager.Target.MaemoDeviceTarget"))
                    deploymentName = QCoreApplication::translate("ProjectExplorer::UserFileHandler", "Deploy to Maemo device");
                else if (targetIt.value().toString() == QLatin1String("Qt4ProjectManager.Target.S60DeviceTarget"))
                    deploymentName = QCoreApplication::translate("ProjectExplorer::UserFileHandler", "Deploy to Symbian device");
            }

            if (!targetKey.startsWith(QLatin1String("ProjectExplorer.Target.BuildConfiguration."))) {
                newTarget.insert(targetKey, targetIt.value());
                continue;
            }

            QVariantMap buildSteps;
            QVariantMap cleanSteps;
            const QVariantMap &originalBc = targetIt.value().toMap();
            QVariantMap newBc;

            QMapIterator<QString, QVariant> bcIt(originalBc);
            while (bcIt.hasNext())
            {
                bcIt.next();
                const QString &bcKey = bcIt.key();
                if (bcKey.startsWith(QLatin1String("ProjectExplorer.BuildConfiguration.BuildStep."))) {
                    QString position = bcKey.mid(45);
                    buildSteps.insert(QString::fromLatin1("ProjectExplorer.BuildStepList.Step.") + position, bcIt.value());
                    continue;
                }
                if (bcKey.startsWith(QLatin1String("ProjectExplorer.BuildConfiguration.BuildStepsCount"))) {
                    buildSteps.insert(QLatin1String("ProjectExplorer.BuildStepList.StepsCount"), bcIt.value());
                    continue;
                }
                if (bcKey.startsWith(QLatin1String("ProjectExplorer.BuildConfiguration.CleanStep."))) {
                    QString position = bcKey.mid(45);
                    cleanSteps.insert(QString::fromLatin1("ProjectExplorer.BuildStepList.Step.") + position, bcIt.value());
                    continue;
                }
                if (bcKey.startsWith(QLatin1String("ProjectExplorer.BuildConfiguration.CleanStepsCount"))) {
                    cleanSteps.insert(QLatin1String("ProjectExplorer.BuildStepList.StepsCount"), bcIt.value());
                    continue;
                }
                if (bcKey.startsWith(QLatin1String("ProjectExplorer.BuildConfiguration.DeployStep."))) {
                    QString position = bcKey.mid(46);
                    deploySteps.insert(QString::fromLatin1("ProjectExplorer.BuildStepList.Step.") + position, bcIt.value());
                    continue;
                }
                if (bcKey.startsWith(QLatin1String("ProjectExplorer.BuildConfiguration.DeployStepsCount"))) {
                    deploySteps.insert(QLatin1String("ProjectExplorer.BuildStepList.StepsCount"), bcIt.value());
                    continue;
                }
                newBc.insert(bcKey, bcIt.value());
            }
            buildSteps.insert(QLatin1String("ProjectExplorer.ProjectConfiguration.DisplayName"), QLatin1String("Build"));
            buildSteps.insert(QLatin1String("ProjectExplorer.ProjectConfiguration.Id"), QLatin1String("ProjectExplorer.BuildSteps.Build"));
            cleanSteps.insert(QLatin1String("ProjectExplorer.ProjectConfiguration.DisplayName"), QLatin1String("Clean"));
            cleanSteps.insert(QLatin1String("ProjectExplorer.ProjectConfiguration.Id"), QLatin1String("ProjectExplorer.BuildSteps.Clean"));
            newBc.insert(QLatin1String("ProjectExplorer.BuildConfiguration.BuildStepList.0"), buildSteps);
            newBc.insert(QLatin1String("ProjectExplorer.BuildConfiguration.BuildStepList.1"), cleanSteps);
            newBc.insert(QLatin1String("ProjectExplorer.BuildConfiguration.BuildStepListCount"), 2);
            newTarget.insert(targetKey, newBc);
        }

        // Only insert one deploy configuration:
        deploySteps.insert(QLatin1String("ProjectExplorer.ProjectConfiguration.DisplayName"), QLatin1String("Deploy"));
        deploySteps.insert(QLatin1String("ProjectExplorer.ProjectConfiguration.Id"), QLatin1String("ProjectExplorer.BuildSteps.Deploy"));
        newDc.insert(QLatin1String("ProjectExplorer.BuildConfiguration.BuildStepList.0"), deploySteps);
        newDc.insert(QLatin1String("ProjectExplorer.BuildConfiguration.BuildStepListCount"), 1);
        newDc.insert(QLatin1String("ProjectExplorer.ProjectConfiguration.DisplayName"), deploymentName);
        newDc.insert(QLatin1String("ProjectExplorer.ProjectConfiguration.Id"), QLatin1String("ProjectExplorer.DefaultDeployConfiguration"));

        newTarget.insert(QLatin1String("ProjectExplorer.Target.DeployConfigurationCount"), 1);
        newTarget.insert(QLatin1String("ProjectExplorer.Target.ActiveDeployConfiguration"), 0);
        newTarget.insert(QLatin1String("ProjectExplorer.Target.DeployConfiguration.0"), newDc);
        result.insert(globalKey, newTarget);
    }
    return result;
}

// -------------------------------------------------------------------------
// Version7Handler
// -------------------------------------------------------------------------

// new implementation of DeployConfiguration
QVariantMap Version7Handler::update(Project *, const QVariantMap &map)
{
    QVariantMap result;
    QMapIterator<QString, QVariant> it(map);
    while (it.hasNext()) {
        it.next();
        const QString &globalKey = it.key();
        // check for target info
        if (!globalKey.startsWith(QLatin1String("ProjectExplorer.Project.Target."))) {
            result.insert(globalKey, it.value());
            continue;
        }
        const QVariantMap &originalTarget = it.value().toMap();
        // check for symbian device target
        if (originalTarget.value(QLatin1String("ProjectExplorer.ProjectConfiguration.Id"))
                != QLatin1String("Qt4ProjectManager.Target.S60DeviceTarget") ) {
            result.insert(globalKey, originalTarget);
            continue;
        }

        QVariantMap newTarget;
        QMapIterator<QString, QVariant> targetIt(originalTarget);
        while (targetIt.hasNext()) {
            targetIt.next();
            const QString &targetKey = targetIt.key();
            if (targetKey.startsWith(QLatin1String("ProjectExplorer.Target.RunConfiguration."))) {
                QVariantMap newRunConfiguration;
                const QVariantMap &originalRc = targetIt.value().toMap();

                QMapIterator<QString, QVariant> rcIt(originalRc);
                while (rcIt.hasNext()) {
                    rcIt.next();
                    const QString &rcKey = rcIt.key();
                    // remove installation related data from RunConfiguration
                    if (rcKey.startsWith(QLatin1String("Qt4ProjectManager.S60DeviceRunConfiguration.InstallationDriveLetter"))) {
                        continue;
                    }
                    if (rcKey.startsWith(QLatin1String("Qt4ProjectManager.S60DeviceRunConfiguration.SerialPortName"))) {
                        continue;
                    }
                    if (rcKey.startsWith(QLatin1String("Qt4ProjectManager.S60DeviceRunConfiguration.SilentInstall"))) {
                        continue;
                    }
                    newRunConfiguration.insert(rcKey, rcIt.value());
                }
                newTarget.insert(targetKey, newRunConfiguration);
            } else {
                newTarget.insert(targetKey, targetIt.value());
                continue;
            }
        }
        result.insert(globalKey, newTarget);
    }
    return result;
}

// -------------------------------------------------------------------------
// Version8Handler
// -------------------------------------------------------------------------

// Argument list reinterpretation

static const char * const argListKeys[] = {
    "ProjectExplorer.Project.Target.",
        "ProjectExplorer.Target.BuildConfiguration."
        "|ProjectExplorer.Target.DeployConfiguration.",
            "ProjectExplorer.BuildConfiguration.BuildStepList.",
                "ProjectExplorer.BuildStepList.Step.",
                    "GenericProjectManager.GenericMakeStep.MakeArguments",
                    "QtProjectManager.QMakeBuildStep.QMakeArguments",
                    "Qt4ProjectManager.MakeStep.MakeArguments",
                    "CMakeProjectManager.MakeStep.AdditionalArguments",
                    0,
                0,
            0,
        "ProjectExplorer.Target.RunConfiguration.",
            "ProjectExplorer.CustomExecutableRunConfiguration.Arguments",
            "Qt4ProjectManager.Qt4RunConfiguration.CommandLineArguments",
            "CMakeProjectManager.CMakeRunConfiguration.Arguments",
            0,
        0,
    0
};

static const char * const lameArgListKeys[] = {
    "ProjectExplorer.Project.Target.",
        "ProjectExplorer.Target.BuildConfiguration."
        "|ProjectExplorer.Target.DeployConfiguration.",
            "ProjectExplorer.BuildConfiguration.BuildStepList.",
                "ProjectExplorer.BuildStepList.Step.",
                    "ProjectExplorer.ProcessStep.Arguments",
                    0,
                0,
            0,
        "ProjectExplorer.Target.RunConfiguration.",
            "Qt4ProjectManager.MaemoRunConfiguration.Arguments",
            "Qt4ProjectManager.S60DeviceRunConfiguration.CommandLineArguments",
            "QmlProjectManager.QmlRunConfiguration.QDeclarativeViewerArguments",
            0,
        0,
    0
};

#ifdef Q_OS_UNIX
inline static bool isSpecialChar(ushort c)
{
    // Chars that should be quoted (TM). This includes:
    static const uchar iqm[] = {
        0xff, 0xff, 0xff, 0xff, 0xdf, 0x07, 0x00, 0xd8,
        0x00, 0x00, 0x00, 0x38, 0x01, 0x00, 0x00, 0x78
    }; // 0-32 \'"$`<>|;&(){}*?#!~[]

    return (c < sizeof(iqm) * 8) && (iqm[c / 8] & (1 << (c & 7)));
}

inline static bool hasSpecialChars(const QString &arg)
{
    for (int x = arg.length() - 1; x >= 0; --x)
        if (isSpecialChar(arg.unicode()[x].unicode()))
            return true;
    return false;
}
#endif

// These were split according to sane (even if a bit arcane) rules
static QVariant version8ArgNodeHandler(const QVariant &var)
{
    QString ret;
    foreach (const QVariant &svar, var.toList()) {
#ifdef Q_OS_UNIX
        // We don't just addArg, so we don't disarm existing env expansions.
        // This is a bit fuzzy logic ...
        QString s = svar.toString();
        s.replace(QLatin1Char('\\'), QLatin1String("\\\\"));
        s.replace(QLatin1Char('"'), QLatin1String("\\\""));
        s.replace(QLatin1Char('`'), QLatin1String("\\`"));
        if (s != svar.toString() || hasSpecialChars(s))
            s.prepend(QLatin1Char('"')).append(QLatin1Char('"'));
        Utils::QtcProcess::addArgs(&ret, s);
#else
        // Under windows, env expansions cannot be quoted anyway.
        Utils::QtcProcess::addArg(&ret, svar.toString());
#endif
    }
    return QVariant(ret);
}

// These were just split on whitespace
static QVariant version8LameArgNodeHandler(const QVariant &var)
{
    QString ret;
    foreach (const QVariant &svar, var.toList())
        Utils::QtcProcess::addArgs(&ret, svar.toString());
    return QVariant(ret);
}

// Environment variable reinterpretation

static const char * const envExpandedKeys[] = {
    "ProjectExplorer.Project.Target.",
        "ProjectExplorer.Target.BuildConfiguration."
        "|ProjectExplorer.Target.DeployConfiguration.",
            "ProjectExplorer.BuildConfiguration.BuildStepList.",
                "ProjectExplorer.BuildStepList.Step.",
                    "ProjectExplorer.ProcessStep.WorkingDirectory",
                    "ProjectExplorer.ProcessStep.Command",
                    "ProjectExplorer.ProcessStep.Arguments",
                    "GenericProjectManager.GenericMakeStep.MakeCommand",
                    "GenericProjectManager.GenericMakeStep.MakeArguments",
                    "GenericProjectManager.GenericMakeStep.BuildTargets",
                    "QtProjectManager.QMakeBuildStep.QMakeArguments",
                    "Qt4ProjectManager.MakeStep.MakeCommand",
                    "Qt4ProjectManager.MakeStep.MakeArguments",
                    "CMakeProjectManager.MakeStep.AdditionalArguments",
                    "CMakeProjectManager.MakeStep.BuildTargets",
                    0,
                0,
            "Qt4ProjectManager.Qt4BuildConfiguration.BuildDirectory",
            0,
        "ProjectExplorer.Target.RunConfiguration.",
            "ProjectExplorer.CustomExecutableRunConfiguration.WorkingDirectory",
            "ProjectExplorer.CustomExecutableRunConfiguration.Executable",
            "ProjectExplorer.CustomExecutableRunConfiguration.Arguments",
            "Qt4ProjectManager.Qt4RunConfiguration.UserWorkingDirectory",
            "Qt4ProjectManager.Qt4RunConfiguration.CommandLineArguments",
            "Qt4ProjectManager.MaemoRunConfiguration.Arguments",
            "Qt4ProjectManager.S60DeviceRunConfiguration.CommandLineArguments",
            "CMakeProjectManager.CMakeRunConfiguration.UserWorkingDirectory",
            "CMakeProjectManager.CMakeRunConfiguration.Arguments",
            0,
        0,
    0
};

static QString version8NewVar(const QString &old)
{
    QString ret = old;
#ifdef Q_OS_UNIX
    ret.prepend(QLatin1String("${"));
    ret.append(QLatin1Char('}'));
#else
    ret.prepend(QLatin1Char('%'));
    ret.append(QLatin1Char('%'));
#endif
    return ret;
}

// Translate DOS-like env var expansions into Unix-like ones and vice versa.
// On the way, change {SOURCE,BUILD}DIR env expansions to %{}-expandos
static QVariant version8EnvNodeTransform(const QVariant &var)
{
    QString result = var.toString();

    result.replace(QRegExp(QLatin1String("%SOURCEDIR%|\\$(SOURCEDIR\\b|\\{SOURCEDIR\\})")),
                   QLatin1String("%{sourceDir}"));
    result.replace(QRegExp(QLatin1String("%BUILDDIR%|\\$(BUILDDIR\\b|\\{BUILDDIR\\})")),
                   QLatin1String("%{buildDir}"));
#ifdef Q_OS_UNIX
    for (int vStart = -1, i = 0; i < result.length(); ) {
        QChar c = result.at(i++);
        if (c == QLatin1Char('%')) {
            if (vStart > 0 && vStart < i - 1) {
                QString nv = version8NewVar(result.mid(vStart, i - 1 - vStart));
                result.replace(vStart - 1, i - vStart + 1, nv);
                i = vStart - 1 + nv.length();
                vStart = -1;
            } else {
                vStart = i;
            }
        } else if (vStart > 0) {
            // Sanity check so we don't catch too much garbage
            if (!c.isLetterOrNumber() && c != QLatin1Char('_'))
                vStart = -1;
        }
    }
#else
    enum { BASE, OPTIONALVARIABLEBRACE, VARIABLE, BRACEDVARIABLE } state = BASE;
    int vStart = -1;

    for (int i = 0; i < result.length();) {
        QChar c = result.at(i++);
        if (state == BASE) {
            if (c == QLatin1Char('$'))
                state = OPTIONALVARIABLEBRACE;
        } else if (state == OPTIONALVARIABLEBRACE) {
            if (c == QLatin1Char('{')) {
                state = BRACEDVARIABLE;
                vStart = i;
            } else if (c.isLetterOrNumber() || c == QLatin1Char('_')) {
                state = VARIABLE;
                vStart = i - 1;
            } else {
                state = BASE;
            }
        } else if (state == BRACEDVARIABLE) {
            if (c == QLatin1Char('}')) {
                QString nv = version8NewVar(result.mid(vStart, i - 1 - vStart));
                result.replace(vStart - 2, i - vStart + 2, nv);
                i = vStart + nv.length();
                state = BASE;
            }
        } else if (state == VARIABLE) {
            if (!c.isLetterOrNumber() && c != QLatin1Char('_')) {
                QString nv = version8NewVar(result.mid(vStart, i - 1 - vStart));
                result.replace(vStart - 1, i - vStart, nv);
                i = vStart - 1 + nv.length(); // On the same char - could be next expansion.
                state = BASE;
            }
        }
    }
    if (state == VARIABLE) {
        QString nv = version8NewVar(result.mid(vStart));
        result.truncate(vStart - 1);
        result += nv;
    }
#endif

    return QVariant(result);
}

static QVariant version8EnvNodeHandler(const QVariant &var)
{
    if (var.type() != QVariant::List)
        return version8EnvNodeTransform(var);

    QVariantList vl;
    foreach (const QVariant &svar, var.toList())
        vl << version8EnvNodeTransform(svar);
    return vl;
}

// VariableManager expando reinterpretation

static const char * const varExpandedKeys[] = {
    "ProjectExplorer.Project.Target.",
        "ProjectExplorer.Target.BuildConfiguration."
        "|ProjectExplorer.Target.DeployConfiguration.",
            "ProjectExplorer.BuildConfiguration.BuildStepList.",
                "ProjectExplorer.BuildStepList.Step.",
                    "GenericProjectManager.GenericMakeStep.MakeCommand",
                    "GenericProjectManager.GenericMakeStep.MakeArguments",
                    "GenericProjectManager.GenericMakeStep.BuildTargets",
                    0,
                0,
            0,
        0,
    0
};

// Translate old-style ${} var expansions into new-style %{} ones
static QVariant version8VarNodeTransform(const QVariant &var)
{
    static const char * const vars[] = {
        "absoluteFilePath",
        "absolutePath",
        "baseName",
        "canonicalPath",
        "canonicalFilePath",
        "completeBaseName",
        "completeSuffix",
        "fileName",
        "filePath",
        "path",
        "suffix"
    };
    static QSet<QString> map;
    if (map.isEmpty())
        for (unsigned i = 0; i < sizeof(vars)/sizeof(vars[0]); ++i)
            map.insert(QLatin1String("CURRENT_DOCUMENT:") + QLatin1String(vars[i]));

    QString str = var.toString();
    int pos = 0;
    forever {
        int openPos = str.indexOf(QLatin1String("${"), pos);
        if (openPos < 0)
            break;
        int varPos = openPos + 2;
        int closePos = str.indexOf(QLatin1Char('}'), varPos);
        if (closePos < 0)
            break;
        if (map.contains(str.mid(varPos, closePos - varPos)))
            str[openPos] = QLatin1Char('%');
        pos = closePos + 1;
    }
    return QVariant(str);
}

static QVariant version8VarNodeHandler(const QVariant &var)
{
    if (var.type() != QVariant::List)
        return version8VarNodeTransform(var);

    QVariantList vl;
    foreach (const QVariant &svar, var.toList())
        vl << version8VarNodeTransform(svar);
    return vl;
}

QVariantMap Version8Handler::update(Project *, const QVariantMap &map)
{
    const char * const *p1 = argListKeys;
    QVariantMap rmap1 = processHandlerNodes(buildHandlerNodes(&p1), map, version8ArgNodeHandler);
    const char * const *p2 = lameArgListKeys;
    QVariantMap rmap2 = processHandlerNodes(buildHandlerNodes(&p2), rmap1, version8LameArgNodeHandler);
    const char * const *p3 = envExpandedKeys;
    QVariantMap rmap3 = processHandlerNodes(buildHandlerNodes(&p3), rmap2, version8EnvNodeHandler);
    const char * const *p4 = varExpandedKeys;
    return processHandlerNodes(buildHandlerNodes(&p4), rmap3, version8VarNodeHandler);
}

QVariantMap Version9Handler::update(Project *project, const QVariantMap &map)
{
    Q_UNUSED(project);

    QVariantMap result;
    QMapIterator<QString, QVariant> globalIt(map);
    while (globalIt.hasNext()) {
        globalIt.next();
        const QString &globalKey = globalIt.key();
        // check for target info
        if (!globalKey.startsWith(QLatin1String("ProjectExplorer.Project.Target."))) {
            result.insert(globalKey, globalIt.value());
            continue;
        }

        const QVariantMap &origTargetMap = globalIt.value().toMap();
        const QString targetIdKey
            = QLatin1String("ProjectExplorer.ProjectConfiguration.Id");
        // check for maemo device target
        if (origTargetMap.value(targetIdKey)
                != QLatin1String("Qt4ProjectManager.Target.MaemoDeviceTarget")
            && origTargetMap.value(targetIdKey)
                != QLatin1String("Qt4ProjectManager.Target.HarmattanDeviceTarget")
            && origTargetMap.value(targetIdKey)
                != QLatin1String("Qt4ProjectManager.Target.MeegoDeviceTarget"))
        {
            result.insert(globalKey, origTargetMap);
            continue;
        }

        QVariantMap newTargetMap;
        QMapIterator<QString, QVariant> targetIt(origTargetMap);
        while (targetIt.hasNext()) {
            targetIt.next();
            if (!targetIt.key().startsWith(QLatin1String("ProjectExplorer.Target.DeployConfiguration."))) {
                newTargetMap.insert(targetIt.key(), targetIt.value());
                continue;
            }

            QVariantMap deployConfMap = targetIt.value().toMap();
            deployConfMap.insert(QLatin1String("ProjectExplorer.ProjectConfiguration.Id"),
                QLatin1String("2.2MaemoDeployConfig"));
            newTargetMap.insert(targetIt.key(), deployConfMap);
        }
        result.insert(globalKey, newTargetMap);
    }
    return result;
}

QVariantMap Version10Handler::update(Project *project, const QVariantMap &map)
{
    Q_UNUSED(project);
    QList<Change> changes;
    changes.append(qMakePair(QLatin1String("ProjectExplorer.ProcessStep.Enabled"),
                             QLatin1String("ProjectExplorer.BuildStep.Enabled")));
    return renameKeys(changes, QVariantMap(map));
}

Version11Handler::Version11Handler()
{
    parseQtversionFile();
    parseToolChainFile();
}

Version11Handler::~Version11Handler()
{
    ProfileManager *pm = ProfileManager::instance();
    if (!pm) // Can happen during teardown!
        return;
    QList<Profile *> knownProfiles = pm->profiles();
    foreach (Profile *p, m_targets.keys()) {
        if (!knownProfiles.contains(p))
            delete p;
    }
    m_targets.clear();
}

QVariantMap Version11Handler::update(Project *project, const QVariantMap &map)
{
    Q_UNUSED(project);
    QVariantMap result;
    ProfileManager *pm = ProfileManager::instance();

    foreach (Profile *p, pm->profiles())
        m_targets.insert(p, QVariantMap());

    QMapIterator<QString, QVariant> globalIt(map);
    int activeTarget = map.value(QLatin1String("ProjectExplorer.Project.ActiveTarget"), 0).toInt();

    while (globalIt.hasNext()) {
        globalIt.next();
        const QString &globalKey = globalIt.key();
        // Keep everything but targets:
        if (!globalKey.startsWith(QLatin1String("ProjectExplorer.Project.Target."))) {
            result.insert(globalKey, globalIt.value());
            continue;
        }

        // Update Targets:
        const QVariantMap &target = globalIt.value().toMap();
        int targetPos = globalKey.mid(globalKey.lastIndexOf(QLatin1Char('.'))).toInt();

        QVariantMap extraTargetData;
        QList<QVariantMap> bcs;
        int activeBc = -1;
        QList<QVariantMap> dcs;
        int activeDc = -1;
        QList<QVariantMap> rcs;
        int activeRc = -1;

        // Read old target:
        QMapIterator<QString, QVariant> targetIt(target);
        while (targetIt.hasNext()) {
            targetIt.next();
            const QString &targetKey = targetIt.key();
    QList<QVariantMap> newTargets;
            // BuildConfigurations:
            if (targetKey == QLatin1String("ProjectExplorer.Target.ActiveBuildConfiguration"))
                activeBc = targetIt.value().toInt();
            else if (targetKey == QLatin1String("ProjectExplorer.Target.BuildConfigurationCount"))
                continue;
            else if (targetKey.startsWith(QLatin1String("ProjectExplorer.Target.BuildConfiguration.")))
                bcs.append(targetIt.value().toMap());
            else

            // DeployConfigurations:
            if (targetKey == QLatin1String("ProjectExplorer.Target.ActiveDeployConfiguration"))
                activeDc = targetIt.value().toInt();
            else if (targetKey == QLatin1String("ProjectExplorer.Target.DeployConfigurationCount"))
                continue;
            else if (targetKey.startsWith(QLatin1String("ProjectExplorer.Target.DeployConfiguration.")))
                dcs.append(targetIt.value().toMap());
            else

            // RunConfigurations:
            if (targetKey == QLatin1String("ProjectExplorer.Target.ActiveRunConfiguration"))
                activeRc = targetIt.value().toInt();
            else if (targetKey == QLatin1String("ProjectExplorer.Target.RunConfigurationCount"))
                continue;
            else if (targetKey.startsWith(QLatin1String("ProjectExplorer.Target.RunConfiguration.")))
                rcs.append(targetIt.value().toMap());

            // Rest (the target's ProjectConfiguration    QList<QVariantMap> newTargets; related settings only as there is nothing else)
            else
                extraTargetData.insert(targetKey, targetIt.value());
        }

        const QString targetId = extraTargetData.value(QLatin1String("ProjectExplorer.ProjectConfiguration.Id")).toString();
        // Check each BCs/DCs and create profiles as needed
        int bcPos = 0;
        foreach (const QVariantMap &bc, bcs) {
            const QString targetId = extraTargetData.value(QLatin1String("ProjectExplorer.ProjectConfiguration.Id")).toString();
            Profile *tmp = new Profile;
            tmp->setDisplayName(extraTargetData.value(QLatin1String("ProjectExplorer.ProjectConfiguration.DisplayName")).toString());

            if (targetId == QLatin1String("Qt4ProjectManager.Target.AndroidDeviceTarget"))
                tmp->setIconPath(QLatin1String(":/android/images/QtAndroid.png"));
            else if (targetId == QLatin1String("Qt4ProjectManager.Target.HarmattanDeviceTarget"))
                tmp->setIconPath(QLatin1String(":/projectexplorer/images/MaemoDevice.png"));
            else if (targetId == QLatin1String("Qt4ProjectManager.Target.MaemoDeviceTarget"))
                tmp->setIconPath(QLatin1String(":/projectexplorer/images/MaemoDevice.png"));
            else if (targetId == QLatin1String("Qt4ProjectManager.Target.MeegoDeviceTarget"))
                tmp->setIconPath(QLatin1String(":/projectexplorer/images/MaemoDevice.png"));
            else if (targetId == QLatin1String("Qt4ProjectManager.Target.S60DeviceTarget"))
                tmp->setIconPath(QLatin1String(":/projectexplorer/images/SymbianDevice.png"));
            else if (targetId == QLatin1String("Qt4ProjectManager.Target.QtSimulatorTarget"))
                tmp->setIconPath(QLatin1String(":/projectexplorer/images/SymbianEmulator.png"));
            // use default desktop icon

            // Tool chain
            QString tcId = bc.value(QLatin1String("CMakeProjectManager.CMakeBuildConfiguration.ToolChain")).toString();
            if (tcId.isEmpty())
                tcId = bc.value(QLatin1String("ProjectExplorer.BuildCOnfiguration.ToolChain")).toString();
            tmp->setValue(Core::Id("PE.Profile.ToolChain"), tcId);

            // QtVersion
            int qtVersionId = bc.value(QLatin1String("Qt4ProjectManager.Qt4BuildConfiguration.QtVersionId"), -1).toInt();
            tmp->setValue(Core::Id("QtSupport.QtInformation"), qtVersionId);

            // Debugger + mkspec
            if (m_toolChainExtras.contains(tcId)) {
                tmp->setValue(Core::Id("Debugger.Information"), m_toolChainExtras.value(tcId).m_debugger);
                tmp->setValue(Core::Id("QtPM4.mkSpecInformation"), m_toolChainExtras.value(tcId).m_mkspec);
            }

            // SysRoot
            if (m_qtVersionExtras.contains(qtVersionId))
                tmp->setValue(Core::Id("PE.Profile.SysRoot"), m_qtVersionExtras.value(qtVersionId));

            // Device
            if (dcs.isEmpty()) {
                QByteArray devId;
                if (targetId == QLatin1String("Qt4ProjectManager.Target.S60DeviceTarget"))
                    devId = QByteArray("Symbian Device");
                else
                    devId = QByteArray("Desktop Device");

                tmp->setValue(Core::Id("PE.Profile.Device"), devId);
            } else {
                foreach (const QVariantMap &dc, dcs) {
                    QByteArray devId = dc.value(QLatin1String("Qt4ProjectManager.MaemoRunConfiguration.DeviceId")).toString().toUtf8();
                    if (devId.isEmpty())
                        devId = QByteArray("Desktop Device");
                    tmp->setValue(Core::Id("PE.Profile.Device"), devId);
                } // dcs
            }

            addBuildConfiguration(targetId, tmp, activeTarget == targetPos, bc, bcPos == activeBc);

            ++bcPos;
        } // bcs

        addOtherConfiguration(targetId, dcs, activeDc, rcs, activeRc);
    }

    int newPos = 0;
    QList<Profile *> knownProfiles = pm->profiles();
    // Generate new target data:
    foreach (Profile *p, m_targets.keys()) {
        QVariantMap data = m_targets.value(p);
        if (data.isEmpty())
            continue;

        if (!knownProfiles.contains(p))
            pm->registerProfile(p);

        data.insert(QLatin1String("ProjectExplorer.ProjectConfiguration.Id"), p->id().name());
        data.insert(QLatin1String("ProjectExplorer.Target.Profile"), p->id().name());
        data.insert(QLatin1String("ProjectExplorer.ProjectConfiguration.DisplayName"), p->displayName());
        data.insert(QLatin1String("ProjectExplorer.ProjectConfiguration.DefaultDisplayName"), p->displayName());

        result.insert(QString::fromLatin1("ProjectExplorer.Project.Target.") + QString::number(newPos), data);
        if (data.value(QLatin1String("IsActive"), false).toBool())
            result.insert(QLatin1String("ProjectExplorer.Project.ActiveTarget"), newPos);
        ++newPos;
    }
    result.insert(QLatin1String("ProjectExplorer.Project.TargetCount"), newPos);

    return result;
}

void Version11Handler::addBuildConfiguration(const QString &origTarget, Profile *p, bool targetActive,
                                             const QVariantMap &bc, bool bcActive)
{
    foreach (Profile *i, m_targets.keys()) {
        if (*i == *p) {
            delete p;
            p = i;
        }
    }
    QVariantMap merged = m_targets.value(p);

    int bcCount = merged.value(QLatin1String("ProjectExplorer.Target.BuildConfigurationCount"), 0).toInt();
    merged.insert(QString::fromLatin1("ProjectExplorer.Target.BuildConfiguration.") + QString::number(bcCount), bc);
    if (bcActive)
        merged.insert(QLatin1String("ProjectExplorer.Target.ActiveBuildConfiguration"), bcCount);
    merged.insert(QLatin1String("ProjectExplorer.Target.BuildConfigurationCount"), bcCount + 1);

    if (targetActive && bcActive)
        merged.insert(QLatin1String("Update.IsActive"), true);
    merged.insert(QLatin1String("Update.OriginalTarget"), origTarget);

    m_targets.insert(p, merged);
}

void Version11Handler::addOtherConfiguration(const QString &origTarget, const QList<QVariantMap> &dcs, int activeDc, const QList<QVariantMap> &rcs, int activeRc)
{
    foreach (Profile *tmp, m_targets.keys()) {
        QVariantMap data = m_targets.value(tmp);
        if (data.isEmpty())
            continue;
        const QString dataTarget = data.value(QLatin1String("Update.OriginalTarget")).toString();
        if (dataTarget != origTarget)
            continue;

        int dcCount = dcs.count();
        data.insert(QLatin1String("ProjectExplorer.Target.DeployConfigurationCount"), dcCount);
        for (int i = 0; i < dcCount; ++i)
            data.insert(QString::fromLatin1("ProjectExplorer.Target.DeployConfiguration.") + QString::number(i), dcs.at(i));
        data.insert(QLatin1String("ProjectExplorer.Target.ActiveDeployConfiguration"), activeDc);

        int rcCount = rcs.count();
        data.insert(QLatin1String("ProjectExplorer.Target.RunConfigurationCount"), rcCount);
        for (int i = 0; i < rcCount; ++i)
            data.insert(QString::fromLatin1("ProjectExplorer.Target.RunConfiguration.") + QString::number(i), rcs.at(i));
        data.insert(QLatin1String("ProjectExplorer.Target.ActiveRunConfiguration"), activeRc);

        m_targets.insert(tmp, data);
    }
}

void Version11Handler::parseQtversionFile()
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    QFileInfo settingsLocation(pm->settings()->fileName());
    QString fileName = settingsLocation.absolutePath() + QLatin1String("/qtversion.xml");
    Utils::PersistentSettingsReader reader;
    if (!reader.load(fileName))
        return;
    QVariantMap data = reader.restoreValues();

    int count = data.value(QLatin1String("QtVersion.Count"), 0).toInt();
    for (int i = 0; i < count; ++i) {
        const QString key = QString::fromLatin1("QtVersion.") + QString::number(i);
        if (!data.contains(key))
            continue;

        const QVariantMap qtversionMap = data.value(key).toMap();
        QString sysRoot = qtversionMap.value(QLatin1String("SystemRoot")).toString();
        int id = qtversionMap.value(QLatin1String("Id")).toInt();
        if (id > -1 && !sysRoot.isEmpty())
            m_qtVersionExtras.insert(id, sysRoot);
    }
}

void Version11Handler::parseToolChainFile()
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    QFileInfo settingsLocation(pm->settings()->fileName());
    QString fileName = settingsLocation.absolutePath() + QLatin1String("/toolChains.xml");
    Utils::PersistentSettingsReader reader;
    if (!reader.load(fileName))
        return;
    QVariantMap data = reader.restoreValues();
    int count = data.value(QLatin1String("ToolChain.Count"), 0).toInt();
    for (int i = 0; i < count; ++i) {
        const QString key = QString::fromLatin1("ToolChain.") + QString::number(i);
        if (!data.contains(key))
            continue;

        const QVariantMap tcMap = data.value(key).toMap();
        QString id = tcMap.value(QLatin1String("ProjectExplorer.ToolChain.Id")).toString();
        if (id.isEmpty())
            continue;
        QString mkspec = tcMap.value(QLatin1String("ProjectExplorer.ToolChain.MkSpecOverride")).toString();
        QString debugger = tcMap.value(QLatin1String("ProjectExplorer.GccToolChain.Debugger")).toString();

        m_toolChainExtras.insert(id, ToolChainExtraData(mkspec, debugger));
    }
}

