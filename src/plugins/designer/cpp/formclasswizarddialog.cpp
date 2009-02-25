/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "formclasswizarddialog.h"
#include "formtemplatewizardpage.h"
#include "formclasswizardpage.h"
#include "formclasswizardparameters.h"

#include <coreplugin/icore.h>

#include <QtCore/QDebug>
#include <QtGui/QAbstractButton>

enum { FormPageId, ClassPageId };

namespace Designer {
namespace Internal {

// ----------------- FormClassWizardDialog
FormClassWizardDialog::FormClassWizardDialog(const WizardPageList &extensionPages,
                                             QWidget *parent) :
    QWizard(parent),
    m_formPage(new FormTemplateWizardPage),
    m_classPage(new FormClassWizardPage)
{
    setWindowTitle(tr("Qt Designer Form Class"));

    setPage(FormPageId, m_formPage);
    connect(m_formPage, SIGNAL(templateActivated()),
            button(QWizard::NextButton), SLOT(animateClick()));

    setPage(ClassPageId, m_classPage);

    foreach (QWizardPage *p, extensionPages)
        addPage(p);

    connect(this, SIGNAL(currentIdChanged(int)), this, SLOT(slotCurrentIdChanged(int)));
}

void FormClassWizardDialog::setSuffixes(const QString &header, const QString &source,  const QString &form)
{
    m_classPage->setSuffixes(header, source, form);
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

void FormClassWizardDialog::slotCurrentIdChanged(int id)
{
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
