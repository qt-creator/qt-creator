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

#pragma once

#include <utils/fileutils.h>

#include <QHash>
#include <QVariantMap>
#include <QMessageBox>

namespace Utils { class PersistentSettingsWriter; }

namespace ProjectExplorer {

// --------------------------------------------------------------------
// VersionUpgrader:
// --------------------------------------------------------------------
// Handles updating a QVariantMap from version() - 1 to version()
class VersionUpgrader
{
public:
    virtual ~VersionUpgrader() = default;

    virtual int version() const = 0;
    virtual QString backupExtension() const = 0;

    virtual QVariantMap upgrade(const QVariantMap &data) = 0;

protected:
    typedef QPair<QLatin1String,QLatin1String> Change;
    QVariantMap renameKeys(const QList<Change> &changes, QVariantMap map) const;
};

class SettingsAccessorPrivate;

class SettingsAccessor
{
public:
    explicit SettingsAccessor(const Utils::FileName &baseFile);
    virtual ~SettingsAccessor();

    QVariantMap restoreSettings(QWidget *parent) const;
    bool saveSettings(const QVariantMap &data, QWidget *parent) const;

    static QVariantMap setVersionInMap(const QVariantMap &data, int version);
    static int versionFromMap(const QVariantMap &data);
    static int originalVersionFromMap(const QVariantMap &data);
    static QVariantMap setOriginalVersionInMap(const QVariantMap &data, int version);

    int currentVersion() const;
    int firstSupportedVersion() const;

    bool addVersionUpgrader(VersionUpgrader *upgrader); // takes ownership of upgrader

    enum ProceedInfo { Continue, DiscardAndContinue };
    typedef QHash<QMessageBox::StandardButton, ProceedInfo> ButtonMap;
    class IssueInfo {
    public:
        IssueInfo() : defaultButton(QMessageBox::NoButton), escapeButton(QMessageBox::NoButton) { }
        IssueInfo(const QString &t, const QString &m,
                  QMessageBox::StandardButton d = QMessageBox::NoButton,
                  QMessageBox::StandardButton e = QMessageBox::NoButton,
                  const ButtonMap &b = ButtonMap()) :
            title(t), message(m), defaultButton(d), escapeButton(e), buttons(b)
        { }
        IssueInfo(const IssueInfo &other) :
            title(other.title),
            message(other.message),
            defaultButton(other.defaultButton),
            escapeButton(other.escapeButton),
            buttons(other.buttons)
        { }

        IssueInfo &operator = (const IssueInfo &other)
        {
            title = other.title;
            message = other.message;
            defaultButton = other.defaultButton;
            escapeButton = other.escapeButton;
            buttons = other.buttons;
            return *this;
        }

        QString title;
        QString message;
        QMessageBox::StandardButton defaultButton;
        QMessageBox::StandardButton escapeButton;
        QHash<QMessageBox::StandardButton, ProceedInfo> buttons;
    };

protected:
    void setSettingsId(const QByteArray &id);
    void setDisplayName(const QString &dn);
    QVariantMap readFile(const Utils::FileName &path) const;
    QVariantMap upgradeSettings(const QVariantMap &data) const;

    ProceedInfo reportIssues(const QVariantMap &data, const Utils::FileName &path, QWidget *parent) const;

    virtual QVariantMap prepareSettings(const QVariantMap &data) const;
    virtual QVariantMap prepareToSaveSettings(const QVariantMap &data) const;

    virtual bool isBetterMatch(const QVariantMap &origData, const QVariantMap &newData) const;

    virtual Utils::FileName backupName(const QVariantMap &data) const;

    virtual IssueInfo findIssues(const QVariantMap &data, const Utils::FileName &path) const;

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

namespace Internal {
class UserFileAccessor : public SettingsAccessor
{
public:
    UserFileAccessor(Project *project);

    Project *project() const;

protected:
    QVariantMap prepareSettings(const QVariantMap &data) const final;
    QVariantMap prepareToSaveSettings(const QVariantMap &data) const final;

    void storeSharedSettings(const QVariantMap &data) const final;
    QVariant retrieveSharedSettings() const final;

private:
    Project *m_project;
};

} // namespace Internal
} // namespace ProjectExplorer
