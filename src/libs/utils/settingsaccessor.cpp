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
#include "qtcassert.h"

#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QRegExp>

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
    for (const QMessageBox::StandardButton &b : buttons.keys())
        result |= b;
    return result;
}

// --------------------------------------------------------------------
// SettingsAccessor:
// --------------------------------------------------------------------

/*!
 * The SettingsAccessor can be used to read/write settings in XML format.
 */
SettingsAccessor::SettingsAccessor(const QString &docType,
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
QVariantMap SettingsAccessor::restoreSettings(QWidget *parent) const
{
    QTC_ASSERT(!m_baseFilePath.isEmpty(), return QVariantMap());

    return restoreSettings(m_baseFilePath, parent);
}

/*!
 * Save \a data to disk and report any issues in a message box centered on \a parent.
 */
bool SettingsAccessor::saveSettings(const QVariantMap &data, QWidget *parent) const
{
    const optional<Issue> result = writeData(m_baseFilePath, data, parent);

    const ProceedInfo pi = result ? reportIssues(result.value(), m_baseFilePath, parent) : ProceedInfo::Continue;
    return pi == ProceedInfo::Continue;
}

/*!
 * Read data from \a path. Do all the necessary postprocessing of the data.
 */
SettingsAccessor::RestoreData SettingsAccessor::readData(const FileName &path, QWidget *parent) const
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
optional<SettingsAccessor::Issue>
SettingsAccessor::writeData(const FileName &path, const QVariantMap &data, QWidget *parent) const
{
    Q_UNUSED(parent);
    return writeFile(path, prepareToWriteSettings(data));
}

QVariantMap SettingsAccessor::restoreSettings(const FileName &settingsPath, QWidget *parent) const
{
    const RestoreData result = readData(settingsPath, parent);

    const ProceedInfo pi = result.hasIssue() ? reportIssues(result.issue.value(), result.path, parent)
                                             : ProceedInfo::Continue;
    return pi == ProceedInfo::DiscardAndContinue ? QVariantMap() : result.data;
}

/*!
 * Read a file at \a path from disk and extract the data into a RestoreData set.
 *
 * This method does not do *any* processing of the file contents.
 */
SettingsAccessor::RestoreData SettingsAccessor::readFile(const FileName &path) const
{
    PersistentSettingsReader reader;
    if (!reader.load(path)) {
        return RestoreData(Issue(QCoreApplication::translate("Utils::SettingsAccessor", "Failed to Read File"),
                                 QCoreApplication::translate("Utils::SettingsAccessor", "Could not open \"%1\".")
                                 .arg(path.toUserOutput()), Issue::Type::ERROR));
    }

    const QVariantMap data = reader.restoreValues();
    if (!m_readOnly && path == m_baseFilePath) {
        if (!m_writer)
            m_writer = std::make_unique<PersistentSettingsWriter>(m_baseFilePath, docType);
        m_writer->setContents(data);
    }

    return RestoreData(path, data);
}

/*!
 * Write a file at \a path to disk and store the \a data in it.
 *
 * This method does not do *any* processing of the file contents.
 */
optional<SettingsAccessor::Issue>
SettingsAccessor::writeFile(const FileName &path, const QVariantMap &data) const
{
    if (data.isEmpty()) {
        return Issue(QCoreApplication::translate("Utils::SettingsAccessor", "Failed to Write File"),
                     QCoreApplication::translate("Utils::SettingsAccessor", "There was nothing to write."),
                     Issue::Type::WARNING);
    }

    QString errorMessage;
    if (!m_readOnly && (!m_writer || m_writer->fileName() != path))
        m_writer = std::make_unique<PersistentSettingsWriter>(path, docType);

    if (!m_writer->save(data, &errorMessage)) {
        return Issue(QCoreApplication::translate("Utils::SettingsAccessor", "Failed to Write File"),
                     errorMessage, Issue::Type::ERROR);
    }
    return {};
}

SettingsAccessor::ProceedInfo
SettingsAccessor::reportIssues(const SettingsAccessor::Issue &issue, const FileName &path,
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
QVariantMap SettingsAccessor::preprocessReadSettings(const QVariantMap &data) const
{
    return data;
}

/*!
 * This method is called right before writing data to disk and modifies \a data.
 */
QVariantMap SettingsAccessor::prepareToWriteSettings(const QVariantMap &data) const
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

int BackUpStrategy::compare(const SettingsAccessor::RestoreData &data1,
                            const SettingsAccessor::RestoreData &data2) const
{
    if (!data1.hasError() && !data1.data.isEmpty())
        return -1;

    if (!data2.hasError() && !data2.data.isEmpty())
        return 1;

    return 0;
}

optional<FileName>
BackUpStrategy::backupName(const QVariantMap &oldData, const FileName &path, const QVariantMap &data) const
{
    if (oldData == data)
        return nullopt;
    FileName backup = path;
    backup.appendString(".bak");
    return backup;
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
    SettingsAccessor(docType, displayName, applicationDisplayName),
    m_strategy(std::move(strategy))
{ }

SettingsAccessor::RestoreData
BackingUpSettingsAccessor::readData(const FileName &path, QWidget *parent) const
{
    const FileNameList fileList = readFileCandidates(path);
    if (fileList.isEmpty()) // No settings found at all.
        return RestoreData(path, QVariantMap());

    RestoreData result = bestReadFileData(fileList, parent);
    if (result.path.isEmpty())
        result.path = baseFilePath().parentDir();

    if (result.data.isEmpty()) {
        Issue i(QApplication::translate("Utils::SettingsAccessor", "No Valid Settings Found"),
                QApplication::translate("Utils::SettingsAccessor",
                                        "<p>No valid settings file could be found.</p>"
                                        "<p>All settings files found in directory \"%1\" "
                                        "were unsuitable for the current version of %2.</p>")
                .arg(path.toUserOutput()).arg(applicationDisplayName), Issue::Type::ERROR);
        i.buttons.insert(QMessageBox::Ok, DiscardAndContinue);
        result.issue = i;
    }

    return result;
}

optional<SettingsAccessor::Issue>
BackingUpSettingsAccessor::writeData(const FileName &path, const QVariantMap &data,
                                     QWidget *parent) const
{
    if (data.isEmpty())
        return {};

    backupFile(path, data, parent);

    return SettingsAccessor::writeData(path, data, parent);
}

FileNameList BackingUpSettingsAccessor::readFileCandidates(const FileName &path) const
{
    FileNameList result = Utils::filteredUnique(m_strategy->readFileCandidates(path));
    if (result.removeOne(baseFilePath()))
        result.prepend(baseFilePath());

    return result;
}

SettingsAccessor::RestoreData
BackingUpSettingsAccessor::bestReadFileData(const FileNameList &candidates, QWidget *parent) const
{
    SettingsAccessor::RestoreData bestMatch;
    for (const FileName &c : candidates) {
        RestoreData cData = SettingsAccessor::readData(c, parent);
        if (m_strategy->compare(bestMatch, cData) > 0)
            bestMatch = cData;
    }
    return bestMatch;
}

void BackingUpSettingsAccessor::backupFile(const FileName &path, const QVariantMap &data,
                                           QWidget *parent) const
{
    RestoreData oldSettings = SettingsAccessor::readData(path, parent);
    if (oldSettings.data.isEmpty())
        return;

    // Do we need to do a backup?
    const QString origName = path.toString();
    optional<FileName> backupFileName = m_strategy->backupName(oldSettings.data, path, data);
    if (backupFileName)
        QFile::copy(origName, backupFileName.value().toString());
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

optional<FileName>
VersionedBackUpStrategy::backupName(const QVariantMap &oldData, const FileName &path, const QVariantMap &data) const
{
    Q_UNUSED(data);
    FileName backupName = path;
    const QByteArray oldEnvironmentId = settingsIdFromMap(oldData);
    const int oldVersion = versionFromMap(oldData);

    if (!oldEnvironmentId.isEmpty() && oldEnvironmentId != m_accessor->settingsId())
        backupName.appendString('.' + QString::fromLatin1(oldEnvironmentId).mid(1, 7));
    if (oldVersion != m_accessor->currentVersion()) {
        VersionUpgrader *upgrader = m_accessor->upgrader(oldVersion);
        if (upgrader)
            backupName.appendString('.' + upgrader->backupExtension());
        else
            backupName.appendString('.' + QString::number(oldVersion));
    }
    if (backupName == path)
        return nullopt;
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

/*!
 * The UpgradingSettingsAccessor keeps version information in the settings file and will
 * upgrade the settings on load to the latest supported version (if possible).
 */
UpgradingSettingsAccessor::UpgradingSettingsAccessor(const QString &docType,
                                                     const QString &displayName,
                                                     const QString &applicationDisplayName) :
    UpgradingSettingsAccessor(std::make_unique<VersionedBackUpStrategy>(this), docType,
                              displayName, applicationDisplayName)
{ }

UpgradingSettingsAccessor::UpgradingSettingsAccessor(std::unique_ptr<BackUpStrategy> &&strategy,
                                                     const QString &docType,
                                                     const QString &displayName,
                                                     const QString &applicationDisplayName) :
    BackingUpSettingsAccessor(std::move(strategy), docType, displayName, applicationDisplayName)
{ }

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

SettingsAccessor::RestoreData UpgradingSettingsAccessor::readData(const FileName &path,
                                                                  QWidget *parent) const
{
    return upgradeSettings(BackingUpSettingsAccessor::readData(path, parent), currentVersion());
}

QVariantMap UpgradingSettingsAccessor::prepareToWriteSettings(const QVariantMap &data) const
{
    QVariantMap tmp = BackingUpSettingsAccessor::prepareToWriteSettings(data);

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
        Issue i(QApplication::translate("Utils::SettingsAccessor", "No Valid Settings Found"),
                QApplication::translate("Utils::SettingsAccessor",
                                        "<p>No valid settings file could be found.</p>"
                                        "<p>All settings files found in directory \"%1\" "
                                        "were either too new or too old to be read.</p>")
                .arg(result.path.toUserOutput()), Issue::Type::ERROR);
        i.buttons.insert(QMessageBox::Ok, DiscardAndContinue);
        result.issue = i;
        return result;
    }

    if (result.path != baseFilePath() && !result.path.endsWith(".shared")
            && version < currentVersion()) {
        Issue i(QApplication::translate("Utils::SettingsAccessor", "Using Old Settings"),
                QApplication::translate("Utils::SettingsAccessor",
                                        "<p>The versioned backup \"%1\" of the settings "
                                        "file is used, because the non-versioned file was "
                                        "created by an incompatible version of %2.</p>"
                                        "<p>Settings changes made since the last time this "
                                        "version of %2 was used are ignored, and "
                                        "changes made now will <b>not</b> be propagated to "
                                        "the newer version.</p>")
                .arg(result.path.toUserOutput()).arg(applicationDisplayName), Issue::Type::WARNING);
        i.buttons.insert(QMessageBox::Ok, Continue);
        result.issue = i;
        return result;
    }

    const QByteArray readId = settingsIdFromMap(result.data);
    if (!settingsId().isEmpty() && !readId.isEmpty() && readId != settingsId()) {
        Issue i(QApplication::translate("Utils::EnvironmentIdAccessor",
                                        "Settings File for \"%1\" from a Different Environment?")
                .arg(applicationDisplayName),
                QApplication::translate("Utils::EnvironmentIdAccessor",
                                        "<p>No settings file created by this instance "
                                        "of %1 was found.</p>"
                                        "<p>Did you work with this project on another machine or "
                                        "using a different settings path before?</p>"
                                        "<p>Do you still want to load the settings file \"%2\"?</p>")
                .arg(applicationDisplayName).arg(result.path.toUserOutput()), Issue::Type::WARNING);
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
MergingSettingsAccessor::MergingSettingsAccessor(std::unique_ptr<BackUpStrategy> &&strategy,
                                                 const QString &docType,
                                                 const QString &displayName,
                                                 const QString &applicationDisplayName) :
    UpgradingSettingsAccessor(std::move(strategy), docType, displayName, applicationDisplayName)
{ }

SettingsAccessor::RestoreData MergingSettingsAccessor::readData(const FileName &path,
                                                                QWidget *parent) const
{
    RestoreData mainData = UpgradingSettingsAccessor::readData(path, parent); // FULLY upgraded!
    if (mainData.hasIssue()) {
        if (reportIssues(mainData.issue.value(), mainData.path, parent) == DiscardAndContinue)
            mainData.data.clear();
        mainData.issue = nullopt;
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

        secondaryData.issue = Issue(QApplication::translate("Utils::SettingsAccessor",
                                                            "Unsupported Merge Settings File"),
                                    QApplication::translate("Utils::SettingsAccessor",
                                                            "\"%1\" is not supported by %2. "
                                                            "Do you want to try loading it anyway?")
                                    .arg(secondaryData.path.toUserOutput())
                                    .arg(applicationDisplayName), Issue::Type::WARNING);
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
        secondaryData.issue = nullopt;
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
    const QVariantMap result = mergeQVariantMaps(main.data, secondary.data, mergeFunction).toMap();

    // Update from the base version to Creator's version.
    return RestoreData(main.path, postprocessMerge(main.data, secondary.data, result));
}

/*!
 * Returns true for housekeeping related keys.
 */
bool MergingSettingsAccessor::isHouseKeepingKey(const QString &key) const
{
    return key == VERSION_KEY || key == ORIGINAL_VERSION_KEY || key == SETTINGS_ID_KEY;
}

QVariantMap MergingSettingsAccessor::postprocessMerge(const QVariantMap &main,
                                                      const QVariantMap &secondary,
                                                      const QVariantMap &result) const
{
    Q_UNUSED(main);
    Q_UNUSED(secondary);
    return result;
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

static QVariant mergeQVariantMapsRecursion(const QVariantMap &mainTree, const QVariantMap &secondaryTree,
                                           const QString &keyPrefix,
                                           const QVariantMap &mainSubtree, const QVariantMap &secondarySubtree,
                                           const SettingsMergeFunction &merge)
{
    QVariantMap result;
    const QList<QString> allKeys = Utils::filteredUnique(mainSubtree.keys() + secondarySubtree.keys());

    MergingSettingsAccessor::SettingsMergeData global = {mainTree, secondaryTree, QString()};
    MergingSettingsAccessor::SettingsMergeData local = {mainSubtree, secondarySubtree, QString()};

    for (const QString &key : allKeys) {
        global.key = keyPrefix + key;
        local.key = key;

        optional<QPair<QString, QVariant>> mergeResult = merge(global, local);
        if (!mergeResult)
            continue;

        QPair<QString, QVariant> kv = mergeResult.value();

        if (kv.second.type() == QVariant::Map) {
            const QString newKeyPrefix = keyPrefix + kv.first + '/';
            kv.second = mergeQVariantMapsRecursion(mainTree, secondaryTree, newKeyPrefix,
                                                   kv.second.toMap(), secondarySubtree.value(kv.first)
                                                   .toMap(), merge);
        }
        if (!kv.second.isNull())
            result.insert(kv.first, kv.second);
    }

    return result;
}

QVariant mergeQVariantMaps(const QVariantMap &mainTree, const QVariantMap &secondaryTree,
                           const SettingsMergeFunction &merge)
{
    return mergeQVariantMapsRecursion(mainTree, secondaryTree, QString(),
                                      mainTree, secondaryTree, merge);
}

} // namespace Utils
