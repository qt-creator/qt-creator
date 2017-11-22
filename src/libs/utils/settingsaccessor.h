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

namespace Utils {

// --------------------------------------------------------------------
// BasicSettingsAccessor:
// --------------------------------------------------------------------

class QTCREATOR_UTILS_EXPORT BasicSettingsAccessor
{
public:
    BasicSettingsAccessor(const Utils::FileName &baseFilePath, const QString &docType);
    virtual ~BasicSettingsAccessor();

    virtual QVariantMap restoreSettings(QWidget *parent) const;
    virtual bool saveSettings(const QVariantMap &data, QWidget *parent) const;

protected:
    QVariantMap readFile(const Utils::FileName &path) const;
    bool writeFile(const Utils::FileName &path, const QVariantMap &data, QWidget *parent) const;

    Utils::FileName baseFilePath() const;

private:
    const Utils::FileName m_baseFilePath;
    const QString m_docType;
    mutable std::unique_ptr<PersistentSettingsWriter> m_writer;
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

class QTCREATOR_UTILS_EXPORT SettingsAccessor : public BasicSettingsAccessor
{
public:
    explicit SettingsAccessor(const Utils::FileName &baseFile, const QString &docType);
    ~SettingsAccessor() override;

    QVariantMap restoreSettings(QWidget *parent) const override;
    bool saveSettings(const QVariantMap &data, QWidget *parent) const override;

    static QVariantMap setVersionInMap(const QVariantMap &data, int version);
    static int versionFromMap(const QVariantMap &data);
    static int originalVersionFromMap(const QVariantMap &data);
    static QVariantMap setOriginalVersionInMap(const QVariantMap &data, int version);

    int currentVersion() const;
    int firstSupportedVersion() const;

    bool addVersionUpgrader(std::unique_ptr<VersionUpgrader> upgrader);

    enum ProceedInfo { Continue, DiscardAndContinue };
    typedef QHash<QMessageBox::StandardButton, ProceedInfo> ButtonMap;
    class IssueInfo {
    public:
        QString title;
        QString message;
        QMessageBox::StandardButton defaultButton = QMessageBox::NoButton;
        QMessageBox::StandardButton escapeButton = QMessageBox::NoButton;
        QHash<QMessageBox::StandardButton, ProceedInfo> buttons;
    };

protected:
    void setSettingsId(const QByteArray &id);
    void setDisplayName(const QString &dn);
    void setApplicationDisplayName(const QString &dn);
    QVariantMap upgradeSettings(const QVariantMap &data) const;
    QVariantMap upgradeSettings(const QVariantMap &data, const int targetVersion) const;

    ProceedInfo reportIssues(const QVariantMap &data, const Utils::FileName &path, QWidget *parent) const;

    virtual QVariantMap prepareSettings(const QVariantMap &data) const;
    virtual QVariantMap prepareToSaveSettings(const QVariantMap &data) const;

    virtual bool isBetterMatch(const QVariantMap &origData, const QVariantMap &newData) const;
    virtual bool isValidVersionAndId(const int version, const QByteArray &id) const;

    virtual Utils::FileName backupName(const QVariantMap &data) const;

    virtual Utils::optional<IssueInfo> findIssues(const QVariantMap &data,
                                                  const Utils::FileName &path) const;

    virtual void storeSharedSettings(const QVariantMap &data) const;
    virtual QVariant retrieveSharedSettings() const;

private:
    Utils::FileNameList settingsFiles(const QString &suffix) const;
    QByteArray settingsId() const;
    QString displayName() const;
    QString defaultFileName(const QString &suffix) const;
    void backupUserFile() const;

    QVariantMap readUserSettings(QWidget *parent) const;
    QVariantMap readSharedSettings(QWidget *parent) const;
    QVariantMap mergeSettings(const QVariantMap &userMap, const QVariantMap &sharedMap) const;

    static QByteArray settingsIdFromMap(const QVariantMap &data);
    static QString differentEnvironmentMsg(const QString &projectName);

    SettingsAccessorPrivate *d;

    friend class SettingsAccessorPrivate;
};

} // namespace Utils
