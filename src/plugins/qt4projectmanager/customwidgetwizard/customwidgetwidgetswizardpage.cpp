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

#include "customwidgetwidgetswizardpage.h"
#include "ui_customwidgetwidgetswizardpage.h"
#include "plugingenerator.h"
#include "classdefinition.h"

#include <QtCore/QFileInfo>

#include <QtGui/QStackedLayout>

namespace Qt4ProjectManager {
namespace Internal {

CustomWidgetWidgetsWizardPage::CustomWidgetWidgetsWizardPage(QWidget *parent) :
    QWizardPage(parent),
    m_ui(new Ui::CustomWidgetWidgetsWizardPage),
    m_complete(false)
{
    m_ui->setupUi(this);
    m_tabStack = new QStackedLayout(m_ui->tabStackWidget);
    m_dummyTab = new QWidget(m_ui->tabStackWidget);
    m_tabStack->addWidget(m_dummyTab);
    connect(m_ui->classList, SIGNAL(currentRowChanged(int)), m_tabStack, SLOT(setCurrentIndex(int)));
}

CustomWidgetWidgetsWizardPage::~CustomWidgetWidgetsWizardPage()
{
    delete m_ui;
}

bool CustomWidgetWidgetsWizardPage::isComplete() const
{
    return m_complete;
}

void CustomWidgetWidgetsWizardPage::on_classList_classAdded(const QString &name)
{
    ClassDefinition *cdef = new ClassDefinition(m_ui->tabStackWidget);
    cdef->setFileNamingParameters(m_fileNamingParameters);
    const int index = m_uiClassDefs.count();
    m_tabStack->insertWidget(index, cdef);
    m_tabStack->setCurrentIndex(index);
    m_uiClassDefs.append(cdef);
    cdef->on_libraryRadio_toggled();
    on_classList_classRenamed(index, name);
    // First class or collection class, re-check.
    slotCheckCompleteness();
}

void CustomWidgetWidgetsWizardPage::on_classList_classDeleted(int index)
{
    delete m_tabStack->widget(index);
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
    return m_ui->classList->item(i)->text();
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
