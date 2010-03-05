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
#ifndef QMLINSPECTORPLUGIN_H
#define QMLINSPECTORPLUGIN_H

#include <extensionsystem/iplugin.h>

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QTimer>

QT_FORWARD_DECLARE_CLASS(QStringList);

namespace Core {
    class IMode;
}

namespace ProjectExplorer {
    class Project;
}

namespace Qml {
    class QmlInspector;

const int MaxConnectionAttempts = 20;

class QmlInspectorPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT

public:
    QmlInspectorPlugin();
    ~QmlInspectorPlugin();

    virtual bool initialize(const QStringList &arguments, QString *errorString);
    virtual void extensionsInitialized();
    virtual void shutdown();

public slots:
    void activateDebugger(const QString &langName);
    void activateDebuggerForProject(ProjectExplorer::Project *project);
    void setDockWidgetArrangement(const QString &activeLanguage);

private slots:
    void pollInspector();
    void prepareDebugger(Core::IMode *mode);

private:
    QmlInspector *m_inspector;
    QTimer *m_connectionTimer;
    int m_connectionAttempts;
};

}

#endif // QMLINSPECTORPLUGIN_H
