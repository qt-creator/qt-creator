/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QMLJSPLUGINDUMPER_H
#define QMLJSPLUGINDUMPER_H

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

private slots:
    void onLoadBuiltinTypes(const QmlJS::ModelManagerInterface::ProjectInfo &info,
                            bool force = false);
    void onLoadPluginTypes(const QString &libraryPath, const QString &importPath,
                           const QString &importUri, const QString &importVersion);
    void dumpBuiltins(const QmlJS::ModelManagerInterface::ProjectInfo &info);
    void dumpAllPlugins();
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

    void dump(const Plugin &plugin);
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

#endif // QMLJSPLUGINDUMPER_H
