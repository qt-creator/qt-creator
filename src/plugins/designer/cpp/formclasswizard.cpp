// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "formclasswizard.h"
#include "formclasswizarddialog.h"
#include <designer/designerconstants.h>
#include <designer/qtdesignerformclasscodegenerator.h>
#include <cppeditor/cppeditorconstants.h>
#include <cppeditor/cpptoolsreuse.h>
#include <projectexplorer/projecttree.h>
#include <qtsupport/qtsupportconstants.h>

#include <QDebug>

using namespace Utils;

namespace Designer {
namespace Internal {

FormClassWizard::FormClassWizard()
{
    setRequiredFeatures({QtSupport::Constants::FEATURE_QWIDGETS});
}

QString FormClassWizard::headerSuffix() const
{
    return CppEditor::preferredCxxHeaderSuffix(ProjectExplorer::ProjectTree::currentProject());
}

QString FormClassWizard::sourceSuffix() const
{
    return CppEditor::preferredCxxSourceSuffix(ProjectExplorer::ProjectTree::currentProject());
}

QString FormClassWizard::formSuffix() const
{
    return preferredSuffix(Constants::FORM_MIMETYPE);
}

Core::BaseFileWizard *FormClassWizard::create(QWidget *parent, const Core::WizardDialogParameters &parameters) const
{
    auto wizardDialog = new FormClassWizardDialog(this, parent);
    wizardDialog->setFilePath(parameters.defaultPath());
    return wizardDialog;
}

Core::GeneratedFiles FormClassWizard::generateFiles(const QWizard *w, QString *errorMessage) const
{
    auto wizardDialog = qobject_cast<const FormClassWizardDialog *>(w);
    const Designer::FormClassWizardParameters params = wizardDialog->parameters();

    if (params.uiTemplate.isEmpty()) {
        *errorMessage = "Internal error: FormClassWizard::generateFiles: empty template contents";
        return Core::GeneratedFiles();
    }

    // header
    const FilePath formFileName = buildFileName(params.path, params.uiFile, formSuffix());
    const FilePath headerFileName = buildFileName(params.path, params.headerFile, headerSuffix());
    const FilePath sourceFileName = buildFileName(params.path, params.sourceFile, sourceSuffix());

    Core::GeneratedFile headerFile(headerFileName);
    headerFile.setAttributes(Core::GeneratedFile::OpenEditorAttribute);

    // Source
    Core::GeneratedFile sourceFile(sourceFileName);
    sourceFile.setAttributes(Core::GeneratedFile::OpenEditorAttribute);

    // UI
    Core::GeneratedFile uiFile(formFileName);
    uiFile.setContents(params.uiTemplate);
    uiFile.setAttributes(Core::GeneratedFile::OpenEditorAttribute);

    QString source;
    QString header;

    QtDesignerFormClassCodeGenerator::generateCpp(params, &header, &source);
    sourceFile.setContents(source);
    headerFile.setContents(header);

    if (Designer::Constants::Internal::debug)
        qDebug() << Q_FUNC_INFO << '\n' << header << '\n' << source;

    return  Core::GeneratedFiles() << headerFile << sourceFile << uiFile;
}

} // Internal
} // Designer
