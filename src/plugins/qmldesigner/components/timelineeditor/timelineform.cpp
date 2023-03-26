// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "timelineform.h"
#include "ui_timelineform.h"

#include <abstractview.h>
#include <bindingproperty.h>
#include <exception>
#include <nodelistproperty.h>
#include <nodemetainfo.h>
#include <rewritertransaction.h>
#include <variantproperty.h>

#include <coreplugin/messagebox.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

namespace QmlDesigner {

TimelineForm::TimelineForm(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::TimelineForm)
{
    ui->setupUi(this);

    connect(ui->expressionBindingLineEdit, &QLineEdit::editingFinished, [this]() {
        QTC_ASSERT(m_timeline.isValid(), return );

        const QString bindingText = ui->expressionBindingLineEdit->text();
        if (bindingText.isEmpty()) {
            ui->animation->setChecked(true);
            try {
                m_timeline.modelNode().removeProperty("currentFrame");
            } catch (const Exception &e) {
                e.showException();
            }
            return;
        }

        ui->expressionBinding->setChecked(true);

        try {
            m_timeline.modelNode()
                .bindingProperty("currentFrame")
                .setExpression(bindingText);
        } catch (const Exception &e) {
            e.showException();
        }
    });

    connect(ui->idLineEdit, &QLineEdit::editingFinished, [this]() {
        QTC_ASSERT(m_timeline.isValid(), return );

        static QString lastString;

        const QString newId = ui->idLineEdit->text();

        if (newId == lastString)
            return;

        lastString = newId;

        if (newId == m_timeline.modelNode().id())
            return;

        bool error = false;

        if (!ModelNode::isValidId(newId)) {
            Core::AsynchronousMessageBox::warning(tr("Invalid Id"),
                                                  tr("%1 is an invalid id.").arg(newId));
            error = true;
        } else if (m_timeline.view()->hasId(newId)) {
            Core::AsynchronousMessageBox::warning(tr("Invalid Id"),
                                                  tr("%1 already exists.").arg(newId));
            error = true;
        } else {
            m_timeline.modelNode().setIdWithRefactoring(newId);
        }

        if (error) {
            lastString.clear();
            ui->idLineEdit->setText(m_timeline.modelNode().id());
        }
    });

    connectSpinBox(ui->startFrame, "startFrame");
    connectSpinBox(ui->endFrame, "endFrame");
}

TimelineForm::~TimelineForm()
{
    delete ui;
}

void TimelineForm::setTimeline(const QmlTimeline &timeline)
{
    m_timeline = timeline;

    ui->expressionBindingLineEdit->clear();

    if (m_timeline.isValid()) {
        ui->idLineEdit->setText(m_timeline.modelNode().displayName());
        ui->startFrame->setValue(
            m_timeline.modelNode().variantProperty("startFrame").value().toInt());
        ui->endFrame->setValue(m_timeline.modelNode().variantProperty("endFrame").value().toInt());

        if (m_timeline.modelNode().hasBindingProperty("currentFrame")) {
            ui->expressionBindingLineEdit->setText(
                m_timeline.modelNode().bindingProperty("currentFrame").expression());
            ui->expressionBinding->setChecked(true);
        } else {
            ui->expressionBinding->setChecked(false);
        }
    }
}

QmlTimeline TimelineForm::timeline() const
{
    return m_timeline;
}

void TimelineForm::setHasAnimation(bool b)
{
    ui->expressionBinding->setChecked(!b);
    ui->animation->setChecked(b);
    ui->expressionBindingLineEdit->setDisabled(b);
}

void TimelineForm::setProperty(const PropertyName &propertyName, const QVariant &value)
{
    QTC_ASSERT(m_timeline.isValid(), return );

    try {
        m_timeline.modelNode().variantProperty(propertyName).setValue(value);
    } catch (const Exception &e) {
        e.showException();
    }
}

void TimelineForm::connectSpinBox(QSpinBox *spinBox, const PropertyName &propertyName)
{
    connect(spinBox, &QSpinBox::editingFinished, [this, propertyName, spinBox]() {
        setProperty(propertyName, spinBox->value());
    });
}

} // namespace QmlDesigner
