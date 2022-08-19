// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "customwidgetpluginwizardpage.h"
#include "customwidgetwidgetswizardpage.h"
#include "ui_customwidgetpluginwizardpage.h"

#include <utils/wizard.h>

namespace QmakeProjectManager {
namespace Internal {

// Determine name for Q_EXPORT_PLUGIN
static inline QString createPluginName(const QString &prefix)
{
    return prefix.toLower() + QLatin1String("plugin");
}

CustomWidgetPluginWizardPage::CustomWidgetPluginWizardPage(QWidget *parent) :
    QWizardPage(parent),
    m_ui(new Ui::CustomWidgetPluginWizardPage),
    m_classCount(-1),
    m_complete(false)
{
    m_ui->setupUi(this);
    connect(m_ui->collectionClassEdit, &QLineEdit::textEdited,
            this, &CustomWidgetPluginWizardPage::slotCheckCompleteness);
    connect(m_ui->collectionClassEdit, &QLineEdit::textChanged,
            this, [this](const QString &collectionClass) {
        m_ui->collectionHeaderEdit->setText(m_fileNamingParameters.headerFileName(collectionClass));
        m_ui->pluginNameEdit->setText(createPluginName(collectionClass));
    });
    connect(m_ui->pluginNameEdit, &QLineEdit::textEdited,
            this, &CustomWidgetPluginWizardPage::slotCheckCompleteness);
    connect(m_ui->collectionHeaderEdit, &QLineEdit::textChanged,
            this, [this](const QString &text) {
        m_ui->collectionSourceEdit->setText(m_fileNamingParameters.headerToSourceFileName(text));
    });

    setProperty(Utils::SHORT_TITLE_PROPERTY, tr("Plugin Details"));
}

CustomWidgetPluginWizardPage::~CustomWidgetPluginWizardPage()
{
    delete m_ui;
}

QString CustomWidgetPluginWizardPage::collectionClassName() const
{
    return m_ui->collectionClassEdit->text();
}

QString CustomWidgetPluginWizardPage::pluginName() const
{
    return m_ui->pluginNameEdit->text();
}

void CustomWidgetPluginWizardPage::init(const CustomWidgetWidgetsWizardPage *widgetsPage)
{
    m_classCount = widgetsPage->classCount();
    const QString empty;
    if (m_classCount == 1) {
        m_ui->pluginNameEdit->setText(createPluginName(widgetsPage->classNameAt(0)));
        setCollectionEnabled(false);
    } else {
        m_ui->pluginNameEdit->setText(empty);
        setCollectionEnabled(true);
    }
    m_ui->collectionClassEdit->setText(empty);
    m_ui->collectionHeaderEdit->setText(empty);
    m_ui->collectionSourceEdit->setText(empty);

    slotCheckCompleteness();
}

void CustomWidgetPluginWizardPage::setCollectionEnabled(bool enColl)
{
    m_ui->collectionClassLabel->setEnabled(enColl);
    m_ui->collectionClassEdit->setEnabled(enColl);
    m_ui->collectionHeaderLabel->setEnabled(enColl);
    m_ui->collectionHeaderEdit->setEnabled(enColl);
    m_ui->collectionSourceLabel->setEnabled(enColl);
    m_ui->collectionSourceEdit->setEnabled(enColl);
}

QSharedPointer<PluginOptions> CustomWidgetPluginWizardPage::basicPluginOptions() const
{
    QSharedPointer<PluginOptions> po(new PluginOptions);
    po->pluginName = pluginName();
    po->resourceFile = m_ui->resourceFileEdit->text();
    po->collectionClassName = collectionClassName();
    po->collectionHeaderFile = m_ui->collectionHeaderEdit->text();
    po->collectionSourceFile = m_ui->collectionSourceEdit->text();
    return po;
}

void CustomWidgetPluginWizardPage::slotCheckCompleteness()
{
    // A collection is complete only with class name
    bool completeNow = false;
    if (!pluginName().isEmpty()) {
        if (m_classCount > 1)
            completeNow = !collectionClassName().isEmpty();
        else
            completeNow = true;
    }
    if (completeNow != m_complete) {
        m_complete = completeNow;
        emit completeChanged();
    }
}

bool CustomWidgetPluginWizardPage::isComplete() const
{
    return m_complete;
}

} // namespace Internal
} // namespace QmakeProjectManager
