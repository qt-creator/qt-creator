/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "modulespage.h"

#include <utils/wizard.h>

#include <qmakeprojectmanager/qtmodulesinfo.h>

#include <QDebug>

#include <QCheckBox>
#include <QLabel>
#include <QLayout>
#include <QWidget>

#include <math.h>

using namespace QmakeProjectManager::Internal;

ModulesPage::ModulesPage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle(tr("Select Required Modules"));
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

    setProperty(Utils::SHORT_TITLE_PROPERTY, tr("Modules"));
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

QStringList ModulesPage::selectedModulesList() const
{
    return modules(true);
}

QStringList ModulesPage::deselectedModulesList() const
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

QStringList ModulesPage::modules(bool selected) const
{
    QStringList modules;
    foreach (const QString &module, QtModulesInfo::modules()) {
        if (selected != QtModulesInfo::moduleIsDefault(module)
            && selected == field(module).toBool())
            modules << module;
    }
    return modules;
}
