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
// BasicSettingsAccessor:
// --------------------------------------------------------------------

// Read/write files incl. error handling suitable for the UI:
class QTCREATOR_UTILS_EXPORT BasicSettingsAccessor
{
public:
    BasicSettingsAccessor(const QString &docType, const QString &displayName,
                          const QString &applicationDisplayName);
    virtual ~BasicSettingsAccessor() = default;

    enum ProceedInfo { Continue, DiscardAndContinue };
    typedef QHash<QMessageBox::StandardButton, ProceedInfo> ButtonMap;
    class Issue {
    public:
        Issue(const QString &title, const QString &message) : title{title}, message{message} { }

        QMessageBox::StandardButtons allButtons() const;

        QString title;
        QString message;
        QMessageBox::StandardButton defaultButton = QMessageBox::NoButton;
        QMessageBox::StandardButton escapeButton = QMessageBox::Ok;
        QHash<QMessageBox::StandardButton, ProceedInfo> buttons = {{QMessageBox::Ok, ProceedInfo::Continue}};
    };

    class RestoreData {
    public:
        RestoreData() = default;
        RestoreData(const Utils::FileName &path, const QVariantMap &data) : path{path}, data{data} { }
        RestoreData(const QString &title, const QString &message) : RestoreData(Issue(title, message)) { }
        RestoreData(const Issue &issue) : issue{issue} { }
        Utils::FileName path;
        QVariantMap data;
        Utils::optional<Issue> issue;
    };

    QVariantMap restoreSettings(QWidget *parent) const;
    bool saveSettings(const QVariantMap &data, QWidget *parent) const;

    const QString docType;
    const QString displayName;
    const QString applicationDisplayName;

    void setBaseFilePath(const Utils::FileName &baseFilePath) { m_baseFilePath = baseFilePath; }
    Utils::FileName baseFilePath() const { return m_baseFilePath; }

    virtual RestoreData readData(const Utils::FileName &path, QWidget *parent) const;
    virtual Utils::optional<Issue> writeData(const Utils::FileName &path, const QVariantMap &data) const;

protected:
    // Report errors:
    ProceedInfo reportIssues(const Issue &issue, const FileName &path, QWidget *parent) const;

    virtual QVariantMap preprocessReadSettings(const QVariantMap &data) const;
    virtual QVariantMap prepareToWriteSettings(const QVariantMap &data) const;

    RestoreData readFile(const Utils::FileName &path) const;
    Utils::optional<Issue> writeFile(const Utils::FileName &path, const QVariantMap &data) const;

private:
    Utils::FileName m_baseFilePath;
    mutable std::unique_ptr<PersistentSettingsWriter> m_writer;
};

// --------------------------------------------------------------------
// BackingUpSettingsAccessor:
// --------------------------------------------------------------------

class QTCREATOR_UTILS_EXPORT BackUpStrategy
{
public:
    virtual ~BackUpStrategy() = default;

    virtual FileNameList readFileCandidates(const Utils::FileName &baseFileName) const;
    // Return -1 if data1 is better that data2, 0 if both are equally worthwhile
    // and 1 if data2 is better than data1
    virtual int compare(const BasicSettingsAccessor::RestoreData &data1,
                        const BasicSettingsAccessor::RestoreData &data2) const;

    virtual optional<FileName>
    backupName(const QVariantMap &oldData, const FileName &path, const QVariantMap &data) const;
};

class QTCREATOR_UTILS_EXPORT NoBackUpStrategy : public BackUpStrategy
{
public:
    FileNameList readFileCandidates(const Utils::FileName &baseFileName) const { return {baseFileName}; }
    optional<FileName>
    backupName(const QVariantMap &, const FileName &, const QVariantMap &) const final { return Utils::nullopt; }
};

class QTCREATOR_UTILS_EXPORT VersionedBackUpStrategy : public BackUpStrategy
{
public:
    // Return -1 if data1 is better that data2, 0 if both are equally worthwhile
    // and 1 if data2 is better than data1
    int compare(const BasicSettingsAccessor::RestoreData &data1,
                const BasicSettingsAccessor::RestoreData &data2) const override;

    optional<FileName>
    backupName(const QVariantMap &oldData, const FileName &path, const QVariantMap &data) const override;

    bool isValidVersionAndId(const int version, const QByteArray &id) const;

protected:
    virtual QByteArray id() const = 0;
    virtual int firstSupportedVersion() const = 0;
    virtual int currentVersion() const = 0;
};

class QTCREATOR_UTILS_EXPORT BackingUpSettingsAccessor : public BasicSettingsAccessor
{
public:
    BackingUpSettingsAccessor(const QString &docType, const QString &displayName,
                              const QString &applicationDisplayName);
    BackingUpSettingsAccessor(std::unique_ptr<BackUpStrategy> &&strategy,
                              const QString &docType, const QString &displayName,
                              const QString &applicationDisplayName);

    RestoreData readData(const Utils::FileName &path, QWidget *parent) const;
    Utils::optional<Issue> writeData(const Utils::FileName &path, const QVariantMap &data) const;

    BackUpStrategy *strategy() const { return m_strategy.get(); }

private:
    Utils::FileNameList readFileCandidates(const FileName &path) const;
    RestoreData bestReadFileData(const FileNameList &candidates, QWidget *parent) const;
    void backupFile(const FileName &path, const QVariantMap &data) const;

    std::unique_ptr<BackUpStrategy> m_strategy;
};

// --------------------------------------------------------------------
// VersionUpgrader:
// --------------------------------------------------------------------

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
    typedef QPair<QLatin1String,QLatin1String> Change;
    QVariantMap renameKeys(const QList<Change> &changes, QVariantMap map) const;

private:
    const int m_version;
    const QString m_extension;
};

class SettingsAccessorPrivate;

class QTCREATOR_UTILS_EXPORT SettingsAccessor : public BackingUpSettingsAccessor
{
public:
    explicit SettingsAccessor(std::unique_ptr<BackUpStrategy> &&strategy,
                              const Utils::FileName &baseFile, const QString &docType,
                              const QString &displayName, const QString &appDisplayName);
    ~SettingsAccessor() override;

    int currentVersion() const;
    int firstSupportedVersion() const;

    bool addVersionUpgrader(std::unique_ptr<VersionUpgrader> upgrader);

    Utils::FileName projectUserFile() const;
    Utils::FileName externalUserFile() const;
    Utils::FileName sharedFile() const;

    QByteArray settingsId() const;

protected:
    RestoreData readData(const Utils::FileName &path, QWidget *parent) const final;

    void setSettingsId(const QByteArray &id);
    QVariantMap upgradeSettings(const QVariantMap &data) const;
    QVariantMap upgradeSettings(const QVariantMap &data, const int targetVersion) const;

    QVariantMap prepareToWriteSettings(const QVariantMap &data) const override;

    virtual void storeSharedSettings(const QVariantMap &data) const;

    QVariantMap mergeSettings(const QVariantMap &userMap, const QVariantMap &sharedMap) const;

    Utils::optional<Issue> findIssues(const QVariantMap &data, const Utils::FileName &path) const;

private:
    RestoreData readSharedSettings(QWidget *parent) const;

    static QString differentEnvironmentMsg(const QString &projectName);

    SettingsAccessorPrivate *d;

    friend class SettingsAccessorPrivate;
};

} // namespace Utils
