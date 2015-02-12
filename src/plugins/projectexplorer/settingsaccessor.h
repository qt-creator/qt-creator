/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef SETTINGSACCESSOR_H
#define SETTINGSACCESSOR_H

#include <utils/fileutils.h>

#include <QHash>
#include <QVariantMap>
#include <QMessageBox>

namespace Utils { class PersistentSettingsWriter; }

namespace ProjectExplorer {

class Project;

namespace Internal { class VersionUpgrader; }

class SettingsAccessorPrivate;

class SettingsAccessor
{
public:
    SettingsAccessor(Project *project);
    virtual ~SettingsAccessor();

    Project *project() const;

    QVariantMap restoreSettings(QWidget *parent) const;
    bool saveSettings(const QVariantMap &data, QWidget *parent) const;

    static QVariantMap setVersionInMap(const QVariantMap &data, int version);
    static int versionFromMap(const QVariantMap &data);
    static int originalVersionFromMap(const QVariantMap &data);
    static QVariantMap setOriginalVersionInMap(const QVariantMap &data, int version);

    int currentVersion() const;
    int firstSupportedVersion() const;

    bool addVersionUpgrader(Internal::VersionUpgrader *upgrader); // takes ownership of upgrader

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
    QVariantMap readFile(const Utils::FileName &path) const;
    QVariantMap upgradeSettings(const QVariantMap &data) const;

    ProceedInfo reportIssues(const QVariantMap &data, const Utils::FileName &path, QWidget *parent) const;

    virtual QVariantMap prepareSettings(const QVariantMap &data) const;
    virtual QVariantMap prepareToSaveSettings(const QVariantMap &data) const;

    virtual bool isBetterMatch(const QVariantMap &origData, const QVariantMap &newData) const;

    virtual Utils::FileName backupName(const QVariantMap &data) const;

    virtual IssueInfo findIssues(const QVariantMap &data, const Utils::FileName &path) const;

private:
    QList<Utils::FileName> settingsFiles(const QString &suffix) const;
    static QByteArray creatorId();
    QString defaultFileName(const QString &suffix) const;
    void backupUserFile() const;

    QVariantMap readUserSettings(QWidget *parent) const;
    QVariantMap readSharedSettings(QWidget *parent) const;
    QVariantMap mergeSettings(const QVariantMap &userMap, const QVariantMap &sharedMap) const;

    static QByteArray environmentIdFromMap(const QVariantMap &data);
    static QString differentEnvironmentMsg(const QString &projectName);

    QString m_userSuffix;
    QString m_sharedSuffix;

    Project *m_project;

    SettingsAccessorPrivate *d;

    friend class SettingsAccessorPrivate;
};

namespace Internal {
class UserFileAccessor : public SettingsAccessor
{
public:
    UserFileAccessor(Project *project);

protected:
    QVariantMap prepareSettings(const QVariantMap &data) const;
    QVariantMap prepareToSaveSettings(const QVariantMap &data) const;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // SETTINGSACCESSOR_H
