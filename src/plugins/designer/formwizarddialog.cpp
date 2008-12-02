/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** 
** Non-Open Source Usage  
** 
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.  
** 
** GNU General Public License Usage 
** 
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception version
** 1.2, included in the file GPL_EXCEPTION.txt in this package.  
** 
***************************************************************************/
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
FormWizardDialog::FormWizardDialog(Core::ICore *core,
                                   const WizardPageList &extensionPages,
                                   QWidget *parent) :
    QWizard(parent),
    m_formPage(new FormTemplateWizardPagePage),
    m_core(core)
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
FormFileWizardDialog::FormFileWizardDialog(Core::ICore *core,
                                           const WizardPageList &extensionPages,
                                           QWidget *parent) :
    FormWizardDialog(core, extensionPages, parent),
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
        if (FormTemplateWizardPagePage::getUIXmlData(templateContents(), &formBaseClass, &uiClassName)) {
            QString fileName = FormTemplateWizardPagePage::stripNamespaces(uiClassName).toLower();
            fileName += QLatin1String(".ui");
            m_filePage->setName(fileName);
        }
    }
}
} // namespace Internal
} // namespace Designer
