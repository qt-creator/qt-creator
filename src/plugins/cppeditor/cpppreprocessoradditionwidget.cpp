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

#include <utils/tooltip/tipcontents.h>
#include <utils/tooltip/tooltip.h>
#include <projectexplorer/project.h>

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
    m_widget = new PreProcessorAdditionWidget();
    m_originalPartAdditions.clear();
    foreach (CppTools::ProjectPart::Ptr projectPart, projectParts) {
        ProjectPartAddition addition;
        addition.projectPart = projectPart;
        m_widget->ui->projectComboBox->addItem(projectPart->displayName);
        addition.additionalDefines = projectPart->project
                ->additionalCppDefines().value(projectPart->projectFile).toByteArray();
        m_originalPartAdditions << addition;
    }
    m_partAdditions = m_originalPartAdditions;

    m_widget->ui->additionalEdit->setPlainText(QLatin1String(
                m_partAdditions[m_widget->ui->projectComboBox->currentIndex()].additionalDefines));

    QPoint pos = parent->mapToGlobal(parent->rect().topRight());
    pos.setX(pos.x() - m_widget->width());
    showInternal(pos, Utils::WidgetContent(m_widget, true), parent, QRect());

    connect(m_widget->ui->additionalEdit, SIGNAL(textChanged()), SLOT(textChanged()));
    connect(m_widget->ui->projectComboBox, SIGNAL(currentIndexChanged(int)),
            SLOT(projectChanged(int)));
    connect(m_widget, SIGNAL(finished()), SLOT(finish()));
    connect(m_widget->ui->buttonBox, SIGNAL(accepted()), SLOT(apply()));
    connect(m_widget->ui->buttonBox, SIGNAL(rejected()), SLOT(cancel()));
}

bool PreProcessorAdditionPopUp::eventFilter(QObject *o, QEvent *event)
{
    // Filter out some events that would hide the widget, when they would be handled by the ToolTip
    switch (event->type()) {
    case QEvent::Leave:
        // This event would hide the ToolTip because the view isn't a child of the WidgetContent
        if (m_widget->ui->projectComboBox->view() == qApp->focusWidget())
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
        if (o == m_widget->ui->projectComboBox->view()->viewport())
            return false;
        break;
    default:
        break;
    }
    return Utils::ToolTip::eventFilter(o, event);
}

void PreProcessorAdditionPopUp::textChanged()
{
    m_partAdditions[m_widget->ui->projectComboBox->currentIndex()].additionalDefines
            = m_widget->ui->additionalEdit->toPlainText().toLatin1();
}


void PreProcessorAdditionPopUp::finish()
{
    m_widget->disconnect(this);
    foreach (ProjectPartAddition partAddition, m_originalPartAdditions) {
        QVariantMap settings = partAddition.projectPart->project->additionalCppDefines();
        if (!settings[partAddition.projectPart->projectFile].toString().isEmpty()
                && !partAddition.additionalDefines.isEmpty()) {
            settings[partAddition.projectPart->projectFile] = partAddition.additionalDefines;
            partAddition.projectPart->project->setAdditionalCppDefines(settings);
        }
    }
    emit finished(m_originalPartAdditions.value(m_widget->ui->projectComboBox->currentIndex())
                  .additionalDefines);
}

void PreProcessorAdditionPopUp::projectChanged(int index)
{
    m_widget->ui->additionalEdit->setPlainText(
                QLatin1String(m_partAdditions[index].additionalDefines));
}

void PreProcessorAdditionPopUp::apply()
{
    m_originalPartAdditions = m_partAdditions;
    hideTipImmediately();
}

void PreProcessorAdditionPopUp::cancel()
{
    m_partAdditions = m_originalPartAdditions;
    hideTipImmediately();
}

PreProcessorAdditionPopUp::PreProcessorAdditionPopUp()
    : m_widget(0)
{}
