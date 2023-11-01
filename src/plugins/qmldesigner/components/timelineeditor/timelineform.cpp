// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "timelineform.h"

#include <abstractview.h>
#include <bindingproperty.h>
#include <exception>
#include <nodelistproperty.h>
#include <nodemetainfo.h>
#include <rewritertransaction.h>
#include <variantproperty.h>

#include <coreplugin/messagebox.h>

#include <utils/algorithm.h>
#include <utils/layoutbuilder.h>
#include <utils/qtcassert.h>

#include <QLabel>
#include <QLineEdit>
#include <QRadioButton>
#include <QSpinBox>

namespace QmlDesigner {

TimelineForm::TimelineForm(QWidget *parent)
    : QWidget(parent)
{
    constexpr int minimumLabelWidth = 160;
    constexpr int spinBoxWidth = 80;

    auto mainL = new QLabel(tr("Timeline Settings"));
    QFont f = mainL->font();
    f.setBold(true);
    mainL->setFont(f);

    auto idL = new QLabel(tr("Timeline ID:"));
    idL->setToolTip(tr("Name for the timeline."));
    m_idLineEdit = new QLineEdit;

    auto startFrameL = new QLabel(tr("Start frame:"));
    startFrameL->setToolTip(tr("First frame of the timeline. Negative numbers are allowed."));
    m_startFrame = new QSpinBox;
    m_startFrame->setFixedWidth(spinBoxWidth);
    m_startFrame->setRange(-100000, 100000);

    auto endFrameL = new QLabel(tr("End frame:"));
    endFrameL->setToolTip(tr("Last frame of the timeline."));
    m_endFrame = new QSpinBox;
    m_endFrame->setFixedWidth(spinBoxWidth);
    m_endFrame->setRange(-100000, 100000);

    m_expressionBinding = new QRadioButton(tr("Expression binding"));
    m_expressionBinding->setToolTip(tr("To create an expression binding animation, delete all animations from this timeline."));
    m_expressionBinding->setEnabled(false);

    m_animation = new QRadioButton(tr("Animation"));
    m_animation->setEnabled(false);
    m_animation->setChecked(true);

    QSizePolicy sp(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);

    auto expressionBindingL = new QLabel(tr("Expression binding:"));
    expressionBindingL->setToolTip(tr("Sets the expression to bind the current keyframe to."));
    expressionBindingL->setMinimumWidth(minimumLabelWidth);
    m_expressionBindingLineEdit = new QLineEdit;
    m_expressionBindingLineEdit->setMinimumWidth(240);
    sp.setHorizontalStretch(2);
    m_expressionBindingLineEdit->setSizePolicy(sp);

    auto str = new QWidget;
    sp.setHorizontalStretch(1);
    str->setSizePolicy(sp);

    using namespace Layouting;
    Grid {
        Span(2, mainL), br,
        idL, m_idLineEdit, br,
        empty(), Row { startFrameL, m_startFrame, st(), endFrameL, m_endFrame }, str, br,
        empty(), Row { m_expressionBinding, m_animation, st() }, br,
        expressionBindingL, m_expressionBindingLineEdit, br,
    }.attachTo(this);

    connect(m_expressionBindingLineEdit, &QLineEdit::editingFinished, [this]() {
        QTC_ASSERT(m_timeline.isValid(), return );

        const QString bindingText = m_expressionBindingLineEdit->text();
        if (bindingText.isEmpty()) {
            m_animation->setChecked(true);
            try {
                m_timeline.modelNode().removeProperty("currentFrame");
            } catch (const Exception &e) {
                e.showException();
            }
            return;
        }

        m_expressionBinding->setChecked(true);

        try {
            m_timeline.modelNode()
                .bindingProperty("currentFrame")
                .setExpression(bindingText);
        } catch (const Exception &e) {
            e.showException();
        }
    });

    connect(m_idLineEdit, &QLineEdit::editingFinished, [this]() {
        QTC_ASSERT(m_timeline.isValid(), return );

        static QString lastString;

        const QString newId = m_idLineEdit->text();

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
            m_idLineEdit->setText(m_timeline.modelNode().id());
        }
    });

    connectSpinBox(m_startFrame, "startFrame");
    connectSpinBox(m_endFrame, "endFrame");
}

void TimelineForm::setTimeline(const QmlTimeline &timeline)
{
    m_timeline = timeline;

    m_expressionBindingLineEdit->clear();

    if (m_timeline.isValid()) {
        m_idLineEdit->setText(m_timeline.modelNode().displayName());
        m_startFrame->setValue(
            m_timeline.modelNode().variantProperty("startFrame").value().toInt());
        m_endFrame->setValue(m_timeline.modelNode().variantProperty("endFrame").value().toInt());

        if (m_timeline.modelNode().hasBindingProperty("currentFrame")) {
            m_expressionBindingLineEdit->setText(
                m_timeline.modelNode().bindingProperty("currentFrame").expression());
            m_expressionBinding->setChecked(true);
        } else {
            m_expressionBinding->setChecked(false);
        }
    }
}

QmlTimeline TimelineForm::timeline() const
{
    return m_timeline;
}

void TimelineForm::setHasAnimation(bool b)
{
    m_expressionBinding->setChecked(!b);
    m_animation->setChecked(b);
    m_expressionBindingLineEdit->setDisabled(b);
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
