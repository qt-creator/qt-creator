/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "qtcassert.h"

#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QRegExp>

namespace {

const char ORIGINAL_VERSION_KEY[] = "OriginalVersion";
const char SETTINGS_ID_KEY[] = "EnvironmentId";
const char USER_STICKY_KEYS_KEY[] = "UserStickyKeys";
const char VERSION_KEY[] = "Version";

static QString generateSuffix(const QString &alt1, const QString &alt2)
{
    QString suffix = alt1;
    if (suffix.isEmpty())
        suffix = alt2;
    suffix.replace(QRegExp("[^a-zA-Z0-9_.-]"), QString('_')); // replace fishy characters:
    if (!suffix.startsWith('.'))
        suffix.prepend('.');
    return suffix;
}

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
            if (key == VERSION_KEY || key == SETTINGS_ID_KEY)
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
        if (key == ORIGINAL_VERSION_KEY || key == VERSION_KEY)
            return;
        if (!userMap.value(USER_STICKY_KEYS_KEY).toList().contains(key))
            userMap.insert(key, sharedValue);
    }
};


class TrackStickyness : public Operation
{
public:
    void apply(QVariantMap &userMap, const QString &key, const QVariant &)
    {
        const QString stickyKey = USER_STICKY_KEYS_KEY;
        QVariantList sticky = userMap.value(stickyKey).toList();
        sticky.append(key);
        userMap.insert(stickyKey, sticky);
    }
};

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

} // end namespace

namespace Utils {

// --------------------------------------------------------------------
// BasicSettingsAccessor:
// --------------------------------------------------------------------

BasicSettingsAccessor::BasicSettingsAccessor(const FileName &baseFilePath, const QString &docType) :
    m_baseFilePath(baseFilePath),
    m_docType(docType)
{
    QTC_CHECK(!m_baseFilePath.isEmpty());
    QTC_CHECK(!m_docType.isEmpty());
}

BasicSettingsAccessor::~BasicSettingsAccessor() = default;

QVariantMap BasicSettingsAccessor::restoreSettings(QWidget *parent) const
{
    Q_UNUSED(parent);
    return readFile(m_baseFilePath);
}

bool BasicSettingsAccessor::saveSettings(const QVariantMap &data, QWidget *parent) const
{
    return writeFile(m_baseFilePath, data, parent);
}

QVariantMap BasicSettingsAccessor::readFile(const FileName &path) const
{
    PersistentSettingsReader reader;
    if (!reader.load(path))
        return QVariantMap();

    return reader.restoreValues();
}

bool BasicSettingsAccessor::writeFile(const FileName &path, const QVariantMap &data, QWidget *parent) const
{
    if (data.isEmpty())
        return false;

    if (!m_writer || m_writer->fileName() != path)
        m_writer = std::make_unique<PersistentSettingsWriter>(path, m_docType);

    return m_writer->save(data, parent);
}

FileName BasicSettingsAccessor::baseFilePath() const
{
    return m_baseFilePath;
}

// -----------------------------------------------------------------------------
// VersionUpgrader:
// -----------------------------------------------------------------------------

VersionUpgrader::VersionUpgrader(const int version, const QString &extension) :
    m_version(version), m_extension(extension)
{ }

int VersionUpgrader::version() const
{
    QTC_CHECK(m_version >= 0);
    return m_version;
}

QString VersionUpgrader::backupExtension() const
{
    QTC_CHECK(!m_extension.isEmpty());
    return m_extension;
}

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

// --------------------------------------------------------------------
// SettingsAccessorPrivate:
// --------------------------------------------------------------------

class SettingsAccessorPrivate
{
public:
    // The relevant data from the settings currently in use.
    class Settings
    {
    public:
        QVariantMap map;
        FileName path;
    };

    int firstVersion() const { return m_upgraders.size() == 0 ? -1 : m_upgraders.front()->version(); }
    int lastVersion() const  { return m_upgraders.size() == 0 ? -1 : m_upgraders.back()->version(); }
    int currentVersion() const { return lastVersion() + 1; }
    VersionUpgrader *upgrader(const int version) const
    {
        QTC_ASSERT(version >= 0 && firstVersion() >= 0, return nullptr);
        const int pos = version - firstVersion();
        if (pos >= 0 && pos < static_cast<int>(m_upgraders.size()))
            return m_upgraders[static_cast<size_t>(pos)].get();
        return nullptr;
    }
    Settings bestSettings(const SettingsAccessor *accessor, const FileNameList &pathList);

    std::vector<std::unique_ptr<VersionUpgrader>> m_upgraders;
    std::unique_ptr<PersistentSettingsWriter> m_writer;
    QByteArray m_settingsId;
    QString m_displayName;
    QString m_applicationDisplayName;

    QString m_userSuffix;
    QString m_sharedSuffix;
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
    const QChar slash('/');
    // Windows network shares: "//server.domain-a.com/foo' -> 'serverdomainacom/foo'
    if (path.startsWith("//")) {
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
    if (path.size() > 3 && path.at(1) == ':') {
        path.remove(1, 1);
        path[0] = path.at(0).toLower();
        return path;
    }
    if (path.startsWith(slash)) // Standard UNIX paths: '/foo' -> 'foo'
        path.remove(0, 1);
    return path;
}

// Return complete file path of the .user file.
static FileName userFilePath(const Utils::FileName &projectFilePath, const QString &suffix)
{
    FileName result;
    if (sharedUserFileDir().isEmpty()) {
        result = projectFilePath;
    } else {
        // Recreate the relative project file hierarchy under the shared directory.
        // PersistentSettingsWriter::write() takes care of creating the path.
        result = FileName::fromString(sharedUserFileDir());
        result.appendString('/' + makeRelative(projectFilePath.toString()));
    }
    result.appendString(suffix);
    return result;
}

// -----------------------------------------------------------------------------
// SettingsAccessor:
// -----------------------------------------------------------------------------

SettingsAccessor::SettingsAccessor(const Utils::FileName &baseFile, const QString &docType) :
    BasicSettingsAccessor(baseFile, docType),
    d(new SettingsAccessorPrivate)
{
    d->m_userSuffix = generateSuffix(QString::fromLocal8Bit(qgetenv("QTC_EXTENSION")), ".user");
    d->m_sharedSuffix = generateSuffix(QString::fromLocal8Bit(qgetenv("QTC_SHARED_EXTENSION")), ".shared");
}

SettingsAccessor::~SettingsAccessor()
{
    delete d;
}

int SettingsAccessor::versionFromMap(const QVariantMap &data)
{
    return data.value(VERSION_KEY, -1).toInt();
}

int SettingsAccessor::originalVersionFromMap(const QVariantMap &data)
{
    return data.value(ORIGINAL_VERSION_KEY, versionFromMap(data)).toInt();
}

QVariantMap SettingsAccessor::setOriginalVersionInMap(const QVariantMap &data, int version)
{
    QVariantMap result = data;
    result.insert(ORIGINAL_VERSION_KEY, version);
    return result;
}

QVariantMap SettingsAccessor::setVersionInMap(const QVariantMap &data, int version)
{
    QVariantMap result = data;
    result.insert(VERSION_KEY, version);
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
 *
 * Compares \a newData against \a origData.
 *
 * Returns \c true if \a newData is a better match than \a origData and \c false otherwise.
 */
bool SettingsAccessor::isBetterMatch(const QVariantMap &origData, const QVariantMap &newData) const
{
    const int origVersion = versionFromMap(origData);
    const bool origValid = isValidVersionAndId(origVersion, settingsIdFromMap(origData));

    const int newVersion = versionFromMap(newData);
    const bool newValid = isValidVersionAndId(newVersion, settingsIdFromMap(newData));

    if (!origValid)
        return newValid;

    if (!newValid)
        return false;

    return newVersion > origVersion;
}

bool SettingsAccessor::isValidVersionAndId(const int version, const QByteArray &id) const
{
    const QByteArray requiredId = settingsId();
    const int firstVersion = firstSupportedVersion();
    const int lastVersion = currentVersion();

    return (version >= firstVersion && version <= lastVersion)
            && ( id == requiredId || requiredId.isEmpty());
}

/*!
 * Upgrade the settings in \a data to the version \a toVersion.
 *
 * Returns settings of the requested version.
 */
QVariantMap SettingsAccessor::upgradeSettings(const QVariantMap &data) const
{
    return upgradeSettings(data, currentVersion());
}

QVariantMap SettingsAccessor::upgradeSettings(const QVariantMap &data, int targetVersion) const
{
    QTC_ASSERT(targetVersion >= firstSupportedVersion(), return data);
    QTC_ASSERT(targetVersion <= currentVersion(), return data);

    const int version = versionFromMap(data);
    if (!isValidVersionAndId(version, settingsIdFromMap(data)))
        return data;

    QVariantMap result;
    if (!data.contains(ORIGINAL_VERSION_KEY))
        result = setOriginalVersionInMap(data, version);
    else
        result = data;

    for (int i = version; i < targetVersion; ++i) {
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
    if (!path.exists())
        return Continue;

    const Utils::optional<IssueInfo> issue = findIssues(data, path);
    if (!issue)
        return Continue;

    const IssueInfo &details = issue.value();

    const QMessageBox::Icon icon
            = details.buttons.count() > 1 ? QMessageBox::Question : QMessageBox::Information;
    const QMessageBox::StandardButtons buttons = [&details]()
    {
        QMessageBox::StandardButtons buttons = QMessageBox::NoButton;
        for (const QMessageBox::StandardButton &b : details.buttons.keys())
            buttons |= b;
        return buttons;
    }();
    QTC_ASSERT(buttons != QMessageBox::NoButton, return Continue);

    QMessageBox msgBox(icon, details.title, details.message, buttons, parent);
    if (details.defaultButton != QMessageBox::NoButton)
        msgBox.setDefaultButton(details.defaultButton);
    if (details.escapeButton != QMessageBox::NoButton)
        msgBox.setEscapeButton(details.escapeButton);

    int boxAction = msgBox.exec();
    return details.buttons.value(static_cast<QMessageBox::StandardButton>(boxAction));
}

/*!
 * Checks \a data located at \a path for issues to be displayed with reportIssues.
 *
 * Returns a IssueInfo object which is then used by reportIssues.
 */
Utils::optional<SettingsAccessor::IssueInfo>
SettingsAccessor::findIssues(const QVariantMap &data, const FileName &path) const
{
    const FileName defaultSettingsPath = userFilePath(baseFilePath(), d->m_userSuffix);
    const int version = versionFromMap(data);
    if (data.isEmpty() || version < firstSupportedVersion() || version > currentVersion()) {
        IssueInfo result;
        result.title = QApplication::translate("Utils::SettingsAccessor", "No Valid Settings Found");
        result.message = QApplication::translate("Utils::SettingsAccessor",
                                                 "<p>No valid settings file could be found.</p>"
                                                 "<p>All settings files found in directory \"%1\" "
                                                 "were either too new or too old to be read.</p>")
                .arg(path.toUserOutput());
        result.buttons.insert(QMessageBox::Ok, DiscardAndContinue);
        return result;
    }
    if ((path != defaultSettingsPath) && (version < currentVersion())) {
        IssueInfo result;
        result.title = QApplication::translate("Utils::SettingsAccessor", "Using Old Settings");
        result.message = QApplication::translate("Utils::SettingsAccessor",
                                                 "<p>The versioned backup \"%1\" of the settings "
                                                 "file is used, because the non-versioned file was "
                                                 "created by an incompatible version of %2.</p>"
                                                 "<p>Settings changes made since the last time this "
                                                 "version of %2 was used are ignored, and "
                                                 "changes made now will <b>not</b> be propagated to "
                                                 "the newer version.</p>").arg(path.toUserOutput())
                .arg(d->m_applicationDisplayName);
        result.buttons.insert(QMessageBox::Ok, Continue);
        return result;
    }

    const QByteArray readId = settingsIdFromMap(data);
    if (!readId.isEmpty() && readId != settingsId()) {
        IssueInfo result;
        result.title = differentEnvironmentMsg(d->m_displayName);
        result.message = QApplication::translate("Utils::EnvironmentIdAccessor",
                                                 "<p>No .user settings file created by this instance "
                                                 "of %1 was found.</p>"
                                                 "<p>Did you work with this project on another machine or "
                                                 "using a different settings path before?</p>"
                                                 "<p>Do you still want to load the settings file \"%2\"?</p>")
                .arg(d->m_applicationDisplayName)
                .arg(path.toUserOutput());
        result.defaultButton = QMessageBox::No;
        result.escapeButton = QMessageBox::No;
        result.buttons.insert(QMessageBox::Yes, SettingsAccessor::Continue);
        result.buttons.insert(QMessageBox::No, SettingsAccessor::DiscardAndContinue);
        return result;
    }

    return Utils::nullopt;
}

void SettingsAccessor::storeSharedSettings(const QVariantMap &data) const
{
    Q_UNUSED(data);
}

QVariant SettingsAccessor::retrieveSharedSettings() const
{
    return QVariant();
}

QString SettingsAccessor::differentEnvironmentMsg(const QString &projectName)
{
    return QApplication::translate("Utils::EnvironmentIdAccessor",
                                   "Settings File for \"%1\" from a different Environment?")
            .arg(projectName);
}

QByteArray SettingsAccessor::settingsIdFromMap(const QVariantMap &data)
{
    return data.value(SETTINGS_ID_KEY).toByteArray();
}

QVariantMap SettingsAccessor::restoreSettings(QWidget *parent) const
{
    QTC_ASSERT(d->lastVersion() >= 0, return QVariantMap());

    const QVariantMap userSettings = readUserSettings(parent);
    const QVariantMap sharedSettings = readSharedSettings(parent);
    return mergeSettings(userSettings, sharedSettings);
}

QVariantMap SettingsAccessor::prepareToSaveSettings(const QVariantMap &data) const
{
    QVariantMap tmp = data;
    const QVariant &shared = retrieveSharedSettings();
    if (shared.isValid())
        trackUserStickySettings(tmp, shared.toMap());

    tmp.insert(VERSION_KEY, d->currentVersion());
    tmp.insert(SETTINGS_ID_KEY, settingsId());

    return tmp;
}

bool SettingsAccessor::saveSettings(const QVariantMap &map, QWidget *parent) const
{
    if (map.isEmpty())
        return false;

    backupUserFile();

    QVariantMap data = prepareToSaveSettings(map);

    FileName path = FileName::fromString(defaultFileName(d->m_userSuffix));
    return writeFile(path, data, parent);
}

bool SettingsAccessor::addVersionUpgrader(std::unique_ptr<VersionUpgrader> upgrader)
{
    QTC_ASSERT(upgrader.get(), return false);
    const int version = upgrader->version();
    QTC_ASSERT(version >= 0, return false);

    const bool haveUpgraders = d->m_upgraders.size() != 0;
    QTC_ASSERT(!haveUpgraders || d->currentVersion() == version, return false);
    d->m_upgraders.push_back(std::move(upgrader));
    return true;
}

void SettingsAccessor::setSettingsId(const QByteArray &id)
{
    d->m_settingsId = id;
}

void SettingsAccessor::setDisplayName(const QString &dn)
{
    d->m_displayName = dn;
}

void SettingsAccessor::setApplicationDisplayName(const QString &dn)
{
    d->m_applicationDisplayName = dn;
}

/* Will always return the default name first (if applicable) */
FileNameList SettingsAccessor::settingsFiles(const QString &suffix) const
{
    FileNameList result;

    QFileInfoList list;
    const QFileInfo pfi = baseFilePath().toFileInfo();
    const QStringList filter(pfi.fileName() + suffix + '*');

    if (!sharedUserFileDir().isEmpty()) {
        const QString sharedPath = sharedUserFileDir() + '/' + makeRelative(pfi.absolutePath());
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

QByteArray SettingsAccessor::settingsId() const
{
    return d->m_settingsId;
}

QString SettingsAccessor::displayName() const
{
    return d->m_displayName;
}

QString SettingsAccessor::defaultFileName(const QString &suffix) const
{
    return userFilePath(baseFilePath(), suffix).toString();
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
    QString backupName = defaultFileName(d->m_userSuffix);
    const QByteArray oldEnvironmentId = settingsIdFromMap(data);
    if (!oldEnvironmentId.isEmpty() && oldEnvironmentId != settingsId())
        backupName += '.' + QString::fromLatin1(oldEnvironmentId).mid(1, 7);
    const int oldVersion = versionFromMap(data);
    if (oldVersion != currentVersion()) {
        VersionUpgrader *upgrader = d->upgrader(oldVersion);
        if (upgrader)
            backupName += '.' + upgrader->backupExtension();
        else
            backupName += '.' + QString::number(oldVersion);
    }
    return FileName::fromString(backupName);
}

void SettingsAccessor::backupUserFile() const
{
    SettingsAccessorPrivate::Settings oldSettings;
    oldSettings.path = FileName::fromString(defaultFileName(d->m_userSuffix));
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
    FileNameList fileList = settingsFiles(d->m_userSuffix);
    if (fileList.isEmpty()) // No settings found at all.
        return result.map;

    result = d->bestSettings(this, fileList);
    if (result.path.isEmpty())
        result.path = baseFilePath().parentDir();

    ProceedInfo proceed = reportIssues(result.map, result.path, parent);
    if (proceed == DiscardAndContinue)
        return QVariantMap();

    return result.map;
}

QVariantMap SettingsAccessor::readSharedSettings(QWidget *parent) const
{
    SettingsAccessorPrivate::Settings sharedSettings;
    QString fn = baseFilePath().toString() + d->m_sharedSuffix;
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
                    QApplication::translate("Utils::SettingsAccessor",
                                            "Unsupported Shared Settings File"),
                    QApplication::translate("Utils::SettingsAccessor",
                                            "The version of your .shared file is not "
                                            "supported by %1. "
                                            "Do you want to try loading it anyway?")
                    .arg(d->m_applicationDisplayName),
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
        const QVariantMap tmp = accessor->prepareSettings(accessor->readFile(path));

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

    storeSharedSettings(newShared);

    // Update from the base version to Creator's version.
    return upgradeSettings(result);
}

} // namespace Utils
