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

#include "cpppreprocessoradditionwidget.h"
#include "ui_cpppreprocessoradditionwidget.h"

#include "cppsnippetprovider.h"

#include "utils/tooltip/tipcontents.h"
#include "utils/tooltip/tooltip.h"
#include "projectexplorer/project.h"

#include <QDebug>

using namespace CppEditor::Internal;

PreProcessorAdditionWidget::PreProcessorAdditionWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::CppPreProcessorAdditionWidget)
{
    ui->setupUi(this);
    CppEditor::Internal::CppSnippetProvider prov;
    prov.decorateEditor(ui->additionalEdit);
    setAttribute(Qt::WA_QuitOnClose, false);
    setFocusPolicy(Qt::StrongFocus);
}

PreProcessorAdditionWidget::~PreProcessorAdditionWidget()
{
    emit finished();
    delete ui;
}

PreProcessorAdditionPopUp *PreProcessorAdditionPopUp::instance()
{
    static PreProcessorAdditionPopUp inst;
    return &inst;
}

void PreProcessorAdditionPopUp::show(QWidget *parent,
                                     const QList<CppTools::ProjectPart::Ptr> &projectParts)
{
    widget = new PreProcessorAdditionWidget();
    originalPartAdditions.clear();
    foreach (CppTools::ProjectPart::Ptr projectPart, projectParts) {
        ProjectPartAddition addition;
        addition.projectPart = projectPart;
        widget->ui->projectComboBox->addItem(projectPart->displayName);
        addition.additionalDefines = projectPart->project
                ->additionalCppDefines().value(projectPart->projectFile).toByteArray();
        originalPartAdditions << addition;
    }
    partAdditions = originalPartAdditions;

    widget->ui->additionalEdit->setPlainText(QLatin1String(
                partAdditions[widget->ui->projectComboBox->currentIndex()].additionalDefines));

    QPoint pos = parent->mapToGlobal(parent->rect().topRight());
    pos.setX(pos.x() - widget->width());
    showInternal(pos, Utils::WidgetContent(widget, true), parent, QRect());

    connect(widget->ui->additionalEdit, SIGNAL(textChanged()), SLOT(textChanged()));
    connect(widget->ui->projectComboBox, SIGNAL(currentIndexChanged(int)),
            SLOT(projectChanged(int)));
    connect(widget, SIGNAL(finished()), SLOT(finish()));
    connect(widget->ui->buttonBox, SIGNAL(accepted()), SLOT(apply()));
    connect(widget->ui->buttonBox, SIGNAL(rejected()), SLOT(cancel()));
}

bool PreProcessorAdditionPopUp::eventFilter(QObject *o, QEvent *event)
{
    // Filter out some events that would hide the widget, when they would be handled by the ToolTip
    switch (event->type()) {
    case QEvent::Leave:
        // This event would hide the ToolTip because the view isn't a child of the WidgetContent
        if (widget->ui->projectComboBox->view() == qApp->focusWidget())
            return false;
        break;
    case QEvent::KeyPress:
    case QEvent::KeyRelease:
    case QEvent::ShortcutOverride:
        // Catch the escape key to close the widget
        if (static_cast<QKeyEvent *>(event)->key() == Qt::Key_Escape) {
            hideTipImmediately();
            return true;
        }
        break;
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonDblClick:
    case QEvent::Wheel:
        // This event would hide the ToolTip because the viewport isn't a child of the WidgetContent
        if (o == widget->ui->projectComboBox->view()->viewport())
            return false;
        break;
    default:
        break;
    }
    return Utils::ToolTip::eventFilter(o, event);
}

void PreProcessorAdditionPopUp::textChanged()
{
    partAdditions[widget->ui->projectComboBox->currentIndex()].additionalDefines
            = widget->ui->additionalEdit->toPlainText().toLatin1();
}


void PreProcessorAdditionPopUp::finish()
{
    widget->disconnect(this);
    foreach (ProjectPartAddition partAddition, originalPartAdditions) {
        QVariantMap settings = partAddition.projectPart->project->additionalCppDefines();
        if (!settings[partAddition.projectPart->projectFile].toString().isEmpty()
                && !partAddition.additionalDefines.isEmpty()) {
            settings[partAddition.projectPart->projectFile] = partAddition.additionalDefines;
            partAddition.projectPart->project->setAdditionalCppDefines(settings);
        }
    }
    emit finished(originalPartAdditions[widget->ui->projectComboBox->currentIndex()].additionalDefines);
}

void PreProcessorAdditionPopUp::projectChanged(int index)
{
    widget->ui->additionalEdit->setPlainText(QLatin1String(partAdditions[index].additionalDefines));
}

void PreProcessorAdditionPopUp::apply()
{
    originalPartAdditions = partAdditions;
    hideTipImmediately();
}

void PreProcessorAdditionPopUp::cancel()
{
    partAdditions = originalPartAdditions;
    hideTipImmediately();
}

PreProcessorAdditionPopUp::PreProcessorAdditionPopUp()
    : widget(0)
{}
