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

#include "formwizarddialog.h"
#include "formtemplatewizardpage.h"
#include "formeditorw.h"

#include <coreplugin/icore.h>
#include <coreplugin/basefilewizard.h>

#include <utils/filewizardpage.h>

#include <QtCore/QDebug>
#include <QtGui/QAbstractButton>

// Make sure there is a gap before the extension pages
enum { FormPageId, FilePageId, FirstExtensionPageId = 10 };

namespace Designer {
namespace Internal {

// ----------------- FormWizardDialog
FormWizardDialog::FormWizardDialog(const WizardPageList &extensionPages,
                                   QWidget *parent)
  : QWizard(parent),
    m_formPage(new FormTemplateWizardPage)
{
    init(extensionPages);
}

void FormWizardDialog::init(const WizardPageList &extensionPages)
{
    Core::BaseFileWizard::setupWizard(this);
    setWindowTitle(tr("Qt Designer Form"));
    setPage(FormPageId, m_formPage);

    if (!extensionPages.empty()) {
        int id = FirstExtensionPageId;
        foreach (QWizardPage *p, extensionPages)
            setPage(id++, p);
    }
}

QString FormWizardDialog::templateContents() const
{
    // Template is expensive, cache
    if (m_templateContents.isEmpty())
        m_templateContents = m_formPage->templateContents();
    return m_templateContents;
}

// ----------------- FormFileWizardDialog
FormFileWizardDialog::FormFileWizardDialog(const WizardPageList &extensionPages,
                                           QWidget *parent)
  : FormWizardDialog(extensionPages, parent),
    m_filePage(new Core::Utils::FileWizardPage)
{
    setPage(FilePageId, m_filePage);
    connect(m_filePage, SIGNAL(activated()),
            button(QWizard::FinishButton), SLOT(animateClick()));

    connect(this, SIGNAL(currentIdChanged(int)), this, SLOT(slotCurrentIdChanged(int)));
}

QString FormFileWizardDialog::path() const
{
    return m_filePage->path();
}

void FormFileWizardDialog::setPath(const QString &path)
{
    m_filePage->setPath(path);
}

QString FormFileWizardDialog::name() const
{
    return m_filePage->name();
}

void FormFileWizardDialog::slotCurrentIdChanged(int id)
{
    if (id == FilePageId) {
        // Change from form to file: Store template and Suggest a name based on
        // the ui class
        QString formBaseClass;
        QString uiClassName;
        if (FormTemplateWizardPage::getUIXmlData(templateContents(), &formBaseClass, &uiClassName)) {
            QString fileName = FormTemplateWizardPage::stripNamespaces(uiClassName).toLower();
            fileName += QLatin1String(".ui");
            m_filePage->setName(fileName);
        }
    }
}

} // namespace Internal
} // namespace Designer
