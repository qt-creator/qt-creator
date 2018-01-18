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

#include "algorithm.h"
#include "asconst.h"
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

static QString generateSuffix(const QString &suffix)
{
    QString result = suffix;
    result.replace(QRegExp("[^a-zA-Z0-9_.-]"), QString('_')); // replace fishy characters:
    if (!result.startsWith('.'))
        result.prepend('.');
    return result;
}

// FIXME: Remove this!
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

} // namespace

namespace Utils {

// --------------------------------------------------------------------
// BasicSettingsAccessor::Issue:
// --------------------------------------------------------------------

QMessageBox::StandardButtons BasicSettingsAccessor::Issue::allButtons() const
{
    QMessageBox::StandardButtons result = QMessageBox::NoButton;
    for (const QMessageBox::StandardButton &b : buttons.keys())
        result |= b;
    return result;
}

// --------------------------------------------------------------------
// BasicSettingsAccessor:
// --------------------------------------------------------------------

/*!
 * The BasicSettingsAccessor can be used to read/write settings in XML format.
 */
BasicSettingsAccessor::BasicSettingsAccessor(const QString &docType,
                                             const QString &displayName,
                                             const QString &applicationDisplayName) :
    docType(docType),
    displayName(displayName),
    applicationDisplayName(applicationDisplayName)
{
    QTC_CHECK(!docType.isEmpty());
    QTC_CHECK(!displayName.isEmpty());
    QTC_CHECK(!applicationDisplayName.isEmpty());
}

/*!
 * Restore settings from disk and report any issues in a message box centered on \a parent.
 */
QVariantMap BasicSettingsAccessor::restoreSettings(QWidget *parent) const
{
    QTC_ASSERT(!m_baseFilePath.isEmpty(), return QVariantMap());

    const RestoreData result = readData(m_baseFilePath, parent);
    const ProceedInfo pi = result.issue ? reportIssues(result.issue.value(), result.path, parent) : ProceedInfo::Continue;
    return pi == ProceedInfo::DiscardAndContinue ? QVariantMap() : result.data;
}

/*!
 * Save \a data to disk and report any issues in a message box centered on \a parent.
 */
bool BasicSettingsAccessor::saveSettings(const QVariantMap &data, QWidget *parent) const
{
    const optional<Issue> result = writeData(m_baseFilePath, data);
    const ProceedInfo pi = result ? reportIssues(result.value(), m_baseFilePath, parent) : ProceedInfo::Continue;
    return pi == ProceedInfo::Continue;
}

/*!
 * Read data from \a path. Do all the necessary postprocessing of the data.
 */
BasicSettingsAccessor::RestoreData BasicSettingsAccessor::readData(const FileName &path,
                                                                   QWidget *parent) const
{
    Q_UNUSED(parent);
    RestoreData result = readFile(path);
    if (!result.data.isEmpty())
        result.data = preprocessReadSettings(result.data);
    return result;
}

/*!
 * Store the \a data in \a path on disk. Do all the necessary preprocessing of the data.
 */
Utils::optional<BasicSettingsAccessor::Issue>
BasicSettingsAccessor::writeData(const FileName &path, const QVariantMap &data) const
{
    return writeFile(path, prepareToWriteSettings(data));
}

/*!
 * Read a file at \a path from disk and extract the data into a RestoreData set.
 *
 * This method does not do *any* processing of the file contents.
 */
BasicSettingsAccessor::RestoreData BasicSettingsAccessor::readFile(const FileName &path) const
{
    PersistentSettingsReader reader;
    if (!reader.load(path)) {
        return RestoreData(Issue(QCoreApplication::translate("Utils::SettingsAccessor", "Failed to Read File"),
                                 QCoreApplication::translate("Utils::SettingsAccessor", "Could not open \"%1\".")
                                 .arg(path.toUserOutput())));
    }

    return RestoreData(path, reader.restoreValues());
}

/*!
 * Write a file at \a path to disk and store the \a data in it.
 *
 * This method does not do *any* processing of the file contents.
 */
Utils::optional<BasicSettingsAccessor::Issue>
BasicSettingsAccessor::writeFile(const FileName &path, const QVariantMap &data) const
{
    if (data.isEmpty()) {
        return Issue(QCoreApplication::translate("Utils::SettingsAccessor", "Failed to Write File"),
                     QCoreApplication::translate("Utils::SettingsAccessor", "There was nothing to write."));
    }

    QString errorMessage;
    if (!m_writer || m_writer->fileName() != path)
        m_writer = std::make_unique<PersistentSettingsWriter>(path, docType);

    if (!m_writer->save(data, &errorMessage)) {
        return Issue(QCoreApplication::translate("Utils::SettingsAccessor", "Failed to Write File"),
                     errorMessage);
    }
    return {};
}

BasicSettingsAccessor::ProceedInfo
BasicSettingsAccessor::reportIssues(const BasicSettingsAccessor::Issue &issue, const FileName &path,
                                    QWidget *parent) const
{
    if (!path.exists())
        return Continue;

    const QMessageBox::Icon icon
            = issue.buttons.count() > 1 ? QMessageBox::Question : QMessageBox::Information;
    const QMessageBox::StandardButtons buttons = issue.allButtons();
    QTC_ASSERT(buttons != QMessageBox::NoButton, return Continue);

    QMessageBox msgBox(icon, issue.title, issue.message, buttons, parent);
    if (issue.defaultButton != QMessageBox::NoButton)
        msgBox.setDefaultButton(issue.defaultButton);
    if (issue.escapeButton != QMessageBox::NoButton)
        msgBox.setEscapeButton(issue.escapeButton);

    int boxAction = msgBox.exec();
    return issue.buttons.value(static_cast<QMessageBox::StandardButton>(boxAction));
}

/*!
 * This method is called right after reading data from disk and modifies \a data.
 */
QVariantMap BasicSettingsAccessor::preprocessReadSettings(const QVariantMap &data) const
{
    return data;
}

/*!
 * This method is called right before writing data to disk and modifies \a data.
 */
QVariantMap BasicSettingsAccessor::prepareToWriteSettings(const QVariantMap &data) const
{
    return data;
}

// --------------------------------------------------------------------
// BackingUpSettingsAccessor:
// --------------------------------------------------------------------

FileNameList BackUpStrategy::readFileCandidates(const FileName &baseFileName) const
{

    const QFileInfo pfi = baseFileName.toFileInfo();
    const QStringList filter(pfi.fileName() + '*');
    const QFileInfoList list = QDir(pfi.dir()).entryInfoList(filter, QDir::Files | QDir::Hidden | QDir::System);

    return Utils::transform(list, [](const QFileInfo &fi) { return FileName::fromString(fi.absoluteFilePath()); });
}

int BackUpStrategy::compare(const BasicSettingsAccessor::RestoreData &data1,
                            const BasicSettingsAccessor::RestoreData &data2) const
{
    if (!data1.issue && !data1.data.isEmpty())
        return -1;

    if (!data2.issue && !data2.data.isEmpty())
        return 1;

    return 0;
}

Utils::optional<Utils::FileName>
BackUpStrategy::backupName(const QVariantMap &oldData, const FileName &path, const QVariantMap &data) const
{
    if (oldData == data)
        return Utils::nullopt;
    FileName backup = path;
    backup.appendString(".bak");
    return backup;
}

int VersionedBackUpStrategy::compare(const BasicSettingsAccessor::RestoreData &data1,
                                     const BasicSettingsAccessor::RestoreData &data2) const
{
    const int origVersion = versionFromMap(data1.data);
    const bool origValid = isValidVersionAndId(origVersion, settingsIdFromMap(data1.data));

    const int newVersion = versionFromMap(data2.data);
    const bool newValid = isValidVersionAndId(newVersion, settingsIdFromMap(data2.data));

    if ((!origValid && !newValid) || (origValid && newValid && origVersion == newVersion))
        return 0;
    if ((!origValid &&  newValid) || (origValid && newValid && origVersion < newVersion))
        return 1;
    return -1;
}

Utils::optional<Utils::FileName>
VersionedBackUpStrategy::backupName(const QVariantMap &oldData, const FileName &path, const QVariantMap &data) const
{
    Q_UNUSED(oldData);
    FileName backupName = path;
    const QByteArray oldEnvironmentId = settingsIdFromMap(data);
    const int oldVersion = versionFromMap(data);

    if (!oldEnvironmentId.isEmpty() && oldEnvironmentId != id())
        backupName.appendString('.' + QString::fromLatin1(oldEnvironmentId).mid(1, 7));
    if (oldVersion != currentVersion())
        backupName.appendString('.' + QString::number(oldVersion));

    if (backupName == path)
        return Utils::nullopt;
    return backupName;
}

bool VersionedBackUpStrategy::isValidVersionAndId(const int version, const QByteArray &otherId) const
{
    return (version >= firstSupportedVersion() && version <= currentVersion())
            && (otherId == id() || id().isEmpty());
}

BackingUpSettingsAccessor::BackingUpSettingsAccessor(const QString &docType,
                                                     const QString &displayName,
                                                     const QString &applicationDisplayName) :
    BackingUpSettingsAccessor(std::make_unique<BackUpStrategy>(), docType, displayName, applicationDisplayName)
{ }

BackingUpSettingsAccessor::BackingUpSettingsAccessor(std::unique_ptr<BackUpStrategy> &&strategy,
                                                     const QString &docType,
                                                     const QString &displayName,
                                                     const QString &applicationDisplayName) :
    BasicSettingsAccessor(docType, displayName, applicationDisplayName),
    m_strategy(std::move(strategy))
{ }

BasicSettingsAccessor::RestoreData
BackingUpSettingsAccessor::readData(const Utils::FileName &path, QWidget *parent) const
{
    const FileNameList fileList = readFileCandidates(path);
    if (fileList.isEmpty()) // No settings found at all.
        return RestoreData(path, QVariantMap());

    RestoreData result = bestReadFileData(fileList, parent);
    if (result.path.isEmpty())
        result.path = baseFilePath().parentDir();

    return result;
}

Utils::optional<BasicSettingsAccessor::Issue>
BackingUpSettingsAccessor::writeData(const Utils::FileName &path, const QVariantMap &data) const
{
    if (data.isEmpty())
        return {};

    backupFile(path, data);

    return BasicSettingsAccessor::writeData(path, data);

}

FileNameList BackingUpSettingsAccessor::readFileCandidates(const Utils::FileName &path) const
{
    FileNameList result = Utils::filteredUnique(m_strategy->readFileCandidates(path));
    if (result.removeOne(baseFilePath()))
        result.prepend(baseFilePath());

    return result;
}

BasicSettingsAccessor::RestoreData
BackingUpSettingsAccessor::bestReadFileData(const FileNameList &candidates, QWidget *parent) const
{
    BasicSettingsAccessor::RestoreData bestMatch;
    for (const FileName &c : candidates) {
        RestoreData cData = BasicSettingsAccessor::readData(c, parent);
        if (m_strategy->compare(bestMatch, cData) > 0)
            bestMatch = cData;
    }
    return bestMatch;
}

void BackingUpSettingsAccessor::backupFile(const FileName &path, const QVariantMap &data) const
{
    RestoreData oldSettings = BasicSettingsAccessor::readFile(path);
    if (oldSettings.data.isEmpty())
        return;

    // Do we need to do a backup?
    const QString origName = path.toString();
    optional<FileName> backupFileName = m_strategy->backupName(oldSettings.data, path, data);
    if (backupFileName)
        QFile::copy(origName, backupFileName.value().toString());
}

// --------------------------------------------------------------------
// SettingsAccessorPrivate:
// --------------------------------------------------------------------

class SettingsAccessorPrivate
{
public:
    SettingsAccessorPrivate(const FileName &projectFilePath) : m_projectFilePath(projectFilePath) { }

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

    std::vector<std::unique_ptr<VersionUpgrader>> m_upgraders;
    QByteArray m_settingsId;

    std::unique_ptr<BackingUpSettingsAccessor> m_sharedFile;

    const FileName m_projectFilePath;
};

// Return path to shared directory for .user files, create if necessary.
static inline Utils::optional<QString> defineExternalUserFileDir()
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
        return nullopt;
    }
    QDir dir;
    if (!dir.mkpath(path)) {
        qWarning() << "Cannot create: " << QDir::toNativeSeparators(path);
        return nullopt;
    }
    return path;
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
static FileName externalUserFilePath(const Utils::FileName &projectFilePath, const QString &suffix)
{
    FileName result;
    static const optional<QString> externalUserFileDir = defineExternalUserFileDir();

    if (!externalUserFileDir) {
        // Recreate the relative project file hierarchy under the shared directory.
        // PersistentSettingsWriter::write() takes care of creating the path.
        result = FileName::fromString(externalUserFileDir.value());
        result.appendString('/' + makeRelative(projectFilePath.toString()));
        result.appendString(suffix);
    }
    return result;
}

// --------------------------------------------------------------------
// VersionUpgrader:
// --------------------------------------------------------------------

VersionUpgrader::VersionUpgrader(const int version, const QString &extension) :
    m_version(version), m_extension(extension)
{
    QTC_CHECK(m_version >= 0);
}

int VersionUpgrader::version() const
{
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

// -----------------------------------------------------------------------------
// SettingsAccessor:
// -----------------------------------------------------------------------------

SettingsAccessor::SettingsAccessor(std::unique_ptr<BackUpStrategy> &&strategy,
                                   const Utils::FileName &baseFile, const QString &docType,
                                   const QString &displayName, const QString &appDisplayName) :
    BackingUpSettingsAccessor(std::move(strategy), docType, displayName, appDisplayName),
    d(new SettingsAccessorPrivate(baseFile))
{
    const FileName externalUser = externalUserFile();
    const FileName projectUser = projectUserFile();
    setBaseFilePath(externalUser.isEmpty() ? projectUser : externalUser);

    d->m_sharedFile
            = std::make_unique<BackingUpSettingsAccessor>(std::make_unique<NoBackUpStrategy>(),
                                                          docType, displayName, appDisplayName);
    d->m_sharedFile->setBaseFilePath(sharedFile());
}

SettingsAccessor::~SettingsAccessor()
{
    delete d;
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
    VersionedBackUpStrategy *strat = dynamic_cast<VersionedBackUpStrategy *>(strategy());
    if (!strat || !strat->isValidVersionAndId(version, settingsIdFromMap(data)))
        return data;

    QVariantMap result = data;
    if (!data.contains(ORIGINAL_VERSION_KEY))
        setOriginalVersionInMap(result, version);

    for (int i = version; i < targetVersion; ++i) {
        VersionUpgrader *upgrader = d->upgrader(i);
        QTC_CHECK(upgrader && upgrader->version() == i);
        result = upgrader->upgrade(result);
        setVersionInMap(result, i + 1);
    }

    return result;
}

QVariantMap SettingsAccessor::prepareToWriteSettings(const QVariantMap &data) const
{
    QVariantMap result = data;
    setVersionInMap(result, currentVersion());
    setSettingsIdInMap(result, settingsId());
    return result;
}

/*!
 * Checks \a data located at \a path for issues to be displayed with reportIssues.
 *
 * Returns a IssueInfo object which is then used by reportIssues.
 */
Utils::optional<SettingsAccessor::Issue>
SettingsAccessor::findIssues(const QVariantMap &data, const FileName &path) const
{
    const int version = versionFromMap(data);
    if (data.isEmpty() || version < firstSupportedVersion() || version > currentVersion()) {
        Issue result(QApplication::translate("Utils::SettingsAccessor", "No Valid Settings Found"),
                     QApplication::translate("Utils::SettingsAccessor",
                                             "<p>No valid settings file could be found.</p>"
                                             "<p>All settings files found in directory \"%1\" "
                                             "were either too new or too old to be read.</p>")
                     .arg(path.toUserOutput()));
        result.buttons.insert(QMessageBox::Ok, DiscardAndContinue);
        return result;
    }
    if ((path != baseFilePath()) && (version < currentVersion())) {
        Issue result(QApplication::translate("Utils::SettingsAccessor", "Using Old Settings"),
                     QApplication::translate("Utils::SettingsAccessor",
                                             "<p>The versioned backup \"%1\" of the settings "
                                             "file is used, because the non-versioned file was "
                                             "created by an incompatible version of %2.</p>"
                                             "<p>Settings changes made since the last time this "
                                             "version of %2 was used are ignored, and "
                                             "changes made now will <b>not</b> be propagated to "
                                             "the newer version.</p>")
                     .arg(path.toUserOutput())
                     .arg(applicationDisplayName));
        result.buttons.insert(QMessageBox::Ok, Continue);
        return result;
    }

    const QByteArray readId = settingsIdFromMap(data);
    if (!readId.isEmpty() && readId != settingsId()) {
        Issue result(differentEnvironmentMsg(displayName),
                     QApplication::translate("Utils::EnvironmentIdAccessor",
                                             "<p>No .user settings file created by this instance "
                                             "of %1 was found.</p>"
                                             "<p>Did you work with this project on another machine or "
                                             "using a different settings path before?</p>"
                                             "<p>Do you still want to load the settings file \"%2\"?</p>")
                     .arg(applicationDisplayName)
                     .arg(path.toUserOutput()));
        result.defaultButton = QMessageBox::No;
        result.escapeButton = QMessageBox::No;
        result.buttons.insert(QMessageBox::Yes, Continue);
        result.buttons.insert(QMessageBox::No, DiscardAndContinue);
        return result;
    }

    return Utils::nullopt;
}

void SettingsAccessor::storeSharedSettings(const QVariantMap &data) const
{
    Q_UNUSED(data);
}

QString SettingsAccessor::differentEnvironmentMsg(const QString &projectName)
{
    return QApplication::translate("Utils::EnvironmentIdAccessor",
                                   "Settings File for \"%1\" from a Different Environment?")
            .arg(projectName);
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

FileName SettingsAccessor::projectUserFile() const
{
    static const QString qtcExt = QLatin1String(qgetenv("QTC_EXTENSION"));
    FileName projectUserFile = d->m_projectFilePath;
    projectUserFile.appendString(generateSuffix(qtcExt.isEmpty() ? ".user" : qtcExt));
    return projectUserFile;
}

FileName SettingsAccessor::externalUserFile() const
{
    static const QString qtcExt = QLatin1String(qgetenv("QTC_EXTENSION"));
    return externalUserFilePath(d->m_projectFilePath, generateSuffix(qtcExt.isEmpty() ? ".user" : qtcExt));
}

FileName SettingsAccessor::sharedFile() const
{
    static const QString qtcExt = QLatin1String(qgetenv("QTC_SHARED_EXTENSION"));
    FileName sharedFile = d->m_projectFilePath;
    sharedFile.appendString(generateSuffix(qtcExt.isEmpty() ? ".shared" : qtcExt));
    return sharedFile;
}

BasicSettingsAccessor::RestoreData SettingsAccessor::readData(const FileName &path,
                                                              QWidget *parent) const
{
    Q_UNUSED(path); // FIXME: This is wrong!

    // FIXME: Do better error handling:
    QTC_ASSERT(d->lastVersion() >= 0, return RestoreData("SETUP FAILED", "SETUP FAILED"));

    RestoreData userSettings = BackingUpSettingsAccessor::readData(path, parent);
    if (userSettings.issue && reportIssues(userSettings.issue.value(), userSettings.path, parent) == DiscardAndContinue)
        userSettings.data.clear();

    RestoreData sharedSettings = readSharedSettings(parent);
    if (sharedSettings.issue && reportIssues(sharedSettings.issue.value(), sharedSettings.path, parent) == DiscardAndContinue)
        sharedSettings.data.clear();
    RestoreData mergedSettings = RestoreData(userSettings.path,
                                             mergeSettings(userSettings.data, sharedSettings.data));
    mergedSettings.issue = findIssues(mergedSettings.data, mergedSettings.path);
    return mergedSettings;
}

void SettingsAccessor::setSettingsId(const QByteArray &id)
{
    d->m_settingsId = id;
}

QByteArray SettingsAccessor::settingsId() const
{
    return d->m_settingsId;
}

int SettingsAccessor::currentVersion() const
{
    return d->currentVersion();
}

int SettingsAccessor::firstSupportedVersion() const
{
    return d->firstVersion();
}

SettingsAccessor::RestoreData SettingsAccessor::readSharedSettings(QWidget *parent) const
{
    RestoreData sharedSettings = d->m_sharedFile->readData(d->m_sharedFile->baseFilePath(), parent);

    if (versionFromMap(sharedSettings.data) > currentVersion()) {
        // The shared file version is newer than Creator... If we have valid user
        // settings we prompt the user whether we could try an *unsupported* update.
        // This makes sense since the merging operation will only replace shared settings
        // that perfectly match corresponding user ones. If we don't have valid user
        // settings to compare against, there's nothing we can do.

        sharedSettings.issue = Issue(QApplication::translate("Utils::SettingsAccessor",
                                                             "Unsupported Shared Settings File"),
                                     QApplication::translate("Utils::SettingsAccessor",
                                                             "The version of your .shared file is not "
                                                             "supported by %1. "
                                                             "Do you want to try loading it anyway?"));
        sharedSettings.issue->buttons.insert(QMessageBox::Yes, Continue);
        sharedSettings.issue->buttons.insert(QMessageBox::No, DiscardAndContinue);
        sharedSettings.issue->defaultButton = QMessageBox::No;
        sharedSettings.issue->escapeButton = QMessageBox::No;
    }
    return sharedSettings;
}

QVariantMap
SettingsAccessor::mergeSettings(const QVariantMap &userMap, const QVariantMap &sharedMap) const
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

// --------------------------------------------------------------------
// Helper functions:
// --------------------------------------------------------------------

int versionFromMap(const QVariantMap &data)
{
    return data.value(VERSION_KEY, -1).toInt();
}

int originalVersionFromMap(const QVariantMap &data)
{
    return data.value(ORIGINAL_VERSION_KEY, versionFromMap(data)).toInt();
}

QByteArray settingsIdFromMap(const QVariantMap &data)
{
    return data.value(SETTINGS_ID_KEY).toByteArray();
}

void setOriginalVersionInMap(QVariantMap &data, int version)
{
    data.insert(ORIGINAL_VERSION_KEY, version);
}

void setVersionInMap(QVariantMap &data, int version)
{
    data.insert(VERSION_KEY, version);
}

void setSettingsIdInMap(QVariantMap &data, const QByteArray &id)
{
    data.insert(SETTINGS_ID_KEY, id);
}

} // namespace Utils
