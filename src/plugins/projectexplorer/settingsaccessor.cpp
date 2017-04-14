/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "settingsaccessor.h"

#include "abi.h"
#include "devicesupport/devicemanager.h"
#include "project.h"
#include "projectexplorer.h"
#include "projectexplorersettings.h"
#include "toolchain.h"
#include "toolchainmanager.h"
#include "kit.h"
#include "kitmanager.h"

#include <coreplugin/icore.h>
#include <utils/persistentsettings.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <QApplication>
#include <QDir>
#include <QRegExp>

using namespace Utils;

namespace {
static QString generateSuffix(const QString &alt1, const QString &alt2)
{
    QString suffix = alt1;
    if (suffix.isEmpty())
        suffix = alt2;
    suffix.replace(QRegExp(QLatin1String("[^a-zA-Z0-9_.-]")), QString(QLatin1Char('_'))); // replace fishy characters:
    if (!suffix.startsWith(QLatin1Char('.')))
        suffix.prepend(QLatin1Char('.'));
    return suffix;
}
} // end namespace

namespace ProjectExplorer {
namespace Internal {

// --------------------------------------------------------------------
// VersionUpgrader:
// --------------------------------------------------------------------
// Handles updating a QVariantMap from version() - 1 to version()
class VersionUpgrader
{
public:
    VersionUpgrader() { }
    virtual ~VersionUpgrader() { }

    virtual int version() const = 0;
    virtual QString backupExtension() const = 0;

    virtual QVariantMap upgrade(const QVariantMap &data) = 0;

protected:
    typedef QPair<QLatin1String,QLatin1String> Change;
    QVariantMap renameKeys(const QList<Change> &changes, QVariantMap map) const;
};

/*!
 * Performs a simple renaming of the listed keys in \a changes recursively on \a map.
 */
QVariantMap VersionUpgrader::renameKeys(const QList<Change> &changes, QVariantMap map) const
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
using namespace ProjectExplorer::Internal;

namespace {

const char USER_STICKY_KEYS_KEY[] = "UserStickyKeys";
const char SHARED_SETTINGS[] = "SharedSettings";
const char ENVIRONMENT_ID_KEY[] = "EnvironmentId";
const char VERSION_KEY[] = "Version";
const char ORIGINAL_VERSION_KEY[] = "OriginalVersion";

// for compatibility with QtC 3.1 and older:
const char OBSOLETE_VERSION_KEY[] = "ProjectExplorer.Project.Updater.FileVersion";

// Version 1 is used in master post Qt Creator 1.3.x.
// It was never used in any official release but is required for the
// transition to later versions (which introduce support for targets).
class UserFileVersion1Upgrader : public VersionUpgrader
{
public:
    UserFileVersion1Upgrader(UserFileAccessor *a) : m_accessor(a) { }
    int version() const { return 1; }
    QString backupExtension() const { return QLatin1String("1.3+git"); }
    QVariantMap upgrade(const QVariantMap &map);

private:
    struct TargetDescription
    {
        TargetDescription(const QString &tid, const QString &dn) :
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

    UserFileAccessor *m_accessor;
};

// Version 2 is used in master post Qt Creator 2.0 alpha.
class UserFileVersion2Upgrader : public VersionUpgrader
{
public:
    int version() const { return 2; }
    QString backupExtension() const { return QLatin1String("2.0-alpha+git"); }
    QVariantMap upgrade(const QVariantMap &map);
};

// Version 3 reflect the move of symbian signing from run to build step.
class UserFileVersion3Upgrader : public VersionUpgrader
{
public:
    int version() const { return 3; }
    QString backupExtension() const { return QLatin1String("2.0-alpha2+git"); }
    QVariantMap upgrade(const QVariantMap &map);
};

// Version 4 reflects the introduction of deploy steps
class UserFileVersion4Upgrader : public VersionUpgrader
{
public:
    int version() const { return 4; }
    QString backupExtension() const { return QLatin1String("2.1pre1"); }
    QVariantMap upgrade(const QVariantMap &map);
};

// Version 5 reflects the introduction of new deploy steps for Symbian/Maemo
class UserFileVersion5Upgrader : public VersionUpgrader
{
public:
    int version() const { return 5; }
    QString backupExtension() const { return QLatin1String("2.1pre2"); }
    QVariantMap upgrade(const QVariantMap &map);
};

// Version 6 reflects the introduction of new deploy steps for Symbian/Maemo
class UserFileVersion6Upgrader : public VersionUpgrader
{
public:
    int version() const { return 6; }
    QString backupExtension() const { return QLatin1String("2.1pre3"); }
    QVariantMap upgrade(const QVariantMap &map);
};

// Version 7 reflects the introduction of new deploy configuration for Symbian
class UserFileVersion7Upgrader : public VersionUpgrader
{
public:
    int version() const { return 7; }
    QString backupExtension() const { return QLatin1String("2.1pre4"); }
    QVariantMap upgrade(const QVariantMap &map);
};

// Version 8 reflects the change of environment variable expansion rules,
// turning some env variables into expandos, the change of argument quoting rules,
// and the change of VariableManager's expansion syntax.
class UserFileVersion8Upgrader : public VersionUpgrader
{
public:
    int version() const { return 8; }
    QString backupExtension() const {
        // pre5 because we renamed 2.2 to 2.1 later, so people already have 2.2pre4 files
        return QLatin1String("2.2pre5");
    }
    QVariantMap upgrade(const QVariantMap &map);
};

// Version 9 reflects the refactoring of the Maemo deploy step.
class UserFileVersion9Upgrader : public VersionUpgrader
{
public:
    int version() const { return 9; }
    QString backupExtension() const { return QLatin1String("2.3pre1"); }
    QVariantMap upgrade(const QVariantMap &map);
};

// Version 10 introduces disabling buildsteps, and handles upgrading custom process steps
class UserFileVersion10Upgrader : public VersionUpgrader
{
public:
    int version() const { return 10; }
    QString backupExtension() const { return QLatin1String("2.5pre1"); }
    QVariantMap upgrade(const QVariantMap &map);
};

// Version 11 introduces kits
class UserFileVersion11Upgrader : public VersionUpgrader
{
public:
    UserFileVersion11Upgrader(UserFileAccessor *a) : m_accessor(a) { }
    ~UserFileVersion11Upgrader();

    int version() const { return 11; }
    QString backupExtension() const { return QLatin1String("2.6pre1"); }
    QVariantMap upgrade(const QVariantMap &map);

private:
    Kit *uniqueKit(Kit *k);
    void addBuildConfiguration(Kit *k, const QVariantMap &bc, int bcPos, int bcCount);
    void addDeployConfiguration(Kit *k, const QVariantMap &dc, int dcPos, int dcActive);
    void addRunConfigurations(Kit *k,
                              const QMap<int, QVariantMap> &rcs, int activeRc, const QString &projectDir);

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

    QHash<Kit *, QVariantMap> m_targets;
    UserFileAccessor *m_accessor;
};

// Version 12 reflects the move of environment settings from CMake/Qt4/Custom into
// LocalApplicationRunConfiguration
class UserFileVersion12Upgrader : public VersionUpgrader
{
public:
    int version() const { return 12; }
    QString backupExtension() const { return QLatin1String("2.7pre1"); }
    QVariantMap upgrade(const QVariantMap &map);
};

// Version 13 reflects the move of environment settings from LocalApplicationRunConfiguration
// into the EnvironmentAspect
class UserFileVersion13Upgrader : public VersionUpgrader
{
public:
    int version() const { return 13; }
    QString backupExtension() const { return QLatin1String("2.8"); }
    QVariantMap upgrade(const QVariantMap &map);
};

// Version 14 Move builddir into BuildConfiguration
class UserFileVersion14Upgrader : public VersionUpgrader
{
public:
    int version() const { return 14; }
    QString backupExtension() const { return QLatin1String("3.0-pre1"); }
    QVariantMap upgrade(const QVariantMap &map);
};

// Version 15 Use settingsaccessor based class for user file reading/writing
class UserFileVersion15Upgrader : public VersionUpgrader
{
public:
    int version() const { return 15; }
    QString backupExtension() const { return QLatin1String("3.2-pre1"); }
    QVariantMap upgrade(const QVariantMap &map);
};

// Version 16 Changed android deployment
class UserFileVersion16Upgrader : public VersionUpgrader
{
public:
    int version() const { return 16; }
    QString backupExtension() const { return QLatin1String("3.3-pre1"); }
    QVariantMap upgrade(const QVariantMap &data);
private:
    class OldStepMaps
    {
    public:
        QString defaultDisplayName;
        QString displayName;
        QVariantMap androidPackageInstall;
        QVariantMap androidDeployQt;
        bool isEmpty()
        {
            return androidPackageInstall.isEmpty() || androidDeployQt.isEmpty();
        }
    };


    QVariantMap removeAndroidPackageStep(QVariantMap deployMap);
    OldStepMaps extractStepMaps(const QVariantMap &deployMap);
    enum NamePolicy { KeepName, RenameBuildConfiguration };
    QVariantMap insertSteps(QVariantMap buildConfigurationMap,
                            const OldStepMaps &oldStepMap,
                            NamePolicy policy);
};

// Version 17 Apply user sticky keys per map
class UserFileVersion17Upgrader : public VersionUpgrader
{
public:
    int version() const { return 17; }
    QString backupExtension() const { return QLatin1String("3.3-pre2"); }
    QVariantMap upgrade(const QVariantMap &map);

    QVariant process(const QVariant &entry);

private:
    QVariantList m_sticky;
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

// --------------------------------------------------------------------
// UserFileAccessor:
// --------------------------------------------------------------------
UserFileAccessor::UserFileAccessor(Project *project)
    : SettingsAccessor(project)
{
    // Register Upgraders:
    addVersionUpgrader(new UserFileVersion1Upgrader(this));
    addVersionUpgrader(new UserFileVersion2Upgrader);
    addVersionUpgrader(new UserFileVersion3Upgrader);
    addVersionUpgrader(new UserFileVersion4Upgrader);
    addVersionUpgrader(new UserFileVersion5Upgrader);
    addVersionUpgrader(new UserFileVersion6Upgrader);
    addVersionUpgrader(new UserFileVersion7Upgrader);
    addVersionUpgrader(new UserFileVersion8Upgrader);
    addVersionUpgrader(new UserFileVersion9Upgrader);
    addVersionUpgrader(new UserFileVersion10Upgrader);
    addVersionUpgrader(new UserFileVersion11Upgrader(this));
    addVersionUpgrader(new UserFileVersion12Upgrader);
    addVersionUpgrader(new UserFileVersion13Upgrader);
    addVersionUpgrader(new UserFileVersion14Upgrader);
    addVersionUpgrader(new UserFileVersion15Upgrader);
    addVersionUpgrader(new UserFileVersion16Upgrader);
    addVersionUpgrader(new UserFileVersion17Upgrader);
}

QVariantMap UserFileAccessor::prepareSettings(const QVariantMap &data) const
{
    // Move from old Version field to new one:
    // This can not be done in a normal upgrader since the version information is needed
    // to decide which upgraders to run
    QVariantMap result = SettingsAccessor::prepareSettings(data);
    const QString obsoleteKey = QLatin1String(OBSOLETE_VERSION_KEY);
    if (data.contains(obsoleteKey)) {
        result = setVersionInMap(result, data.value(obsoleteKey, versionFromMap(data)).toInt());
        result.remove(obsoleteKey);
    }
    return result;
}

QVariantMap UserFileAccessor::prepareToSaveSettings(const QVariantMap &data) const
{
    QVariantMap tmp = SettingsAccessor::prepareToSaveSettings(data);

    // for compatibility with QtC 3.1 and older:
    tmp.insert(QLatin1String(OBSOLETE_VERSION_KEY), currentVersion());
    return tmp;
}

namespace ProjectExplorer {
// --------------------------------------------------------------------
// SettingsAccessorPrivate:
// --------------------------------------------------------------------
class SettingsAccessorPrivate
{
public:
    SettingsAccessorPrivate() :
        m_writer(0)
    { }

    ~SettingsAccessorPrivate()
    {
        qDeleteAll(m_upgraders);
        delete m_writer;
    }

    // The relevant data from the settings currently in use.
    class Settings
    {
    public:
        bool isValid() const;

        QVariantMap map;
        FileName path;
    };

    int firstVersion() const { return m_upgraders.isEmpty() ? -1 : m_upgraders.first()->version(); }
    int lastVersion() const { return m_upgraders.isEmpty() ? -1 : m_upgraders.last()->version(); }
    int currentVersion() const { return lastVersion() + 1; }
    VersionUpgrader *upgrader(const int version) const
    {
        int pos = version - firstVersion();
        if (pos >= 0 && pos < m_upgraders.count())
            return m_upgraders.at(pos);
        return 0;
    }
    Settings bestSettings(const SettingsAccessor *accessor, const FileNameList &pathList);

    QList<VersionUpgrader *> m_upgraders;
    PersistentSettingsWriter *m_writer;
};

// Return path to shared directory for .user files, create if necessary.
static inline QString determineSharedUserFileDir()
{
    const char userFilePathVariable[] = "QTC_USER_FILE_PATH";
    if (!qEnvironmentVariableIsSet(userFilePathVariable))
        return QString();
    const QFileInfo fi(QFile::decodeName(qgetenv(userFilePathVariable)));
    const QString path = fi.absoluteFilePath();
    if (fi.isDir() || fi.isSymLink())
        return path;
    if (fi.exists()) {
        qWarning() << userFilePathVariable << '=' << QDir::toNativeSeparators(path)
            << " points to an existing file";
        return QString();
    }
    QDir dir;
    if (!dir.mkpath(path)) {
        qWarning() << "Cannot create: " << QDir::toNativeSeparators(path);
        return QString();
    }
    return path;
}

static QString sharedUserFileDir()
{
    static const QString sharedDir = determineSharedUserFileDir();
    return sharedDir;
}

// Return a suitable relative path to be created under the shared .user directory.
static QString makeRelative(QString path)
{
    const QChar slash(QLatin1Char('/'));
    // Windows network shares: "//server.domain-a.com/foo' -> 'serverdomainacom/foo'
    if (path.startsWith(QLatin1String("//"))) {
        path.remove(0, 2);
        const int nextSlash = path.indexOf(slash);
        if (nextSlash > 0) {
            for (int p = nextSlash; p >= 0; --p) {
                if (!path.at(p).isLetterOrNumber())
                    path.remove(p, 1);
            }
        }
        return path;
    }
    // Windows drives: "C:/foo' -> 'c/foo'
    if (path.size() > 3 && path.at(1) == QLatin1Char(':')) {
        path.remove(1, 1);
        path[0] = path.at(0).toLower();
        return path;
    }
    if (path.startsWith(slash)) // Standard UNIX paths: '/foo' -> 'foo'
        path.remove(0, 1);
    return path;
}

// Return complete file path of the .user file.
static FileName userFilePath(const Project *project, const QString &suffix)
{
    FileName result;
    const FileName projectFilePath = project->projectFilePath();
    if (sharedUserFileDir().isEmpty()) {
        result = projectFilePath;
    } else {
        // Recreate the relative project file hierarchy under the shared directory.
        // PersistentSettingsWriter::write() takes care of creating the path.
        result = FileName::fromString(sharedUserFileDir());
        result.appendString(QLatin1Char('/') + makeRelative(projectFilePath.toString()));
    }
    result.appendString(suffix);
    return result;
}

} // end namespace

SettingsAccessor::SettingsAccessor(Project *project) :
    m_project(project),
    d(new SettingsAccessorPrivate)
{
    QTC_CHECK(m_project);
    m_userSuffix = generateSuffix(QString::fromLocal8Bit(qgetenv("QTC_EXTENSION")), QLatin1String(".user"));
    m_sharedSuffix = generateSuffix(QString::fromLocal8Bit(qgetenv("QTC_SHARED_EXTENSION")), QLatin1String(".shared"));
}

SettingsAccessor::~SettingsAccessor()
{
    delete d;
}

Project *SettingsAccessor::project() const
{ return m_project; }

namespace {

class Operation {
public:
    virtual ~Operation() { }

    virtual void apply(QVariantMap &userMap, const QString &key, const QVariant &sharedValue) = 0;

    void synchronize(QVariantMap &userMap, const QVariantMap &sharedMap)
    {
        QVariantMap::const_iterator it = sharedMap.begin();
        QVariantMap::const_iterator eit = sharedMap.end();

        for (; it != eit; ++it) {
            const QString &key = it.key();
            if (key == QLatin1String(VERSION_KEY) || key == QLatin1String(ENVIRONMENT_ID_KEY))
                continue;
            const QVariant &sharedValue = it.value();
            const QVariant &userValue = userMap.value(key);
            if (sharedValue.type() == QVariant::Map) {
                if (userValue.type() != QVariant::Map) {
                    // This should happen only if the user manually changed the file in such a way.
                    continue;
                }
                QVariantMap nestedUserMap = userValue.toMap();
                synchronize(nestedUserMap, sharedValue.toMap());
                userMap.insert(key, nestedUserMap);
                continue;
            }
            if (userMap.contains(key) && userValue != sharedValue) {
                apply(userMap, key, sharedValue);
                continue;
            }
        }
    }
};

class MergeSettingsOperation : public Operation
{
public:
    void apply(QVariantMap &userMap, const QString &key, const QVariant &sharedValue)
    {
        // Do not override bookkeeping settings:
        if (key == QLatin1String(ORIGINAL_VERSION_KEY) || key == QLatin1String(VERSION_KEY))
            return;
        if (!userMap.value(QLatin1String(USER_STICKY_KEYS_KEY)).toList().contains(key))
            userMap.insert(key, sharedValue);
    }
};


class TrackStickyness : public Operation
{
public:
    void apply(QVariantMap &userMap, const QString &key, const QVariant &)
    {
        const QString stickyKey = QLatin1String(USER_STICKY_KEYS_KEY);
        QVariantList sticky = userMap.value(stickyKey).toList();
        sticky.append(key);
        userMap.insert(stickyKey, sticky);
    }
};

} // namespace

int SettingsAccessor::versionFromMap(const QVariantMap &data)
{
    return data.value(QLatin1String(VERSION_KEY), -1).toInt();
}

int SettingsAccessor::originalVersionFromMap(const QVariantMap &data)
{
    return data.value(QLatin1String(ORIGINAL_VERSION_KEY), versionFromMap(data)).toInt();
}

QVariantMap SettingsAccessor::setOriginalVersionInMap(const QVariantMap &data, int version)
{
    QVariantMap result = data;
    result.insert(QLatin1String(ORIGINAL_VERSION_KEY), version);
    return result;
}

QVariantMap SettingsAccessor::setVersionInMap(const QVariantMap &data, int version)
{
    QVariantMap result = data;
    result.insert(QLatin1String(VERSION_KEY), version);
    return result;
}

/*!
 * Run directly after reading the \a data.
 *
 * This method is called right after reading the data before any attempt at interpreting the data
 * is made.
 *
 * Returns the prepared data.
 */
QVariantMap SettingsAccessor::prepareSettings(const QVariantMap &data) const
{
    return data;
}

/*!
 * Check which of two sets of data are a better match to load.
 *
 * This method is used to compare data extracted from two XML settings files.
 * It will never be called with a version too old or too new to be read by
 * the current instance of Qt Creator.
 *
 * Compares \a newData against \a origData.
 *
 * Returns \c true if \a newData is a better match than \a origData and \c false otherwise.
 */
bool SettingsAccessor::isBetterMatch(const QVariantMap &origData, const QVariantMap &newData) const
{
    if (origData.isEmpty())
        return true;

    int origVersion = versionFromMap(origData);
    QByteArray origEnv = environmentIdFromMap(origData);

    int newVersion = versionFromMap(newData);
    QByteArray newEnv = environmentIdFromMap(newData);

    if (origEnv != newEnv) {
        if (origEnv == creatorId())
            return false;
        if (newEnv == creatorId())
            return true;
    }

    return newVersion > origVersion;
}

/*!
 * Upgrade the settings in \a data to the version \a toVersion.
 *
 * Returns settings of the requested version.
 */
QVariantMap SettingsAccessor::upgradeSettings(const QVariantMap &data) const
{
    const int version = versionFromMap(data);

    if (data.isEmpty())
        return data;

    QVariantMap result;
    if (!data.contains(QLatin1String(ORIGINAL_VERSION_KEY)))
        result = setOriginalVersionInMap(data, version);
    else
        result = data;

    const int toVersion = currentVersion();
    if (version >= toVersion || version < d->firstVersion())
        return result;

    for (int i = version; i < toVersion; ++i) {
        VersionUpgrader *upgrader = d->upgrader(i);
        QTC_CHECK(upgrader && upgrader->version() == i);
        result = upgrader->upgrade(result);
        result = setVersionInMap(result, i + 1);
    }

    return result;
}

/*!
 * Find issues with the settings file and warn the user about them.
 *
 * \a data is the data from the settings file.
 * \a path is the full path to the settings used.
 * \a parent is the widget to be set as parent of any dialogs that are opened.
 *
 * Returns \c true if the settings are not usable anymore and \c false otherwise.
 */
SettingsAccessor::ProceedInfo SettingsAccessor::reportIssues(const QVariantMap &data,
                                                             const FileName &path,
                                                             QWidget *parent) const
{
    IssueInfo issue = findIssues(data, path);
    QMessageBox::Icon icon = QMessageBox::Information;

    if (issue.buttons.count() > 1)
        icon = QMessageBox::Question;

    QMessageBox::StandardButtons buttons = QMessageBox::NoButton;
    foreach (QMessageBox::StandardButton b, issue.buttons.keys())
        buttons |= b;

    if (buttons == QMessageBox::NoButton)
        return Continue;

    QMessageBox msgBox(icon, issue.title, issue.message, buttons, parent);
    if (issue.defaultButton != QMessageBox::NoButton)
        msgBox.setDefaultButton(issue.defaultButton);
    if (issue.escapeButton != QMessageBox::NoButton)
        msgBox.setEscapeButton(issue.escapeButton);

    int boxAction = msgBox.exec();
    return issue.buttons.value(static_cast<QMessageBox::StandardButton>(boxAction));
}

/*!
 * Checks \a data located at \a path for issues to be displayed with reportIssues.
 *
 * Returns a IssueInfo object which is then used by reportIssues.
 */
SettingsAccessor::IssueInfo SettingsAccessor::findIssues(const QVariantMap &data, const FileName &path) const
{
    SettingsAccessor::IssueInfo result;

    const FileName defaultSettingsPath = userFilePath(project(), m_userSuffix);

    int version = versionFromMap(data);
    if (!path.exists()) {
        return result;
    } else if (data.isEmpty() || version < firstSupportedVersion() || version > currentVersion()) {
        result.title = QApplication::translate("Utils::SettingsAccessor", "No Valid Settings Found");
        result.message = QApplication::translate("Utils::SettingsAccessor",
                                                 "<p>No valid settings file could be found.</p>"
                                                 "<p>All settings files found in directory \"%1\" "
                                                 "were either too new or too old to be read.</p>")
                .arg(path.toUserOutput());
        result.buttons.insert(QMessageBox::Ok, DiscardAndContinue);
    } else if ((path != defaultSettingsPath) && (version < currentVersion())) {
        result.title = QApplication::translate("Utils::SettingsAccessor", "Using Old Settings");
        result.message = QApplication::translate("Utils::SettingsAccessor",
                                                 "<p>The versioned backup \"%1\" of the settings "
                                                 "file is used, because the non-versioned file was "
                                                 "created by an incompatible version of Qt Creator.</p>"
                                                 "<p>Settings changes made since the last time this "
                                                 "version of Qt Creator was used are ignored, and "
                                                 "changes made now will <b>not</b> be propagated to "
                                                 "the newer version.</p>").arg(path.toUserOutput());
        result.buttons.insert(QMessageBox::Ok, Continue);
    }

    if (!result.buttons.isEmpty())
        return result;

    QByteArray readId = environmentIdFromMap(data);
    if (!readId.isEmpty() && readId != creatorId()) {
        result.title = differentEnvironmentMsg(project()->displayName());
        result.message = QApplication::translate("ProjectExplorer::EnvironmentIdAccessor",
                                                 "<p>No .user settings file created by this instance "
                                                 "of Qt Creator was found.</p>"
                                                 "<p>Did you work with this project on another machine or "
                                                 "using a different settings path before?</p>"
                                                 "<p>Do you still want to load the settings file \"%1\"?</p>")
                .arg(path.toUserOutput());
        result.defaultButton = QMessageBox::No;
        result.escapeButton = QMessageBox::No;
        result.buttons.insert(QMessageBox::Yes, SettingsAccessor::Continue);
        result.buttons.insert(QMessageBox::No, SettingsAccessor::DiscardAndContinue);
    }
    return result;
}

QString SettingsAccessor::differentEnvironmentMsg(const QString &projectName)
{
    return QApplication::translate("ProjectExplorer::EnvironmentIdAccessor",
                                   "Settings File for \"%1\" from a different Environment?")
            .arg(projectName);
}

namespace {

// When restoring settings...
//   We check whether a .shared file exists. If so, we compare the settings in this file with
//   corresponding ones in the .user file. Whenever we identify a corresponding setting which
//   has a different value and which is not marked as sticky, we merge the .shared value into
//   the .user value.
QVariantMap mergeSharedSettings(const QVariantMap &userMap, const QVariantMap &sharedMap)
{
    QVariantMap result = userMap;
    if (sharedMap.isEmpty())
        return result;
    if (userMap.isEmpty())
        return sharedMap;

    MergeSettingsOperation op;
    op.synchronize(result, sharedMap);
    return result;
}

// When saving settings...
//   If a .shared file was considered in the previous restoring step, we check whether for
//   any of the current .shared settings there's a .user one which is different. If so, this
//   means the user explicitly changed it and we mark this setting as sticky.
//   Note that settings are considered sticky only when they differ from the .shared ones.
//   Although this approach is more flexible than permanent/forever sticky settings, it has
//   the side-effect that if a particular value unintentionally becomes the same in both
//   the .user and .shared files, this setting will "unstick".
void trackUserStickySettings(QVariantMap &userMap, const QVariantMap &sharedMap)
{
    if (sharedMap.isEmpty())
        return;

    TrackStickyness op;
    op.synchronize(userMap, sharedMap);
}

} // Anonymous

QByteArray SettingsAccessor::environmentIdFromMap(const QVariantMap &data)
{
    return data.value(QLatin1String(ENVIRONMENT_ID_KEY)).toByteArray();
}

QVariantMap SettingsAccessor::restoreSettings(QWidget *parent) const
{
    if (d->lastVersion() < 0)
        return QVariantMap();

    QVariantMap userSettings = readUserSettings(parent);
    QVariantMap sharedSettings = readSharedSettings(parent);
    return mergeSettings(userSettings, sharedSettings);
}

QVariantMap SettingsAccessor::prepareToSaveSettings(const QVariantMap &data) const
{
    QVariantMap tmp = data;
    const QVariant &shared = m_project->property(SHARED_SETTINGS);
    if (shared.isValid())
        trackUserStickySettings(tmp, shared.toMap());

    tmp.insert(QLatin1String(VERSION_KEY), d->currentVersion());
    tmp.insert(QLatin1String(ENVIRONMENT_ID_KEY), SettingsAccessor::creatorId());

    return tmp;
}

bool SettingsAccessor::saveSettings(const QVariantMap &map, QWidget *parent) const
{
    if (map.isEmpty())
        return false;

    backupUserFile();

    QVariantMap data = prepareToSaveSettings(map);

    FileName path = FileName::fromString(defaultFileName(m_userSuffix));
    if (!d->m_writer || d->m_writer->fileName() != path) {
        delete d->m_writer;
        d->m_writer = new PersistentSettingsWriter(path, QLatin1String("QtCreatorProject"));
    }

    return d->m_writer->save(data, parent);
}

bool SettingsAccessor::addVersionUpgrader(VersionUpgrader *upgrader)
{
    QTC_ASSERT(upgrader, return false);
    int version = upgrader->version();
    QTC_ASSERT(version >= 0, return false);

    if (d->m_upgraders.isEmpty() || d->currentVersion() == version)
        d->m_upgraders.append(upgrader);
    else if (d->firstVersion() - 1 == version)
        d->m_upgraders.prepend(upgrader);
    else
        QTC_ASSERT(false, return false); // Upgrader was added out of sequence or twice

    return true;
}

/* Will always return the default name first (if applicable) */
FileNameList SettingsAccessor::settingsFiles(const QString &suffix) const
{
    FileNameList result;

    QFileInfoList list;
    const QFileInfo pfi = project()->projectFilePath().toFileInfo();
    const QStringList filter(pfi.fileName() + suffix + QLatin1Char('*'));

    if (!sharedUserFileDir().isEmpty()) {
        const QString sharedPath = sharedUserFileDir() + QLatin1Char('/')
            + makeRelative(pfi.absolutePath());
        list.append(QDir(sharedPath).entryInfoList(filter, QDir::Files | QDir::Hidden | QDir::System));
    }
    list.append(QDir(pfi.dir()).entryInfoList(filter, QDir::Files | QDir::Hidden | QDir::System));

    foreach (const QFileInfo &fi, list) {
        const FileName path = FileName::fromString(fi.absoluteFilePath());
        if (!result.contains(path)) {
            if (path.endsWith(suffix))
                result.prepend(path);
            else
                result.append(path);
        }
    }

    return result;
}

QByteArray SettingsAccessor::creatorId()
{
    return ProjectExplorerPlugin::projectExplorerSettings().environmentId.toByteArray();
}

QString SettingsAccessor::defaultFileName(const QString &suffix) const
{
    return userFilePath(project(), suffix).toString();
}

int SettingsAccessor::currentVersion() const
{
    return d->currentVersion();
}

int SettingsAccessor::firstSupportedVersion() const
{
    return d->firstVersion();
}

FileName SettingsAccessor::backupName(const QVariantMap &data) const
{
    QString backupName = defaultFileName(m_userSuffix);
    const QByteArray oldEnvironmentId = environmentIdFromMap(data);
    if (!oldEnvironmentId.isEmpty() && oldEnvironmentId != creatorId())
        backupName += QLatin1Char('.') + QString::fromLatin1(oldEnvironmentId).mid(1, 7);
    const int oldVersion = versionFromMap(data);
    if (oldVersion != currentVersion()) {
        VersionUpgrader *upgrader = d->upgrader(oldVersion);
        if (upgrader)
            backupName += QLatin1Char('.') + upgrader->backupExtension();
        else
            backupName += QLatin1Char('.') + QString::number(oldVersion);
    }
    return FileName::fromString(backupName);
}

void SettingsAccessor::backupUserFile() const
{
    SettingsAccessorPrivate::Settings oldSettings;
    oldSettings.path = FileName::fromString(defaultFileName(m_userSuffix));
    oldSettings.map = readFile(oldSettings.path);
    if (oldSettings.map.isEmpty())
        return;

    // Do we need to do a backup?
    const QString origName = oldSettings.path.toString();
    QString backupFileName = backupName(oldSettings.map).toString();
    if (backupFileName != origName)
        QFile::copy(origName, backupFileName);
}

QVariantMap SettingsAccessor::readUserSettings(QWidget *parent) const
{
    SettingsAccessorPrivate::Settings result;
    FileNameList fileList = settingsFiles(m_userSuffix);
    if (fileList.isEmpty()) // No settings found at all.
        return result.map;

    result = d->bestSettings(this, fileList);
    if (result.path.isEmpty())
        result.path = project()->projectDirectory();

    ProceedInfo proceed = reportIssues(result.map, result.path, parent);
    if (proceed == DiscardAndContinue)
        return QVariantMap();

    return result.map;
}

QVariantMap SettingsAccessor::readSharedSettings(QWidget *parent) const
{
    SettingsAccessorPrivate::Settings sharedSettings;
    QString fn = project()->projectFilePath().toString() + m_sharedSuffix;
    sharedSettings.path = FileName::fromString(fn);
    sharedSettings.map = readFile(sharedSettings.path);

    if (versionFromMap(sharedSettings.map) > currentVersion()) {
        // The shared file version is newer than Creator... If we have valid user
        // settings we prompt the user whether we could try an *unsupported* update.
        // This makes sense since the merging operation will only replace shared settings
        // that perfectly match corresponding user ones. If we don't have valid user
        // settings to compare against, there's nothing we can do.

        QMessageBox msgBox(
                    QMessageBox::Question,
                    QApplication::translate("ProjectExplorer::SettingsAccessor",
                                            "Unsupported Shared Settings File"),
                    QApplication::translate("ProjectExplorer::SettingsAccessor",
                                            "The version of your .shared file is not "
                                            "supported by Qt Creator. "
                                            "Do you want to try loading it anyway?"),
                    QMessageBox::Yes | QMessageBox::No,
                    parent);
        msgBox.setDefaultButton(QMessageBox::No);
        msgBox.setEscapeButton(QMessageBox::No);
        if (msgBox.exec() == QMessageBox::No)
            sharedSettings.map.clear();
        else
            sharedSettings.map = setVersionInMap(sharedSettings.map, currentVersion());
    }
    return sharedSettings.map;
}

SettingsAccessorPrivate::Settings SettingsAccessorPrivate::bestSettings(const SettingsAccessor *accessor,
                                                                        const FileNameList &pathList)
{
    Settings bestMatch;
    foreach (const FileName &path, pathList) {
        QVariantMap tmp = accessor->readFile(path);

        int version = SettingsAccessor::versionFromMap(tmp);
        if (version < firstVersion() || version > currentVersion())
            continue;

        if (accessor->isBetterMatch(bestMatch.map, tmp)) {
            bestMatch.path = path;
            bestMatch.map = tmp;
        }
    }
    return bestMatch;
}

QVariantMap SettingsAccessor::mergeSettings(const QVariantMap &userMap,
                                            const QVariantMap &sharedMap) const
{
    QVariantMap newUser = userMap;
    QVariantMap newShared = sharedMap;
    QVariantMap result;
    if (!newUser.isEmpty() && !newShared.isEmpty()) {
        newUser = upgradeSettings(newUser);
        newShared = upgradeSettings(newShared);
        result = mergeSharedSettings(newUser, newShared);
    } else if (!sharedMap.isEmpty()) {
        result = sharedMap;
    } else if (!userMap.isEmpty()) {
        result = userMap;
    }

    m_project->setProperty(SHARED_SETTINGS, newShared);

    // Update from the base version to Creator's version.
    return upgradeSettings(result);
}

// -------------------------------------------------------------------------
// SettingsData
// -------------------------------------------------------------------------
bool SettingsAccessorPrivate::Settings::isValid() const
{
    return SettingsAccessor::versionFromMap(map) > -1 && !path.isEmpty();
}

QVariantMap SettingsAccessor::readFile(const FileName &path) const
{
    PersistentSettingsReader reader;
    if (!reader.load(path))
        return QVariantMap();

    return prepareSettings(reader.restoreValues());
}

// -------------------------------------------------------------------------
// UserFileVersion1Upgrader:
// -------------------------------------------------------------------------

QVariantMap UserFileVersion1Upgrader::upgrade(const QVariantMap &map)
{
    QVariantMap result;

    // The only difference between version 1 and 2 of the user file is that
    // we need to add targets.

    // Generate a list of all possible targets for the project:
    Project *project = m_accessor->project();
    QList<TargetDescription> targets;
    if (project->id() == "GenericProjectManager.GenericProject")
        targets << TargetDescription(QString::fromLatin1("GenericProjectManager.GenericTarget"),
                                     QCoreApplication::translate("GenericProjectManager::GenericTarget",
                                                                 "Desktop",
                                                                 "Generic desktop target display name"));
    else if (project->id() == "CMakeProjectManager.CMakeProject")
        targets << TargetDescription(QString::fromLatin1("CMakeProjectManager.DefaultCMakeTarget"),
                                     QCoreApplication::translate("CMakeProjectManager::Internal::CMakeTarget",
                                                                 "Desktop",
                                                                 "CMake Default target display name"));
    else if (project->id() == "Qt4ProjectManager.Qt4Project")
        targets << TargetDescription(QString::fromLatin1("Qt4ProjectManager.Target.DesktopTarget"),
                                     QCoreApplication::translate("QmakeProjectManager::Internal::Qt4Target",
                                                                 "Desktop",
                                                                 "Qt4 Desktop target display name"))
        << TargetDescription(QString::fromLatin1("Qt4ProjectManager.Target.MaemoEmulatorTarget"),
                                     QCoreApplication::translate("QmakeProjectManager::Internal::Qt4Target",
                                                                 "Maemo Emulator",
                                                                 "Qt4 Maemo Emulator target display name"))
        << TargetDescription(QString::fromLatin1("Qt4ProjectManager.Target.MaemoDeviceTarget"),
                                     QCoreApplication::translate("QmakeProjectManager::Internal::Qt4Target",
                                                                 "Maemo Device",
                                                                 "Qt4 Maemo Device target display name"));
    else if (project->id() == "QmlProjectManager.QmlProject")
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
// UserFileVersion2Upgrader:
// -------------------------------------------------------------------------

QVariantMap UserFileVersion2Upgrader::upgrade(const QVariantMap &map)
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
// UserFileVersion3Upgrader:
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

QVariantMap UserFileVersion3Upgrader::upgrade(const QVariantMap &map)
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
        result.insert(targetKey, originalTarget);
    }
    return result;
}

// -------------------------------------------------------------------------
// UserFileVersion4Upgrader:
// -------------------------------------------------------------------------

// Move packaging steps from build steps into deploy steps
QVariantMap UserFileVersion4Upgrader::upgrade(const QVariantMap &map)
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
        // check for maemo device target
        if (originalTarget.value(QLatin1String("ProjectExplorer.ProjectConfiguration.Id"))
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
            while (bcIt.hasNext()) {
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
// UserFileVersion5Upgrader:
// -------------------------------------------------------------------------

// Move packaging steps from build steps into deploy steps
QVariantMap UserFileVersion5Upgrader::upgrade(const QVariantMap &map)
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
        // check for maemo device target
        if (originalTarget.value(QLatin1String("ProjectExplorer.ProjectConfiguration.Id"))
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
// UserFileVersion6Upgrader:
// -------------------------------------------------------------------------

// Introduce DeployConfiguration and BuildStepLists
QVariantMap UserFileVersion6Upgrader::upgrade(const QVariantMap &map)
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
// UserFileVersion7Upgrader:
// -------------------------------------------------------------------------

// new implementation of DeployConfiguration
QVariantMap UserFileVersion7Upgrader::upgrade(const QVariantMap &map)
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
        result.insert(globalKey, originalTarget);
    }
    return result;
}

// -------------------------------------------------------------------------
// UserFileVersion8Upgrader:
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

// These were split according to sane (even if a bit arcane) rules
static QVariant version8ArgNodeHandler(const QVariant &var)
{
    QString ret;
    foreach (const QVariant &svar, var.toList()) {
        if (HostOsInfo::isAnyUnixHost()) {
            // We don't just addArg, so we don't disarm existing env expansions.
            // This is a bit fuzzy logic ...
            QString s = svar.toString();
            s.replace(QLatin1Char('\\'), QLatin1String("\\\\"));
            s.replace(QLatin1Char('"'), QLatin1String("\\\""));
            s.replace(QLatin1Char('`'), QLatin1String("\\`"));
            if (s != svar.toString() || hasSpecialChars(s))
                s.prepend(QLatin1Char('"')).append(QLatin1Char('"'));
            QtcProcess::addArgs(&ret, s);
        } else {
            // Under windows, env expansions cannot be quoted anyway.
            QtcProcess::addArg(&ret, svar.toString());
        }
    }
    return QVariant(ret);
}

// These were just split on whitespace
static QVariant version8LameArgNodeHandler(const QVariant &var)
{
    QString ret;
    foreach (const QVariant &svar, var.toList())
        QtcProcess::addArgs(&ret, svar.toString());
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
    if (HostOsInfo::isAnyUnixHost()) {
        ret.prepend(QLatin1String("${"));
        ret.append(QLatin1Char('}'));
    } else {
        ret.prepend(QLatin1Char('%'));
        ret.append(QLatin1Char('%'));
    }
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
    if (HostOsInfo::isAnyUnixHost()) {
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
    } else {
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
    }

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
    static const QSet<QString> map({
            "CURRENT_DOCUMENT:absoluteFilePath",
            "CURRENT_DOCUMENT:absolutePath",
            "CURRENT_DOCUMENT:baseName",
            "CURRENT_DOCUMENT:canonicalPath",
            "CURRENT_DOCUMENT:canonicalFilePath",
            "CURRENT_DOCUMENT:completeBaseName",
            "CURRENT_DOCUMENT:completeSuffix",
            "CURRENT_DOCUMENT:fileName",
            "CURRENT_DOCUMENT:filePath",
            "CURRENT_DOCUMENT:path",
            "CURRENT_DOCUMENT:suffix"
        });

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

QVariantMap UserFileVersion8Upgrader::upgrade(const QVariantMap &map)
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

// --------------------------------------------------------------------
// UserFileVersion9Upgrader:
// --------------------------------------------------------------------

QVariantMap UserFileVersion9Upgrader::upgrade(const QVariantMap &map)
{
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

// --------------------------------------------------------------------
// UserFileVersion10Upgrader:
// --------------------------------------------------------------------

QVariantMap UserFileVersion10Upgrader::upgrade(const QVariantMap &map)
{
    QList<Change> changes;
    changes.append(qMakePair(QLatin1String("ProjectExplorer.ProcessStep.Enabled"),
                             QLatin1String("ProjectExplorer.BuildStep.Enabled")));
    return renameKeys(changes, QVariantMap(map));
}

// --------------------------------------------------------------------
// UserFileVersion11Upgrader:
// --------------------------------------------------------------------

UserFileVersion11Upgrader::~UserFileVersion11Upgrader()
{
    QList<Kit *> knownKits = KitManager::kits();
    foreach (Kit *k, m_targets.keys()) {
        if (!knownKits.contains(k))
            KitManager::deleteKit(k);
    }
    m_targets.clear();
}

static inline int targetId(const QString &targetKey)
{
    return targetKey.midRef(targetKey.lastIndexOf(QLatin1Char('.')) + 1).toInt();
}

QVariantMap UserFileVersion11Upgrader::upgrade(const QVariantMap &map)
{
    // Read in old data to help with the transition:
    parseQtversionFile();
    parseToolChainFile();

    QVariantMap result;
    foreach (Kit *k, KitManager::kits())
        m_targets.insert(k, QVariantMap());

    QMapIterator<QString, QVariant> globalIt(map);
    int activeTarget = map.value(QLatin1String("ProjectExplorer.Project.ActiveTarget"), 0).toInt();

    while (globalIt.hasNext()) {
        globalIt.next();
        const QString &globalKey = globalIt.key();
        // Keep everything but targets:
        if (globalKey == QLatin1String("ProjectExplorer.Project.ActiveTarget"))
            continue;
        if (!globalKey.startsWith(QLatin1String("ProjectExplorer.Project.Target."))) {
            result.insert(globalKey, globalIt.value());
            continue;
        }

        // Update Targets:
        const QVariantMap &target = globalIt.value().toMap();
        int targetPos = targetId(globalKey);

        QVariantMap extraTargetData;
        QMap<int, QVariantMap> bcs;
        int activeBc = -1;
        QMap<int, QVariantMap> dcs;
        int activeDc = -1;
        QMap<int, QVariantMap> rcs;
        int activeRc = -1;

        // Read old target:
        QMapIterator<QString, QVariant> targetIt(target);
        while (targetIt.hasNext()) {
            targetIt.next();
            const QString &targetKey = targetIt.key();

            // BuildConfigurations:
            if (targetKey == QLatin1String("ProjectExplorer.Target.ActiveBuildConfiguration"))
                activeBc = targetIt.value().toInt();
            else if (targetKey == QLatin1String("ProjectExplorer.Target.BuildConfigurationCount"))
                continue;
            else if (targetKey.startsWith(QLatin1String("ProjectExplorer.Target.BuildConfiguration.")))
                bcs.insert(targetId(targetKey), targetIt.value().toMap());
            else

            // DeployConfigurations:
            if (targetKey == QLatin1String("ProjectExplorer.Target.ActiveDeployConfiguration"))
                activeDc = targetIt.value().toInt();
            else if (targetKey == QLatin1String("ProjectExplorer.Target.DeployConfigurationCount"))
                continue;
            else if (targetKey.startsWith(QLatin1String("ProjectExplorer.Target.DeployConfiguration.")))
                dcs.insert(targetId(targetKey), targetIt.value().toMap());
            else

            // RunConfigurations:
            if (targetKey == QLatin1String("ProjectExplorer.Target.ActiveRunConfiguration"))
                activeRc = targetIt.value().toInt();
            else if (targetKey == QLatin1String("ProjectExplorer.Target.RunConfigurationCount"))
                continue;
            else if (targetKey.startsWith(QLatin1String("ProjectExplorer.Target.RunConfiguration.")))
                rcs.insert(targetId(targetKey), targetIt.value().toMap());

            // Rest (the target's ProjectConfiguration settings only as there is nothing else):
            else
                extraTargetData.insert(targetKey, targetIt.value());
        }
        const QString oldTargetId = extraTargetData.value(QLatin1String("ProjectExplorer.ProjectConfiguration.Id")).toString();

        // Check each BCs/DCs and create profiles as needed
        auto rawKit = new Kit; // Do not needlessly use Core::Ids
        QMapIterator<int, QVariantMap> buildIt(bcs);
        while (buildIt.hasNext()) {
            buildIt.next();
            int bcPos = buildIt.key();
            const QVariantMap &bc = buildIt.value();
            Kit *tmpKit = rawKit;

            const auto desktopDeviceIcon = FileName::fromLatin1(":///DESKTOP///");

            if (oldTargetId == QLatin1String("Qt4ProjectManager.Target.AndroidDeviceTarget")) {
                tmpKit->setIconPath(FileName::fromLatin1(":/android/images/QtAndroid.png"));
                tmpKit->setValue("PE.Profile.DeviceType", QString::fromLatin1("Desktop"));
                tmpKit->setValue("PE.Profile.Device", QString());
            } else if (oldTargetId == QLatin1String("RemoteLinux.EmbeddedLinuxTarget")) {
                tmpKit->setIconPath(desktopDeviceIcon);
                tmpKit->setValue("PE.Profile.DeviceType", QString::fromLatin1("GenericLinuxOsType"));
                tmpKit->setValue("PE.Profile.Device", QString());
            } else if (oldTargetId == QLatin1String("Qt4ProjectManager.Target.HarmattanDeviceTarget")) {
                tmpKit->setIconPath(FileName::fromLatin1(":/projectexplorer/images/MaemoDevice.png"));
                tmpKit->setValue("PE.Profile.DeviceType", QString::fromLatin1("HarmattanOsType"));
                tmpKit->setValue("PE.Profile.Device", QString());
            } else if (oldTargetId == QLatin1String("Qt4ProjectManager.Target.MaemoDeviceTarget")) {
                tmpKit->setIconPath(FileName::fromLatin1(":/projectexplorer/images/MaemoDevice.png"));
                tmpKit->setValue("PE.Profile.DeviceType", QString::fromLatin1("Maemo5OsType"));
                tmpKit->setValue("PE.Profile.Device", QString());
            } else if (oldTargetId == QLatin1String("Qt4ProjectManager.Target.MeegoDeviceTarget")) {
                tmpKit->setIconPath(FileName::fromLatin1(":/projectexplorer/images/MaemoDevice.png"));
                tmpKit->setValue("PE.Profile.DeviceType", QString::fromLatin1("MeegoOsType"));
                tmpKit->setValue("PE.Profile.Device", QString());
            } else if (oldTargetId == QLatin1String("Qt4ProjectManager.Target.S60DeviceTarget")) {
                tmpKit->setIconPath(FileName::fromLatin1(":/projectexplorer/images/SymbianDevice.png"));
                tmpKit->setValue("PE.Profile.DeviceType", QString::fromLatin1("Qt4ProjectManager.SymbianDevice"));
                tmpKit->setValue("PE.Profile.Device", QString::fromLatin1("Symbian Device"));
            } else if (oldTargetId == QLatin1String("Qt4ProjectManager.Target.QtSimulatorTarget")) {
                tmpKit->setIconPath(FileName::fromLatin1(":/projectexplorer/images/Simulator.png"));
                tmpKit->setValue("PE.Profile.DeviceType", QString::fromLatin1("Desktop"));
                tmpKit->setValue("PE.Profile.Device", QString::fromLatin1("Desktop Device"));
            } else {
                tmpKit->setIconPath(desktopDeviceIcon);
                tmpKit->setValue("PE.Profile.DeviceType", QString::fromLatin1("Desktop"));
                tmpKit->setValue("PE.Profile.Device", QString::fromLatin1("Desktop Device"));
            }

            // Tool chain
            QString tcId = bc.value(QLatin1String("CMakeProjectManager.CMakeBuildConfiguration.ToolChain")).toString();
            if (tcId.isEmpty())
                tcId = bc.value(QLatin1String("ProjectExplorer.BuildCOnfiguration.ToolChain")).toString();
            const QString origTcId = tcId;
            tcId.replace(QLatin1String("Qt4ProjectManager.ToolChain.Maemo:"),
                           QLatin1String("ProjectExplorer.ToolChain.Gcc:")); // convert Maemo to GCC
            QString data = tcId.mid(tcId.indexOf(QLatin1Char(':')) + 1);
            QStringList split = data.split(QLatin1Char('.'), QString::KeepEmptyParts);
            QString compilerPath;
            QString debuggerPath;
            Abi compilerAbi;
            int debuggerEngine = 1; // GDB
            for (int i = 1; i < split.count() - 1; ++i) {
                compilerAbi = Abi(split.at(i));
                if (!compilerAbi.isValid())
                    continue;
                if (compilerAbi.os() == Abi::WindowsOS
                        && compilerAbi.osFlavor() != Abi::WindowsMSysFlavor)
                    debuggerEngine = 4; // CDB
                compilerPath = split.at(0);
                for (int j = 1; j < i; ++j)
                    compilerPath = compilerPath + QLatin1Char('.') + split.at(j);
                debuggerPath = split.at(i + 1);
                for (int j = i + 2; j < split.count(); ++j)
                    debuggerPath = debuggerPath + QLatin1Char('.') + split.at(j);

                ToolChain *tc = ToolChainManager::toolChain([cp = FileName::fromString(compilerPath),
                                                            compilerAbi](const ToolChain *t) {
                    return t->compilerCommand() == cp && t->targetAbi() == compilerAbi;
                });
                if (tc)
                    tcId = QString::fromUtf8(tc->id());
            }
            tmpKit->setValue("PE.Profile.ToolChain", tcId);

            // QtVersion
            int qtVersionId = bc.value(QLatin1String("Qt4ProjectManager.Qt4BuildConfiguration.QtVersionId"), -1).toInt();
            tmpKit->setValue("QtSupport.QtInformation", qtVersionId);

            // Debugger + mkspec
            QVariantMap debugger;
            QString mkspec;
            if (m_toolChainExtras.contains(origTcId)) {
                debuggerPath = m_toolChainExtras.value(origTcId).m_debugger;
                if (!debuggerPath.isEmpty() && !QFileInfo(debuggerPath).isAbsolute())
                    debuggerPath = Environment::systemEnvironment().searchInPath(debuggerPath).toString();
                if (debuggerPath.contains(QLatin1String("cdb")))
                    debuggerEngine = 4; // CDB
                mkspec = m_toolChainExtras.value(origTcId).m_mkspec;
            }
            debugger.insert(QLatin1String("EngineType"), debuggerEngine);
            debugger.insert(QLatin1String("Binary"), debuggerPath);
            tmpKit->setValue("Debugger.Information", debugger);
            tmpKit->setValue("QtPM4.mkSpecInformation", mkspec);

            // SysRoot
            tmpKit->setValue("PE.Profile.SysRoot", m_qtVersionExtras.value(qtVersionId));

            QMapIterator<int, QVariantMap> deployIt(dcs);
            while (deployIt.hasNext()) {
                deployIt.next();
                int dcPos = deployIt.key();
                const QVariantMap &dc = deployIt.value();
                // Device
                QByteArray devId = dc.value(QLatin1String("Qt4ProjectManager.MaemoRunConfiguration.DeviceId")).toByteArray();
                if (devId.isEmpty())
                    devId = QByteArray("Desktop Device");
                if (!devId.isEmpty() && !DeviceManager::instance()->find(Core::Id::fromName(devId))) // We do not know that device
                    devId.clear();
                tmpKit->setValue("PE.Profile.Device", devId);

                // Set display name last:
                tmpKit->setUnexpandedDisplayName(extraTargetData.value(QLatin1String("ProjectExplorer.ProjectConfiguration.DisplayName")).toString());

                Kit *k = uniqueKit(tmpKit);

                addBuildConfiguration(k, bc, bcPos, activeBc);
                addDeployConfiguration(k, dc, dcPos, activeDc);
                addRunConfigurations(k, rcs, activeRc, m_accessor->project()->projectDirectory().toString());
                if (targetPos == activeTarget && bcPos == activeBc && dcPos == activeDc)
                    m_targets[k].insert(QLatin1String("Update.IsActive"), true);
            } // dcs
        } // bcs
        KitManager::deleteKit(rawKit);
    } // read in map data

    int newPos = 0;
    // Generate new target data:
    foreach (Kit *k, m_targets.keys()) {
        QVariantMap data = m_targets.value(k);
        if (data.isEmpty())
            continue;

        KitManager::registerKit(k);

        data.insert(QLatin1String("ProjectExplorer.ProjectConfiguration.Id"), k->id().name());
        data.insert(QLatin1String("ProjectExplorer.Target.Profile"), k->id().name());
        data.insert(QLatin1String("ProjectExplorer.ProjectConfiguration.DisplayName"), k->displayName());
        data.insert(QLatin1String("ProjectExplorer.ProjectConfiguration.DefaultDisplayName"), k->displayName());

        result.insert(QString::fromLatin1("ProjectExplorer.Project.Target.") + QString::number(newPos), data);
        if (data.value(QLatin1String("Update.IsActive"), false).toBool())
            result.insert(QLatin1String("ProjectExplorer.Project.ActiveTarget"), newPos);
        ++newPos;
    }
    result.insert(QLatin1String("ProjectExplorer.Project.TargetCount"), newPos);

    return result;
}

Kit *UserFileVersion11Upgrader::uniqueKit(Kit *k)
{
    const QString tc = k->value("PE.Profile.ToolChain").toString();
    const int qt = k->value("QtSupport.QtInformation").toInt();
    const QString debugger = k->value("Debugger.Information").toString();
    const QString mkspec = k->value("QtPM4.mkSpecInformation").toString();
    const QString deviceType = k->value("PE.Profile.DeviceType").toString();
    const QString device = k->value("PE.Profile.Device").toString();
    const QString sysroot = k->value("PE.Profile.SysRoot").toString();

    foreach (Kit *i, m_targets.keys()) {
        const QString currentTc = i->value("PE.Profile.ToolChain").toString();
        const int currentQt = i->value("QtSupport.QtInformation").toInt();
        const QString currentDebugger = i->value("Debugger.Information").toString();
        const QString currentMkspec = i->value("QtPM4.mkSpecInformation").toString();
        const QString currentDeviceType = i->value("PE.Profile.DeviceType").toString();
        const QString currentDevice = i->value("PE.Profile.Device").toString();
        const QString currentSysroot = i->value("PE.Profile.SysRoot").toString();

        bool deviceTypeOk = deviceType == currentDeviceType;
        bool deviceOk = device.isEmpty() || currentDevice == device;
        bool tcOk = tc.isEmpty() || currentTc.isEmpty() || currentTc == tc;
        bool qtOk = qt == -1 || currentQt == qt;
        bool debuggerOk = debugger.isEmpty() || currentDebugger.isEmpty() || currentDebugger == debugger;
        bool mkspecOk = mkspec.isEmpty() || currentMkspec.isEmpty() || currentMkspec == mkspec;
        bool sysrootOk = sysroot.isEmpty() || currentSysroot == sysroot;

        if (deviceTypeOk && deviceOk && tcOk && qtOk && debuggerOk && mkspecOk && sysrootOk)
            return i;
    }
    return k->clone(true);
}

void UserFileVersion11Upgrader::addBuildConfiguration(Kit *k, const QVariantMap &bc, int bcPos, int bcActive)
{
    QVariantMap merged = m_targets.value(k);
    int internalCount = merged.value(QLatin1String("ProjectExplorer.Target.BuildConfigurationCount"), 0).toInt();

    for (int i = 0; i < internalCount; ++i) {
        QVariantMap bcData = merged.value(QString::fromLatin1("ProjectExplorer.Target.BuildConfiguration.") + QString::number(i)).toMap();
        if (bcData.value(QLatin1String("Update.BCPos"), -1).toInt() == bcPos)
            return;
    }
    QVariantMap data = bc;
    data.insert(QLatin1String("Update.BCPos"), bcPos);

    merged.insert(QString::fromLatin1("ProjectExplorer.Target.BuildConfiguration.") + QString::number(internalCount), data);
    if (bcPos == bcActive)
        merged.insert(QLatin1String("ProjectExplorer.Target.ActiveBuildConfiguration"), internalCount);
    merged.insert(QLatin1String("ProjectExplorer.Target.BuildConfigurationCount"), internalCount + 1);

    m_targets.insert(k, merged);
}

void UserFileVersion11Upgrader::addDeployConfiguration(Kit *k, const QVariantMap &dc, int dcPos, int dcActive)
{
    QVariantMap merged = m_targets.value(k);
    int internalCount = merged.value(QLatin1String("ProjectExplorer.Target.DeployConfigurationCount"), 0).toInt();

    for (int i = 0; i < internalCount; ++i) {
        QVariantMap dcData = merged.value(QString::fromLatin1("ProjectExplorer.Target.DeployConfiguration.") + QString::number(i)).toMap();
        if (dcData.value(QLatin1String("Update.DCPos"), -1).toInt() == dcPos)
            return;
    }
    QVariantMap data = dc;
    data.insert(QLatin1String("Update.DCPos"), dcPos);

    merged.insert(QString::fromLatin1("ProjectExplorer.Target.DeployConfiguration.") + QString::number(internalCount), data);
    if (dcPos == dcActive)
        merged.insert(QLatin1String("ProjectExplorer.Target.ActiveDeployConfiguration"), internalCount);
    merged.insert(QLatin1String("ProjectExplorer.Target.DeployConfigurationCount"), internalCount + 1);

    m_targets.insert(k, merged);
}

void UserFileVersion11Upgrader::addRunConfigurations(Kit *k,
                                            const QMap<int, QVariantMap> &rcs, int activeRc,
                                            const QString &projectDir)
{
    QVariantMap data = m_targets.value(k);
    data.insert(QLatin1String("ProjectExplorer.Target.RunConfigurationCount"), rcs.count());
    QMapIterator<int, QVariantMap> runIt(rcs);
    while (runIt.hasNext()) {
        runIt.next();
        QVariantMap rcData = runIt.value();
        QString proFile = rcData.value(QLatin1String("Qt4ProjectManager.MaemoRunConfiguration.ProFile")).toString();
        if (proFile.isEmpty())
            proFile = rcData.value(QLatin1String("Qt4ProjectManager.Qt4RunConfiguration.ProFile")).toString();
        if (!proFile.isEmpty()) {
            QString newId = rcData.value(QLatin1String("ProjectExplorer.ProjectConfiguration.Id")).toString();
            newId.append(QLatin1Char(':'));
            FileName fn = FileName::fromString(projectDir);
            fn.appendPath(proFile);
            newId.append(fn.toString());
            rcData.insert(QLatin1String("ProjectExplorer.ProjectConfiguration.Id"), newId);
        }
        data.insert(QString::fromLatin1("ProjectExplorer.Target.RunConfiguration.") + QString::number(runIt.key()), rcData);
    }
    data.insert(QLatin1String("ProjectExplorer.Target.ActiveRunConfiguration"), activeRc);

    m_targets.insert(k, data);
}

static QString targetRoot(const QString &qmakePath)
{
    return QDir::cleanPath(qmakePath).remove(QLatin1String("/bin/qmake" QTC_HOST_EXE_SUFFIX),
            HostOsInfo::fileNameCaseSensitivity());
}

static QString maddeRoot(const QString &qmakePath)
{
    QDir dir(targetRoot(qmakePath));
    dir.cdUp(); dir.cdUp();
    return dir.absolutePath();
}

void UserFileVersion11Upgrader::parseQtversionFile()
{
    PersistentSettingsReader reader;
    QFileInfo settingsLocation = QFileInfo(Core::ICore::settings()->fileName());
    reader.load(FileName::fromString(settingsLocation.absolutePath() + QLatin1String("/qtversion.xml")));
    QVariantMap data = reader.restoreValues();

    int count = data.value(QLatin1String("QtVersion.Count"), 0).toInt();
    for (int i = 0; i < count; ++i) {
        const QString key = QString::fromLatin1("QtVersion.") + QString::number(i);
        if (!data.contains(key))
            continue;
        const QVariantMap qtversionMap = data.value(key).toMap();

        QString sysRoot = qtversionMap.value(QLatin1String("SystemRoot")).toString();
        const QString type = qtversionMap.value(QLatin1String("QtVersion.Type")).toString();
        const QString qmake = qtversionMap.value(QLatin1String("QMakePath")).toString();

        if (type == QLatin1String("Qt4ProjectManager.QtVersion.Maemo")) {
            QFile file(QDir::cleanPath(targetRoot(qmake)) + QLatin1String("/information"));
            if (file.exists() && file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QTextStream stream(&file);
                while (!stream.atEnd()) {
                    const QString &line = stream.readLine().trimmed();
                    const QStringList &list = line.split(QLatin1Char(' '));
                    if (list.count() <= 1)
                        continue;
                    if (list.at(0) == QLatin1String("sysroot"))
                        sysRoot = maddeRoot(qmake) + QLatin1String("/sysroots/") + list.at(1);
                }
            }
        }

        int id = qtversionMap.value(QLatin1String("Id")).toInt();
        if (id > -1 && !sysRoot.isEmpty())
            m_qtVersionExtras.insert(id, sysRoot);
    }
}

void UserFileVersion11Upgrader::parseToolChainFile()
{
    PersistentSettingsReader reader;
    QFileInfo settingsLocation(Core::ICore::settings()->fileName());
    reader.load(FileName::fromString(settingsLocation.absolutePath() + QLatin1String("/toolChains.xml")));
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

// --------------------------------------------------------------------
// UserFileVersion12Upgrader:
// --------------------------------------------------------------------

QVariantMap UserFileVersion12Upgrader::upgrade(const QVariantMap &map)
{
    QVariantMap result;
    QMapIterator<QString, QVariant> it(map);
    while (it.hasNext()) {
        it.next();
        if (it.value().type() == QVariant::Map)
            result.insert(it.key(), upgrade(it.value().toMap()));
        else if (it.key() == QLatin1String("CMakeProjectManager.CMakeRunConfiguration.UserEnvironmentChanges")
                 || it.key() == QLatin1String("ProjectExplorer.CustomExecutableRunConfiguration.UserEnvironmentChanges")
                 || it.key() == QLatin1String("Qt4ProjectManager.Qt4RunConfiguration.UserEnvironmentChanges")
                 || it.key() == QLatin1String("Qt4ProjectManager.MaemoRunConfiguration.UserEnvironmentChanges"))
            result.insert(QLatin1String("PE.UserEnvironmentChanges"), it.value());
        else if (it.key() == QLatin1String("CMakeProjectManager.BaseEnvironmentBase")
                 || it.key() == QLatin1String("ProjectExplorer.CustomExecutableRunConfiguration.BaseEnvironmentBase")
                 || it.key() == QLatin1String("Qt4ProjectManager.MaemoRunConfiguration.BaseEnvironmentBase"))
            result.insert(QLatin1String("PE.BaseEnvironmentBase"), it.value());
        else
            result.insert(it.key(), it.value());
    }
    return result;
}

// --------------------------------------------------------------------
// UserFileVersion13Upgrader:
// --------------------------------------------------------------------

QVariantMap UserFileVersion13Upgrader::upgrade(const QVariantMap &map)
{
    QVariantMap result;
    QMapIterator<QString, QVariant> it(map);
    while (it.hasNext()) {
        it.next();
        if (it.value().type() == QVariant::Map)
            result.insert(it.key(), upgrade(it.value().toMap()));
        else if (it.key() == QLatin1String("PE.UserEnvironmentChanges"))
            result.insert(QLatin1String("PE.EnvironmentAspect.Changes"), it.value());
        else if (it.key() == QLatin1String("PE.BaseEnvironmentBase"))
            result.insert(QLatin1String("PE.EnvironmentAspect.Base"), it.value());
        else
            result.insert(it.key(), it.value());
    }
    return result;
}

// --------------------------------------------------------------------
// UserFileVersion14Upgrader:
// --------------------------------------------------------------------

QVariantMap UserFileVersion14Upgrader::upgrade(const QVariantMap &map)
{
    QVariantMap result;
    QMapIterator<QString, QVariant> it(map);
    while (it.hasNext()) {
        it.next();
        if (it.value().type() == QVariant::Map)
            result.insert(it.key(), upgrade(it.value().toMap()));
        else if (it.key() == QLatin1String("AutotoolsProjectManager.AutotoolsBuildConfiguration.BuildDirectory")
                 || it.key() == QLatin1String("CMakeProjectManager.CMakeBuildConfiguration.BuildDirectory")
                 || it.key() == QLatin1String("GenericProjectManager.GenericBuildConfiguration.BuildDirectory")
                 || it.key() == QLatin1String("Qbs.BuildDirectory")
                 || it.key() == QLatin1String("Qt4ProjectManager.Qt4BuildConfiguration.BuildDirectory"))
            result.insert(QLatin1String("ProjectExplorer.BuildConfiguration.BuildDirectory"), it.value());
        else
            result.insert(it.key(), it.value());
    }
    return result;
}

// --------------------------------------------------------------------
// UserFileVersion15Upgrader:
// --------------------------------------------------------------------

QVariantMap UserFileVersion15Upgrader::upgrade(const QVariantMap &map)
{
    QList<Change> changes;
    changes.append(qMakePair(QLatin1String("ProjectExplorer.Project.Updater.EnvironmentId"),
                             QLatin1String("EnvironmentId")));
    // This is actually handled in the SettingsAccessor itself:
    // changes.append(qMakePair(QLatin1String("ProjectExplorer.Project.Updater.FileVersion"),
    //                          QLatin1String("Version")));
    changes.append(qMakePair(QLatin1String("ProjectExplorer.Project.UserStickyKeys"),
                             QLatin1String("UserStickyKeys")));

    return renameKeys(changes, QVariantMap(map));
}

// --------------------------------------------------------------------
// UserFileVersion16Upgrader:
// --------------------------------------------------------------------

UserFileVersion16Upgrader::OldStepMaps UserFileVersion16Upgrader::extractStepMaps(const QVariantMap &deployMap)
{
    OldStepMaps result;
    result.defaultDisplayName = deployMap.value(QLatin1String("ProjectExplorer.ProjectConfiguration.DefaultDisplayName")).toString();
    result.displayName = deployMap.value(QLatin1String("ProjectExplorer.ProjectConfiguration.DisplayName")).toString();
    const QString stepListKey = QLatin1String("ProjectExplorer.BuildConfiguration.BuildStepList.0");
    QVariantMap stepListMap = deployMap.value(stepListKey).toMap();
    int stepCount = stepListMap.value(QLatin1String("ProjectExplorer.BuildStepList.StepsCount"), 0).toInt();
    QString stepKey = QLatin1String("ProjectExplorer.BuildStepList.Step.");
    for (int i = 0; i < stepCount; ++i) {
        QVariantMap stepMap = stepListMap.value(stepKey + QString::number(i)).toMap();
        const QString id = stepMap.value(QLatin1String("ProjectExplorer.ProjectConfiguration.Id")).toString();
        if (id == QLatin1String("Qt4ProjectManager.AndroidDeployQtStep"))
            result.androidDeployQt = stepMap;
        else if (id == QLatin1String("Qt4ProjectManager.AndroidPackageInstallationStep"))
            result.androidPackageInstall = stepMap;
        if (!result.isEmpty())
            return result;

    }
    return result;
}

QVariantMap UserFileVersion16Upgrader::removeAndroidPackageStep(QVariantMap deployMap)
{
    const QString stepListKey = QLatin1String("ProjectExplorer.BuildConfiguration.BuildStepList.0");
    QVariantMap stepListMap = deployMap.value(stepListKey).toMap();
    const QString stepCountKey = QLatin1String("ProjectExplorer.BuildStepList.StepsCount");
    int stepCount = stepListMap.value(stepCountKey, 0).toInt();
    QString stepKey = QLatin1String("ProjectExplorer.BuildStepList.Step.");
    int targetPosition = 0;
    for (int sourcePosition = 0; sourcePosition < stepCount; ++sourcePosition) {
        QVariantMap stepMap = stepListMap.value(stepKey + QString::number(sourcePosition)).toMap();
        if (stepMap.value(QLatin1String("ProjectExplorer.ProjectConfiguration.Id")).toString()
                != QLatin1String("Qt4ProjectManager.AndroidPackageInstallationStep")) {
            stepListMap.insert(stepKey + QString::number(targetPosition), stepMap);
            ++targetPosition;
        }
    }

    stepListMap.insert(stepCountKey, targetPosition);

    for (int i = targetPosition; i < stepCount; ++i)
        stepListMap.remove(stepKey + QString::number(i));

    deployMap.insert(stepListKey, stepListMap);
    return deployMap;
}

QVariantMap UserFileVersion16Upgrader::insertSteps(QVariantMap buildConfigurationMap,
                                                   const OldStepMaps &oldStepMap,
                                                   NamePolicy policy)
{
    const QString bslCountKey = QLatin1String("ProjectExplorer.BuildConfiguration.BuildStepListCount");
    int stepListCount = buildConfigurationMap.value(bslCountKey).toInt();

    const QString bslKey = QLatin1String("ProjectExplorer.BuildConfiguration.BuildStepList.");
    const QString bslTypeKey = QLatin1String("ProjectExplorer.ProjectConfiguration.Id");
    for (int bslNumber = 0; bslNumber < stepListCount; ++bslNumber) {
        QVariantMap buildStepListMap = buildConfigurationMap.value(bslKey + QString::number(bslNumber)).toMap();
        if (buildStepListMap.value(bslTypeKey) != QLatin1String("ProjectExplorer.BuildSteps.Build"))
            continue;

        const QString bslStepCountKey = QLatin1String("ProjectExplorer.BuildStepList.StepsCount");

        int stepCount = buildStepListMap.value(bslStepCountKey).toInt();
        buildStepListMap.insert(bslStepCountKey, stepCount + 2);

        QVariantMap androidPackageInstallStep;
        QVariantMap androidBuildApkStep;

        // common settings of all buildsteps
        const QString enabledKey = QLatin1String("ProjectExplorer.BuildStep.Enabled");
        const QString idKey = QLatin1String("ProjectExplorer.ProjectConfiguration.Id");
        const QString displayNameKey = QLatin1String("ProjectExplorer.ProjectConfiguration.DisplayName");
        const QString defaultDisplayNameKey = QLatin1String("ProjectExplorer.ProjectConfiguration.DefaultDisplayName");

        QString displayName = oldStepMap.androidPackageInstall.value(displayNameKey).toString();
        QString defaultDisplayName = oldStepMap.androidPackageInstall.value(defaultDisplayNameKey).toString();
        bool enabled = oldStepMap.androidPackageInstall.value(enabledKey).toBool();

        androidPackageInstallStep.insert(idKey, Core::Id("Qt4ProjectManager.AndroidPackageInstallationStep").toSetting());
        androidPackageInstallStep.insert(displayNameKey, displayName);
        androidPackageInstallStep.insert(defaultDisplayNameKey, defaultDisplayName);
        androidPackageInstallStep.insert(enabledKey, enabled);

        displayName = oldStepMap.androidDeployQt.value(displayName).toString();
        defaultDisplayName = oldStepMap.androidDeployQt.value(defaultDisplayNameKey).toString();
        enabled = oldStepMap.androidDeployQt.value(enabledKey).toBool();

        androidBuildApkStep.insert(idKey, Core::Id("QmakeProjectManager.AndroidBuildApkStep").toSetting());
        androidBuildApkStep.insert(displayNameKey, displayName);
        androidBuildApkStep.insert(defaultDisplayNameKey, defaultDisplayName);
        androidBuildApkStep.insert(enabledKey, enabled);

        // settings transferred from AndroidDeployQtStep to QmakeBuildApkStep
        const QString ProFilePathForInputFile = QLatin1String("ProFilePathForInputFile");
        const QString DeployActionKey = QLatin1String("Qt4ProjectManager.AndroidDeployQtStep.DeployQtAction");
        const QString KeystoreLocationKey = QLatin1String("KeystoreLocation");
        const QString BuildTargetSdkKey = QLatin1String("BuildTargetSdk");
        const QString VerboseOutputKey = QLatin1String("VerboseOutput");

        QString inputFile = oldStepMap.androidDeployQt.value(ProFilePathForInputFile).toString();
        int oldDeployAction = oldStepMap.androidDeployQt.value(DeployActionKey).toInt();
        QString keyStorePath = oldStepMap.androidDeployQt.value(KeystoreLocationKey).toString();
        QString buildTargetSdk = oldStepMap.androidDeployQt.value(BuildTargetSdkKey).toString();
        bool verbose = oldStepMap.androidDeployQt.value(VerboseOutputKey).toBool();
        androidBuildApkStep.insert(ProFilePathForInputFile, inputFile);
        androidBuildApkStep.insert(DeployActionKey, oldDeployAction);
        androidBuildApkStep.insert(KeystoreLocationKey, keyStorePath);
        androidBuildApkStep.insert(BuildTargetSdkKey, buildTargetSdk);
        androidBuildApkStep.insert(VerboseOutputKey, verbose);

        const QString buildStepKey = QLatin1String("ProjectExplorer.BuildStepList.Step.");
        buildStepListMap.insert(buildStepKey + QString::number(stepCount), androidPackageInstallStep);
        buildStepListMap.insert(buildStepKey + QString::number(stepCount + 1), androidBuildApkStep);

        buildConfigurationMap.insert(bslKey + QString::number(bslNumber), buildStepListMap);
    }

    if (policy == RenameBuildConfiguration) {
        const QString displayNameKey = QLatin1String("ProjectExplorer.ProjectConfiguration.DisplayName");
        const QString defaultDisplayNameKey = QLatin1String("ProjectExplorer.ProjectConfiguration.DefaultDisplayName");

        QString defaultDisplayName = buildConfigurationMap.value(defaultDisplayNameKey).toString();
        QString displayName = buildConfigurationMap.value(displayNameKey).toString();
        if (displayName.isEmpty())
            displayName = defaultDisplayName;
        QString oldDisplayname = oldStepMap.displayName;
        if (oldDisplayname.isEmpty())
            oldDisplayname = oldStepMap.defaultDisplayName;

        displayName.append(QLatin1String(" - "));
        displayName.append(oldDisplayname);
        buildConfigurationMap.insert(displayNameKey, displayName);

        defaultDisplayName.append(QLatin1String(" - "));
        defaultDisplayName.append(oldStepMap.defaultDisplayName);
        buildConfigurationMap.insert(defaultDisplayNameKey, defaultDisplayName);
    }

    return buildConfigurationMap;
}

QVariantMap UserFileVersion16Upgrader::upgrade(const QVariantMap &data)
{
    int targetCount = data.value(QLatin1String("ProjectExplorer.Project.TargetCount"), 0).toInt();
    if (!targetCount)
        return data;

    QVariantMap result = data;

    for (int i = 0; i < targetCount; ++i) {
        QString targetKey = QLatin1String("ProjectExplorer.Project.Target.") + QString::number(i);
        QVariantMap targetMap = data.value(targetKey).toMap();

        const QString dcCountKey = QLatin1String("ProjectExplorer.Target.DeployConfigurationCount");
        int deployconfigurationCount = targetMap.value(dcCountKey).toInt();
        if (!deployconfigurationCount) // should never happen
            continue;

        QList<OldStepMaps> oldSteps;
        QList<QVariantMap> oldBuildConfigurations;

        QString deployKey = QLatin1String("ProjectExplorer.Target.DeployConfiguration.");
        for (int j = 0; j < deployconfigurationCount; ++j) {
            QVariantMap deployConfigurationMap
                    = targetMap.value(deployKey + QString::number(j)).toMap();
            OldStepMaps oldStep = extractStepMaps(deployConfigurationMap);
            if (!oldStep.isEmpty()) {
                oldSteps.append(oldStep);
                deployConfigurationMap = removeAndroidPackageStep(deployConfigurationMap);
                targetMap.insert(deployKey + QString::number(j), deployConfigurationMap);
            }
        }

        if (oldSteps.isEmpty()) // no android target?
            continue;

        const QString bcCountKey = QLatin1String("ProjectExplorer.Target.BuildConfigurationCount");
        int buildConfigurationCount
                = targetMap.value(bcCountKey).toInt();

        if (!buildConfigurationCount) // should never happen
            continue;

        QString bcKey = QLatin1String("ProjectExplorer.Target.BuildConfiguration.");
        for (int j = 0; j < buildConfigurationCount; ++j) {
            QVariantMap oldBuildConfigurationMap = targetMap.value(bcKey + QString::number(j)).toMap();
            oldBuildConfigurations.append(oldBuildConfigurationMap);
        }

        QList<QVariantMap> newBuildConfigurations;

        NamePolicy policy = oldSteps.size() > 1 ? RenameBuildConfiguration : KeepName;

        foreach (const QVariantMap &oldBuildConfiguration, oldBuildConfigurations) {
            foreach (const OldStepMaps &oldStep, oldSteps) {
                QVariantMap newBuildConfiguration = insertSteps(oldBuildConfiguration, oldStep, policy);
                if (!newBuildConfiguration.isEmpty())
                    newBuildConfigurations.append(newBuildConfiguration);
            }
        }

        targetMap.insert(bcCountKey, newBuildConfigurations.size());

        for (int j = 0; j < newBuildConfigurations.size(); ++j)
            targetMap.insert(bcKey + QString::number(j), newBuildConfigurations.at(j));
        result.insert(targetKey, targetMap);
    }

    return result;
}

QVariantMap UserFileVersion17Upgrader::upgrade(const QVariantMap &map)
{
    m_sticky = map.value(QLatin1String(USER_STICKY_KEYS_KEY)).toList();
    if (m_sticky.isEmpty())
        return map;
    return process(map).toMap();
}

QVariant UserFileVersion17Upgrader::process(const QVariant &entry)
{
    switch (entry.type()) {
    case QVariant::List: {
        QVariantList result;
        foreach (const QVariant &item, entry.toList())
            result.append(process(item));
        return result;
    }
    case QVariant::Map: {
        QVariantMap result = entry.toMap();
        for (QVariantMap::iterator i = result.begin(), end = result.end(); i != end; ++i) {
            QVariant &v = i.value();
            v = process(v);
        }
        result.insert(QLatin1String(USER_STICKY_KEYS_KEY), m_sticky);
        return result;
    }
    default:
        return entry;
    }
}
