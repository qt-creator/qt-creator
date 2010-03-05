/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef QTVERSIONMANAGER_H
#define QTVERSIONMANAGER_H

#include <projectexplorer/environment.h>
#include <projectexplorer/toolchain.h>
#include <QSharedPointer>

#include <QtCore/QHash>
#include <QtCore/QSet>
#include <QtCore/QSharedPointer>

namespace ProjectExplorer {
    class ToolChain;
}

namespace Qt4ProjectManager {

namespace Internal {
class QtOptionsPageWidget;
class QtOptionsPage;
}

class QtVersion
{
    friend class QtVersionManager;
public:
    QtVersion(const QString &name, const QString &qmakeCommand,
              bool isAutodetected = false, const QString &autodetectionSource = QString());

    QtVersion(const QString &path, bool isAutodetected = false, const QString &autodetectionSource = QString());

    QtVersion(const QString &name, const QString &qmakeCommand, int id,
              bool isAutodetected = false, const QString &autodetectionSource = QString());
    QtVersion();
    ~QtVersion();

    bool isValid() const; //TOOD check that the dir exists and the name is non empty
    QString invalidReason() const;
    bool isAutodetected() const { return m_isAutodetected; }
    QString autodetectionSource() const { return m_autodetectionSource; }

    QString displayName() const;
    QString sourcePath() const;
    QString qmakeCommand() const;
    QString uicCommand() const;
    QString designerCommand() const;
    QString linguistCommand() const;

    bool supportsTargetId(const QString &id) const;
    QSet<QString> supportedTargetIds() const;

    QList<ProjectExplorer::ToolChain::ToolChainType> possibleToolChainTypes() const;
    ProjectExplorer::ToolChain *toolChain(ProjectExplorer::ToolChain::ToolChainType type) const;

    /// @returns the name of the mkspec, which is generally not enough
    /// to pass to qmake.
    QString mkspec() const;
    /// @returns the full path to the default directory
    /// specifally not the directory the symlink/ORIGINAL_QMAKESPEC points to
    QString mkspecPath() const;

    void setDisplayName(const QString &name);
    void setQMakeCommand(const QString &path);

    QString qtVersionString() const;
    // Returns the PREFIX, BINPREFIX, DOCPREFIX and similar information
    QHash<QString,QString> versionInfo() const;

    QString mwcDirectory() const;
    void setMwcDirectory(const QString &directory);
    QString s60SDKDirectory() const;
    void setS60SDKDirectory(const QString &directory);
    QString gcceDirectory() const;
    void setGcceDirectory(const QString &directory);

    QString mingwDirectory() const;
    void setMingwDirectory(const QString &directory);
    QString msvcVersion() const;
    void setMsvcVersion(const QString &version);
    void addToEnvironment(ProjectExplorer::Environment &env) const;

    bool hasDebuggingHelper() const;
    QString debuggingHelperLibrary() const;
    QStringList debuggingHelperLibraryLocations() const;

    // Builds a debugging library
    // returns the output of the commands
    QString buildDebuggingHelperLibrary();

    bool hasExamples() const;
    QString examplesPath() const;

    bool hasDocumentation() const;
    QString documentationPath() const;

    bool hasDemos() const;
    QString demosPath() const;

    // All valid Ids are >= 0
    int uniqueId() const;
    bool isQt64Bit() const;

    enum QmakeBuildConfig
    {
        NoBuild = 1,
        DebugBuild = 2,
        BuildAll = 8
    };

    Q_DECLARE_FLAGS(QmakeBuildConfigs, QmakeBuildConfig)

    QmakeBuildConfigs defaultBuildConfig() const;
    QString toHtml() const;
private:
    QList<QSharedPointer<ProjectExplorer::ToolChain> > toolChains() const;
    static int getUniqueId();
    // Also used by QtOptionsPageWidget
    void updateSourcePath();
    void updateVersionInfo() const;
    QString findQtBinary(const QStringList &possibleName) const;
    void updateToolChainAndMkspec() const;
    QString m_displayName;
    QString m_sourcePath;
    QString m_mingwDirectory;
    QString m_msvcVersion;
    int m_id;
    bool m_isAutodetected;
    QString m_autodetectionSource;
    bool m_hasDebuggingHelper;

    QString m_mwcDirectory;
    QString m_s60SDKDirectory;
    QString m_gcceDirectory;

    mutable bool m_toolChainUpToDate;
    mutable QString m_mkspec; // updated lazily
    mutable QString m_mkspecFullPath;
    mutable QList<QSharedPointer<ProjectExplorer::ToolChain> > m_toolChains;

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
    mutable QSet<QString> m_targetIds;
};

struct QMakeAssignment
{
    QString variable;
    QString op;
    QString value;
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

    // This will *always* return at least one (Qt in Path), even if that is
    // unconfigured.
    QList<QtVersion *> versions() const;

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
    QList<QtVersion *> versionsForTargetId(const QString &id) const;
    QSet<QString> supportedTargetIds() const;

    // Static Methods
    static QPair<QtVersion::QmakeBuildConfigs, QStringList> scanMakeFile(const QString &directory,
                                                                         QtVersion::QmakeBuildConfigs defaultBuildConfig);
    static QString findQMakeBinaryFromMakefile(const QString &directory);
    bool isValidId(int id) const;

signals:
    void qtVersionsChanged(const QList<int> &uniqueIds);
    void updateExamples(QString, QString, QString);

private slots:
    void updateExamples();
private:
    // This function is really simplistic...
    static bool equals(QtVersion *a, QtVersion *b);
    static QString findQMakeLine(const QString &directory);
    static QString trimLine(const QString line);
    static QStringList splitLine(const QString &line);
    static void parseParts(const QStringList &parts,
                           QList<QMakeAssignment> *assignments,
                           QList<QMakeAssignment> *afterAssignments,
                           QStringList *additionalArguments);
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
    QList<QtVersion *> m_versions;
    QMap<int, int> m_uniqueIdToIndex;
    int m_idcount;
    // managed by QtProjectManagerPlugin
    static QtVersionManager *m_self;
};

} // namespace Qt4ProjectManager

Q_DECLARE_OPERATORS_FOR_FLAGS(Qt4ProjectManager::QtVersion::QmakeBuildConfigs)

#endif // QTVERSIONMANAGER_H
