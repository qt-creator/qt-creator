/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://www.qtsoftware.com/contact.
**
**************************************************************************/

#ifndef QTVERSIONMANAGER_H
#define QTVERSIONMANAGER_H

#include <projectexplorer/environment.h>
#include <projectexplorer/toolchain.h>

#include <QtCore/QSharedPointer>
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
    QtVersion(const QString &name, const QString &path, int id,
              bool isAutodetected = false, const QString &autodetectionSource = QString());
    QtVersion()
        :m_name(QString::null), m_id(-1), m_toolChain(0)
    { setPath(QString::null); }
    ~QtVersion();

    bool isValid() const; //TOOD check that the dir exists and the name is non empty
    bool isInstalled() const;
    bool isAutodetected() const { return m_isAutodetected; }
    QString autodetectionSource() const { return m_autodetectionSource; }

    QString name() const;
    QString path() const;
    QString sourcePath() const;
    QString mkspec() const;
    QString mkspecPath() const;
    QString qmakeCommand() const;
    QString uicCommand() const;
    QString designerCommand() const;
    QString linguistCommand() const;
    QString qmakeCXX() const;
    ProjectExplorer::ToolChain *toolChain() const;

    QString qtVersionString() const;
    // Returns the PREFIX, BINPREFIX, DOCPREFIX and similar information
    QHash<QString,QString> versionInfo() const;

    ProjectExplorer::ToolChain::ToolChainType toolchainType() const;

    QString mingwDirectory() const;
    void setMingwDirectory(const QString &directory);
    QString msvcVersion() const;
    QString wincePlatform() const;
    void setMsvcVersion(const QString &version);
    void addToEnvironment(ProjectExplorer::Environment &env) const;

    bool hasDebuggingHelper() const;
    QString debuggingHelperLibrary() const;

    // Builds a debugging library
    // returns the output of the commands
    QString buildDebuggingHelperLibrary();

    bool hasExamples() const;
    QString examplesPath() const;

    bool hasDocumentation() const;
    QString documentationPath() const;

    bool hasDemos() const;
    QString demosPath() const;

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
    void updateMkSpec() const;
    void updateVersionInfo() const;
    void updateQMakeCXX() const;
    void updateToolChain() const;
    QString findQtBinary(const QStringList &possibleName) const;
    QString m_name;
    QString m_path;
    QString m_sourcePath;
    QString m_mingwDirectory;
    QString m_msvcVersion;
    int m_id;
    bool m_isAutodetected;
    QString m_autodetectionSource;
    bool m_hasDebuggingHelper;

    mutable bool m_mkspecUpToDate;
    mutable QString m_mkspec; // updated lazily
    mutable QString m_mkspecFullPath;

    mutable bool m_versionInfoUpToDate;
    mutable QHash<QString,QString> m_versionInfo; // updated lazily
    mutable bool m_notInstalled;
    mutable bool m_defaultConfigIsDebug;
    mutable bool m_defaultConfigIsDebugAndRelease;
    mutable bool m_hasExamples;
    mutable bool m_hasDemos;
    mutable bool m_hasDocumentation;

    mutable QString m_qmakeCommand;
    mutable QString m_qtVersionString;
    mutable QString m_uicCommand;
    mutable QString m_designerCommand;
    mutable QString m_linguistCommand;

    mutable bool m_qmakeCXXUpToDate;
    mutable QString m_qmakeCXX;

    mutable bool m_toolChainUpToDate;
    mutable QSharedPointer<ProjectExplorer::ToolChain> m_toolChain;
};

class QtVersionManager : public QObject
{
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
    void updatedExamples(const QString& examplesPath, const QString& demosPath);
private:
    // Used by QtOptionsPage
    void setNewQtVersions(QList<QtVersion *> newVersions, int newDefaultVersion);
    // Used by QtVersion
    int getUniqueId();
    void writeVersionsIntoSettings();
    void addNewVersionsFromInstaller();
    void updateSystemVersion();
    void updateDocumentation();
    void updateExamples();

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
