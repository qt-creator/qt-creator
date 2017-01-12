/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "changestyleaction.h"

#include <projectexplorer/project.h>
#include <projectexplorer/session.h>

#include <QComboBox>
#include <QSettings>

namespace QmlDesigner {

static QString styleConfigFileName(const QString &qmlFileName)
{
    ProjectExplorer::Project *currentProject = ProjectExplorer::SessionManager::projectForFile(Utils::FileName::fromString(qmlFileName));

    if (currentProject)
        foreach (const QString &fileName, currentProject->files(ProjectExplorer::Project::SourceFiles))
            if (fileName.endsWith("qtquickcontrols2.conf"))
                return fileName;

    return QString();
}

ChangeStyleWidgetAction::ChangeStyleWidgetAction(QObject *parent) : QWidgetAction(parent)
{
}

void ChangeStyleWidgetAction::handleModelUpdate(const QString &style)
{
    emit modelUpdated(style);
}

const char enabledTooltip[] = QT_TRANSLATE_NOOP("ChangeStyleWidgetAction",
                                                "Change style for Qt Quick Controls 2.");
const char disbledTooltip[] = QT_TRANSLATE_NOOP("ChangeStyleWidgetAction",
                                                "Change style for Qt Quick Controls 2. Configuration file qtquickcontrols2.conf not found.");

QWidget *ChangeStyleWidgetAction::createWidget(QWidget *parent)
{
    QComboBox *comboBox = new QComboBox(parent);
    comboBox->setToolTip(tr(enabledTooltip));
    comboBox->addItem("Default");
    comboBox->addItem("Material");
    comboBox->addItem("Universal");
    comboBox->setEditable(true);
    comboBox->setCurrentIndex(0);

    connect(this, &ChangeStyleWidgetAction::modelUpdated, comboBox, [comboBox](const QString &style) {

        if (!comboBox)
            return;

        bool block = comboBox->blockSignals(true);

        if (style.isEmpty()) { /* The .conf file is misssing. */
            comboBox->setDisabled(true);
            comboBox->setToolTip(tr(disbledTooltip));
            comboBox->setCurrentIndex(0);
        } else {
            comboBox->setDisabled(false);
            comboBox->setToolTip(tr(enabledTooltip));
            comboBox->setEditText(style);
        }

        comboBox->blockSignals(block);
    });

    connect(comboBox,
            static_cast<void (QComboBox::*)(const QString &)>(&QComboBox::activated),
            this,
            [comboBox, this](const QString &style) {

        if (style.isEmpty())
            return;

        const Utils::FileName configFileName = Utils::FileName::fromString(styleConfigFileName(qmlFileName));

        if (configFileName.exists()) {
             QSettings infiFile(configFileName.toString(), QSettings::IniFormat);
             infiFile.setValue("Controls/Style", style);
             if (view)
                 view->resetPuppet();
        }
    });

    return comboBox;
}

void ChangeStyleAction::currentContextChanged(const SelectionContext &selectionContext)
{
    AbstractView *view = selectionContext.view();
    if (view && view->model()) {
        m_action->view = view;

        const QString fileName = view->model()->fileUrl().toLocalFile();

        if (m_action->qmlFileName == fileName)
            return;

        m_action->qmlFileName = fileName;

        const QString confFileName = styleConfigFileName(fileName);

        if (Utils::FileName::fromString(confFileName).exists()) {
            QSettings infiFile(confFileName, QSettings::IniFormat);
            m_action->handleModelUpdate(infiFile.value("Controls/Style", "Default").toString());
        } else {
            m_action->handleModelUpdate("");
        }

    }
}

} // namespace QmlDesigner
