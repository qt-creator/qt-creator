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

#include "glsleditorconstants.h"
#include "glslfilewizard.h"

#include <utils/filewizarddialog.h>
#include <utils/qtcassert.h>
#include <utils/filewizarddialog.h>

#include <QtCore/QFileInfo>
#include <QtCore/QTextStream>
#include <QtGui/QWizard>
#include <QtGui/QPushButton>

namespace {
class GLSLFileWizardDialog : public Utils::FileWizardDialog
{
    Q_OBJECT
public:
    GLSLFileWizardDialog(QWidget *parent = 0)
        : Utils::FileWizardDialog(parent)
    {
    }
};
} // anonymous namespace

using namespace GLSLEditor;

GLSLFileWizard::GLSLFileWizard(const BaseFileWizardParameters &parameters,
                               ShaderType shaderType, QObject *parent):
    Core::BaseFileWizard(parameters, parent),
    m_shaderType(shaderType)
{
}

Core::GeneratedFiles GLSLFileWizard::generateFiles(const QWizard *w,
                                                 QString * /*errorMessage*/) const
{
    const GLSLFileWizardDialog *wizardDialog = qobject_cast<const GLSLFileWizardDialog *>(w);
    const QString path = wizardDialog->path();
    const QString name = wizardDialog->fileName();

    const QString mimeType = QLatin1String(Constants::GLSL_MIMETYPE);
    const QString fileName = Core::BaseFileWizard::buildFileName(path, name, preferredSuffix(m_shaderType));

    Core::GeneratedFile file(fileName);
    file.setContents(fileContents(fileName, m_shaderType));
    file.setAttributes(Core::GeneratedFile::OpenEditorAttribute);
    return Core::GeneratedFiles() << file;
}

QString GLSLFileWizard::fileContents(const QString &, ShaderType shaderType) const
{
    QString contents;
    QTextStream str(&contents);

    switch (shaderType) {
    case GLSLFileWizard::VertexShader:
        str << QLatin1String("attribute highp vec4 qgl_Vertex;\n")
            << QLatin1String("attribute highp vec4 qgl_TexCoord0;\n")
            << QLatin1String("uniform highp mat4 qgl_ModelViewProjectionMatrix;\n")
            << QLatin1String("varying highp vec4 qTexCoord0;\n")
            << QLatin1String("\n")
            << QLatin1String("void main(void)\n")
            << QLatin1String("{\n")
            << QLatin1String("    gl_Position = qgl_ModelViewProjectionMatrix * qgl_Vertex;\n")
            << QLatin1String("    qTexCoord0 = qgl_TexCoord0;\n")
            << QLatin1String("}\n");
        break;
    case GLSLFileWizard::FragmentShader:
        str << QLatin1String("uniform sampler2D qgl_Texture0;\n")
            << QLatin1String("varying highp vec4 qTexCoord0;\n")
            << QLatin1String("\n")
            << QLatin1String("void main(void)\n")
            << QLatin1String("{\n")
            << QLatin1String("    gl_FragColor = texture2D(qgl_Texture0, qTexCoord0.st);\n")
            << QLatin1String("}\n");
        break;
    default: break;
    }

    return contents;
}

QWizard *GLSLFileWizard::createWizardDialog(QWidget *parent, const QString &defaultPath,
                                          const WizardPageList &extensionPages) const
{
    GLSLFileWizardDialog *wizardDialog = new GLSLFileWizardDialog(parent);
    wizardDialog->setWindowTitle(tr("New %1").arg(displayName()));
    setupWizard(wizardDialog);
    wizardDialog->setPath(defaultPath);
    foreach (QWizardPage *p, extensionPages)
        BaseFileWizard::applyExtensionPageShortTitle(wizardDialog, wizardDialog->addPage(p));
    return wizardDialog;
}

QString GLSLFileWizard::preferredSuffix(ShaderType shaderType) const
{
    switch (shaderType) {
    case GLSLFileWizard::VertexShader:
        return QLatin1String("vert");
    case GLSLFileWizard::FragmentShader:
        return QLatin1String("frag");
    default:
        return QLatin1String("glsl");
    }
}

#include "glslfilewizard.moc"
