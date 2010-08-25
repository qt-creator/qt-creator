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

#ifndef QMLSTANDALONEAPP_H
#define QMLSTANDALONEAPP_H

#include <QtCore/QStringList>
#include <QtCore/QFileInfo>
#include <QtCore/QHash>

#ifndef CREATORLESSTEST
#include <coreplugin/basefilewizard.h>
#endif // CREATORLESSTEST

namespace Qt4ProjectManager {
namespace Internal {

class QmlStandaloneApp;

struct QmlModule
{
    enum Path {
        // Example: Module "com.foo.bar" in "c:/modules/".
        // "qmldir" file is in "c:/modules/com/foo/bar/".
        // Application .pro file is "c:/app/app.pro".
        Root,                   // "c:/modules/" (absolute)
        ContentDir,             // "../modules/com/foo/bar" (relative form .pro file)
        ContentBase,            // "com/foo/"
        DeployedContentBase     // "<qmlmodules>/com/foo" (on deploy target)
    };

    QmlModule(const QString &name, const QFileInfo &rootDir, const QFileInfo &qmldir,
              bool isExternal, QmlStandaloneApp *qmlStandaloneApp);
    QString path(Path path) const;
    const QString uri;              // "com.foo.bar"
    const QFileInfo rootDir;        // Location of "com/"
    const QFileInfo qmldir;         // 'qmldir' file.
    const bool isExternal;          // Either external or inside a source paths
    const QmlStandaloneApp *qmlStandaloneApp;
    QHash<QString, struct QmlCppPlugin*> cppPlugins;   // Just as info. No ownership.
};

struct QmlCppPlugin
{
    QmlCppPlugin(const QString &name, const QFileInfo &path,
                 const QmlModule *module, const QFileInfo &proFile);
    const QString name;             // Original name
    const QFileInfo path;           // Plugin path where qmldir points to
    const QmlModule *module;
    const QFileInfo proFile;        // .pro file for the plugin
};

struct GeneratedFileInfo
{
    enum File {
        MainQmlFile,
        MainCppFile,
        AppProFile,
        AppViewerPriFile,
        AppViewerCppFile,
        AppViewerHFile,
        SymbianSvgIconFile
    };

    enum UpdateReason {
        Undefined,
        IsUpToDate,
        HasOutdatedVersion,
        HasFutureVersion,
        ContentChanged
    };

    GeneratedFileInfo();

    File file;
    QFileInfo fileInfo;
    int version;
    quint16 dataChecksum;
    quint16 statedChecksum;
    UpdateReason updateReason;
};

class QmlStandaloneApp: public QObject
{
public:
    enum Orientation {
        LockLandscape,
        LockPortrait,
        Auto
    };

    enum Path {
        MainQml,
        MainQmlDeployed,
        MainQmlOrigin,
        MainCpp,
        MainCppOrigin,
        AppPro,
        AppProOrigin,
        AppProPath,
        AppViewerPri,
        AppViewerPriOrigin,
        AppViewerCpp,
        AppViewerCppOrigin,
        AppViewerH,
        AppViewerHOrigin,
        SymbianSvgIcon,
        SymbianSvgIconOrigin,
        QmlDir,
        QmlDirProFileRelative,
        ModulesDir
    };

    QmlStandaloneApp();
    ~QmlStandaloneApp();

    void setMainQmlFile(const QString &qmlFile);
    QString mainQmlFile() const;
    void setOrientation(Orientation orientation);
    Orientation orientation() const;
    void setProjectName(const QString &name);
    QString projectName() const;
    void setProjectPath(const QString &path);
    void setSymbianSvgIcon(const QString &icon);
    QString symbianSvgIcon() const;
    void setSymbianTargetUid(const QString &uid);
    QString symbianTargetUid() const;
    void setLoadDummyData(bool loadIt);
    bool loadDummyData() const;
    void setNetworkEnabled(bool enabled);
    bool networkEnabled() const;
    bool setExternalModules(const QStringList &uris, const QStringList &importPaths);

    static QString symbianUidForPath(const QString &path);
#ifndef CREATORLESSTEST
    Core::GeneratedFiles generateFiles(QString *errorMessage) const;
#else
    bool generateFiles(QString *errorMessage) const;
#endif // CREATORLESSTEST
    QString path(Path path) const;
    bool useExistingMainQml() const;
    QString error() const;
    const QList<QmlModule*> modules() const;
    QByteArray generateFile(GeneratedFileInfo::File file, const QString *errorMessage) const;
    static int stubVersion();
    static QList<GeneratedFileInfo> fileUpdates(const QString &mainProFile);

private:
    QByteArray generateMainCpp(const QString *errorMessage) const;
    QByteArray generateProFile(const QString *errorMessage) const;
    static QString templatesRoot();
    bool addExternalModule(const QString &uri, const QFileInfo &dir,
                           const QFileInfo &contentDir);
    bool addCppPlugins(QmlModule *module);
    bool addCppPlugin(const QString &qmldirLine, QmlModule *module);
    void clearModulesAndPlugins();

    QString m_projectName;
    QFileInfo m_projectPath;
    QString m_symbianSvgIcon;
    QString m_symbianTargetUid;
    bool m_loadDummyData;
    Orientation m_orientation;
    bool m_networkEnabled;
    QFileInfo m_mainQmlFile;
    QStringList m_importPaths;
    QList <QmlModule*> m_modules;
    QList <QmlCppPlugin*> m_cppPlugins;
    QString m_error;
};

} // end of namespace Internal
} // end of namespace Qt4ProjectManager

#endif // QMLSTANDALONEAPP_H
