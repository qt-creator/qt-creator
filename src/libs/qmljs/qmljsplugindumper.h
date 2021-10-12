/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <qmljs/qmljsmodelmanagerinterface.h>

#include <utils/qtcprocess.h>

#include <QObject>
#include <QHash>

QT_BEGIN_NAMESPACE
class QDir;
QT_END_NAMESPACE

namespace Utils { class FileSystemWatcher; }

namespace QmlJS {

class PluginDumper : public QObject
{
    Q_OBJECT
public:
    explicit PluginDumper(ModelManagerInterface *modelManager);

public:
    void loadBuiltinTypes(const QmlJS::ModelManagerInterface::ProjectInfo &info);
    void loadPluginTypes(const QString &libraryPath, const QString &importPath,
                         const QString &importUri, const QString &importVersion);
    void scheduleRedumpPlugins();

private:
    Q_INVOKABLE void onLoadBuiltinTypes(const QmlJS::ModelManagerInterface::ProjectInfo &info,
                                        bool force = false);
    Q_INVOKABLE void onLoadPluginTypes(const QString &libraryPath, const QString &importPath,
                                       const QString &importUri, const QString &importVersion);
    Q_INVOKABLE void dumpAllPlugins();
    void qmlPluginTypeDumpDone(Utils::QtcProcess *process);
    void qmlPluginTypeDumpError(Utils::QtcProcess *process);
    void pluginChanged(const QString &pluginLibrary);

private:
    class Plugin {
    public:
        Utils::FilePath qmldirPath;
        QString importPath;
        QString importUri;
        QString importVersion;
        QStringList typeInfoPaths;
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
    QFuture<QmlTypeDescription> loadQmlTypeDescription(const QStringList &path) const;
    QString buildQmltypesPath(const QString &name) const;

    QFuture<PluginDumper::DependencyInfo> loadDependencies(const QStringList &dependencies,
                                                           QSharedPointer<QSet<QString>> visited) const;

    void loadQmltypesFile(const QStringList &qmltypesFilePaths,
                          const Utils::FilePath &libraryPath,
                          QmlJS::LibraryInfo libraryInfo);
    QString resolvePlugin(const QDir &qmldirPath, const QString &qmldirPluginPath,
                          const QString &baseName);
    QString resolvePlugin(const QDir &qmldirPath, const QString &qmldirPluginPath,
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
    QHash<Utils::QtcProcess *, Utils::FilePath> m_runningQmldumps;
    QList<Plugin> m_plugins;
    QHash<QString, int> m_libraryToPluginIndex;
    QHash<QString, QmlJS::ModelManagerInterface::ProjectInfo> m_qtToInfo;
};

} // namespace QmlJS
