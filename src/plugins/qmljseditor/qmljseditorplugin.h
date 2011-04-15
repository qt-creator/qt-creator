/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef QMLJSEDITORPLUGIN_H
#define QMLJSEDITORPLUGIN_H

#include <extensionsystem/iplugin.h>
#include <coreplugin/icontext.h>
#include <QtCore/QPointer>

QT_FORWARD_DECLARE_CLASS(QAction)
QT_FORWARD_DECLARE_CLASS(QTimer)

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

public Q_SLOTS:
    void followSymbolUnderCursor();
    void findUsages();
    void showContextPane();

private Q_SLOTS:
    void currentEditorChanged(Core::IEditor *editor);

private:
    Core::Command *addToolAction(QAction *a, Core::ActionManager *am, Core::Context &context, const QString &name,
                                 Core::ActionContainer *c1, const QString &keySequence);

    static QmlJSEditorPlugin *m_instance;

    QAction *m_actionPreview;
    QmlJSPreviewRunner *m_previewRunner;

    QmlJS::ModelManagerInterface *m_modelManager;
    QmlFileWizard *m_wizard;
    QmlJSEditorFactory *m_editor;
    TextEditor::TextEditorActionHandler *m_actionHandler;

    QmlJSQuickFixAssistProvider *m_quickFixAssistProvider;

    QPointer<TextEditor::ITextEditor> m_currentTextEditable;
    QmlTaskManager *m_qmlTaskManager;
};

} // namespace Internal
} // namespace QmlJSEditor

#endif // QMLJSEDITORPLUGIN_H
