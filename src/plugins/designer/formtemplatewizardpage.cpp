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

#include "formtemplatewizardpage.h"
#include "formeditorw.h"

#include <projectexplorer/jsonwizard/jsonwizardpagefactory.h>

#include <utils/qtcassert.h>
#include <utils/wizard.h>

#include <QCoreApplication>
#include <QDesignerNewFormWidgetInterface>
#include <QDebug>
#include <QXmlStreamReader>
#include <QXmlStreamAttributes>
#include <QByteArray>
#include <QBuffer>

#include <QVBoxLayout>
#include <QMessageBox>

#include <QDomDocument>

namespace Designer {
namespace Internal {

FormPageFactory::FormPageFactory()
{
    setTypeIdsSuffix("Form");
}

Utils::WizardPage *FormPageFactory::create(ProjectExplorer::JsonWizard *wizard, Core::Id typeId,
                                           const QVariant &data)
{
    Q_UNUSED(wizard);
    Q_UNUSED(data);

    QTC_ASSERT(canCreate(typeId), return 0);

    FormTemplateWizardPage *page = new FormTemplateWizardPage;
    return page;
}

bool FormPageFactory::validateData(Core::Id typeId, const QVariant &data, QString *errorMessage)
{
    QTC_ASSERT(canCreate(typeId), return false);
    if (!data.isNull() && (data.type() != QVariant::Map || !data.toMap().isEmpty())) {
        *errorMessage = QCoreApplication::translate("ProjectExplorer::JsonWizard",
                                                    "\"data\" for a \"Form\" page needs to be unset or an empty object.");
        return false;
    }

    return true;
}

// ----------------- FormTemplateWizardPage

FormTemplateWizardPage::FormTemplateWizardPage(QWidget * parent) :
    Utils::WizardPage(parent),
    m_newFormWidget(QDesignerNewFormWidgetInterface::createNewFormWidget(FormEditorW::designerEditor())),
    m_templateSelected(m_newFormWidget->hasCurrentTemplate())
{
    setTitle(tr("Choose a Form Template"));
    QVBoxLayout *layout = new QVBoxLayout;

    connect(m_newFormWidget, &QDesignerNewFormWidgetInterface::currentTemplateChanged,
            this, &FormTemplateWizardPage::slotCurrentTemplateChanged);
    connect(m_newFormWidget, &QDesignerNewFormWidgetInterface::templateActivated,
            this, &FormTemplateWizardPage::templateActivated);
    layout->addWidget(m_newFormWidget);

    setLayout(layout);
    setProperty(Utils::SHORT_TITLE_PROPERTY, tr("Form Template"));
}

bool FormTemplateWizardPage::isComplete() const
{
    return m_templateSelected;
}

void FormTemplateWizardPage::slotCurrentTemplateChanged(bool templateSelected)
{
    if (m_templateSelected == templateSelected)
        return;
    m_templateSelected = templateSelected;
    emit completeChanged();
}

bool FormTemplateWizardPage::validatePage()
{
    QString errorMessage;
    m_templateContents = m_newFormWidget->currentTemplate(&errorMessage);
    if (m_templateContents.isEmpty()) {
        QMessageBox::critical(this, tr("%1 - Error").arg(title()), errorMessage);
        return false;
    }
    wizard()->setProperty("FormContents", m_templateContents.split('\n'));
    return true;
}

QString FormTemplateWizardPage::stripNamespaces(const QString &className)
{
    QString rc = className;
    const int namespaceIndex = rc.lastIndexOf("::");
    if (namespaceIndex != -1)
        rc.remove(0, namespaceIndex + 2);
    return rc;
}

} // namespace Internal
} // namespace Designer
