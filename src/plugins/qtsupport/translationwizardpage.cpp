/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "translationwizardpage.h"

#include <projectexplorer/jsonwizard/jsonwizard.h>
#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/macroexpander.h>
#include <utils/wizardpage.h>

#include <QComboBox>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QPair>
#include <QVBoxLayout>

using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;

namespace QtSupport {
namespace Internal {

class TranslationWizardPage : public WizardPage
{
    Q_OBJECT

public:
    TranslationWizardPage(const QString &enabledExpr);

private:
    void initializePage() override;
    bool isComplete() const override;
    bool validatePage() override;

    QString tsBaseName() const { return m_fileNameLineEdit.text(); }
    void updateLineEdit();

    QComboBox m_languageComboBox;
    QLineEdit m_fileNameLineEdit;
    QLabel m_suffixLabel;
    const QString m_enabledExpr;
    bool m_lineEditEdited = false;
};

TranslationWizardPageFactory::TranslationWizardPageFactory()
{
    setTypeIdsSuffix("QtTranslation");
}

WizardPage *TranslationWizardPageFactory::create(JsonWizard *wizard, Id typeId,
                                                 const QVariant &data)
{
    Q_UNUSED(wizard)
    Q_UNUSED(typeId)
    return new TranslationWizardPage(data.toMap().value("enabled").toString());
}

TranslationWizardPage::TranslationWizardPage(const QString &enabledExpr)
    : m_enabledExpr(enabledExpr)
{
    const auto mainLayout = new QVBoxLayout(this);
    const auto descriptionLabel = new QLabel(
                tr("If you plan to provide translations for your project's "
                   "user interface via the Qt Linguist tool, please select a language here. "
                   "A corresponding translation (.ts) file will be generated for you."));
    descriptionLabel->setWordWrap(true);
    mainLayout->addWidget(descriptionLabel);
    const auto formLayout = new QFormLayout;
    mainLayout->addLayout(formLayout);
    m_languageComboBox.addItem(tr("<none>"));
    QList<QLocale> allLocales = QLocale::matchingLocales(
                QLocale::AnyLanguage, QLocale::AnyScript, QLocale::AnyCountry);
    allLocales.removeOne(QLocale::C);
    using LocalePair = QPair<QString, QString>;
    auto localeStrings = transform<QList<LocalePair>>(allLocales,
                [](const QLocale &l) {
                    const QString displayName = QLocale::languageToString(l.language()).append(" (")
                            .append(QLocale::countryToString(l.country())).append(')');
                    const QString tsFileBaseName = l.name();
                    return qMakePair(displayName, tsFileBaseName);
                });
    sort(localeStrings, [](const LocalePair &l1, const LocalePair &l2) {
        return l1.first < l2.first; });
    for (const LocalePair &lp : qAsConst(localeStrings))
        m_languageComboBox.addItem(lp.first, lp.second);
    formLayout->addRow(tr("Language:"), &m_languageComboBox);
    const auto fileNameLayout = new QHBoxLayout;
    fileNameLayout->addWidget(&m_fileNameLineEdit);
    m_suffixLabel.setText(".ts");
    fileNameLayout->addWidget(&m_suffixLabel);
    fileNameLayout->addStretch(1);
    formLayout->addRow(tr("Translation file:"), fileNameLayout);
    connect(&m_fileNameLineEdit, &QLineEdit::textEdited, this, [this] {
        emit completeChanged();
        m_lineEditEdited = true;
    });
    connect(&m_languageComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &TranslationWizardPage::updateLineEdit);
}

void TranslationWizardPage::initializePage()
{
    const bool isEnabled = m_enabledExpr.isEmpty()
            || static_cast<JsonWizard *>(wizard())->expander()->expand(m_enabledExpr) == "yes";
    setEnabled(isEnabled);
    if (!isEnabled)
        m_languageComboBox.setCurrentIndex(0);
    updateLineEdit();
}

bool TranslationWizardPage::isComplete() const
{
    return m_languageComboBox.currentIndex() == 0 || !tsBaseName().isEmpty();
}

bool TranslationWizardPage::validatePage()
{
    const auto w = static_cast<JsonWizard *>(wizard());
    w->setValue("TsFileName", tsBaseName().isEmpty() ? QString() : QString(tsBaseName() + ".ts"));
    w->setValue("TsLanguage", m_fileNameLineEdit.text());
    return true;
}

void TranslationWizardPage::updateLineEdit()
{
    m_fileNameLineEdit.setEnabled(m_languageComboBox.currentIndex() != 0);
    m_suffixLabel.setEnabled(m_fileNameLineEdit.isEnabled());
    if (m_fileNameLineEdit.isEnabled()) {
        if (!m_lineEditEdited) {
            const QString projectName
                    = static_cast<JsonWizard *>(wizard())->stringValue("ProjectName");
            m_fileNameLineEdit.setText(projectName + '_'
                                       + m_languageComboBox.currentData().toString());
        }
    } else {
        m_fileNameLineEdit.clear();
        m_fileNameLineEdit.setPlaceholderText(tr("<none>"));
    }
    emit completeChanged();
}

} // namespace Internal
} // namespace QtSupport

#include <translationwizardpage.moc>
