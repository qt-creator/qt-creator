// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "translationwizardpage.h"

#include "qtsupporttr.h"

#include <projectexplorer/jsonwizard/jsonwizard.h>

#include <utils/algorithm.h>
#include <utils/filepath.h>
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

#include <algorithm>

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
    const QString m_enabledExpr;
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
                Tr::tr("If you plan to provide translations for your project's "
                       "user interface via the Qt Linguist tool, please select a language here. "
                       "A corresponding translation (.ts) file will be generated for you."));
    descriptionLabel->setWordWrap(true);
    mainLayout->addWidget(descriptionLabel);
    const auto formLayout = new QFormLayout;
    mainLayout->addLayout(formLayout);
    m_languageComboBox.addItem(Tr::tr("<none>"));
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
    localeStrings.erase(std::unique(localeStrings.begin(), localeStrings.end()),
                        localeStrings.end());
    for (const LocalePair &lp : std::as_const(localeStrings))
        m_languageComboBox.addItem(lp.first, lp.second);
    formLayout->addRow(Tr::tr("Language:"), &m_languageComboBox);
    const auto fileNameLayout = new QHBoxLayout;
    m_fileNameLineEdit.setReadOnly(true);
    fileNameLayout->addWidget(&m_fileNameLineEdit);
    fileNameLayout->addStretch(1);
    formLayout->addRow(Tr::tr("Translation file:"), fileNameLayout);
    connect(&m_languageComboBox, &QComboBox::currentIndexChanged,
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
    w->setValue("TsLanguage", m_languageComboBox.currentData().toString());
    return true;
}

void TranslationWizardPage::updateLineEdit()
{
    m_fileNameLineEdit.setEnabled(m_languageComboBox.currentIndex() != 0);
    if (m_fileNameLineEdit.isEnabled()) {
        const QString projectName = static_cast<JsonWizard *>(wizard())->stringValue("ProjectName");
        m_fileNameLineEdit.setText(projectName + '_' + m_languageComboBox.currentData().toString());
    } else {
        m_fileNameLineEdit.clear();
        m_fileNameLineEdit.setPlaceholderText(Tr::tr("<none>"));
    }
    emit completeChanged();
}

} // namespace Internal
} // namespace QtSupport

#include <translationwizardpage.moc>
