// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "settingsaccessor.h"

#include "algorithm.h"
#include "persistentsettings.h"
#include "qtcassert.h"
#include "utilstr.h"

#include <QDir>

namespace {

const char ORIGINAL_VERSION_KEY[] = "OriginalVersion";
const char SETTINGS_ID_KEY[] = "EnvironmentId";
const char VERSION_KEY[] = "Version";

} // namespace

namespace Utils {

// --------------------------------------------------------------------
// SettingsAccessor::Issue:
// --------------------------------------------------------------------

QMessageBox::StandardButtons SettingsAccessor::Issue::allButtons() const
{
    QMessageBox::StandardButtons result = QMessageBox::NoButton;
    for (auto it = buttons.cbegin(); it != buttons.cend(); ++it)
        result |= it.key();
    return result;
}

// --------------------------------------------------------------------
// SettingsAccessor:
// --------------------------------------------------------------------

/*!
 * The SettingsAccessor can be used to read/write settings in XML format.
 */
SettingsAccessor::SettingsAccessor() = default;

SettingsAccessor::~SettingsAccessor() = default;

/*!
 * Restore settings from disk and report any issues in a message box centered on \a parent.
 */
Store SettingsAccessor::restoreSettings(QWidget *parent) const
{
    QTC_ASSERT(!m_baseFilePath.isEmpty(), return Store());

    return restoreSettings(m_baseFilePath, parent);
}

/*!
 * Save \a data to disk and report any issues in a message box centered on \a parent.
 */
bool SettingsAccessor::saveSettings(const Store &data, QWidget *parent) const
{
    QTC_CHECK(!m_docType.isEmpty());
    QTC_CHECK(!m_applicationDisplayName.isEmpty());

    const std::optional<Issue> result = writeData(m_baseFilePath, data, parent);

    const ProceedInfo pi = result ? reportIssues(result.value(), m_baseFilePath, parent) : ProceedInfo::Continue;
    return pi == ProceedInfo::Continue;
}

/*!
 * Read data from \a path. Do all the necessary postprocessing of the data.
 */
SettingsAccessor::RestoreData SettingsAccessor::readData(const FilePath &path, QWidget *parent) const
{
    Q_UNUSED(parent)
    RestoreData result = readFile(path);
    if (!result.data.isEmpty())
        result.data = preprocessReadSettings(result.data);
    return result;
}

/*!
 * Store the \a data in \a path on disk. Do all the necessary preprocessing of the data.
 */
std::optional<SettingsAccessor::Issue> SettingsAccessor::writeData(const FilePath &path,
                                                                   const Store &data,
                                                                   QWidget *parent) const
{
    Q_UNUSED(parent)
    return writeFile(path, prepareToWriteSettings(data));
}

Store SettingsAccessor::restoreSettings(const FilePath &settingsPath, QWidget *parent) const
{
    QTC_CHECK(!m_docType.isEmpty());
    QTC_CHECK(!m_applicationDisplayName.isEmpty());

    const RestoreData result = readData(settingsPath, parent);

    const ProceedInfo pi = result.hasIssue() ? reportIssues(result.issue.value(), result.path, parent)
                                             : ProceedInfo::Continue;
    return pi == ProceedInfo::DiscardAndContinue ? Store() : result.data;
}

/*!
 * Read a file at \a path from disk and extract the data into a RestoreData set.
 *
 * This method does not do *any* processing of the file contents.
 */
SettingsAccessor::RestoreData SettingsAccessor::readFile(const FilePath &path) const
{
    PersistentSettingsReader reader;
    if (!reader.load(path)) {
        return RestoreData(Issue(Tr::tr("Failed to Read File"),
                                 Tr::tr("Could not open \"%1\".")
                                 .arg(path.toUserOutput()), Issue::Type::ERROR));
    }

    const Store data = reader.restoreValues();
    if (!m_readOnly && path == m_baseFilePath) {
        if (!m_writer)
            m_writer = std::make_unique<PersistentSettingsWriter>(m_baseFilePath, m_docType);
        m_writer->setContents(data);
    }

    return RestoreData(path, data);
}

/*!
 * Write a file at \a path to disk and store the \a data in it.
 *
 * This method does not do *any* processing of the file contents.
 */
std::optional<SettingsAccessor::Issue> SettingsAccessor::writeFile(const FilePath &path,
                                                                   const Store &data) const
{
    if (data.isEmpty()) {
        return Issue(Tr::tr("Failed to Write File"),
                     Tr::tr("There was nothing to write."),
                     Issue::Type::WARNING);
    }

    QString errorMessage;
    if (!m_readOnly && (!m_writer || m_writer->fileName() != path))
        m_writer = std::make_unique<PersistentSettingsWriter>(path, m_docType);

    if (!m_writer->save(data, &errorMessage)) {
        return Issue(Tr::tr("Failed to Write File"),
                     errorMessage, Issue::Type::ERROR);
    }
    return {};
}

SettingsAccessor::ProceedInfo
SettingsAccessor::reportIssues(const Issue &issue, const FilePath &path, QWidget *parent)
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
Store SettingsAccessor::preprocessReadSettings(const Store &data) const
{
    return data;
}

/*!
 * This method is called right before writing data to disk and modifies \a data.
 */
Store SettingsAccessor::prepareToWriteSettings(const Store &data) const
{
    return data;
}

// --------------------------------------------------------------------
// BackingUpSettingsAccessor:
// --------------------------------------------------------------------

FilePaths BackUpStrategy::readFileCandidates(const FilePath &baseFileName) const
{
    const QStringList filter(baseFileName.fileName() + '*');
    const FilePath baseFileDir = baseFileName.parentDir();

    return baseFileDir.dirEntries({filter, QDir::Files | QDir::Hidden | QDir::System});
}

int BackUpStrategy::compare(const SettingsAccessor::RestoreData &data1,
                            const SettingsAccessor::RestoreData &data2) const
{
    if (!data1.hasError() && !data1.data.isEmpty())
        return -1;

    if (!data2.hasError() && !data2.data.isEmpty())
        return 1;

    return 0;
}

std::optional<FilePath> BackUpStrategy::backupName(const Store &oldData,
                                                   const FilePath &path,
                                                   const Store &data) const
{
    if (oldData == data)
        return std::nullopt;
    return path.stringAppended(".bak");
}

BackingUpSettingsAccessor::BackingUpSettingsAccessor()
    : m_strategy(std::make_unique<BackUpStrategy>())
{ }

SettingsAccessor::RestoreData
BackingUpSettingsAccessor::readData(const FilePath &path, QWidget *parent) const
{
    const FilePaths fileList = readFileCandidates(path);
    if (fileList.isEmpty()) // No settings found at all.
        return RestoreData(path, Store());

    RestoreData result = bestReadFileData(fileList, parent);
    if (result.path.isEmpty())
        result.path = baseFilePath().parentDir();

    if (result.data.isEmpty()) {
        Issue i(Tr::tr("No Valid Settings Found"),
                Tr::tr("<p>No valid settings file could be found.</p>"
                       "<p>All settings files found in directory \"%1\" "
                       "were unsuitable for the current version of %2, "
                       "for instance because they were written by an incompatible "
                       "version of %2, or because a different settings path "
                       "was used.</p>")
                .arg(path.toUserOutput(), m_applicationDisplayName), Issue::Type::ERROR);
        i.buttons.insert(QMessageBox::Ok, DiscardAndContinue);
        result.issue = i;
    }

    return result;
}

std::optional<SettingsAccessor::Issue> BackingUpSettingsAccessor::writeData(const FilePath &path,
                                                                            const Store &data,
                                                                            QWidget *parent) const
{
    if (data.isEmpty())
        return {};

    backupFile(path, data, parent);

    return SettingsAccessor::writeData(path, data, parent);
}

void BackingUpSettingsAccessor::setStrategy(std::unique_ptr<BackUpStrategy> &&strategy)
{
    m_strategy = std::move(strategy);
}

FilePaths BackingUpSettingsAccessor::readFileCandidates(const FilePath &path) const
{
    FilePaths result = filteredUnique(m_strategy->readFileCandidates(path));
    if (result.removeOne(baseFilePath()))
        result.prepend(baseFilePath());

    return result;
}

SettingsAccessor::RestoreData
BackingUpSettingsAccessor::bestReadFileData(const FilePaths &candidates, QWidget *parent) const
{
    SettingsAccessor::RestoreData bestMatch;
    for (const FilePath &c : candidates) {
        RestoreData cData = SettingsAccessor::readData(c, parent);
        if (m_strategy->compare(bestMatch, cData) > 0)
            bestMatch = cData;
    }
    return bestMatch;
}

void BackingUpSettingsAccessor::backupFile(const FilePath &path, const Store &data,
                                           QWidget *parent) const
{
    RestoreData oldSettings = SettingsAccessor::readData(path, parent);
    if (oldSettings.data.isEmpty())
        return;

    // Do we need to do a backup?
    if (std::optional<FilePath> backupFileName = m_strategy->backupName(oldSettings.data,
                                                                        path,
                                                                        data)) {
        path.copyFile(backupFileName.value());
    }
}

// --------------------------------------------------------------------
// UpgradingSettingsAccessor:
// --------------------------------------------------------------------

VersionedBackUpStrategy::VersionedBackUpStrategy(const UpgradingSettingsAccessor *accessor) :
    m_accessor(accessor)
{
    QTC_CHECK(accessor);
}

int VersionedBackUpStrategy::compare(const SettingsAccessor::RestoreData &data1,
                                     const SettingsAccessor::RestoreData &data2) const
{
    const int origVersion = versionFromMap(data1.data);
    const bool origValid = m_accessor->isValidVersionAndId(origVersion, settingsIdFromMap(data1.data));

    const int newVersion = versionFromMap(data2.data);
    const bool newValid = m_accessor->isValidVersionAndId(newVersion, settingsIdFromMap(data2.data));

    if ((!origValid && !newValid) || (origValid && newValid && origVersion == newVersion))
        return 0;
    if ((!origValid &&  newValid) || (origValid && newValid && origVersion < newVersion))
        return 1;
    return -1;
}

std::optional<FilePath> VersionedBackUpStrategy::backupName(const Store &oldData,
                                                            const FilePath &path,
                                                            const Store &data) const
{
    Q_UNUSED(data)
    FilePath backupName = path;
    const QByteArray oldEnvironmentId = settingsIdFromMap(oldData);
    const int oldVersion = versionFromMap(oldData);

    if (!oldEnvironmentId.isEmpty() && oldEnvironmentId != m_accessor->settingsId())
        backupName = backupName.stringAppended
                ('.' + QString::fromLatin1(oldEnvironmentId).mid(1, 7));
    if (oldVersion != m_accessor->currentVersion()) {
        VersionUpgrader *upgrader = m_accessor->upgrader(oldVersion);
        if (upgrader)
            backupName = backupName.stringAppended('.' + upgrader->backupExtension());
        else
            backupName = backupName.stringAppended('.' + QString::number(oldVersion));
    }
    if (backupName == path)
        return std::nullopt;
    return backupName;
}

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
Store VersionUpgrader::renameKeys(const QList<Change> &changes, Store map) const
{
    for (const Change &change : changes) {
        const auto oldSetting = map.constFind(change.first);
        if (oldSetting != map.constEnd()) {
            map.insert(change.second, oldSetting.value());
            map.erase(oldSetting);
        }
    }

    Store::iterator i = map.begin();
    while (i != map.end()) {
        QVariant v = i.value();
        if (Utils::isStore(v))
            i.value() = variantFromStore(renameKeys(changes, storeFromVariant(v)));

        ++i;
    }

    return map;
}

/*!
 * The UpgradingSettingsAccessor keeps version information in the settings file and will
 * upgrade the settings on load to the latest supported version (if possible).
 */
UpgradingSettingsAccessor::UpgradingSettingsAccessor()
{
    setStrategy(std::make_unique<VersionedBackUpStrategy>(this));
}

int UpgradingSettingsAccessor::currentVersion() const
{
    return lastSupportedVersion() + 1;
}

int UpgradingSettingsAccessor::firstSupportedVersion() const
{
    return m_upgraders.size() == 0 ? -1 : m_upgraders.front()->version();
}

int UpgradingSettingsAccessor::lastSupportedVersion() const
{
    return m_upgraders.size() == 0 ? -1 : m_upgraders.back()->version();
}

bool UpgradingSettingsAccessor::isValidVersionAndId(const int version, const QByteArray &id) const
{
    return (version >= 0
            && version >= firstSupportedVersion() && version <= currentVersion())
            && (id.isEmpty() || id == m_id || m_id.isEmpty());
}

SettingsAccessor::RestoreData UpgradingSettingsAccessor::readData(const FilePath &path,
                                                                  QWidget *parent) const
{
    return upgradeSettings(BackingUpSettingsAccessor::readData(path, parent), currentVersion());
}

Store UpgradingSettingsAccessor::prepareToWriteSettings(const Store &data) const
{
    Store tmp = BackingUpSettingsAccessor::prepareToWriteSettings(data);

    setVersionInMap(tmp,currentVersion());
    if (!m_id.isEmpty())
        setSettingsIdInMap(tmp, m_id);

    return tmp;
}

bool UpgradingSettingsAccessor::addVersionUpgrader(std::unique_ptr<VersionUpgrader> &&upgrader)
{
    QTC_ASSERT(upgrader.get(), return false);
    const int version = upgrader->version();
    QTC_ASSERT(version >= 0, return false);

    const bool haveUpgraders = m_upgraders.size() != 0;
    QTC_ASSERT(!haveUpgraders || currentVersion() == version, return false);
    m_upgraders.push_back(std::move(upgrader));
    return true;
}

VersionUpgrader *UpgradingSettingsAccessor::upgrader(const int version) const
{
    QTC_ASSERT(version >= 0 && firstSupportedVersion() >= 0, return nullptr);
    const int pos = version - firstSupportedVersion();
    VersionUpgrader *upgrader = nullptr;
    if (pos >= 0 && pos < static_cast<int>(m_upgraders.size()))
        upgrader = m_upgraders[static_cast<size_t>(pos)].get();
    QTC_CHECK(upgrader == nullptr || upgrader->version() == version);
    return upgrader;
}

SettingsAccessor::RestoreData
UpgradingSettingsAccessor::upgradeSettings(const RestoreData &data, const int targetVersion) const
{
    if (data.hasError() || data.data.isEmpty())
        return data;

    QTC_ASSERT(targetVersion >= firstSupportedVersion(), return data);
    QTC_ASSERT(targetVersion <= currentVersion(), return data);

    RestoreData result = validateVersionRange(data);
    if (result.hasError())
        return result;

    const int version = versionFromMap(result.data);
    if (!result.data.contains(ORIGINAL_VERSION_KEY))
        setOriginalVersionInMap(result.data, version);

    for (int i = version; i < targetVersion; ++i) {
        VersionUpgrader *u = upgrader(i);
        QTC_ASSERT(u, continue);
        result.data = u->upgrade(result.data);
        setVersionInMap(result.data, i + 1);
    }

    return result;
}

SettingsAccessor::RestoreData
UpgradingSettingsAccessor::validateVersionRange(const RestoreData &data) const
{
    RestoreData result = data;
    if (data.data.isEmpty())
        return result;
    const int version = versionFromMap(result.data);
    if (version < firstSupportedVersion() || version > currentVersion()) {
        Issue i(Tr::tr("No Valid Settings Found"),
                Tr::tr("<p>No valid settings file could be found.</p>"
                       "<p>All settings files found in directory \"%1\" "
                       "were either too new or too old to be read.</p>")
                .arg(result.path.toUserOutput()), Issue::Type::ERROR);
        i.buttons.insert(QMessageBox::Ok, DiscardAndContinue);
        result.issue = i;
        return result;
    }

    if (result.path != baseFilePath() && !result.path.endsWith(".shared")
            && version < currentVersion()) {
        Issue i(Tr::tr("Using Old Settings"),
                Tr::tr("<p>The versioned backup \"%1\" of the settings "
                       "file is used, because the non-versioned file was "
                       "created by an incompatible version of %2.</p>"
                       "<p>Settings changes made since the last time this "
                       "version of %2 was used are ignored, and "
                       "changes made now will <b>not</b> be propagated to "
                       "the newer version.</p>")
                .arg(result.path.toUserOutput(), m_applicationDisplayName), Issue::Type::WARNING);
        i.buttons.insert(QMessageBox::Ok, Continue);
        result.issue = i;
        return result;
    }

    const QByteArray readId = settingsIdFromMap(result.data);
    if (!settingsId().isEmpty() && !readId.isEmpty() && readId != settingsId()) {
        Issue i(Tr::tr("Settings File for \"%1\" from a Different Environment?")
                .arg(m_applicationDisplayName),
                Tr::tr("<p>No settings file created by this instance "
                       "of %1 was found.</p>"
                       "<p>Did you work with this project on another machine or "
                       "using a different settings path before?</p>"
                       "<p>Do you still want to load the settings file \"%2\"?</p>")
                .arg(m_applicationDisplayName, result.path.toUserOutput()), Issue::Type::WARNING);
        i.defaultButton = QMessageBox::No;
        i.escapeButton = QMessageBox::No;
        i.buttons.clear();
        i.buttons.insert(QMessageBox::Yes, Continue);
        i.buttons.insert(QMessageBox::No, DiscardAndContinue);
        result.issue = i;
        return result;
    }

    return result;
}

// --------------------------------------------------------------------
// MergingSettingsAccessor:
// --------------------------------------------------------------------

/*!
 * MergingSettingsAccessor allows to merge secondary settings into the main settings.
 * This is useful to e.g. handle .shared files together with .user files.
 */
MergingSettingsAccessor::MergingSettingsAccessor() = default;

SettingsAccessor::RestoreData MergingSettingsAccessor::readData(const FilePath &path,
                                                                QWidget *parent) const
{
    RestoreData mainData = UpgradingSettingsAccessor::readData(path, parent); // FULLY upgraded!
    if (mainData.hasIssue()) {
        if (reportIssues(mainData.issue.value(), mainData.path, parent) == DiscardAndContinue)
            mainData.data.clear();
        mainData.issue = std::nullopt;
    }

    RestoreData secondaryData
            = m_secondaryAccessor ? m_secondaryAccessor->readData(m_secondaryAccessor->baseFilePath(), parent)
                                  : RestoreData();
    secondaryData.data = preprocessReadSettings(secondaryData.data);
    int secondaryVersion = versionFromMap(secondaryData.data);
    if (secondaryVersion == -1)
        secondaryVersion = currentVersion(); // No version information, use currentVersion since
                                             // trying to upgrade makes no sense without an idea
                                             // of what might have changed in the meantime.b
    if (!secondaryData.hasIssue() && !secondaryData.data.isEmpty()
            && (secondaryVersion < firstSupportedVersion() || secondaryVersion > currentVersion())) {
        // The shared file version is too old/too new for Creator... If we have valid user
        // settings we prompt the user whether we could try an *unsupported* update.
        // This makes sense since the merging operation will only replace shared settings
        // that perfectly match corresponding user ones. If we don't have valid user
        // settings to compare against, there's nothing we can do.

        secondaryData.issue = Issue(Tr::tr("Unsupported Merge Settings File"),
                                    Tr::tr("\"%1\" is not supported by %2. "
                                           "Do you want to try loading it anyway?")
                                    .arg(secondaryData.path.toUserOutput(), m_applicationDisplayName),
                                    Issue::Type::WARNING);
        secondaryData.issue->buttons.clear();
        secondaryData.issue->buttons.insert(QMessageBox::Yes, Continue);
        secondaryData.issue->buttons.insert(QMessageBox::No, DiscardAndContinue);
        secondaryData.issue->defaultButton = QMessageBox::No;
        secondaryData.issue->escapeButton = QMessageBox::No;
        setVersionInMap(secondaryData.data, std::max(secondaryVersion, firstSupportedVersion()));
    }

    if (secondaryData.hasIssue()) {
        if (reportIssues(secondaryData.issue.value(), secondaryData.path, parent) == DiscardAndContinue)
            secondaryData.data.clear();
        secondaryData.issue = std::nullopt;
    }

    if (!secondaryData.data.isEmpty())
        secondaryData = upgradeSettings(secondaryData, currentVersion());

    return mergeSettings(mainData, secondaryData);
}

void MergingSettingsAccessor::setSecondaryAccessor(std::unique_ptr<SettingsAccessor> &&secondary)
{
    m_secondaryAccessor = std::move(secondary);
}

/*!
 * Merge \a secondary into \a main. Both need to be at the newest possible version.
 */
SettingsAccessor::RestoreData
MergingSettingsAccessor::mergeSettings(const SettingsAccessor::RestoreData &main,
                                       const SettingsAccessor::RestoreData &secondary) const
{
    const int mainVersion = versionFromMap(main.data);
    const int secondaryVersion = versionFromMap(secondary.data);

    QTC_CHECK(main.data.isEmpty() || mainVersion == currentVersion());
    QTC_CHECK(secondary.data.isEmpty() || secondaryVersion == currentVersion());

    if (main.data.isEmpty())
        return secondary;
    else if (secondary.data.isEmpty())
        return main;

    SettingsMergeFunction mergeFunction
            = [this](const SettingsMergeData &global, const SettingsMergeData &local) {
        return merge(global, local);
    };
    const Store result = storeFromVariant(mergeQVariantMaps(main.data, secondary.data, mergeFunction));

    // Update from the base version to Creator's version.
    return RestoreData(main.path, postprocessMerge(main.data, secondary.data, result));
}

/*!
 * Returns true for housekeeping related keys.
 */
bool MergingSettingsAccessor::isHouseKeepingKey(const Key &key)
{
    return key == VERSION_KEY || key == ORIGINAL_VERSION_KEY || key == SETTINGS_ID_KEY;
}

Store MergingSettingsAccessor::postprocessMerge(const Store &main,
                                                      const Store &secondary,
                                                      const Store &result) const
{
    Q_UNUSED(main)
    Q_UNUSED(secondary)
    return result;
}

// --------------------------------------------------------------------
// Helper functions:
// --------------------------------------------------------------------

int versionFromMap(const Store &data)
{
    return data.value(VERSION_KEY, -1).toInt();
}

int originalVersionFromMap(const Store &data)
{
    return data.value(ORIGINAL_VERSION_KEY, versionFromMap(data)).toInt();
}

QByteArray settingsIdFromMap(const Store &data)
{
    return data.value(SETTINGS_ID_KEY).toByteArray();
}

void setOriginalVersionInMap(Store &data, int version)
{
    data.insert(ORIGINAL_VERSION_KEY, version);
}

void setVersionInMap(Store &data, int version)
{
    data.insert(VERSION_KEY, version);
}

void setSettingsIdInMap(Store &data, const QByteArray &id)
{
    data.insert(SETTINGS_ID_KEY, id);
}

static QVariant mergeQVariantMapsRecursion(const Store &mainTree, const Store &secondaryTree,
                                           const Key &keyPrefix,
                                           const Store &mainSubtree, const Store &secondarySubtree,
                                           const SettingsMergeFunction &merge)
{
    Store result;
    const QList<Key> allKeys = filteredUnique(mainSubtree.keys() + secondarySubtree.keys());

    MergingSettingsAccessor::SettingsMergeData global = {mainTree, secondaryTree, Key()};
    MergingSettingsAccessor::SettingsMergeData local = {mainSubtree, secondarySubtree, Key()};

    for (const Key &key : allKeys) {
        global.key = keyPrefix + key;
        local.key = key;

        std::optional<QPair<Key, QVariant>> mergeResult = merge(global, local);
        if (!mergeResult)
            continue;

        QPair<Key, QVariant> kv = mergeResult.value();

        if (Utils::isStore(kv.second)) {
            const Key newKeyPrefix = keyPrefix + kv.first + '/';
            kv.second = mergeQVariantMapsRecursion(mainTree, secondaryTree, newKeyPrefix,
                                                   storeFromVariant(kv.second),
                                                   storeFromVariant(secondarySubtree.value(kv.first)),
                                                   merge);
        }
        if (!kv.second.isNull())
            result.insert(kv.first, kv.second);
    }

    return variantFromStore(result);
}

QVariant mergeQVariantMaps(const Store &mainTree, const Store &secondaryTree,
                           const SettingsMergeFunction &merge)
{
    return mergeQVariantMapsRecursion(mainTree, secondaryTree, Key(),
                                      mainTree, secondaryTree, merge);
}

} // namespace Utils
