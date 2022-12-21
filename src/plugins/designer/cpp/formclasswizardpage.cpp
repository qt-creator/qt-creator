// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "formclasswizardpage.h"

#include "formclasswizardparameters.h"
#include "newclasswidget.h"
#include "../designertr.h"

#include <coreplugin/icore.h>

#include <cppeditor/cppeditorconstants.h>

#include <utils/mimeutils.h>
#include <utils/wizard.h>

#include <QGridLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QVariant>

namespace Designer {
namespace Internal {

FormClassWizardPage::FormClassWizardPage()
{
    setTitle(Tr::tr("Choose a Class Name"));

    auto classGroupBox = new QGroupBox(this);
    classGroupBox->setTitle(Tr::tr("Class"));

    m_newClassWidget = new NewClassWidget(classGroupBox);
    m_newClassWidget->setHeaderExtension(
            Utils::mimeTypeForName(CppEditor::Constants::CPP_HEADER_MIMETYPE).preferredSuffix());
    m_newClassWidget->setSourceExtension(
            Utils::mimeTypeForName(CppEditor::Constants::CPP_SOURCE_MIMETYPE).preferredSuffix());
    m_newClassWidget->setLowerCaseFiles(lowercaseHeaderFiles());

    connect(m_newClassWidget, &NewClassWidget::validChanged,
            this, &FormClassWizardPage::slotValidChanged);

    setProperty(Utils::SHORT_TITLE_PROPERTY, Tr::tr("Class Details"));

    auto verticalLayout = new QVBoxLayout(classGroupBox);
    verticalLayout->addWidget(m_newClassWidget);

    auto gridLayout = new QGridLayout(this);
    gridLayout->addWidget(classGroupBox, 0, 0, 1, 1);
}

FormClassWizardPage::~FormClassWizardPage() = default;

// Retrieve settings of CppEditor plugin.
bool FormClassWizardPage::lowercaseHeaderFiles()
{
    QString lowerCaseSettingsKey = CppEditor::Constants::CPPEDITOR_SETTINGSGROUP;
    lowerCaseSettingsKey += '/';
    lowerCaseSettingsKey += CppEditor::Constants::LOWERCASE_CPPFILES_KEY;
    const bool lowerCaseDefault = CppEditor::Constants::LOWERCASE_CPPFILES_DEFAULT;
    return Core::ICore::settings()->value(lowerCaseSettingsKey, QVariant(lowerCaseDefault)).toBool();
}

void FormClassWizardPage::setClassName(const QString &suggestedClassName)
{
    // Is it valid, now?
    m_newClassWidget->setClassName(suggestedClassName);
    slotValidChanged();
}

Utils::FilePath FormClassWizardPage::filePath() const
{
    return m_newClassWidget->filePath();
}

void FormClassWizardPage::setFilePath(const Utils::FilePath &p)
{
    m_newClassWidget->setFilePath(p);
}

void FormClassWizardPage::getParameters(FormClassWizardParameters *p) const
{
    p->className = m_newClassWidget->className();
    p->path = filePath();
    p->sourceFile = m_newClassWidget->sourceFileName();
    p->headerFile = m_newClassWidget->headerFileName();
    p->uiFile = m_newClassWidget-> formFileName();
}

void FormClassWizardPage::slotValidChanged()
{
    const bool validNow = m_newClassWidget->isValid();
    if (m_isValid != validNow) {
        m_isValid = validNow;
        emit completeChanged();
    }
}

bool FormClassWizardPage::isComplete() const
{
    return m_isValid;
}

bool FormClassWizardPage::validatePage()
{
    QString errorMessage;
    const bool rc = m_newClassWidget->isValid(&errorMessage);
    if (!rc)
        QMessageBox::warning(this, Tr::tr("%1 - Error").arg(title()), errorMessage);
    return rc;
}

} // namespace Internal
} // namespace Designer
