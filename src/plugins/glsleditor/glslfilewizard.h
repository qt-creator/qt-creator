/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#ifndef GLSLFILEWIZARD_H
#define GLSLFILEWIZARD_H

#include <coreplugin/basefilewizard.h>

namespace GLSLEditor {

class GLSLFileWizard: public Core::BaseFileWizard
{
    Q_OBJECT

public:
    typedef Core::BaseFileWizardParameters BaseFileWizardParameters;

    enum ShaderType
    {
        VertexShaderES,
        FragmentShaderES,
        VertexShaderDesktop,
        FragmentShaderDesktop
    };

    explicit GLSLFileWizard(const BaseFileWizardParameters &parameters,
                            ShaderType shaderType, QObject *parent = 0);

    virtual Core::FeatureSet requiredFeatures() const;
    virtual WizardFlags flags() const;

protected:
    QString fileContents(const QString &baseName, ShaderType shaderType) const;

    virtual QWizard *createWizardDialog(QWidget *parent,
                                        const Core::WizardDialogParameters &wizardDialogParameters) const;

    virtual Core::GeneratedFiles generateFiles(const QWizard *w,
                                               QString *errorMessage) const;

    virtual QString preferredSuffix(ShaderType shaderType) const;

private:
    ShaderType m_shaderType;
};

} // namespace GLSLEditor

#endif // GLSLFILEWIZARD_H
