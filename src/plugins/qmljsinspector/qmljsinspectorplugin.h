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
#ifndef QMLJSINSPECTORPLUGIN_H
#define QMLJSINSPECTORPLUGIN_H

#include <extensionsystem/iplugin.h>
#include <qmljs/qmljsmodelmanagerinterface.h>

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QTimer>

QT_FORWARD_DECLARE_CLASS(QStringList)

namespace Core {
    class IMode;
}

namespace ProjectExplorer {
    class Project;
}

namespace QmlJSInspector {
namespace Internal {

class QmlInspectorToolbar;
class ClientProxy;
class Inspector;

class InspectorPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT

public:
    InspectorPlugin();
    virtual ~InspectorPlugin();

    static InspectorPlugin *instance();

    QmlJS::ModelManagerInterface *modelManager() const;
    ClientProxy *clientProxy() const;
    Inspector *inspector() const;

    // ExtensionSystem::IPlugin interface
    virtual bool initialize(const QStringList &arguments, QString *errorString);
    virtual void extensionsInitialized();
    virtual ExtensionSystem::IPlugin::ShutdownFlag aboutToShutdown();

public slots:
    void activateDebuggerForProject(ProjectExplorer::Project *project, const QString &runMode);
    void setDockWidgetArrangement(const QString &activeLanguage);

private slots:
    void prepareDebugger(Core::IMode *mode);

private:
    void createActions();

private:
    ClientProxy *_clientProxy;
    Inspector *_inspector;
    QmlInspectorToolbar *m_toolbar;
};

} // end of namespace Internal
} // end of QmlJSInspector

#endif // QMLINSPECTORPLUGIN_H
