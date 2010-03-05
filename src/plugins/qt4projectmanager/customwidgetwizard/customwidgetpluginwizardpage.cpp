/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "customwidgetpluginwizardpage.h"
#include "customwidgetwidgetswizardpage.h"
#include "pluginoptions.h"
#include "ui_customwidgetpluginwizardpage.h"

namespace Qt4ProjectManager {
namespace Internal {

CustomWidgetPluginWizardPage::CustomWidgetPluginWizardPage(QWidget *parent) :
    QWizardPage(parent),
    m_ui(new Ui::CustomWidgetPluginWizardPage),
    m_classCount(-1),
    m_complete(false)
{
    m_ui->setupUi(this);
    connect(m_ui->collectionClassEdit, SIGNAL(textEdited(QString)), this, SLOT(slotCheckCompleteness()));
    connect(m_ui->pluginNameEdit, SIGNAL(textEdited(QString)), this, SLOT(slotCheckCompleteness()));
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

// Determine name for Q_EXPORT_PLUGIN
static inline QString createPluginName(const QString &prefix)
{
    return prefix.toLower() + QLatin1String("plugin");
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

void CustomWidgetPluginWizardPage::on_collectionClassEdit_textChanged()
{
    const QString collectionClass = collectionClassName();
    m_ui->collectionHeaderEdit->setText(m_fileNamingParameters.headerFileName(collectionClass));
    m_ui->pluginNameEdit->setText(createPluginName(collectionClass));
}

void CustomWidgetPluginWizardPage::on_collectionHeaderEdit_textChanged()
{
    m_ui->collectionSourceEdit->setText(m_fileNamingParameters.headerToSourceFileName(m_ui->collectionHeaderEdit->text()));
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
        if (m_classCount > 1) {
            completeNow = !collectionClassName().isEmpty();
        } else {
            completeNow = true;
        }
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

void CustomWidgetPluginWizardPage::changeEvent(QEvent *e)
{
    QWizardPage::changeEvent(e);
    switch (e->type()) {
            case QEvent::LanguageChange:
        m_ui->retranslateUi(this);
        break;
            default:
        break;
    }
}

} // namespace Internal
} // namespace Qt4ProjectManager
