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

#ifndef GLSLEDITORPLUGIN_H
#define GLSLEDITORPLUGIN_H

#include <extensionsystem/iplugin.h>
#include <glsl/glsl.h>

namespace GlslEditor {
namespace Internal {

class GlslEditorWidget;

class GlslEditorPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "GLSLEditor.json")

public:
    GlslEditorPlugin();
    ~GlslEditorPlugin();

    // IPlugin
    bool initialize(const QStringList &arguments, QString *errorMessage = 0);
    void extensionsInitialized();
    ShutdownFlag aboutToShutdown();

    struct InitFile
    {
        InitFile(GLSL::Engine *engine = 0, GLSL::TranslationUnitAST *ast = 0)
            : engine(engine), ast(ast)
        {}

        ~InitFile();

        GLSL::Engine *engine;
        GLSL::TranslationUnitAST *ast;
    };

    static const InitFile *fragmentShaderInit(int variant);
    static const InitFile *vertexShaderInit(int variant);
    static const InitFile *shaderInit(int variant);
};

} // namespace Internal
} // namespace GlslEditor

#endif // GLSLEDITORPLUGIN_H
