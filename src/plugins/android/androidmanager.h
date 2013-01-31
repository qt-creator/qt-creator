/**************************************************************************
**
** Copyright (c) 2013 BogDan Vatra <bog_dan_ro@yahoo.com>
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

#ifndef ANDROIDMANAGER_H
#define ANDROIDMANAGER_H

#include <utils/fileutils.h>

#include <QDomDocument>
#include <QObject>
#include <QStringList>

namespace ProjectExplorer { class Target; }

namespace Android {
namespace Internal {

class AndroidManager : public QObject
{
    Q_OBJECT

public:
    enum BuildType
    {
        DebugBuild,
        ReleaseBuildUnsigned,
        ReleaseBuildSigned
    };

    static bool supportsAndroid(ProjectExplorer::Target *target);

    static QString packageName(ProjectExplorer::Target *target);
    static bool setPackageName(ProjectExplorer::Target *target, const QString &name);

    static QString applicationName(ProjectExplorer::Target *target);
    static bool setApplicationName(ProjectExplorer::Target *target, const QString &name);

    static QStringList permissions(ProjectExplorer::Target *target);
    static bool setPermissions(ProjectExplorer::Target *target, const QStringList &permissions);

    static QString intentName(ProjectExplorer::Target *target);
    static QString activityName(ProjectExplorer::Target *target);

    static int versionCode(ProjectExplorer::Target *target);
    static bool setVersionCode(ProjectExplorer::Target *target, int version);
    static QString versionName(ProjectExplorer::Target *target);
    static bool setVersionName(ProjectExplorer::Target *target, const QString &version);

    static QIcon highDpiIcon(ProjectExplorer::Target *target);
    static bool setHighDpiIcon(ProjectExplorer::Target *target, const QString &iconFilePath);
    static QIcon mediumDpiIcon(ProjectExplorer::Target *target);
    static bool setMediumDpiIcon(ProjectExplorer::Target *target, const QString &iconFilePath);
    static QIcon lowDpiIcon(ProjectExplorer::Target *target);
    static bool setLowDpiIcon(ProjectExplorer::Target *target, const QString &iconFilePath);

    static QStringList availableTargetApplications(ProjectExplorer::Target *target);
    static QString targetApplication(ProjectExplorer::Target *target);
    static bool setTargetApplication(ProjectExplorer::Target *target, const QString &name);
    static QString targetApplicationPath(ProjectExplorer::Target *target);

    static QString targetSDK(ProjectExplorer::Target *target);
    static bool setTargetSDK(ProjectExplorer::Target *target, const QString &sdk);

    static Utils::FileName dirPath(ProjectExplorer::Target *target);
    static Utils::FileName manifestPath(ProjectExplorer::Target *target);
    static Utils::FileName libsPath(ProjectExplorer::Target *target);
    static Utils::FileName stringsPath(ProjectExplorer::Target *target);
    static Utils::FileName defaultPropertiesPath(ProjectExplorer::Target *target);
    static Utils::FileName srcPath(ProjectExplorer::Target *target);
    static Utils::FileName apkPath(ProjectExplorer::Target *target, BuildType buildType);

    static bool createAndroidTemplatesIfNecessary(ProjectExplorer::Target *target);
    static void updateTarget(ProjectExplorer::Target *target, const QString &targetSDK,
                             const QString &name = QString());

    static Utils::FileName localLibsRulesFilePath(ProjectExplorer::Target *target);
    static QString loadLocalLibs(ProjectExplorer::Target *target, int apiLevel);
    static QString loadLocalJars(ProjectExplorer::Target *target, int apiLevel);
    static QString loadLocalJarsInitClasses(ProjectExplorer::Target *target, int apiLevel);

    static QStringList availableQtLibs(ProjectExplorer::Target *target);
    static QStringList qtLibs(ProjectExplorer::Target *target);
    static bool setQtLibs(ProjectExplorer::Target *target, const QStringList &libs);

    static QStringList availablePrebundledLibs(ProjectExplorer::Target *target);
    static QStringList prebundledLibs(ProjectExplorer::Target *target);
    static bool setPrebundledLibs(ProjectExplorer::Target *target, const QStringList &libs);

private:
    static void raiseError(const QString &reason);
    static bool openXmlFile(QDomDocument &doc, const Utils::FileName &fileName);
    static bool saveXmlFile(ProjectExplorer::Target *target, QDomDocument &doc, const Utils::FileName &fileName);
    static bool openManifest(ProjectExplorer::Target *target, QDomDocument &doc);
    static bool saveManifest(ProjectExplorer::Target *target, QDomDocument &doc);
    static bool openLibsXml(ProjectExplorer::Target *target, QDomDocument &doc);
    static bool saveLibsXml(ProjectExplorer::Target *target, QDomDocument &doc);
    static QStringList libsXml(ProjectExplorer::Target *target, const QString &tag);
    static bool setLibsXml(ProjectExplorer::Target *target, const QStringList &libs, const QString &tag);

    enum ItemType
    {
        Lib,
        Jar
    };
    static QString loadLocal(ProjectExplorer::Target *target, int apiLevel, ItemType item, const QString &attribute=QLatin1String("file"));

    class Library
    {
    public:
        Library()
        { level = -1; }
        int level;
        QStringList dependencies;
        QString name;
    };
    typedef QMap<QString, Library> LibrariesMap;

    enum IconType
    {
        HighDPI,
        MediumDPI,
        LowDPI
    };
    static QString iconPath(ProjectExplorer::Target *target, IconType type);
    static QIcon icon(ProjectExplorer::Target *target, IconType type);
    static bool setIcon(ProjectExplorer::Target *target, IconType type, const QString &iconFileName);

    static QStringList dependencies(const Utils::FileName &readelfPath, const QString &lib);
    static int setLibraryLevel(const QString &library, LibrariesMap &mapLibs);
    static bool qtLibrariesLessThan(const Library &a, const Library &b);
};

} // namespace Internal
} // namespace Android

#endif // ANDROIDMANAGER_H
