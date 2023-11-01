// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include "filepath.h"
#include "store.h"

#include <QHash>
#include <QMessageBox>

#include <memory>
#include <optional>

namespace Utils {

// -----------------------------------------------------------------------------
// Helper:
// -----------------------------------------------------------------------------

QTCREATOR_UTILS_EXPORT int versionFromMap(const Store &data);
QTCREATOR_UTILS_EXPORT int originalVersionFromMap(const Store &data);
QTCREATOR_UTILS_EXPORT QByteArray settingsIdFromMap(const Store &data);

QTCREATOR_UTILS_EXPORT void setVersionInMap(Store &data, int version);
QTCREATOR_UTILS_EXPORT void setOriginalVersionInMap(Store &data, int version);
QTCREATOR_UTILS_EXPORT void setSettingsIdInMap(Store &data, const QByteArray &id);

// --------------------------------------------------------------------
// Helpers:
// --------------------------------------------------------------------

QTCREATOR_UTILS_EXPORT int versionFromMap(const Store &data);
QTCREATOR_UTILS_EXPORT int originalVersionFromMap(const Store &data);
QTCREATOR_UTILS_EXPORT QByteArray settingsIdFromMap(const Store &data);

QTCREATOR_UTILS_EXPORT void setVersionInMap(Store &data, int version);
QTCREATOR_UTILS_EXPORT void setOriginalVersionInMap(Store &data, int version);
QTCREATOR_UTILS_EXPORT void setSettingsIdInMap(Store &data, const QByteArray &id);

class PersistentSettingsWriter;
using SettingsMergeResult = std::optional<QPair<Utils::Key, QVariant>>;

// --------------------------------------------------------------------
// SettingsAccessor:
// --------------------------------------------------------------------

// Read/write files incl. error handling suitable for the UI:
class QTCREATOR_UTILS_EXPORT SettingsAccessor
{
public:
    SettingsAccessor();
    virtual ~SettingsAccessor();

    enum ProceedInfo { Continue, DiscardAndContinue };
    using ButtonMap = QHash<QMessageBox::StandardButton, ProceedInfo>;
    class Issue {
    public:
        enum class Type { ERROR, WARNING };
        Issue(const QString &title, const QString &message, const Type type) :
            title{title}, message{message}, type{type}
        { }

        QMessageBox::StandardButtons allButtons() const;

        QString title;
        QString message;
        Type type;
        QMessageBox::StandardButton defaultButton = QMessageBox::NoButton;
        QMessageBox::StandardButton escapeButton = QMessageBox::Ok;
        QHash<QMessageBox::StandardButton, ProceedInfo> buttons = {{QMessageBox::Ok, ProceedInfo::Continue}};
    };

    class RestoreData {
    public:
        RestoreData() = default;
        RestoreData(const FilePath &path, const Store &data) : path{path}, data{data} { }
        RestoreData(const QString &title, const QString &message, const Issue::Type type) :
            RestoreData(Issue(title, message, type))
        { }
        RestoreData(const Issue &issue) : issue{issue} { }

        bool hasIssue() const { return bool(issue); }
        bool hasError() const { return hasIssue() && issue.value().type == Issue::Type::ERROR; }
        bool hasWarning() const { return hasIssue() && issue.value().type == Issue::Type::WARNING; }

        FilePath path;
        Store data;
        std::optional<Issue> issue;
    };

    Store restoreSettings(QWidget *parent) const;
    bool saveSettings(const Store &data, QWidget *parent) const;

    void setBaseFilePath(const FilePath &baseFilePath) { m_baseFilePath = baseFilePath; }
    void setReadOnly() { m_readOnly = true; }
    FilePath baseFilePath() const { return m_baseFilePath; }

    virtual RestoreData readData(const FilePath &path, QWidget *parent) const;
    virtual std::optional<Issue> writeData(const FilePath &path,
                                           const Store &data,
                                           QWidget *parent) const;

    void setDocType(const QString &docType) { m_docType = docType; }
    void setApplicationDisplayName(const QString &name) { m_applicationDisplayName = name; }

protected:
    // Report errors:
    Store restoreSettings(const FilePath &settingsPath, QWidget *parent) const;
    static ProceedInfo reportIssues(const Issue &issue, const FilePath &path, QWidget *parent);

    virtual Store preprocessReadSettings(const Store &data) const;
    virtual Store prepareToWriteSettings(const Store &data) const;

    virtual RestoreData readFile(const FilePath &path) const;
    virtual std::optional<Issue> writeFile(const FilePath &path, const Store &data) const;

    QString m_docType;
    QString m_applicationDisplayName;

private:
    FilePath m_baseFilePath;
    mutable std::unique_ptr<PersistentSettingsWriter> m_writer;
    bool m_readOnly = false;
};

// --------------------------------------------------------------------
// BackingUpSettingsAccessor:
// --------------------------------------------------------------------

class QTCREATOR_UTILS_EXPORT BackUpStrategy
{
public:
    virtual ~BackUpStrategy() = default;

    virtual FilePaths readFileCandidates(const FilePath &baseFileName) const;
    // Return -1 if data1 is better that data2, 0 if both are equally worthwhile
    // and 1 if data2 is better than data1
    virtual int compare(const SettingsAccessor::RestoreData &data1,
                        const SettingsAccessor::RestoreData &data2) const;

    virtual std::optional<FilePath> backupName(const Store &oldData,
                                               const FilePath &path,
                                               const Store &data) const;
};

class QTCREATOR_UTILS_EXPORT BackingUpSettingsAccessor : public SettingsAccessor
{
public:
    BackingUpSettingsAccessor();

    RestoreData readData(const FilePath &path, QWidget *parent) const override;
    std::optional<Issue> writeData(const FilePath &path,
                                   const Store &data,
                                   QWidget *parent) const override;

    BackUpStrategy *strategy() const { return m_strategy.get(); }
    void setStrategy(std::unique_ptr<BackUpStrategy> &&strategy);

private:
    FilePaths readFileCandidates(const FilePath &path) const;
    RestoreData bestReadFileData(const FilePaths &candidates, QWidget *parent) const;
    void backupFile(const FilePath &path, const Store &data, QWidget *parent) const;

    std::unique_ptr<BackUpStrategy> m_strategy;
};

// --------------------------------------------------------------------
// UpgradingSettingsAccessor:
// --------------------------------------------------------------------

class UpgradingSettingsAccessor;

class QTCREATOR_UTILS_EXPORT VersionedBackUpStrategy : public BackUpStrategy
{
public:
    VersionedBackUpStrategy(const UpgradingSettingsAccessor *accessor);

    // Return -1 if data1 is better that data2, 0 if both are equally worthwhile
    // and 1 if data2 is better than data1
    int compare(const SettingsAccessor::RestoreData &data1,
                const SettingsAccessor::RestoreData &data2) const override;

    std::optional<FilePath> backupName(const Store &oldData,
                                       const FilePath &path,
                                       const Store &data) const override;

    const UpgradingSettingsAccessor *accessor() const { return m_accessor; }

protected:
    const UpgradingSettingsAccessor *m_accessor = nullptr;
};

// Handles updating a Store from version() to version() + 1
class QTCREATOR_UTILS_EXPORT VersionUpgrader
{
public:
    VersionUpgrader(const int version, const QString &extension);
    virtual ~VersionUpgrader() = default;

    int version() const;
    QString backupExtension() const;

    virtual Store upgrade(const Store &data) = 0;

protected:
    using Change = QPair<Key, Key>;
    Store renameKeys(const QList<Change> &changes, Store map) const;

private:
    const int m_version;
    const QString m_extension;
};

class MergingSettingsAccessor;

class QTCREATOR_UTILS_EXPORT UpgradingSettingsAccessor : public BackingUpSettingsAccessor
{
public:
    UpgradingSettingsAccessor();

    int currentVersion() const;
    int firstSupportedVersion() const;
    int lastSupportedVersion() const;

    QByteArray settingsId() const { return m_id; }

    bool isValidVersionAndId(const int version, const QByteArray &id) const;
    VersionUpgrader *upgrader(const int version) const;

    RestoreData readData(const FilePath &path, QWidget *parent) const override;

protected:
    Store prepareToWriteSettings(const Store &data) const override;

    void setSettingsId(const QByteArray &id) { m_id = id; }

    bool addVersionUpgrader(std::unique_ptr<VersionUpgrader> &&upgrader);
    RestoreData upgradeSettings(const RestoreData &data, const int targetVersion) const;
    RestoreData validateVersionRange(const RestoreData &data) const;

private:
    QByteArray m_id;
    std::vector<std::unique_ptr<VersionUpgrader>> m_upgraders;
};

// --------------------------------------------------------------------
// MergingSettingsAccessor:
// --------------------------------------------------------------------

class QTCREATOR_UTILS_EXPORT MergingSettingsAccessor : public UpgradingSettingsAccessor
{
public:
    struct SettingsMergeData {
        Store main;
        Store secondary;
        Key key;
    };

    MergingSettingsAccessor();

    RestoreData readData(const FilePath &path, QWidget *parent) const final;

    void setSecondaryAccessor(std::unique_ptr<SettingsAccessor> &&secondary);

protected:

    RestoreData mergeSettings(const RestoreData &main, const RestoreData &secondary) const;

    virtual SettingsMergeResult merge(const SettingsMergeData &global,
                                      const SettingsMergeData &local) const = 0;
    static bool isHouseKeepingKey(const Key &key);

    virtual Store postprocessMerge(const Store &main, const Store &secondary,
                                   const Store &result) const;

private:
    std::unique_ptr<SettingsAccessor> m_secondaryAccessor;
};

using SettingsMergeFunction = std::function<SettingsMergeResult(const MergingSettingsAccessor::SettingsMergeData &,
                                                                const MergingSettingsAccessor::SettingsMergeData &)>;
QTCREATOR_UTILS_EXPORT QVariant mergeQVariantMaps(const Store &mainTree, const Store &secondaryTree,
                                                  const SettingsMergeFunction &merge);

} // namespace Utils
