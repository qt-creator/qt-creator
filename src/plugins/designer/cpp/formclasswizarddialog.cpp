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

#include "formclasswizarddialog.h"
#include "formclasswizardpage.h"
#include "formclasswizardparameters.h"
#include <designer/formtemplatewizardpage.h>
#include <qtsupport/codegenerator.h>

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
    setWindowTitle(tr("Qt Designer Form Class"));

    setPage(FormPageId, m_formPage);
    setPage(ClassPageId, m_classPage);

    foreach (QWizardPage *p, extensionPages())
        addPage(p);
}

QString FormClassWizardDialog::path() const
{
    return m_classPage->path();
}

void FormClassWizardDialog::setPath(const QString &p)
{
    m_classPage->setPath(p);
}

bool FormClassWizardDialog::validateCurrentPage()
{
    return QWizard::validateCurrentPage();
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
    return rc;
}

} // namespace Internal
} // namespace Designer
