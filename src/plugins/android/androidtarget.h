/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 BogDan Vatra <bog_dan_ro@yahoo.com>
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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

#ifndef ANDROIDTEMPLATESCREATOR_H
#define ANDROIDTEMPLATESCREATOR_H

#include "qt4projectmanager/qt4target.h"
#include "qt4projectmanager/qt4buildconfiguration.h"

#include <QMap>
#include <QIcon>
#include <QDomDocument>

QT_FORWARD_DECLARE_CLASS(QFileSystemWatcher);

namespace ProjectExplorer {
class Project;
class ProjectNode;
class Target;
}

namespace Qt4ProjectManager {
class Qt4Project;
class Qt4Target;
class Qt4ProFileNode;
}


namespace Android {
namespace Internal {
class AndroidTargetFactory;

class AndroidTarget : public Qt4ProjectManager::Qt4BaseTarget
{
    friend class AndroidTargetFactory;
    Q_OBJECT
    enum AndroidIconType
    {
        HighDPI,
        MediumDPI,
        LowDPI
    };

    struct Library
    {
        Library()
        {
            level = -1;
        }
        int level;
        QStringList dependencies;
        QString name;
    };
    typedef QMap<QString, Library> LibrariesMap;
public:
    enum BuildType
    {
        DebugBuild,
        ReleaseBuildUnsigned,
        ReleaseBuildSigned
    };

    explicit AndroidTarget(Qt4ProjectManager::Qt4Project *parent, const Core::Id id);
    virtual ~AndroidTarget();

    Qt4ProjectManager::Qt4BuildConfigurationFactory *buildConfigurationFactory() const;

    void createApplicationProFiles(bool reparse);

    QList<ProjectExplorer::RunConfiguration *> runConfigurationsForNode(ProjectExplorer::Node *n);

    static QString defaultDisplayName();


    QString packageName() const;
    bool setPackageName(const QString &name) const;

    QString intentName() const;
    QString activityName() const;

    QString applicationName() const;
    bool setApplicationName(const QString &name) const;

    QStringList availableTargetApplications() const;
    QString targetApplication() const;
    bool setTargetApplication(const QString &name) const;
    QString targetApplicationPath() const;

    QString targetSDK() const;
    bool setTargetSDK(const QString &target) const;

    int versionCode() const;
    bool setVersionCode(int version) const;

    QString versionName() const;
    bool setVersionName(const QString &version) const;

    QStringList permissions() const;
    bool setPermissions(const QStringList &permissions) const;

    QStringList availableQtLibs() const;
    QStringList qtLibs() const;
    bool setQtLibs(const QStringList &qtLibs) const;

    QStringList availablePrebundledLibs() const;
    QStringList prebundledLibs() const;
    bool setPrebundledLibs(const QStringList &qtLibs) const;

    QIcon highDpiIcon() const;
    bool setHighDpiIcon(const QString &iconFilePath) const;

    QIcon mediumDpiIcon() const;
    bool setMediumDpiIcon(const QString &iconFilePath) const;

    QIcon lowDpiIcon() const;
    bool setLowDpiIcon(const QString &iconFilePath) const;

    QString androidDirPath() const;
    QString androidManifestPath() const;
    QString androidLibsPath() const;
    QString androidStringsPath() const;
    QString androidDefaultPropertiesPath() const;
    QString androidSrcPath() const;
    QString apkPath(BuildType buildType) const;
    QString localLibsRulesFilePath() const;
    QString loadLocalLibs(int apiLevel) const;
    QString loadLocalJars(int apiLevel) const;

public slots:
    bool createAndroidTemplatesIfNecessary() const;
    void updateProject(const QString &targetSDK, const QString &name = QString()) const;

signals:
    void androidDirContentsChanged();

private slots:
    void handleTargetChanged(ProjectExplorer::Target *target);
    void handleTargetToBeRemoved(ProjectExplorer::Target *target);

private:
    enum ItemType
    {
        Lib,
        Jar
    };

    QString loadLocal(int apiLevel, ItemType item) const;
    void raiseError(const QString &reason) const;
    bool openXmlFile(QDomDocument &doc, const QString &fileName, bool createAndroidTemplates = true) const;
    bool saveXmlFile(QDomDocument &doc, const QString &fileName) const;
    bool openAndroidManifest(QDomDocument &doc) const;
    bool saveAndroidManifest(QDomDocument &doc) const;
    bool openLibsXml(QDomDocument &doc) const;
    bool saveLibsXml(QDomDocument &doc) const;

    QIcon androidIcon(AndroidIconType type) const;
    bool setAndroidIcon(AndroidIconType type, const QString &iconFileName) const;

    QStringList libsXml(const QString &tag) const;
    bool setLibsXml(const QStringList &qtLibs, const QString &tag) const;

    static bool qtLibrariesLessThan(const AndroidTarget::Library &a, const AndroidTarget::Library &b);
    QStringList getDependencies(const QString &readelfPath, const QString &lib) const;
    int setLibraryLevel(const QString &library, LibrariesMap &mapLibs) const;


    QFileSystemWatcher *const m_androidFilesWatcher;

    Qt4ProjectManager::Qt4BuildConfigurationFactory *m_buildConfigurationFactory;
};

} // namespace Internal
} // namespace Android

#endif // ANDROIDTEMPLATESCREATOR_H
