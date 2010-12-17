/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef QMLSTANDALONEAPP_H
#define QMLSTANDALONEAPP_H

#include "abstractmobileapp.h"

#include <QtCore/QHash>
#include <QtCore/QStringList>

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

struct QmlAppGeneratedFileInfo : public AbstractGeneratedFileInfo
{
    enum ExtendedFileType {
        MainQmlFile = ExtendedFile,
        AppViewerPriFile,
        AppViewerCppFile,
        AppViewerHFile
    };

    QmlAppGeneratedFileInfo() : AbstractGeneratedFileInfo() {}
    virtual bool isOutdated() const;
};

class QmlStandaloneApp : public AbstractMobileApp
{
public:
    enum ExtendedFileType {
        MainQml = ExtendedFile,
        MainQmlDeployed,
        MainQmlOrigin,
        AppViewerPri,
        AppViewerPriOrigin,
        AppViewerCpp,
        AppViewerCppOrigin,
        AppViewerH,
        AppViewerHOrigin,
        QmlDir,
        QmlDirProFileRelative,
        ModulesDir
    };

    QmlStandaloneApp();
    virtual ~QmlStandaloneApp();

    void setMainQmlFile(const QString &qmlFile);
    QString mainQmlFile() const;
    bool setExternalModules(const QStringList &uris, const QStringList &importPaths);

#ifndef CREATORLESSTEST
    virtual Core::GeneratedFiles generateFiles(QString *errorMessage) const;
#else
    bool generateFiles(QString *errorMessage) const;
#endif // CREATORLESSTEST
    bool useExistingMainQml() const;
    const QList<QmlModule*> modules() const;
    static QList<QmlAppGeneratedFileInfo> fileUpdates(const QString &mainProFile);
    static bool updateFiles(const QList<QmlAppGeneratedFileInfo> &list, QString &error);

    static const int StubVersion;

private:
    virtual QByteArray generateFileExtended(int fileType,
        bool *versionAndCheckSum, QString *comment, QString *errorMessage) const;
    virtual QString pathExtended(int fileType) const;
    virtual QString originsRoot() const;
    virtual QString mainWindowClassName() const;
    virtual int stubVersionMinor() const;
    virtual bool adaptCurrentMainCppTemplateLine(QString &line) const;
    virtual void handleCurrentProFileTemplateLine(const QString &line,
        QTextStream &proFileTemplate, QTextStream &proFile,
        bool &uncommentNextLine) const;

    bool addExternalModule(const QString &uri, const QFileInfo &dir,
                           const QFileInfo &contentDir);
    bool addCppPlugins(QmlModule *module);
    bool addCppPlugin(const QString &qmldirLine, QmlModule *module);
    void clearModulesAndPlugins();

    QFileInfo m_mainQmlFile;
    QStringList m_importPaths;
    QList <QmlModule*> m_modules;
    QList <QmlCppPlugin*> m_cppPlugins;
};

} // end of namespace Internal
} // end of namespace Qt4ProjectManager

#endif // QMLSTANDALONEAPP_H
