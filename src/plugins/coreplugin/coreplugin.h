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

#ifndef COREPLUGIN_H
#define COREPLUGIN_H

#include <extensionsystem/iplugin.h>

namespace Utils { class Theme; }

namespace Core {

class DesignMode;
class FindPlugin;

namespace Internal {

class EditMode;
class MainWindow;
class Locator;

class CorePlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Core.json")

public:
    CorePlugin();
    ~CorePlugin();

    bool initialize(const QStringList &arguments, QString *errorMessage = 0);
    void extensionsInitialized();
    bool delayedInitialize();
    ShutdownFlag aboutToShutdown();
    QObject *remoteCommand(const QStringList & /* options */,
                           const QString &workingDirectory,
                           const QStringList &args);

public slots:
    void fileOpenRequest(const QString&);

private slots:
#if defined(WITH_TESTS)
    void testVcsManager_data();
    void testVcsManager();
    // Locator:
    void test_basefilefilter();
    void test_basefilefilter_data();
#endif

private:
    void parseArguments(const QStringList & arguments);

    MainWindow *m_mainWindow;
    EditMode *m_editMode;
    DesignMode *m_designMode;
    FindPlugin *m_findPlugin;
    Locator *m_locator;
};

} // namespace Internal
} // namespace Core

#endif // COREPLUGIN_H
