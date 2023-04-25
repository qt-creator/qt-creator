// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "customwidgetpluginwizardpage.h"
#include "customwidgetwidgetswizardpage.h"
#include "../qmakeprojectmanagertr.h"

#include <utils/layoutbuilder.h>
#include <utils/wizard.h>

#include <QLabel>
#include <QLineEdit>

namespace QmakeProjectManager {
namespace Internal {

// Determine name for Q_EXPORT_PLUGIN
static inline QString createPluginName(const QString &prefix)
{
    return prefix.toLower() + QLatin1String("plugin");
}

CustomWidgetPluginWizardPage::CustomWidgetPluginWizardPage(QWidget *parent) :
    QWizardPage(parent),
    m_classCount(-1),
    m_complete(false)
{
    m_collectionClassLabel = new QLabel(Tr::tr("Collection class:"));
    m_collectionClassEdit = new QLineEdit;
    m_collectionHeaderLabel = new QLabel(Tr::tr("Collection header file:"));
    m_collectionHeaderEdit = new QLineEdit;
    m_collectionSourceLabel = new QLabel(Tr::tr("Collection source file:"));
    m_collectionSourceEdit = new QLineEdit;
    m_pluginNameEdit = new QLineEdit;
    m_resourceFileEdit = new QLineEdit(Tr::tr("icons.qrc"));

    using namespace Layouting;
    Column {
        Tr::tr("Specify the properties of the plugin library and the collection class."),
        Space(10),
        Form {
            m_collectionClassLabel, m_collectionClassEdit, br,
            m_collectionHeaderLabel, m_collectionHeaderEdit, br,
            m_collectionSourceLabel, m_collectionSourceEdit, br,
            Tr::tr("Plugin name:"), m_pluginNameEdit, br,
            Tr::tr("Resource file:"), m_resourceFileEdit, br,
        }
    }.attachTo(this);

    connect(m_collectionClassEdit, &QLineEdit::textEdited,
            this, &CustomWidgetPluginWizardPage::slotCheckCompleteness);
    connect(m_collectionClassEdit, &QLineEdit::textChanged,
            this, [this](const QString &collectionClass) {
        m_collectionHeaderEdit->setText(m_fileNamingParameters.headerFileName(collectionClass));
        m_pluginNameEdit->setText(createPluginName(collectionClass));
    });
    connect(m_pluginNameEdit, &QLineEdit::textEdited,
            this, &CustomWidgetPluginWizardPage::slotCheckCompleteness);
    connect(m_collectionHeaderEdit, &QLineEdit::textChanged,
            this, [this](const QString &text) {
        m_collectionSourceEdit->setText(m_fileNamingParameters.headerToSourceFileName(text));
    });

    setProperty(Utils::SHORT_TITLE_PROPERTY, Tr::tr("Plugin Details"));
}

QString CustomWidgetPluginWizardPage::collectionClassName() const
{
    return m_collectionClassEdit->text();
}

QString CustomWidgetPluginWizardPage::pluginName() const
{
    return m_pluginNameEdit->text();
}

void CustomWidgetPluginWizardPage::init(const CustomWidgetWidgetsWizardPage *widgetsPage)
{
    m_classCount = widgetsPage->classCount();
    const QString empty;
    if (m_classCount == 1) {
        m_pluginNameEdit->setText(createPluginName(widgetsPage->classNameAt(0)));
        setCollectionEnabled(false);
    } else {
        m_pluginNameEdit->setText(empty);
        setCollectionEnabled(true);
    }
    m_collectionClassEdit->setText(empty);
    m_collectionHeaderEdit->setText(empty);
    m_collectionSourceEdit->setText(empty);

    slotCheckCompleteness();
}

void CustomWidgetPluginWizardPage::setCollectionEnabled(bool enColl)
{
    m_collectionClassLabel->setEnabled(enColl);
    m_collectionClassEdit->setEnabled(enColl);
    m_collectionHeaderLabel->setEnabled(enColl);
    m_collectionHeaderEdit->setEnabled(enColl);
    m_collectionSourceLabel->setEnabled(enColl);
    m_collectionSourceEdit->setEnabled(enColl);
}

QSharedPointer<PluginOptions> CustomWidgetPluginWizardPage::basicPluginOptions() const
{
    QSharedPointer<PluginOptions> po(new PluginOptions);
    po->pluginName = pluginName();
    po->resourceFile = m_resourceFileEdit->text();
    po->collectionClassName = collectionClassName();
    po->collectionHeaderFile = m_collectionHeaderEdit->text();
    po->collectionSourceFile = m_collectionSourceEdit->text();
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
