/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QTVERSIONMANAGER_H
#define QTVERSIONMANAGER_H

#include "qtsupport_global.h"
#include "baseqtversion.h"

#include <QSet>
#include <QStringList>

namespace Utils {
class FileSystemWatcher;
class PersistentSettingsWriter;
} // namespace Utils

namespace ProjectExplorer { class KitInformation; }

namespace QtSupport {
namespace Internal {
class QtOptionsPage;
}

struct QMakeAssignment
{
    QString variable;
    QString op;
    QString value;
};

class QTSUPPORT_EXPORT QtVersionManager : public QObject
{
    Q_OBJECT
    // for getUniqueId();
    friend class BaseQtVersion;
    friend class Internal::QtOptionsPageWidget;
public:
    static QtVersionManager *instance();
    QtVersionManager();
    ~QtVersionManager();
    void extensionsInitialized();
    bool delayedInitialize();

    // This will *always* return at least one (Qt in Path), even if that is
    // unconfigured.
    QList<BaseQtVersion *> versions() const;
    QList<BaseQtVersion *> validVersions() const;

    // Note: DO NOT STORE THIS POINTER!
    //       The QtVersionManager will delete it at random times and you will
    //       need to get a new pointer by calling this method again!
    BaseQtVersion *version(int id) const;

    BaseQtVersion *qtVersionForQMakeBinary(const Utils::FileName &qmakePath);

    void addVersion(BaseQtVersion *version);
    void removeVersion(BaseQtVersion *version);

    // Static Methods
    enum MakefileCompatible { CouldNotParse, DifferentProject, SameProject };
    static MakefileCompatible makefileIsFor(const QString &makefile, const QString &proFile);
    static QPair<BaseQtVersion::QmakeBuildConfigs, QString> scanMakeFile(const QString &makefile,
                                                                     BaseQtVersion::QmakeBuildConfigs defaultBuildConfig);
    static Utils::FileName findQMakeBinaryFromMakefile(const QString &directory);
    bool isValidId(int id) const;

    Core::FeatureSet availableFeatures(const QString &platformName) const;
    QStringList availablePlatforms() const;
    QString displayNameForPlatform(const QString &string) const;

signals:
    // content of BaseQtVersion objects with qmake path might have changed
    void dumpUpdatedFor(const Utils::FileName &qmakeCommand);
    void qtVersionsChanged(const QList<int> &addedIds, const QList<int> &removedIds, const QList<int> &changedIds);

public slots:
    void updateDumpFor(const Utils::FileName &qmakeCommand);

private slots:
    void updateFromInstaller(bool emitSignal = true);

private:
    // This function is really simplistic...
    static bool equals(BaseQtVersion *a, BaseQtVersion *b);
    static QString findQMakeLine(const QString &directory, const QString &key);
    static QString trimLine(const QString line);
    static void parseArgs(const QString &args,
                          QList<QMakeAssignment> *assignments,
                          QList<QMakeAssignment> *afterAssignments,
                          QString *additionalArguments);
    static BaseQtVersion::QmakeBuildConfigs qmakeBuildConfigFromCmdArgs(QList<QMakeAssignment> *assignments,
                                                                    BaseQtVersion::QmakeBuildConfigs defaultBuildConfig);
    bool restoreQtVersions();
    void findSystemQt();
    void saveQtVersions();
    void updateDocumentation();
    // Used by QtOptionsPage
    void setNewQtVersions(QList<BaseQtVersion *> newVersions);
    // Used by QtVersion
    int getUniqueId();
    void addNewVersionsFromInstaller();

    static int indexOfVersionInList(const BaseQtVersion * const version, const QList<BaseQtVersion *> &list);
    void updateUniqueIdToIndexMap();

    QMap<int, BaseQtVersion *> m_versions;
    int m_idcount;
    // managed by QtProjectManagerPlugin
    static QtVersionManager *m_self;

    Utils::FileSystemWatcher *m_configFileWatcher;
    QTimer *m_fileWatcherTimer;
    Utils::PersistentSettingsWriter *m_writer;
};

namespace Internal {

class QTSUPPORT_EXPORT QtFeatureProvider : public Core::IFeatureProvider
{
    Q_OBJECT

public:
    QtFeatureProvider() {}
    Core::FeatureSet availableFeatures(const QString &platformName) const;
    QStringList availablePlatforms() const;
    QString displayNameForPlatform(const QString &string) const;
};

}

} // namespace QtSupport

#endif // QTVERSIONMANAGER_H
