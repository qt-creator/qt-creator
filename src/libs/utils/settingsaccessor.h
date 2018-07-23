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

#pragma once

#include "utils_global.h"

#include "fileutils.h"
#include "optional.h"
#include "persistentsettings.h"

#include <QHash>
#include <QMessageBox>
#include <QVariantMap>

#include <memory>

namespace Utils {

// -----------------------------------------------------------------------------
// Helper:
// -----------------------------------------------------------------------------

QTCREATOR_UTILS_EXPORT int versionFromMap(const QVariantMap &data);
QTCREATOR_UTILS_EXPORT int originalVersionFromMap(const QVariantMap &data);
QTCREATOR_UTILS_EXPORT QByteArray settingsIdFromMap(const QVariantMap &data);

QTCREATOR_UTILS_EXPORT void setVersionInMap(QVariantMap &data, int version);
QTCREATOR_UTILS_EXPORT void setOriginalVersionInMap(QVariantMap &data, int version);
QTCREATOR_UTILS_EXPORT void setSettingsIdInMap(QVariantMap &data, const QByteArray &id);

// --------------------------------------------------------------------
// Helpers:
// --------------------------------------------------------------------

QTCREATOR_UTILS_EXPORT int versionFromMap(const QVariantMap &data);
QTCREATOR_UTILS_EXPORT int originalVersionFromMap(const QVariantMap &data);
QTCREATOR_UTILS_EXPORT QByteArray settingsIdFromMap(const QVariantMap &data);

QTCREATOR_UTILS_EXPORT void setVersionInMap(QVariantMap &data, int version);
QTCREATOR_UTILS_EXPORT void setOriginalVersionInMap(QVariantMap &data, int version);
QTCREATOR_UTILS_EXPORT void setSettingsIdInMap(QVariantMap &data, const QByteArray &id);

using SettingsMergeResult = optional<QPair<QString, QVariant>>;

// --------------------------------------------------------------------
// SettingsAccessor:
// --------------------------------------------------------------------

// Read/write files incl. error handling suitable for the UI:
class QTCREATOR_UTILS_EXPORT SettingsAccessor
{
public:
    SettingsAccessor(const QString &docType, const QString &displayName,
                     const QString &applicationDisplayName);
    virtual ~SettingsAccessor() = default;

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
        RestoreData(const FileName &path, const QVariantMap &data) : path{path}, data{data} { }
        RestoreData(const QString &title, const QString &message, const Issue::Type type) :
            RestoreData(Issue(title, message, type))
        { }
        RestoreData(const Issue &issue) : issue{issue} { }

        bool hasIssue() const { return bool(issue); }
        bool hasError() const { return hasIssue() && issue.value().type == Issue::Type::ERROR; }
        bool hasWarning() const { return hasIssue() && issue.value().type == Issue::Type::WARNING; }

        FileName path;
        QVariantMap data;
        optional<Issue> issue;
    };

    QVariantMap restoreSettings(QWidget *parent) const;
    bool saveSettings(const QVariantMap &data, QWidget *parent) const;

    const QString docType;
    const QString displayName;
    const QString applicationDisplayName;

    void setBaseFilePath(const FileName &baseFilePath) { m_baseFilePath = baseFilePath; }
    void setReadOnly() { m_readOnly = true; }
    FileName baseFilePath() const { return m_baseFilePath; }

    virtual RestoreData readData(const FileName &path, QWidget *parent) const;
    virtual optional<Issue> writeData(const FileName &path, const QVariantMap &data, QWidget *parent) const;

protected:
    // Report errors:
    QVariantMap restoreSettings(const FileName &settingsPath, QWidget *parent) const;
    ProceedInfo reportIssues(const Issue &issue, const FileName &path, QWidget *parent) const;

    virtual QVariantMap preprocessReadSettings(const QVariantMap &data) const;
    virtual QVariantMap prepareToWriteSettings(const QVariantMap &data) const;

    virtual RestoreData readFile(const FileName &path) const;
    virtual optional<Issue> writeFile(const FileName &path, const QVariantMap &data) const;

private:
    FileName m_baseFilePath;
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

    virtual FileNameList readFileCandidates(const FileName &baseFileName) const;
    // Return -1 if data1 is better that data2, 0 if both are equally worthwhile
    // and 1 if data2 is better than data1
    virtual int compare(const SettingsAccessor::RestoreData &data1,
                        const SettingsAccessor::RestoreData &data2) const;

    virtual optional<FileName>
    backupName(const QVariantMap &oldData, const FileName &path, const QVariantMap &data) const;
};

class QTCREATOR_UTILS_EXPORT BackingUpSettingsAccessor : public SettingsAccessor
{
public:
    BackingUpSettingsAccessor(const QString &docType, const QString &displayName,
                              const QString &applicationDisplayName);
    BackingUpSettingsAccessor(std::unique_ptr<BackUpStrategy> &&strategy, const QString &docType,
                              const QString &displayName, const QString &applicationDisplayName);

    RestoreData readData(const FileName &path, QWidget *parent) const override;
    optional<Issue> writeData(const FileName &path, const QVariantMap &data,
                              QWidget *parent) const override;

    BackUpStrategy *strategy() const { return m_strategy.get(); }

private:
    FileNameList readFileCandidates(const FileName &path) const;
    RestoreData bestReadFileData(const FileNameList &candidates, QWidget *parent) const;
    void backupFile(const FileName &path, const QVariantMap &data, QWidget *parent) const;

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

    optional<FileName>
    backupName(const QVariantMap &oldData, const FileName &path, const QVariantMap &data) const override;

    const UpgradingSettingsAccessor *accessor() const { return m_accessor; }

protected:
    const UpgradingSettingsAccessor *m_accessor = nullptr;
};

// Handles updating a QVariantMap from version() to version() + 1
class QTCREATOR_UTILS_EXPORT VersionUpgrader
{
public:
    VersionUpgrader(const int version, const QString &extension);
    virtual ~VersionUpgrader() = default;

    int version() const;
    QString backupExtension() const;

    virtual QVariantMap upgrade(const QVariantMap &data) = 0;

protected:
    using Change = QPair<QLatin1String,QLatin1String>;
    QVariantMap renameKeys(const QList<Change> &changes, QVariantMap map) const;

private:
    const int m_version;
    const QString m_extension;
};

class MergingSettingsAccessor;

class QTCREATOR_UTILS_EXPORT UpgradingSettingsAccessor : public BackingUpSettingsAccessor
{
public:
    UpgradingSettingsAccessor(const QString &docType,
                              const QString &displayName, const QString &applicationDisplayName);
    UpgradingSettingsAccessor(std::unique_ptr<BackUpStrategy> &&strategy, const QString &docType,
                              const QString &displayName, const QString &appDisplayName);

    int currentVersion() const;
    int firstSupportedVersion() const;
    int lastSupportedVersion() const;

    QByteArray settingsId() const { return m_id; }

    bool isValidVersionAndId(const int version, const QByteArray &id) const;
    VersionUpgrader *upgrader(const int version) const;

    RestoreData readData(const FileName &path, QWidget *parent) const override;

protected:
    QVariantMap prepareToWriteSettings(const QVariantMap &data) const override;

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
        QVariantMap main;
        QVariantMap secondary;
        QString key;
    };

    MergingSettingsAccessor(std::unique_ptr<BackUpStrategy> &&strategy,
                            const QString &docType, const QString &displayName,
                            const QString &applicationDisplayName);

    RestoreData readData(const FileName &path, QWidget *parent) const final;

    void setSecondaryAccessor(std::unique_ptr<SettingsAccessor> &&secondary);

protected:

    RestoreData mergeSettings(const RestoreData &main, const RestoreData &secondary) const;

    virtual SettingsMergeResult merge(const SettingsMergeData &global,
                                      const SettingsMergeData &local) const = 0;
    bool isHouseKeepingKey(const QString &key) const;

    virtual QVariantMap postprocessMerge(const QVariantMap &main, const QVariantMap &secondary,
                                         const QVariantMap &result) const;

private:
    std::unique_ptr<SettingsAccessor> m_secondaryAccessor;
};

using SettingsMergeFunction = std::function<SettingsMergeResult(const MergingSettingsAccessor::SettingsMergeData &,
                                                                const MergingSettingsAccessor::SettingsMergeData &)>;
QTCREATOR_UTILS_EXPORT QVariant mergeQVariantMaps(const QVariantMap &mainTree, const QVariantMap &secondaryTree,
                                                  const SettingsMergeFunction &merge);

} // namespace Utils
