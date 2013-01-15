/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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

#include <extensionsystem/iplugin.h>

#include <extensionsystem/pluginmanager.h>

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

class BauhausPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "QmlDesigner.json")

public:
    BauhausPlugin();
    virtual ~BauhausPlugin();

    //Plugin
    virtual bool initialize(const QStringList &arguments, QString *errorMessage = 0);
    virtual void extensionsInitialized();

    static BauhausPlugin *pluginInstance();

    DesignerSettings settings();
    void setSettings(const DesignerSettings &s);

private slots:

    void switchTextDesign();
    void textEditorsClosed(QList<Core::IEditor *> editors);
    void updateActions(Core::IEditor* editor);
    void updateEditor(Core::IEditor *editor);
    void contextChanged(Core::IContext *context, const Core::Context &additionalContexts);

private:
    void createDesignModeWidget();

    QStringList m_mimeTypes;
    DesignModeWidget *m_mainWidget;

    ExtensionSystem::PluginManager m_pluginManager;
    static BauhausPlugin *m_pluginInstance;
    DesignerSettings m_settings;
    DesignModeContext *m_context;
    Core::DesignMode *m_designMode;
    Core::EditorManager *m_editorManager;
    bool m_isActive;

    QAction *m_revertToSavedAction;
    QAction *m_saveAction;
    QAction *m_saveAsAction;
    QAction *m_closeCurrentEditorAction;
    QAction *m_closeAllEditorsAction;
    QAction *m_closeOtherEditorsAction;
};

} // namespace Internal
} // namespace QmlDesigner

#endif // QMLDESIGNERPLUGIN_H
