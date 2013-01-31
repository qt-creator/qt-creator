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

#include "formclasswizarddialog.h"
#include "formtemplatewizardpage.h"
#include "formclasswizardpage.h"
#include "formclasswizardparameters.h"

#include <coreplugin/basefilewizard.h>

#include <QDebug>
#include <QAbstractButton>

enum { FormPageId, ClassPageId };

namespace Designer {
namespace Internal {

// ----------------- FormClassWizardDialog
FormClassWizardDialog::FormClassWizardDialog(const WizardPageList &extensionPages,
                                             QWidget *parent) :
    Utils::Wizard(parent),
    m_formPage(new FormTemplateWizardPage),
    m_classPage(new FormClassWizardPage)
{
    setWindowTitle(tr("Qt Designer Form Class"));

    setPage(FormPageId, m_formPage);
    wizardProgress()->item(FormPageId)->setTitle(tr("Form Template"));

    setPage(ClassPageId, m_classPage);
    wizardProgress()->item(ClassPageId)->setTitle(tr("Class Details"));

    foreach (QWizardPage *p, extensionPages)
        Core::BaseFileWizard::applyExtensionPageShortTitle(this, addPage(p));
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
    QWizard::initializePage(id);
    // Switching from form to class page: store XML template and set a suitable
    // class name in the class page based on the form base class
    if (id == ClassPageId) {
        QString formBaseClass;
        QString uiClassName;
        m_rawFormTemplate = m_formPage->templateContents();
        // Strip namespaces from the ui class and suggest it as a new class
        // name
        if (FormTemplateWizardPage::getUIXmlData(m_rawFormTemplate, &formBaseClass, &uiClassName))
            m_classPage->setClassName(FormTemplateWizardPage::stripNamespaces(uiClassName));
    }
}

FormClassWizardParameters FormClassWizardDialog::parameters() const
{
    FormClassWizardParameters rc;
    m_classPage->getParameters(&rc);
    // Name the ui class in the Ui namespace after the class specified
    rc.uiTemplate = FormTemplateWizardPage::changeUiClassName(m_rawFormTemplate, rc.className);
    return rc;
}

} // namespace Internal
} // namespace Designer
