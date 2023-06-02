// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "customwidgetwidgetswizardpage.h"
#include "classdefinition.h"
#include "classlist.h"
#include "../qmakeprojectmanagertr.h"

#include <utils/layoutbuilder.h>
#include <utils/utilsicons.h>
#include <utils/wizard.h>

#include <QIcon>
#include <QLabel>
#include <QStackedLayout>
#include <QTimer>
#include <QToolButton>

namespace QmakeProjectManager {
namespace Internal {

CustomWidgetWidgetsWizardPage::CustomWidgetWidgetsWizardPage(QWidget *parent) :
    QWizardPage(parent),
    m_tabStackLayout(new QStackedLayout),
    m_complete(false)
{
    auto classListLabel = new QLabel(Tr::tr("Widget &Classes:"));
    auto addButton = new QToolButton;
    addButton->setIcon(Utils::Icons::PLUS.icon());
    m_deleteButton = new QToolButton;
    m_deleteButton->setIcon(Utils::Icons::MINUS.icon());
    m_deleteButton->setEnabled(false);
    m_classList = new ClassList;
    classListLabel->setBuddy(m_classList);

    // Disabled dummy for <new class> column>.
    auto *dummy = new ClassDefinition;
    dummy->setFileNamingParameters(m_fileNamingParameters);
    dummy->setEnabled(false);
    m_tabStackLayout->addWidget(dummy);

    using namespace Layouting;
    Column {
        Tr::tr("Specify the list of custom widgets and their properties."),
        Space(10),
        Row {
            Column {
                Row { classListLabel, addButton, m_deleteButton },
                m_classList,
            },
            m_tabStackLayout,
        }
    }.attachTo(this);

    connect(m_deleteButton, &QAbstractButton::clicked, m_classList, &ClassList::removeCurrentClass);
    connect(addButton, &QAbstractButton::clicked, m_classList, &ClassList::startEditingNewClassItem);
    connect(m_classList, &ClassList::currentRowChanged,
            this, &CustomWidgetWidgetsWizardPage::slotCurrentRowChanged);
    connect(m_classList, &ClassList::classAdded,
            this, &CustomWidgetWidgetsWizardPage::slotClassAdded);
    connect(m_classList, &ClassList::classDeleted,
            this, &CustomWidgetWidgetsWizardPage::slotClassDeleted);
    connect(m_classList, &ClassList::classRenamed,
            this, &CustomWidgetWidgetsWizardPage::slotClassRenamed);

    setProperty(Utils::SHORT_TITLE_PROPERTY, Tr::tr("Custom Widgets"));
}

bool CustomWidgetWidgetsWizardPage::isComplete() const
{
    return m_complete;
}

void CustomWidgetWidgetsWizardPage::initializePage()
{
    // Takes effect only if visible.
    QTimer::singleShot(0, m_classList, &ClassList::startEditingNewClassItem);
}

void CustomWidgetWidgetsWizardPage::slotCurrentRowChanged(int row)
{
    const bool onDummyItem = row == m_tabStackLayout->count() - 1;
    m_deleteButton->setEnabled(!onDummyItem);
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
    return m_classList->className(i);
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
