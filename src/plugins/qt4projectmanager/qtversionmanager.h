/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef QTVERSIONMANAGER_H
#define QTVERSIONMANAGER_H

#include <projectexplorer/environment.h>
#include <projectexplorer/toolchain.h>

#include <QtCore/QHash>

namespace Qt4ProjectManager {

namespace Internal {
class QtOptionsPageWidget;
class QtOptionsPage;
class Qt4ProjectManagerPlugin;
}

class QtVersion
{
    friend class Internal::QtOptionsPageWidget; //for changing name and path
    friend class QtVersionManager;
public:
    QtVersion(const QString &name, const QString &path);
    QtVersion(const QString &name, const QString &path, int id, bool isSystemVersion = false);
    QtVersion()
        :m_name(QString::null), m_path(QString::null), m_id(-1)
    { }

    bool isValid() const; //TOOD check that the dir exists and the name is non empty
    bool isInstalled() const;
    bool isSystemVersion() const { return m_isSystemVersion; }

    QString name() const;
    QString path() const;
    QString sourcePath() const;
    QString mkspec() const;
    QString mkspecPath() const;
    QString qmakeCommand() const;
    QString uicCommand() const;
    QString designerCommand() const;
    QString linguistCommand() const;

    QString qtVersionString() const;
    // Returns the PREFIX, BINPREFIX, DOCPREFIX and similar information
    QHash<QString,QString> versionInfo() const;

    ProjectExplorer::ToolChain::ToolChainType toolchainType() const;

    QString mingwDirectory() const;
    void setMingwDirectory(const QString &directory);
    QString msvcVersion() const;
    QString wincePlatform() const;
    void setMsvcVersion(const QString &version);
    void addToEnvironment(ProjectExplorer::Environment &env);

    bool hasDebuggingHelper() const;
    QString debuggingHelperLibrary() const;
    // Builds a debugging library
    // returns the output of the commands
    QString buildDebuggingHelperLibrary();

    int uniqueId() const;
    bool isMSVC64Bit() const;

    enum QmakeBuildConfig
    {
        NoBuild = 1,
        DebugBuild = 2,
        BuildAll = 8
    };

    QmakeBuildConfig defaultBuildConfig() const;
private:
    static int getUniqueId();
    // Also used by QtOptionsPageWidget
    void setName(const QString &name);
    void setPath(const QString &path);
    void updateSourcePath();
    void updateVersionInfo() const;
    void updateMkSpec() const;
    QString findQtBinary(const QStringList &possibleName) const;
    QString m_name;
    mutable bool m_versionInfoUpToDate;
    mutable bool m_mkspecUpToDate;
    QString m_path;
    QString m_sourcePath;
    mutable QString m_mkspec; // updated lazily
    mutable QString m_mkspecFullPath;
    QString m_mingwDirectory;
    QString m_msvcVersion;
    mutable QHash<QString,QString> m_versionInfo; // updated lazily
    int m_id;
    bool m_isSystemVersion;
    mutable bool m_notInstalled;
    mutable bool m_defaultConfigIsDebug;
    mutable bool m_defaultConfigIsDebugAndRelease;
    mutable QString m_qmakeCommand;
    mutable QString m_uicCommand;
    mutable QString m_designerCommand;
    mutable QString m_linguistCommand;
    // This is updated on first call to qmakeCommand
    // That function is called from updateVersionInfo()
    mutable QString m_qtVersionString;
    bool m_hasDebuggingHelper;
};

class QtVersionManager : public QObject
{
    friend class Internal::Qt4ProjectManagerPlugin;
    Q_OBJECT
    // for getUniqueId();
    friend class QtVersion;
    friend class Internal::QtOptionsPage;
public:
    static QtVersionManager *instance();
    QtVersionManager();
    ~QtVersionManager();

    QList<QtVersion *> versions() const;

    QtVersion *version(int id) const;
    QtVersion *currentQtVersion() const;

    QtVersion *qtVersionForDirectory(const QString &directory);
    // Used by the projectloadwizard
    void addVersion(QtVersion *version);

    // Static Methods
    static QtVersion::QmakeBuildConfig scanMakefileForQmakeConfig(const QString &directory, QtVersion::QmakeBuildConfig defaultBuildConfig);
    static QString findQtVersionFromMakefile(const QString &directory);
signals:
    void defaultQtVersionChanged();
    void qtVersionsChanged();
private:
    // Used by QtOptionsPage
    void setNewQtVersions(QList<QtVersion *> newVersions, int newDefaultVersion);
    // Used by QtVersion
    int getUniqueId();
    void writeVersionsIntoSettings();
    void addNewVersionsFromInstaller();
    void updateSystemVersion();
    void updateDocumentation();

    static int indexOfVersionInList(const QtVersion * const version, const QList<QtVersion *> &list);
    void updateUniqueIdToIndexMap();

    QtVersion *m_emptyVersion;
    int m_defaultVersion;
    QList<QtVersion *> m_versions;
    QMap<int, int> m_uniqueIdToIndex;
    int m_idcount;
    // managed by QtProjectManagerPlugin
    static QtVersionManager *m_self;
};

} // namespace Qt4ProjectManager

#endif // QTVERSIONMANAGER_H
