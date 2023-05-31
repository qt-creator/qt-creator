// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "formclasswizarddialog.h"

#include "formclasswizardpage.h"
#include "formclasswizardparameters.h"
#include "../designertr.h"

#include <cppeditor/abstracteditorsupport.h>
#include <designer/formtemplatewizardpage.h>
#include <projectexplorer/projecttree.h>
#include <qtsupport/codegenerator.h>
#include <utils/filepath.h>

using namespace Utils;

enum { FormPageId, ClassPageId };

namespace Designer {
namespace Internal {

// ----------------- FormClassWizardDialog
FormClassWizardDialog::FormClassWizardDialog(const Core::BaseFileWizardFactory *factory,
                                             QWidget *parent) :
    Core::BaseFileWizard(factory, QVariantMap(), parent),
    m_formPage(new FormTemplateWizardPage),
    m_classPage(new FormClassWizardPage)
{
    setWindowTitle(Tr::tr("Qt Designer Form Class"));

    setPage(FormPageId, m_formPage);
    setPage(ClassPageId, m_classPage);

    const auto pages = extensionPages();
    for (QWizardPage *p : pages)
        addPage(p);
}

FilePath FormClassWizardDialog::filePath() const
{
    return m_classPage->filePath();
}

void FormClassWizardDialog::setFilePath(const FilePath &p)
{
    m_classPage->setFilePath(p);
}

void FormClassWizardDialog::initializePage(int id)
{
    Core::BaseFileWizard::initializePage(id);
    // Switching from form to class page: store XML template and set a suitable
    // class name in the class page based on the form base class
    if (id == ClassPageId) {
        QString formBaseClass;
        QString uiClassName;
        m_rawFormTemplate = m_formPage->templateContents();
        // Strip namespaces from the ui class and suggest it as a new class
        // name
        if (QtSupport::CodeGenerator::uiData(m_rawFormTemplate, &formBaseClass, &uiClassName))
            m_classPage->setClassName(FormTemplateWizardPage::stripNamespaces(uiClassName));
    }
}

FormClassWizardParameters FormClassWizardDialog::parameters() const
{
    FormClassWizardParameters rc;
    m_classPage->getParameters(&rc);
    // Name the ui class in the Ui namespace after the class specified
    rc.uiTemplate = QtSupport::CodeGenerator::changeUiClassName(m_rawFormTemplate, rc.className);
    rc.usePragmaOnce = CppEditor::AbstractEditorSupport::usePragmaOnce(
        ProjectExplorer::ProjectTree::currentProject());
    return rc;
}

} // namespace Internal
} // namespace Designer
