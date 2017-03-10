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

#include <qmldesigner/designersettings.h>
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
}

namespace QmlDesigner {

class QmlDesignerPluginPrivate;

namespace Internal { class DesignModeWidget; }

class QMLDESIGNERCORE_EXPORT QmlDesignerPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "QmlDesigner.json")

public:
    QmlDesignerPlugin();
    virtual ~QmlDesignerPlugin();

    //Plugin
    bool initialize(const QStringList &arguments, QString *errorMessage = 0) override;
    bool delayedInitialize() override;
    void extensionsInitialized() override;

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
    void emitCurrentTextEditorChanged(Core::IEditor *editor);

    static double formEditorDevicePixelRatio();

private: // functions
    void integrateIntoQtCreator(QWidget *modeWidget);
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
    QmlDesignerPluginPrivate *d = nullptr;
    static QmlDesignerPlugin *m_instance;

};

} // namespace QmlDesigner
