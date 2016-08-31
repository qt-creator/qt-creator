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

#include <QObject>
#include <QHash>
#include <QProcess>

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
    void scheduleMaybeRedumpBuiltins(const QmlJS::ModelManagerInterface::ProjectInfo &info);

private:
    Q_INVOKABLE void onLoadBuiltinTypes(const QmlJS::ModelManagerInterface::ProjectInfo &info,
                                        bool force = false);
    Q_INVOKABLE void onLoadPluginTypes(const QString &libraryPath, const QString &importPath,
                                       const QString &importUri, const QString &importVersion);
    Q_INVOKABLE void dumpBuiltins(const QmlJS::ModelManagerInterface::ProjectInfo &info);
    Q_INVOKABLE void dumpAllPlugins();
    void qmlPluginTypeDumpDone(int exitCode);
    void qmlPluginTypeDumpError(QProcess::ProcessError error);
    void pluginChanged(const QString &pluginLibrary);

private:
    class Plugin {
    public:
        QString qmldirPath;
        QString importPath;
        QString importUri;
        QString importVersion;
        QStringList typeInfoPaths;
    };

    void runQmlDump(const QmlJS::ModelManagerInterface::ProjectInfo &info, const QStringList &arguments, const QString &importPath);
    void dump(const Plugin &plugin);
    void loadQmlTypeDescription(const QStringList &path, QStringList &errors, QStringList &warnings,
                                QList<LanguageUtils::FakeMetaObject::ConstPtr> &objects,
                                QList<ModuleApiInfo> *moduleApi,
                                QStringList *dependencies) const;
    QString buildQmltypesPath(const QString &name) const;
    void loadDependencies(const QStringList &dependencies,
                          QStringList &errors,
                          QStringList &warnings,
                          QList<LanguageUtils::FakeMetaObject::ConstPtr> &objects,
                          QSet<QString> *visited=0) const;
    void loadQmltypesFile(const QStringList &qmltypesFilePaths,
                          const QString &libraryPath,
                          QmlJS::LibraryInfo libraryInfo);
    QString resolvePlugin(const QDir &qmldirPath, const QString &qmldirPluginPath,
                          const QString &baseName);
    QString resolvePlugin(const QDir &qmldirPath, const QString &qmldirPluginPath,
                          const QString &baseName, const QStringList &suffixes,
                          const QString &prefix = QString());

private:
    Utils::FileSystemWatcher *pluginWatcher();

    ModelManagerInterface *m_modelManager;
    Utils::FileSystemWatcher *m_pluginWatcher;
    QHash<QProcess *, QString> m_runningQmldumps;
    QList<Plugin> m_plugins;
    QHash<QString, int> m_libraryToPluginIndex;
    QHash<QString, QmlJS::ModelManagerInterface::ProjectInfo> m_qtToInfo;
};

} // namespace QmlJS
