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

#ifndef GLSLEDITORPLUGIN_H
#define GLSLEDITORPLUGIN_H

#include <extensionsystem/iplugin.h>
#include <coreplugin/icontext.h>
#include <QtCore/QPointer>
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
class ITextEditable;
}

namespace GLSL {
class ModelManagerInterface;
}

namespace GLSLEditor {

class GLSLTextEditor;

namespace Internal {

class GLSLEditorFactory;
class GLSLPreviewRunner;
class GLSLQuickFixCollector;

class GLSLEditorPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT

public:
    GLSLEditorPlugin();
    virtual ~GLSLEditorPlugin();

    // IPlugin
    bool initialize(const QStringList &arguments, QString *errorMessage = 0);
    void extensionsInitialized();
    ShutdownFlag aboutToShutdown();

    static GLSLEditorPlugin *instance()
    { return m_instance; }

    void initializeEditor(GLSLEditor::GLSLTextEditor *editor);

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
    QByteArray glslFile(const QString &fileName);
    void parseGlslFile(const QString &fileName, InitFile *initFile);

    Core::Command *addToolAction(QAction *a, Core::ActionManager *am, Core::Context &context, const QString &name,
                                 Core::ActionContainer *c1, const QString &keySequence);

    static GLSLEditorPlugin *m_instance;

    GLSLEditorFactory *m_editor;
    TextEditor::TextEditorActionHandler *m_actionHandler;

    QPointer<TextEditor::ITextEditable> m_currentTextEditable;

    InitFile m_glsl_120_frag;
    InitFile m_glsl_120_vert;
    InitFile m_glsl_120_common;
    InitFile m_glsl_es_100_frag;
    InitFile m_glsl_es_100_vert;
    InitFile m_glsl_es_100_common;
};

} // namespace Internal
} // namespace GLSLEditor

#endif // GLSLEDITORPLUGIN_H
