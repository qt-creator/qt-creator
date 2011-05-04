/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef QTVERSIONMANAGER_H
#define QTVERSIONMANAGER_H

#include "qt4projectmanager_global.h"

#include <projectexplorer/abi.h>

#include <QtCore/QHash>
#include <QtCore/QSet>
#include <QtCore/QSharedPointer>
#include <QtCore/QFutureInterface>
#include <QtCore/QStringList>

namespace Utils {
class Environment;
}

namespace ProjectExplorer {
class HeaderPath;
class IOutputParser;
class Task;
}

namespace Qt4ProjectManager {

namespace Internal {
class QtOptionsPageWidget;
class QtOptionsPage;
}

class QT4PROJECTMANAGER_EXPORT QtVersionNumber
{
public:
    QtVersionNumber(int ma, int mi, int p);
    QtVersionNumber(const QString &versionString);
    QtVersionNumber();

    int majorVersion;
    int minorVersion;
    int patchVersion;
    bool operator <(const QtVersionNumber &b) const;
    bool operator <=(const QtVersionNumber &b) const;
    bool operator >(const QtVersionNumber &b) const;
    bool operator >=(const QtVersionNumber &b) const;
    bool operator !=(const QtVersionNumber &b) const;
    bool operator ==(const QtVersionNumber &b) const;
private:
    bool checkVersionString(const QString &version) const;
};


class QT4PROJECTMANAGER_EXPORT QtVersion
{
    friend class QtVersionManager;
public:
    QtVersion(const QString &name, const QString &qmakeCommand,
              bool isAutodetected = false, const QString &autodetectionSource = QString());

    explicit QtVersion(const QString &path, bool isAutodetected = false, const QString &autodetectionSource = QString());

    QtVersion(const QString &name, const QString &qmakeCommand, int id,
              bool isAutodetected = false, const QString &autodetectionSource = QString());
    QtVersion();
    ~QtVersion();

    bool isValid() const;
    bool toolChainAvailable(const QString &id) const;

    QString invalidReason() const;
    QString description() const;
    bool isAutodetected() const { return m_isAutodetected; }
    QString autodetectionSource() const { return m_autodetectionSource; }

    QString displayName() const;
    QString sourcePath() const;
    QString qmakeCommand() const;
    QString uicCommand() const;
    QString designerCommand() const;
    QString linguistCommand() const;
    QString qmlviewerCommand() const;
    QString systemRoot() const;
    void setSystemRoot(const QString &);

    bool supportsTargetId(const QString &id) const;
    QSet<QString> supportedTargetIds() const;

    QList<ProjectExplorer::Abi> qtAbis() const;

    void setForcedTargetIds(const QSet<QString> &ids);
    void setForcedQtAbis(const QList<ProjectExplorer::Abi> &abis);

    /// @returns the name of the mkspec, which is generally not enough
    /// to pass to qmake.
    QString mkspec() const;
    /// @returns the full path to the default directory
    /// specifally not the directory the symlink/ORIGINAL_QMAKESPEC points to
    QString mkspecPath() const;

    bool isBuildWithSymbianSbsV2() const;

    void setDisplayName(const QString &name);
    void setQMakeCommand(const QString &path);

    QString qtVersionString() const;
    QtVersionNumber qtVersion() const;
    // Returns the PREFIX, BINPREFIX, DOCPREFIX and similar information
    QHash<QString,QString> versionInfo() const;

    QString sbsV2Directory() const;
    void setSbsV2Directory(const QString &directory);

    void addToEnvironment(Utils::Environment &env) const;
    QList<ProjectExplorer::HeaderPath> systemHeaderPathes() const;

    bool supportsBinaryDebuggingHelper() const;
    QString gdbDebuggingHelperLibrary() const;
    QString qmlDebuggingHelperLibrary(bool debugVersion) const;
    QString qmlDumpTool(bool debugVersion) const;
    QString qmlObserverTool() const;
    QStringList debuggingHelperLibraryLocations() const;

    bool hasGdbDebuggingHelper() const;
    bool hasQmlDump() const;
    bool hasQmlDebuggingLibrary() const;
    bool hasQmlObserver() const;
    Utils::Environment qmlToolsEnvironment() const;

    void invalidateCache();

    bool hasExamples() const;
    QString examplesPath() const;

    bool hasDocumentation() const;
    QString documentationPath() const;

    bool hasDemos() const;
    QString demosPath() const;

    QString headerInstallPath() const;
    QString frameworkInstallPath() const;
    QString libraryInstallPath() const;

    // All valid Ids are >= 0
    int uniqueId() const;

    enum QmakeBuildConfig
    {
        NoBuild = 1,
        DebugBuild = 2,
        BuildAll = 8
    };

    Q_DECLARE_FLAGS(QmakeBuildConfigs, QmakeBuildConfig)

    QmakeBuildConfigs defaultBuildConfig() const;
    QString toHtml(bool verbose) const;

    bool supportsShadowBuilds() const;

    /// Check a .pro-file/Qt version combination on possible issues with
    /// its symbian setup.
    /// @return a list of tasks, ordered on severity (errors first, then
    ///         warnings and finally info items.
    QList<ProjectExplorer::Task> reportIssues(const QString &proFile, const QString &buildDir, bool includeTargetSpecificErrors);

    ProjectExplorer::IOutputParser *createOutputParser() const;

private:
    static int getUniqueId();
    // Also used by QtOptionsPageWidget
    void updateSourcePath();
    void updateVersionInfo() const;
    QString findQtBinary(const QStringList &possibleName) const;
    void updateAbiAndMkspec() const;
    QString resolveLink(const QString &path) const;
    QString qtCorePath() const;

    QString m_displayName;
    QString m_sourcePath;
    int m_id;
    bool m_isAutodetected;
    QString m_autodetectionSource;
    mutable bool m_hasDebuggingHelper; // controlled by m_versionInfoUpToDate
    mutable bool m_hasQmlDump;         // controlled by m_versionInfoUpToDate
    mutable bool m_hasQmlDebuggingLibrary; // controlled by m_versionInfoUpdate
    mutable bool m_hasQmlObserver;     // controlled by m_versionInfoUpToDate

    QString m_sbsV2Directory;
    mutable QString m_systemRoot;

    mutable bool m_abiUpToDate;
    mutable QString m_mkspec; // updated lazily
    mutable QString m_mkspecFullPath;
    mutable QList<ProjectExplorer::Abi> m_abis;
    mutable QList<ProjectExplorer::Abi> m_forcedAbis;

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
    mutable QString m_qmlviewerCommand;
    mutable QSet<QString> m_targetIds;
    mutable QSet<QString> m_forcedTargetIds;

    mutable bool m_isBuildUsingSbsV2;
    mutable bool m_qmakeIsExecutable;
    mutable bool m_validSystemRoot;
};

struct QMakeAssignment
{
    QString variable;
    QString op;
    QString value;
};

class QT4PROJECTMANAGER_EXPORT QtVersionManager : public QObject
{
    Q_OBJECT
    // for getUniqueId();
    friend class QtVersion;
    friend class Internal::QtOptionsPage;
public:
    static QtVersionManager *instance();
    QtVersionManager();
    ~QtVersionManager();

    // This will *always* return at least one (Qt in Path), even if that is
    // unconfigured.
    QList<QtVersion *> versions() const;
    QList<QtVersion *> validVersions() const;

    // Note: DO NOT STORE THIS POINTER!
    //       The QtVersionManager will delete it at random times and you will
    //       need to get a new pointer by calling this method again!
    QtVersion *version(int id) const;
    QtVersion *emptyVersion() const;

    QtVersion *qtVersionForQMakeBinary(const QString &qmakePath);

    // Used by the projectloadwizard
    void addVersion(QtVersion *version);
    void removeVersion(QtVersion *version);

    // Target Support:
    bool supportsTargetId(const QString &id) const;
    // This returns a list of versions that support the target with the given id.
    // @return A list of QtVersions that supports a target. This list may be empty!

    QList<QtVersion *> versionsForTargetId(const QString &id, const QtVersionNumber &minimumQtVersion = QtVersionNumber()) const;
    QSet<QString> supportedTargetIds() const;

    // Static Methods
    static bool makefileIsFor(const QString &makefile, const QString &proFile);
    static QPair<QtVersion::QmakeBuildConfigs, QString> scanMakeFile(const QString &makefile,
                                                                     QtVersion::QmakeBuildConfigs defaultBuildConfig);
    static QString findQMakeBinaryFromMakefile(const QString &directory);
    bool isValidId(int id) const;

    // Compatibility with pre-2.2:
    QString popPendingMwcUpdate();
    QString popPendingGcceUpdate();

signals:
    void qtVersionsChanged(const QList<int> &uniqueIds);
    void updateExamples(QString, QString, QString);

private slots:
    void updateSettings();
private:
    // This function is really simplistic...
    static bool equals(QtVersion *a, QtVersion *b);
    static QString findQMakeLine(const QString &directory, const QString &key);
    static QString trimLine(const QString line);
    static void parseArgs(const QString &args,
                          QList<QMakeAssignment> *assignments,
                          QList<QMakeAssignment> *afterAssignments,
                          QString *additionalArguments);
    static QtVersion::QmakeBuildConfigs qmakeBuildConfigFromCmdArgs(QList<QMakeAssignment> *assignments,
                                                                    QtVersion::QmakeBuildConfigs defaultBuildConfig);
    // Used by QtOptionsPage
    void setNewQtVersions(QList<QtVersion *> newVersions);
    // Used by QtVersion
    int getUniqueId();
    void writeVersionsIntoSettings();
    void addNewVersionsFromInstaller();
    void updateSystemVersion();
    void updateDocumentation();

    static int indexOfVersionInList(const QtVersion * const version, const QList<QtVersion *> &list);
    void updateUniqueIdToIndexMap();

    QtVersion *m_emptyVersion;
    QMap<int, QtVersion *> m_versions;
    int m_idcount;
    // managed by QtProjectManagerPlugin
    static QtVersionManager *m_self;

    // Compatibility with pre-2.2:
    QStringList m_pendingMwcUpdates;
    QStringList m_pendingGcceUpdates;
};

} // namespace Qt4ProjectManager

Q_DECLARE_OPERATORS_FOR_FLAGS(Qt4ProjectManager::QtVersion::QmakeBuildConfigs)

#endif // QTVERSIONMANAGER_H
