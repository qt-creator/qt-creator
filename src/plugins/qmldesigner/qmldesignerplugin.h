/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QMLDESIGNERPLUGIN_H
#define QMLDESIGNERPLUGIN_H

#include <qmldesigner/designersettings.h>
#include <qmldesigner/components/pluginmanager/pluginmanager.h>

#include <extensionsystem/iplugin.h>

#include "documentmanager.h"
#include "viewmanager.h"
#include "shortcutmanager.h"

#include <QWeakPointer>
#include <QStringList>

QT_BEGIN_NAMESPACE
class QAction;
QT_END_NAMESPACE

namespace Core {
    class Context;
    class IContext;
    class IEditor;
    class DesignMode;
    class EditorManager;
}

namespace QmlDesigner {

namespace Internal {
class DesignModeWidget;
class DesignModeContext;
}

class QmlDesignerPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "QmlDesigner.json")

public:
    QmlDesignerPlugin();
    virtual ~QmlDesignerPlugin();

    //Plugin
    virtual bool initialize(const QStringList &arguments, QString *errorMessage = 0);
    virtual void extensionsInitialized();

    static QmlDesignerPlugin *instance();

    DocumentManager &documentManager();
    const DocumentManager &documentManager() const;

    ViewManager &viewManager();
    const ViewManager &viewManager() const;

    DesignerSettings settings();
    void setSettings(const DesignerSettings &s);

    DesignDocument *currentDesignDocument() const;
    Internal::DesignModeWidget *mainWidget() const;

private slots:
    void switchTextDesign();
    void onTextEditorsClosed(QList<Core::IEditor *> editors);
    void onCurrentEditorChanged(Core::IEditor *editor);
    void onCurrentModeChanged(Core::IMode *mode, Core::IMode *oldMode);

private: // functions
    void createDesignModeWidget();
    void showDesigner();
    void hideDesigner();
    void changeEditor();
    void jumpTextCursorToSelectedModelNode();
    void selectModelNodeUnderTextCursor();
    void activateAutoSynchronization();
    void deactivateAutoSynchronization();
    void resetModelSelection();

private: // variables
    ViewManager m_viewManager;
    DocumentManager m_documentManager;
    ShortCutManager m_shortCutManager;

    Internal::DesignModeWidget *m_mainWidget;

    QmlDesigner::PluginManager m_pluginManager;
    static QmlDesignerPlugin *m_instance;
    DesignerSettings m_settings;
    Internal::DesignModeContext *m_context;
    bool m_isActive;
};

} // namespace QmlDesigner

#endif // QMLDESIGNERPLUGIN_H
