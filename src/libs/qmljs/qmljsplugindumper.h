// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmljs/qmljsmodelmanagerinterface.h>

#include <QObject>
#include <QHash>

QT_BEGIN_NAMESPACE
class QDir;
QT_END_NAMESPACE

namespace Utils {
class FileSystemWatcher;
class Process;
}

namespace QmlJS {

class PluginDumper : public QObject
{
    Q_OBJECT
public:
    explicit PluginDumper(ModelManagerInterface *modelManager);

public:
    void loadBuiltinTypes(const QmlJS::ModelManagerInterface::ProjectInfo &info);
    void loadPluginTypes(const Utils::FilePath &libraryPath,
                         const Utils::FilePath &importPath,
                         const QString &importUri,
                         const QString &importVersion);
    void scheduleRedumpPlugins();

private:
    Q_INVOKABLE void onLoadBuiltinTypes(const QmlJS::ModelManagerInterface::ProjectInfo &info,
                                        bool force = false);
    Q_INVOKABLE void onLoadPluginTypes(const Utils::FilePath &libraryPath,
                                       const Utils::FilePath &importPath,
                                       const QString &importUri,
                                       const QString &importVersion);
    Q_INVOKABLE void dumpAllPlugins();
    void qmlPluginTypeDumpDone(Utils::Process *process);
    void pluginChanged(const QString &pluginLibrary);

private:
    class Plugin {
    public:
        Utils::FilePath qmldirPath;
        Utils::FilePath importPath;
        QString importUri;
        QString importVersion;
        Utils::FilePaths typeInfoPaths;
    };

    class QmlTypeDescription {
    public:
        QStringList errors;
        QStringList warnings;
        QList<LanguageUtils::FakeMetaObject::ConstPtr> objects;
        QList<ModuleApiInfo> moduleApis;
        QStringList dependencies;
    };

    class DependencyInfo {
    public:
        QStringList errors;
        QStringList warnings;
        QList<LanguageUtils::FakeMetaObject::ConstPtr> objects;
    };

    void runQmlDump(const QmlJS::ModelManagerInterface::ProjectInfo &info, const QStringList &arguments,
                    const Utils::FilePath &importPath);
    void dump(const Plugin &plugin);
    QFuture<QmlTypeDescription> loadQmlTypeDescription(const Utils::FilePaths &path) const;
    Utils::FilePath buildQmltypesPath(const QString &name) const;

    QFuture<PluginDumper::DependencyInfo> loadDependencies(const Utils::FilePaths &dependencies,
                                                           QSharedPointer<QSet<Utils::FilePath> > visited) const;

    void loadQmltypesFile(const Utils::FilePaths &qmltypesFilePaths,
                          const Utils::FilePath &libraryPath,
                          QmlJS::LibraryInfo libraryInfo);
    Utils::FilePath resolvePlugin(const Utils::FilePath &qmldirPath,
                                  const QString &qmldirPluginPath,
                                  const QString &baseName);
    Utils::FilePath resolvePlugin(const Utils::FilePath &qmldirPath,
                                  const QString &qmldirPluginPath,
                                  const QString &baseName, const QStringList &suffixes,
                                  const QString &prefix = QString());

private:
    Utils::FileSystemWatcher *pluginWatcher();
    void prepareLibraryInfo(LibraryInfo &libInfo,
                            const Utils::FilePath &libraryPath,
                            const QStringList &deps,
                            const QStringList &errors,
                            const QStringList &warnings,
                            const QList<ModuleApiInfo> &moduleApis,
                            QList<LanguageUtils::FakeMetaObject::ConstPtr> &objects);

    ModelManagerInterface *m_modelManager;
    Utils::FileSystemWatcher *m_pluginWatcher;
    QHash<Utils::Process *, Utils::FilePath> m_runningQmldumps;
    QList<Plugin> m_plugins;
    QHash<Utils::FilePath, int> m_libraryToPluginIndex;
    QHash<QString, QmlJS::ModelManagerInterface::ProjectInfo> m_qtToInfo;
};

} // namespace QmlJS
