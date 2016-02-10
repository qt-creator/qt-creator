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

#include <QString>

QT_BEGIN_NAMESPACE
class QSettings;
class QDir;
QT_END_NAMESPACE

namespace Perforce {
namespace Internal {

struct Settings {
    Settings();
    bool equals(const Settings &s) const;
    QStringList commonP4Arguments() const;

    // Checks. On success, errorMessage will contains the client root.
    bool check(QString *repositoryRoot /* = 0*/, QString *errorMessage) const;
    static bool doCheck(const QString &binary, const QStringList &basicArgs,
                        QString *repositoryRoot /* = 0 */,
                        QString *errorMessage);

    QString p4Command;
    QString p4BinaryPath;
    QString p4Port;
    QString p4Client;
    QString p4User;
    QString errorString;
    int logCount;
    bool defaultEnv = true;
    int timeOutS;
    bool promptToSubmit = true;
    bool autoOpen = true;
};

inline bool operator==(const Settings &s1, const Settings &s2) { return s1.equals(s2); }
inline bool operator!=(const Settings &s1, const Settings &s2) { return !s1.equals(s2); }

/* PerforceSettings: Aggregates settings struct and toplevel directory
 * which is determined externally by background checks and provides a convenience
 * for determining the common arguments.
 * Those must contain (apart from server connection settings) the working directory
 * with the "-d" option. This is because the p4 command line client detects its path
 * from the PWD environment variable which breaks relative paths since that is set by
 * the shell running Creator and is not necessarily that of the working directory
 * (see p4 documentation).
 * An additional complication is that the repository might be a symbolic link on Unix,
 * say "$HOME/dev" linked to "/depot". If the p4 client specification contains
 * "$HOME/dev", paths containing "/depot" will be refused as "not under client's view" by
 * p4. This is why the client root portion of working directory must be mapped for the
 * "-d" option, so that running p4 in "/depot/dev/foo" results in "-d $HOME/dev/foo". */

class PerforceSettings
{
public:
    PerforceSettings() = default;
    ~PerforceSettings();
    PerforceSettings(const PerforceSettings &other) = delete;

    inline bool isValid() const
    {
        return !m_topLevel.isEmpty() && !m_settings.p4BinaryPath.isEmpty();
    }

    void fromSettings(QSettings *settings);
    void toSettings(QSettings *) const;

    void setSettings(const Settings &s);
    Settings settings() const;

    inline int timeOutS()      const { return m_settings.timeOutS;  }
    inline int longTimeOutS() const { return m_settings.timeOutS * 10; }
    inline int timeOutMS()     const { return m_settings.timeOutS * 1000;  }

    inline int logCount() const { return m_settings.logCount; }

    QString topLevel() const;
    QString topLevelSymLinkTarget() const;

    void setTopLevel(const QString &);

    // Return relative path to top level. Returns "" if it is the same directory,
    // ".." if it is not within.
    QString relativeToTopLevel(const QString &dir) const;
    // Return argument list relative to top level (empty meaning,
    // it is the same directory).
    QString relativeToTopLevelArguments(const QString &dir) const;

    // Map p4 path back to file system in case of a symlinked top-level
    QString mapToFileSystem(const QString &perforceFilePath) const;

    QString p4Command() const;
    QString p4BinaryPath() const;
    QString p4Port() const;
    QString p4Client() const;
    QString p4User() const;
    bool defaultEnv() const;
    bool promptToSubmit() const;
    void setPromptToSubmit(bool p);
    bool autoOpen() const;
    void setAutoOpen(bool p);

    // Return basic arguments, including -d and server connection parameters.
    QStringList commonP4Arguments(const QString &workingDir) const;

private:
    inline QStringList workingDirectoryArguments(const QString &workingDir) const;
    void clearTopLevel();

    Settings m_settings;
    QString m_topLevel;
    QString m_topLevelSymLinkTarget;
    QDir *m_topLevelDir = nullptr;
};

} // namespace Internal
} // namespace Perforce
