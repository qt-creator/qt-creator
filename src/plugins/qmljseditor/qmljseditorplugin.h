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

#ifndef QMLJSEDITORPLUGIN_H
#define QMLJSEDITORPLUGIN_H

#include <extensionsystem/iplugin.h>
#include <coreplugin/icontext.h>
#include <coreplugin/id.h>

#include <QPointer>

QT_FORWARD_DECLARE_CLASS(QAction)

namespace Utils { class JsonSchemaManager; }

namespace Core {
class Command;
class ActionContainer;
class IEditor;
}

namespace QmlJS { class ModelManagerInterface; }

namespace QmlJSEditor {

class QmlJSEditorDocument;

namespace Internal {

class QmlJSQuickFixAssistProvider;
class QmlTaskManager;

class QmlJSEditorPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "QmlJSEditor.json")

public:
    QmlJSEditorPlugin();
    virtual ~QmlJSEditorPlugin();

    // IPlugin
    bool initialize(const QStringList &arguments, QString *errorMessage = 0);
    void extensionsInitialized();
    ShutdownFlag aboutToShutdown();

    static QmlJSEditorPlugin *instance()
    { return m_instance; }

    QmlJSQuickFixAssistProvider *quickFixAssistProvider() const;

    Utils::JsonSchemaManager *jsonManager() const;

public Q_SLOTS:
    void findUsages();
    void renameUsages();
    void reformatFile();
    void showContextPane();

private Q_SLOTS:
    void currentEditorChanged(Core::IEditor *editor);
    void runSemanticScan();
    void checkCurrentEditorSemanticInfoUpToDate();

private:
    Core::Command *addToolAction(QAction *a, Core::Context &context, Core::Id id,
                                 Core::ActionContainer *c1, const QString &keySequence);

    static QmlJSEditorPlugin *m_instance;

    QmlJS::ModelManagerInterface *m_modelManager;
    QmlJSQuickFixAssistProvider *m_quickFixAssistProvider;
    QmlTaskManager *m_qmlTaskManager;

    QAction *m_reformatFileAction;

    QPointer<QmlJSEditorDocument> m_currentDocument;
    QScopedPointer<Utils::JsonSchemaManager> m_jsonManager;
};

} // namespace Internal
} // namespace QmlJSEditor

#endif // QMLJSEDITORPLUGIN_H
