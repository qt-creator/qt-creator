// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "designertr.h"
#include "formeditor.h"
#include "formtemplatewizardpage.h"

#include <projectexplorer/jsonwizard/jsonwizardpagefactory.h>
#include <projectexplorer/projectexplorertr.h>

#include <utils/qtcassert.h>
#include <utils/wizard.h>

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

Utils::WizardPage *FormPageFactory::create(ProjectExplorer::JsonWizard *wizard, Utils::Id typeId,
                                           const QVariant &data)
{
    Q_UNUSED(wizard)
    Q_UNUSED(data)

    QTC_ASSERT(canCreate(typeId), return nullptr);

    return new FormTemplateWizardPage;
}

bool FormPageFactory::validateData(Utils::Id typeId, const QVariant &data, QString *errorMessage)
{
    QTC_ASSERT(canCreate(typeId), return false);
    if (!data.isNull() && (data.type() != QVariant::Map || !data.toMap().isEmpty())) {
        *errorMessage = ::ProjectExplorer::Tr::tr(
                    "\"data\" for a \"Form\" page needs to be unset or an empty object.");
        return false;
    }

    return true;
}

// ----------------- FormTemplateWizardPage

FormTemplateWizardPage::FormTemplateWizardPage(QWidget * parent) :
    Utils::WizardPage(parent),
    m_newFormWidget(QDesignerNewFormWidgetInterface::createNewFormWidget(designerEditor())),
    m_templateSelected(m_newFormWidget->hasCurrentTemplate())
{
    setTitle(Tr::tr("Choose a Form Template"));
    QVBoxLayout *layout = new QVBoxLayout;

    connect(m_newFormWidget, &QDesignerNewFormWidgetInterface::currentTemplateChanged,
            this, &FormTemplateWizardPage::slotCurrentTemplateChanged);
    connect(m_newFormWidget, &QDesignerNewFormWidgetInterface::templateActivated,
            this, &FormTemplateWizardPage::templateActivated);
    layout->addWidget(m_newFormWidget);

    setLayout(layout);
    setProperty(Utils::SHORT_TITLE_PROPERTY, Tr::tr("Form Template"));
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
        QMessageBox::critical(this, Tr::tr("%1 - Error").arg(title()), errorMessage);
        return false;
    }
    wizard()->setProperty("FormContents", m_templateContents);
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
