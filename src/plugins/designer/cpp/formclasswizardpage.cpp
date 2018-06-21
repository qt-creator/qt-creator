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

#include "formclasswizardpage.h"
#include "ui_formclasswizardpage.h"
#include "formclasswizardparameters.h"

#include <utils/wizard.h>

#include <coreplugin/icore.h>
#include <cpptools/cpptoolsconstants.h>
#include <utils/mimetypes/mimedatabase.h>
#
#include <QDebug>

#include <QMessageBox>

namespace Designer {
namespace Internal {

// ----------------- FormClassWizardPage

FormClassWizardPage::FormClassWizardPage(QWidget * parent) :
    QWizardPage(parent),
    m_ui(new Ui::FormClassWizardPage)
{
    m_ui->setupUi(this);

    m_ui->newClassWidget->setBaseClassInputVisible(false);
    m_ui->newClassWidget->setNamespacesEnabled(true);
    m_ui->newClassWidget->setAllowDirectories(true);
    m_ui->newClassWidget->setClassTypeComboVisible(false);

    connect(m_ui->newClassWidget, &Utils::NewClassWidget::validChanged, this, &FormClassWizardPage::slotValidChanged);

    initFileGenerationSettings();

    setProperty(Utils::SHORT_TITLE_PROPERTY, tr("Class Details"));
}

FormClassWizardPage::~FormClassWizardPage()
{
    delete m_ui;
}

// Retrieve settings of CppTools plugin.
bool FormClassWizardPage::lowercaseHeaderFiles()
{
    QString lowerCaseSettingsKey = CppTools::Constants::CPPTOOLS_SETTINGSGROUP;
    lowerCaseSettingsKey += '/';
    lowerCaseSettingsKey += CppTools::Constants::LOWERCASE_CPPFILES_KEY;
    const bool lowerCaseDefault = CppTools::Constants::lowerCaseFilesDefault;
    return Core::ICore::settings()->value(lowerCaseSettingsKey, QVariant(lowerCaseDefault)).toBool();
}

// Set up new class widget from settings
void FormClassWizardPage::initFileGenerationSettings()
{
    m_ui->newClassWidget->setHeaderExtension(
                Utils::mimeTypeForName(CppTools::Constants::CPP_HEADER_MIMETYPE).preferredSuffix());
    m_ui->newClassWidget->setSourceExtension(
                Utils::mimeTypeForName(CppTools::Constants::CPP_SOURCE_MIMETYPE).preferredSuffix());
    m_ui->newClassWidget->setLowerCaseFiles(lowercaseHeaderFiles());
}

void FormClassWizardPage::setClassName(const QString &suggestedClassName)
{
    // Is it valid, now?
    m_ui->newClassWidget->setClassName(suggestedClassName);
    slotValidChanged();
}

QString FormClassWizardPage::path() const
{
    return m_ui->newClassWidget->path();
}

void FormClassWizardPage::setPath(const QString &p)
{
    m_ui->newClassWidget->setPath(p);
}

void FormClassWizardPage::getParameters(FormClassWizardParameters *p) const
{
    p->className = m_ui->newClassWidget->className();
    p->path = path();
    p->sourceFile = m_ui->newClassWidget->sourceFileName();
    p->headerFile = m_ui->newClassWidget->headerFileName();
    p->uiFile = m_ui->newClassWidget-> formFileName();
}

void FormClassWizardPage::slotValidChanged()
{
    const bool validNow = m_ui->newClassWidget->isValid();
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
    const bool rc = m_ui->newClassWidget->isValid(&errorMessage);
    if (!rc)
        QMessageBox::warning(this, tr("%1 - Error").arg(title()), errorMessage);
    return rc;
}

} // namespace Internal
} // namespace Designer
