/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#ifndef GLSLEDITORPLUGIN_H
#define GLSLEDITORPLUGIN_H

#include <extensionsystem/iplugin.h>
#include <coreplugin/icontext.h>
#include <coreplugin/id.h>
#include <QPointer>
#include <glsl/glsl.h>

QT_FORWARD_DECLARE_CLASS(QAction)
QT_FORWARD_DECLARE_CLASS(QTimer)

namespace TextEditor {
class TextEditorActionHandler;
} // namespace TextEditor

namespace Core {
class Command;
class ActionContainer;
class ActionManager;
}

namespace TextEditor {
class ITextEditor;
}

namespace GLSL {
class ModelManagerInterface;
}

namespace GLSLEditor {

class GLSLTextEditorWidget;

namespace Internal {

class GLSLEditorFactory;
class GLSLPreviewRunner;
class GLSLQuickFixCollector;

class GLSLEditorPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "GLSLEditor.json")

public:
    GLSLEditorPlugin();
    virtual ~GLSLEditorPlugin();

    // IPlugin
    bool initialize(const QStringList &arguments, QString *errorMessage = 0);
    void extensionsInitialized();
    ShutdownFlag aboutToShutdown();

    static GLSLEditorPlugin *instance()
    { return m_instance; }

    void initializeEditor(GLSLEditor::GLSLTextEditorWidget *editor);

    struct InitFile {
        GLSL::Engine *engine;
        GLSL::TranslationUnitAST *ast;

        InitFile(GLSL::Engine *engine = 0, GLSL::TranslationUnitAST *ast = 0)
            : engine(engine), ast(ast) {}

        ~InitFile();
    };

    const InitFile *fragmentShaderInit(int variant) const;
    const InitFile *vertexShaderInit(int variant) const;
    const InitFile *shaderInit(int variant) const;

private:
    QByteArray glslFile(const QString &fileName) const;
    InitFile *getInitFile(const QString &fileName, InitFile **initFile) const;
    void parseGlslFile(const QString &fileName, InitFile *initFile) const;

    // FIXME: Unused?
    Core::Command *addToolAction(QAction *a, Core::Context &context, const Core::Id &name,
                                 Core::ActionContainer *c1, const QString &keySequence);

    static GLSLEditorPlugin *m_instance;

    GLSLEditorFactory *m_editor;
    TextEditor::TextEditorActionHandler *m_actionHandler;

    QPointer<TextEditor::ITextEditor> m_currentTextEditable;

    mutable InitFile *m_glsl_120_frag;
    mutable InitFile *m_glsl_120_vert;
    mutable InitFile *m_glsl_120_common;
    mutable InitFile *m_glsl_es_100_frag;
    mutable InitFile *m_glsl_es_100_vert;
    mutable InitFile *m_glsl_es_100_common;
};

} // namespace Internal
} // namespace GLSLEditor

#endif // GLSLEDITORPLUGIN_H
