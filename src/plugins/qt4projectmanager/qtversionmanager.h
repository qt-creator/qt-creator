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

#include "ui_qtversionmanager.h"

#include <coreplugin/dialogs/ioptionspage.h>
#include <projectexplorer/projectexplorer.h>

#include <QtCore/QDebug>
#include <QtCore/QPointer>
#include <QtGui/QWidget>

namespace Qt4ProjectManager {
namespace Internal {

class QtDirWidget;

class QtVersion {
    friend class QtDirWidget; //for changing name and path
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
    QString makeCommand() const;
    QString qmakeCommand() const;
    QString qtVersionString() const;
    // Returns the PREFIX, BINPREFIX, DOCPREFIX and similar information
    QHash<QString,QString> versionInfo() const;

    enum ToolchainType { MinGW, MSVC, WINCE, OTHER, INVALID };

    ToolchainType toolchainType() const;

    QString mingwDirectory() const;
    void setMingwDirectory(const QString &directory);
    QString prependPath() const;
    void setPrependPath(const QString &string);
    QString msvcVersion() const;
    void setMsvcVersion(const QString &version);
    ProjectExplorer::Environment addToEnvironment(const ProjectExplorer::Environment &env);

    int uniqueId() const;

    enum QmakeBuildConfig
    {
        NoBuild = 1,
        DebugBuild = 2,
        BuildAll = 8
    };

    QmakeBuildConfig defaultBuildConfig() const;
private:
    static int getUniqueId();
    void setName(const QString &name);
    void setPath(const QString &path);
    void updateSourcePath();
    void updateVersionInfo() const;
    void updateMkSpec() const;
    QString m_name;
    mutable bool m_versionInfoUpToDate;
    mutable bool m_mkspecUpToDate;
    QString m_path;
    QString m_sourcePath;
    mutable QString m_mkspec; // updated lazily
    mutable QString m_mkspecFullPath;
    QString m_mingwDirectory;
    QString m_prependPath;
    QString m_msvcVersion;
    mutable QHash<QString,QString> m_versionInfo; // updated lazily
    int m_id;
    bool m_isSystemVersion;
    mutable bool m_notInstalled;
    mutable bool m_defaultConfigIsDebug;
    mutable bool m_defaultConfigIsDebugAndRelease;
    mutable QString m_qmakeCommand;
    // This is updated on first call to qmakeCommand
    // That function is called from updateVersionInfo()
    mutable QString m_qtVersionString;
};


class QtDirWidget : public QWidget
{
    Q_OBJECT
public:
    QtDirWidget(QWidget *parent, QList<QtVersion *> versions, int defaultVersion);
    ~QtDirWidget();
    QList<QtVersion *> versions() const;
    int defaultVersion() const;
    void finish();
private:
    void showEnvironmentPage(QTreeWidgetItem * item);
    void fixQtVersionName(int index);
    Ui::QtVersionManager m_ui;
    QList<QtVersion *> m_versions;
    int m_defaultVersion;
    QString m_specifyNameString;
    QString m_specifyPathString;

private slots:
    void versionChanged(QTreeWidgetItem *item, QTreeWidgetItem *old);
    void addQtDir();
    void removeQtDir();
    void updateState();
    void makeMingwVisible(bool visible);
    void onQtBrowsed();
    void onMingwBrowsed();
    void defaultChanged(int index);
    void updateCurrentQtName();
    void updateCurrentQtPath();
    void updateCurrentMingwDirectory();
    void msvcVersionChanged();
};

class QtVersionManager : public Core::IOptionsPage
{
    Q_OBJECT

public:
    QtVersionManager();
    ~QtVersionManager();

    QString name() const;
    QString category() const;
    QString trCategory() const;

    QWidget *createPage(QWidget *parent);
    void apply();
    void finish() { }

    void writeVersionsIntoSettings();

    QList<QtVersion *> versions() const;

    QtVersion * version(int id) const;
    QtVersion * currentQtVersion() const;

    // internal
    int getUniqueId();

    QtVersion::QmakeBuildConfig scanMakefileForQmakeConfig(const QString &directory, QtVersion::QmakeBuildConfig defaultBuildConfig);
    QString findQtVersionFromMakefile(const QString &directory);
    QtVersion *qtVersionForDirectory(const QString &directory);

    // Used by the projectloadwizard
    void addVersion(QtVersion *version);
    
    // returns something like qmake4, qmake, qmake-qt4 or whatever distributions have chosen (used by QtVersion)
    static QStringList possibleQMakeCommands();
    // return true if the qmake at qmakePath is qt4 (used by QtVersion)
    static QString qtVersionForQMake(const QString &qmakePath);
signals:
    void defaultQtVersionChanged();
    void qtVersionsChanged();
private:

    void addNewVersionsFromInstaller();
    void updateSystemVersion();
    void updateDocumentation();
    QString findSystemQt() const;
    static int indexOfVersionInList(const QtVersion * const version, const QList<QtVersion *> &list);
    void updateUniqueIdToIndexMap();

    QPointer<QtDirWidget> m_widget;

    QtVersion *m_emptyVersion;
    int m_defaultVersion;
    QList<QtVersion *> m_versions;
    QMap<int, int> m_uniqueIdToIndex;
    int m_idcount;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // QTVERSIONMANAGER_H
