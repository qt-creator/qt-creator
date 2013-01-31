/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "customwidgetwidgetswizardpage.h"
#include "ui_customwidgetwidgetswizardpage.h"
#include "plugingenerator.h"
#include "classdefinition.h"

#include <coreplugin/coreconstants.h>

#include <QFileInfo>
#include <QTimer>

#include <QStackedLayout>
#include <QIcon>

namespace Qt4ProjectManager {
namespace Internal {

CustomWidgetWidgetsWizardPage::CustomWidgetWidgetsWizardPage(QWidget *parent) :
    QWizardPage(parent),
    m_ui(new Ui::CustomWidgetWidgetsWizardPage),
    m_tabStackLayout(new QStackedLayout),
    m_complete(false)
{
    m_ui->setupUi(this);
    m_ui->tabStackWidget->setLayout(m_tabStackLayout);
    m_ui->addButton->setIcon(QIcon(QLatin1String(Core::Constants::ICON_PLUS)));
    connect(m_ui->addButton, SIGNAL(clicked()), m_ui->classList, SLOT(startEditingNewClassItem()));
    m_ui->deleteButton->setIcon(QIcon(QLatin1String(Core::Constants::ICON_MINUS)));
    connect(m_ui->deleteButton, SIGNAL(clicked()), m_ui->classList, SLOT(removeCurrentClass()));
    m_ui->deleteButton->setEnabled(false);

    // Disabled dummy for <new class> column>.
    ClassDefinition *dummy = new ClassDefinition;
    dummy->setFileNamingParameters(m_fileNamingParameters);
    dummy->setEnabled(false);
    m_tabStackLayout->addWidget(dummy);

    connect(m_ui->classList, SIGNAL(currentRowChanged(int)),
            this, SLOT(slotCurrentRowChanged(int)));
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
    QTimer::singleShot(0, m_ui->classList, SLOT(startEditingNewClassItem()));
}

void CustomWidgetWidgetsWizardPage::slotCurrentRowChanged(int row)
{
    const bool onDummyItem = row == m_tabStackLayout->count() - 1;
    m_ui->deleteButton->setEnabled(!onDummyItem);
    m_tabStackLayout->setCurrentIndex(row);
}

void CustomWidgetWidgetsWizardPage::on_classList_classAdded(const QString &name)
{
    ClassDefinition *cdef = new ClassDefinition;
    cdef->setFileNamingParameters(m_fileNamingParameters);
    const int index = m_uiClassDefs.count();
    m_tabStackLayout->insertWidget(index, cdef);
    m_tabStackLayout->setCurrentIndex(index);
    m_uiClassDefs.append(cdef);
    cdef->enableButtons();
    on_classList_classRenamed(index, name);
    // First class or collection class, re-check.
    slotCheckCompleteness();
}

void CustomWidgetWidgetsWizardPage::on_classList_classDeleted(int index)
{
    delete m_tabStackLayout->widget(index);
    m_uiClassDefs.removeAt(index);
    if (m_uiClassDefs.empty())
        slotCheckCompleteness();
}

void CustomWidgetWidgetsWizardPage::on_classList_classRenamed(int index, const QString &name)
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
