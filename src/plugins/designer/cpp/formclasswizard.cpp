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

#include "formclasswizard.h"
#include "formclasswizarddialog.h"
#include <designer/designerconstants.h>
#include <designer/qtdesignerformclasscodegenerator.h>
#include <qtsupport/qtsupportconstants.h>

#include <cpptools/cpptoolsconstants.h>

#include <QDebug>

namespace Designer {
namespace Internal {

FormClassWizard::FormClassWizard()
{
    setRequiredFeatures({ QtSupport::Constants::FEATURE_QWIDGETS });
}

QString FormClassWizard::headerSuffix() const
{
    return preferredSuffix(QLatin1String(CppTools::Constants::CPP_HEADER_MIMETYPE));
}

QString FormClassWizard::sourceSuffix() const
{
    return preferredSuffix(QLatin1String(CppTools::Constants::CPP_SOURCE_MIMETYPE));
}

QString FormClassWizard::formSuffix() const
{
    return preferredSuffix(QLatin1String(Constants::FORM_MIMETYPE));
}

Core::BaseFileWizard *FormClassWizard::create(QWidget *parent, const Core::WizardDialogParameters &parameters) const
{
    FormClassWizardDialog *wizardDialog = new FormClassWizardDialog(this, parent);
    wizardDialog->setPath(parameters.defaultPath());
    return wizardDialog;
}

Core::GeneratedFiles FormClassWizard::generateFiles(const QWizard *w, QString *errorMessage) const
{
    const FormClassWizardDialog *wizardDialog = qobject_cast<const FormClassWizardDialog *>(w);
    const Designer::FormClassWizardParameters params = wizardDialog->parameters();

    if (params.uiTemplate.isEmpty()) {
        *errorMessage = QLatin1String("Internal error: FormClassWizard::generateFiles: empty template contents");
        return Core::GeneratedFiles();
    }

    // header
    const QString formFileName = buildFileName(params.path, params.uiFile, formSuffix());
    const QString headerFileName = buildFileName(params.path, params.headerFile, headerSuffix());
    const QString sourceFileName = buildFileName(params.path, params.sourceFile, sourceSuffix());

    Core::GeneratedFile headerFile(headerFileName);
    headerFile.setAttributes(Core::GeneratedFile::OpenEditorAttribute);

    // Source
    Core::GeneratedFile sourceFile(sourceFileName);
    sourceFile.setAttributes(Core::GeneratedFile::OpenEditorAttribute);

    // UI
    Core::GeneratedFile uiFile(formFileName);
    uiFile.setContents(params.uiTemplate);
    uiFile.setAttributes(Core::GeneratedFile::OpenEditorAttribute);

    QString source, header;

    QtDesignerFormClassCodeGenerator::generateCpp(params, &header, &source);
    sourceFile.setContents(source);
    headerFile.setContents(header);

    if (Designer::Constants::Internal::debug)
        qDebug() << Q_FUNC_INFO << '\n' << header << '\n' << source;

    return  Core::GeneratedFiles() << headerFile << sourceFile << uiFile;
}

}
}
