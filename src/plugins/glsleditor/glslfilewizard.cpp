/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#include "glslfilewizard.h"
#include "glsleditorconstants.h"

#include <coreplugin/basefilewizard.h>

#include <utils/filewizardpage.h>
#include <utils/qtcassert.h>

#include <QFileInfo>
#include <QTextStream>
#include <QWizard>
#include <QPushButton>

using namespace Core;
using namespace Utils;

namespace GLSLEditor {

GlslFileWizard::GlslFileWizard(ShaderType shaderType)
    : m_shaderType(shaderType)
{
    setFlags(IWizardFactory::PlatformIndependent);
}

GeneratedFiles GlslFileWizard::generateFiles(const QWizard *w, QString * /*errorMessage*/) const
{
    const BaseFileWizard *wizard = qobject_cast<const BaseFileWizard *>(w);
    const FileWizardPage *page = wizard->find<FileWizardPage>();
    QTC_ASSERT(page, return GeneratedFiles());

    const QString path = page->path();
    const QString name = page->fileName();

    const QString fileName = BaseFileWizardFactory::buildFileName(path, name, preferredSuffix(m_shaderType));

    GeneratedFile file(fileName);
    file.setContents(fileContents(fileName, m_shaderType));
    file.setAttributes(GeneratedFile::OpenEditorAttribute);
    return GeneratedFiles() << file;
}

QString GlslFileWizard::fileContents(const QString &, ShaderType shaderType) const
{
    QString contents;
    QTextStream str(&contents);

    switch (shaderType) {
    case GlslFileWizard::VertexShaderES:
        str << QLatin1String("attribute highp vec4 qt_Vertex;\n")
            << QLatin1String("attribute highp vec4 qt_MultiTexCoord0;\n")
            << QLatin1String("uniform highp mat4 qt_ModelViewProjectionMatrix;\n")
            << QLatin1String("varying highp vec4 qt_TexCoord0;\n")
            << QLatin1String("\n")
            << QLatin1String("void main(void)\n")
            << QLatin1String("{\n")
            << QLatin1String("    gl_Position = qt_ModelViewProjectionMatrix * qt_Vertex;\n")
            << QLatin1String("    qt_TexCoord0 = qt_MultiTexCoord0;\n")
            << QLatin1String("}\n");
        break;
    case GlslFileWizard::FragmentShaderES:
        str << QLatin1String("uniform sampler2D qt_Texture0;\n")
            << QLatin1String("varying highp vec4 qt_TexCoord0;\n")
            << QLatin1String("\n")
            << QLatin1String("void main(void)\n")
            << QLatin1String("{\n")
            << QLatin1String("    gl_FragColor = texture2D(qt_Texture0, qt_TexCoord0.st);\n")
            << QLatin1String("}\n");
        break;
    case GlslFileWizard::VertexShaderDesktop:
        str << QLatin1String("attribute vec4 qt_Vertex;\n")
            << QLatin1String("attribute vec4 qt_MultiTexCoord0;\n")
            << QLatin1String("uniform mat4 qt_ModelViewProjectionMatrix;\n")
            << QLatin1String("varying vec4 qt_TexCoord0;\n")
            << QLatin1String("\n")
            << QLatin1String("void main(void)\n")
            << QLatin1String("{\n")
            << QLatin1String("    gl_Position = qt_ModelViewProjectionMatrix * qt_Vertex;\n")
            << QLatin1String("    qt_TexCoord0 = qt_MultiTexCoord0;\n")
            << QLatin1String("}\n");
        break;
    case GlslFileWizard::FragmentShaderDesktop:
        str << QLatin1String("uniform sampler2D qt_Texture0;\n")
            << QLatin1String("varying vec4 qt_TexCoord0;\n")
            << QLatin1String("\n")
            << QLatin1String("void main(void)\n")
            << QLatin1String("{\n")
            << QLatin1String("    gl_FragColor = texture2D(qt_Texture0, qt_TexCoord0.st);\n")
            << QLatin1String("}\n");
        break;
    default: break;
    }

    return contents;
}

BaseFileWizard *GlslFileWizard::create(QWidget *parent, const WizardDialogParameters &parameters) const
{
    BaseFileWizard *wizard = new BaseFileWizard(parent);
    wizard->setWindowTitle(tr("New %1").arg(displayName()));
    FileWizardPage *page = new FileWizardPage;
    page->setPath(parameters.defaultPath());
    wizard->addPage(page);

    foreach (QWizardPage *p, parameters.extensionPages())
        wizard->addPage(p);
    return wizard;
}

QString GlslFileWizard::preferredSuffix(ShaderType shaderType) const
{
    switch (shaderType) {
    case GlslFileWizard::VertexShaderES:
        return QLatin1String("vsh");
    case GlslFileWizard::FragmentShaderES:
        return QLatin1String("fsh");
    case GlslFileWizard::VertexShaderDesktop:
        return QLatin1String("vert");
    case GlslFileWizard::FragmentShaderDesktop:
        return QLatin1String("frag");
    default:
        return QLatin1String("glsl");
    }
}

} // namespace GlslEditor
