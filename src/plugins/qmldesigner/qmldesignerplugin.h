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
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
****************************************************************************/

#ifndef QMLDESIGNERPLUGIN_H
#define QMLDESIGNERPLUGIN_H

#include <qmldesigner/designersettings.h>
#include <qmldesigner/components/pluginmanager/pluginmanager.h>
#include <qmldesignercorelib_global.h>

#include <extensionsystem/iplugin.h>

#include "documentmanager.h"
#include "viewmanager.h"
#include "shortcutmanager.h"
#include <designeractionmanager.h>

#include <QStringList>

QT_BEGIN_NAMESPACE
class QAction;
QT_END_NAMESPACE

namespace Core {
    class IEditor;
    class IMode;
}

namespace QmlDesigner {

class QmlDesignerPluginData;

namespace Internal { class DesignModeWidget; }

class QMLDESIGNERCORE_EXPORT QmlDesignerPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "QmlDesigner.json")

public:
    QmlDesignerPlugin();
    virtual ~QmlDesignerPlugin();

    //Plugin
    bool initialize(const QStringList &arguments, QString *errorMessage = 0);
    void extensionsInitialized();

    static QmlDesignerPlugin *instance();

    DocumentManager &documentManager();
    const DocumentManager &documentManager() const;

    ViewManager &viewManager();
    const ViewManager &viewManager() const;

    DesignerActionManager &designerActionManager();
    const DesignerActionManager &designerActionManager() const;

    DesignerSettings settings();
    void setSettings(const DesignerSettings &s);

    DesignDocument *currentDesignDocument() const;
    Internal::DesignModeWidget *mainWidget() const;

    void switchToTextModeDeferred();

private slots:
    void switchTextDesign();
    void switschToTextMode();
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
    RewriterView *rewriterView() const;
    Model *currentModel() const;

private: // variables
    QmlDesignerPluginData *data;
    static QmlDesignerPlugin *m_instance;

};

} // namespace QmlDesigner

#endif // QMLDESIGNERPLUGIN_H
