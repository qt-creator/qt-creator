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

#include "modulespage.h"

#include "qtmodulesinfo.h"

#include <utils/qtcassert.h>

#include <QtCore/QDebug>
#include <QtCore/QVariant>

#include <QtGui/QCheckBox>
#include <QtGui/QLabel>
#include <QtGui/QLayout>
#include <QtGui/QWidget>

#include <math.h>

using namespace Qt4ProjectManager::Internal;

ModulesPage::ModulesPage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle(tr("Select required modules"));
    QLabel *label = new QLabel(tr("Select the modules you want to include in your "
        "project. The recommended modules for this project are selected by default."));
    label->setWordWrap(true);

    QVBoxLayout *vlayout = new QVBoxLayout();
    vlayout->addWidget(label);
    vlayout->addItem(new QSpacerItem(0, 20));

    QGridLayout *layout = new QGridLayout;

    const QStringList &modulesList = QtModulesInfo::modules();
    int moduleId = 0;
    int rowsCount = (modulesList.count() + 1) / 2;
    foreach (const QString &module, modulesList) {
        QCheckBox *moduleCheckBox = new QCheckBox(QtModulesInfo::moduleName(module));
        moduleCheckBox->setToolTip(QtModulesInfo::moduleDescription(module));
        moduleCheckBox->setWhatsThis(QtModulesInfo::moduleDescription(module));
        registerField(module, moduleCheckBox);
        int row = moduleId % rowsCount;
        int column = moduleId / rowsCount;
        layout->addWidget(moduleCheckBox, row, column);
        m_moduleCheckBoxMap[module] = moduleCheckBox;
        moduleId++;
    }

    vlayout->addLayout(layout);
    setLayout(vlayout);
}

// Return the key that goes into the Qt config line for a module
QString ModulesPage::idOfModule(const QString &module)
{
    const QStringList &moduleIdList = QtModulesInfo::modules();
    foreach (const QString &id, moduleIdList)
        if (QtModulesInfo::moduleName(id).startsWith(module))
            return id;
    return QString();
}

QString ModulesPage::selectedModules() const
{
    return modules(true);
}

QString ModulesPage::deselectedModules() const
{
    return modules(false);
}

void ModulesPage::setModuleSelected(const QString &module, bool selected) const
{
    QCheckBox *checkBox = m_moduleCheckBoxMap[module];
    Q_ASSERT(checkBox);
    checkBox->setCheckState(selected?Qt::Checked:Qt::Unchecked);
}

void ModulesPage::setModuleEnabled(const QString &module, bool enabled) const
{
    QCheckBox *checkBox = m_moduleCheckBoxMap[module];
    Q_ASSERT(checkBox);
    checkBox->setEnabled(enabled);
}

QString ModulesPage::modules(bool selected) const
{
    QStringList modules;
    foreach (const QString &module, QtModulesInfo::modules()) {
        if (selected != QtModulesInfo::moduleIsDefault(module)
            && selected == field(module).toBool())
            modules << module;
    }
    return modules.join(QString(QLatin1Char(' ')));
}
