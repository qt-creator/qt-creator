/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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

#ifndef QMLJSPLUGINDUMPER_H
#define QMLJSPLUGINDUMPER_H

#include <QtCore/QObject>
#include <QtCore/QHash>
#include <QtCore/QProcess>

QT_BEGIN_NAMESPACE
class QDir;
QT_END_NAMESPACE

namespace ProjectExplorer {
class FileWatcher;
}

namespace QmlJSTools {
namespace Internal {

class ModelManager;

class PluginDumper : public QObject
{
    Q_OBJECT
public:
    explicit PluginDumper(ModelManager *modelManager);

public:
    void loadPluginTypes(const QString &libraryPath, const QString &importPath,
                         const QString &importUri, const QString &importVersion);
    void scheduleCompleteRedump();

private slots:
    void onLoadPluginTypes(const QString &libraryPath, const QString &importPath,
                           const QString &importUri, const QString &importVersion);
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

        bool hasPredumpedQmlTypesFile() const;
        QString predumpedQmlTypesFilePath() const;
    };

    void dump(const Plugin &plugin);
    QString resolvePlugin(const QDir &qmldirPath, const QString &qmldirPluginPath,
                          const QString &baseName);
    QString resolvePlugin(const QDir &qmldirPath, const QString &qmldirPluginPath,
                          const QString &baseName, const QStringList &suffixes,
                          const QString &prefix = QString());

private:
    ModelManager *m_modelManager;
    ProjectExplorer::FileWatcher *m_pluginWatcher;
    QHash<QProcess *, QString> m_runningQmldumps;
    QList<Plugin> m_plugins;
    QHash<QString, int> m_libraryToPluginIndex;
};

} // namespace Internal
} // namespace QmlJSTools

#endif // QMLJSPLUGINDUMPER_H
