// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "customwidgetwidgetswizardpage.h"
#include "ui_customwidgetwidgetswizardpage.h"
#include "classdefinition.h"

#include <utils/utilsicons.h>
#include <utils/wizard.h>

#include <QTimer>

#include <QStackedLayout>
#include <QIcon>

namespace QmakeProjectManager {
namespace Internal {

CustomWidgetWidgetsWizardPage::CustomWidgetWidgetsWizardPage(QWidget *parent) :
    QWizardPage(parent),
    m_ui(new Ui::CustomWidgetWidgetsWizardPage),
    m_tabStackLayout(new QStackedLayout),
    m_complete(false)
{
    m_ui->setupUi(this);
    m_ui->tabStackWidget->setLayout(m_tabStackLayout);
    m_ui->addButton->setIcon(Utils::Icons::PLUS_TOOLBAR.icon());
    connect(m_ui->addButton, &QAbstractButton::clicked, m_ui->classList, &ClassList::startEditingNewClassItem);
    m_ui->deleteButton->setIcon(Utils::Icons::MINUS_TOOLBAR.icon());
    connect(m_ui->deleteButton, &QAbstractButton::clicked, m_ui->classList, &ClassList::removeCurrentClass);
    m_ui->deleteButton->setEnabled(false);

    // Disabled dummy for <new class> column>.
    auto *dummy = new ClassDefinition;
    dummy->setFileNamingParameters(m_fileNamingParameters);
    dummy->setEnabled(false);
    m_tabStackLayout->addWidget(dummy);

    connect(m_ui->classList, &ClassList::currentRowChanged,
            this, &CustomWidgetWidgetsWizardPage::slotCurrentRowChanged);
    connect(m_ui->classList, &ClassList::classAdded,
            this, &CustomWidgetWidgetsWizardPage::slotClassAdded);
    connect(m_ui->classList, &ClassList::classDeleted,
            this, &CustomWidgetWidgetsWizardPage::slotClassDeleted);
    connect(m_ui->classList, &ClassList::classRenamed,
            this, &CustomWidgetWidgetsWizardPage::slotClassRenamed);

    setProperty(Utils::SHORT_TITLE_PROPERTY, tr("Custom Widgets"));
}

CustomWidgetWidgetsWizardPage::~CustomWidgetWidgetsWizardPage()
{
    delete m_ui;
}

bool CustomWidgetWidgetsWizardPage::isComplete() const
{
    return m_complete;
}

void CustomWidgetWidgetsWizardPage::initializePage()
{
    // Takes effect only if visible.
    QTimer::singleShot(0, m_ui->classList, &ClassList::startEditingNewClassItem);
}

void CustomWidgetWidgetsWizardPage::slotCurrentRowChanged(int row)
{
    const bool onDummyItem = row == m_tabStackLayout->count() - 1;
    m_ui->deleteButton->setEnabled(!onDummyItem);
    m_tabStackLayout->setCurrentIndex(row);
}

void CustomWidgetWidgetsWizardPage::slotClassAdded(const QString &name)
{
    auto *cdef = new ClassDefinition;
    cdef->setFileNamingParameters(m_fileNamingParameters);
    const int index = m_uiClassDefs.count();
    m_tabStackLayout->insertWidget(index, cdef);
    m_tabStackLayout->setCurrentIndex(index);
    m_uiClassDefs.append(cdef);
    cdef->enableButtons();
    slotClassRenamed(index, name);
    // First class or collection class, re-check.
    slotCheckCompleteness();
}

void CustomWidgetWidgetsWizardPage::slotClassDeleted(int index)
{
    delete m_tabStackLayout->widget(index);
    m_uiClassDefs.removeAt(index);
    if (m_uiClassDefs.empty())
        slotCheckCompleteness();
}

void CustomWidgetWidgetsWizardPage::slotClassRenamed(int index, const QString &name)
{
    m_uiClassDefs[index]->setClassName(name);
}

QString CustomWidgetWidgetsWizardPage::classNameAt(int i) const
{
    return m_ui->classList->className(i);
}

QList<PluginOptions::WidgetOptions> CustomWidgetWidgetsWizardPage::widgetOptions() const
{
    QList<PluginOptions::WidgetOptions> rc;
    for (int i = 0; i < m_uiClassDefs.count(); i++) {
        const ClassDefinition *cdef = m_uiClassDefs[i];
        rc.push_back(cdef->widgetOptions(classNameAt(i)));
    }
    return rc;
}

void CustomWidgetWidgetsWizardPage::slotCheckCompleteness()
{
    // Complete if either a single custom widget or a collection
    // with a collection class name specified.
    bool completeNow = !m_uiClassDefs.isEmpty();
    if (completeNow != m_complete) {
        m_complete = completeNow;
        emit completeChanged();
    }
}
}
}
