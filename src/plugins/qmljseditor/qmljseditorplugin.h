/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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

#ifndef QMLJSEDITORPLUGIN_H
#define QMLJSEDITORPLUGIN_H

#include <extensionsystem/iplugin.h>
#include <coreplugin/icontext.h>
#include <coreplugin/id.h>

#include <QPointer>

QT_FORWARD_DECLARE_CLASS(QAction)

namespace Utils {
class JsonSchemaManager;
}

namespace TextEditor {
class TextEditorActionHandler;
} // namespace TextEditor

namespace Core {
class Command;
class ActionContainer;
class ActionManager;
class IEditor;
}

namespace TextEditor {
class ITextEditor;
}

namespace QmlJS {
    class ModelManagerInterface;
}

namespace QmlJSEditor {

class QmlFileWizard;
class QmlJSTextEditorWidget;

namespace Internal {

class QmlJSEditorFactory;
class QmlJSPreviewRunner;
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

    void initializeEditor(QmlJSEditor::QmlJSTextEditorWidget *editor);

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
    Core::Command *addToolAction(QAction *a, Core::Context &context, const Core::Id &id,
                                 Core::ActionContainer *c1, const QString &keySequence);

    static QmlJSEditorPlugin *m_instance;

    QmlJS::ModelManagerInterface *m_modelManager;
    QmlJSEditorFactory *m_editor;
    TextEditor::TextEditorActionHandler *m_actionHandler;
    QmlJSQuickFixAssistProvider *m_quickFixAssistProvider;
    QmlTaskManager *m_qmlTaskManager;

    QAction *m_reformatFileAction;

    QPointer<QmlJSTextEditorWidget> m_currentEditor;
    QScopedPointer<Utils::JsonSchemaManager> m_jsonManager;
};

} // namespace Internal
} // namespace QmlJSEditor

#endif // QMLJSEDITORPLUGIN_H
