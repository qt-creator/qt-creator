/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#ifndef QMLJSTOOLS_H
#define QMLJSTOOLS_H

#include <extensionsystem/iplugin.h>
#include <projectexplorer/projectexplorer.h>

#include <QTextDocument>
#include <QSharedPointer>

QT_BEGIN_NAMESPACE
class QFileInfo;
class QDir;
class QAction;
QT_END_NAMESPACE

namespace QmlJSTools {

class QmlJSToolsSettings;

namespace Internal {

class ModelManager;

class QmlJSToolsPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "QmlJSTools.json")

public:
    static QmlJSToolsPlugin *instance() { return m_instance; }

    QmlJSToolsPlugin();
    ~QmlJSToolsPlugin();

    bool initialize(const QStringList &arguments, QString *errorMessage);
    void extensionsInitialized();
    ShutdownFlag aboutToShutdown();
    ModelManager *modelManager() { return m_modelManager; }

private slots:
    void onTaskStarted(const QString &type);
    void onAllTasksFinished(const QString &type);

#ifdef WITH_TESTS
    void test_basic();
#endif

private:
    ModelManager *m_modelManager;
    QmlJSToolsSettings *m_settings;
    QAction *m_resetCodeModelAction;

    static QmlJSToolsPlugin *m_instance;
};

} // namespace Internal
} // namespace CppTools

#endif // QMLJSTOOLS_H
