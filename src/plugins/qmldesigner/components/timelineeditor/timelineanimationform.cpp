// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "timelineanimationform.h"

#include <abstractview.h>
#include <bindingproperty.h>
#include <exception>
#include <nodelistproperty.h>
#include <nodemetainfo.h>
#include <rewritertransaction.h>
#include <signalhandlerproperty.h>
#include <variantproperty.h>
#include <qmlitemnode.h>
#include <qmlobjectnode.h>

#include <coreplugin/messagebox.h>

#include <utils/algorithm.h>
#include <utils/layoutbuilder.h>
#include <utils/qtcassert.h>

#include <QCheckBox>
#include <QSpinBox>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>

namespace QmlDesigner {

TimelineAnimationForm::TimelineAnimationForm(QWidget *parent)
    : QWidget(parent)
{
    constexpr int minimumLabelWidth = 160;
    constexpr int spinBoxWidth = 80;

    auto mainL = new QLabel(tr("Animation Settings"));
    QFont f = mainL->font();
    f.setBold(true);
    mainL->setFont(f);

    auto idL = new QLabel(tr("Animation ID:"));
    idL->setToolTip(tr("Name for the animation."));
    idL->setMinimumWidth(minimumLabelWidth);
    m_idLineEdit = new QLineEdit;

    auto runningL = new QLabel(tr("Running in base state"));
    runningL->setToolTip(
        tr("Runs the animation automatically when the base state is active."));
    m_running = new QCheckBox;
    m_running->setEnabled(false);

    auto startFrameL = new QLabel(tr("Start frame:"));
    startFrameL->setToolTip(tr("First frame of the animation."));
    m_startFrame = new QSpinBox;
    m_startFrame->setFixedWidth(spinBoxWidth);
    m_startFrame->setRange(-100000, 100000);

    auto endFrameL = new QLabel(tr("End frame:"));
    endFrameL->setToolTip(tr("Last frame of the animation."));
    m_endFrame = new QSpinBox;
    m_endFrame->setFixedWidth(spinBoxWidth);
    m_endFrame->setRange(-100000, 100000);

    auto durationL = new QLabel(tr("Duration:"));
    durationL->setToolTip(tr("Length of the animation in milliseconds. If you set a shorter duration than the number of frames, frames are left out from the end of the animation."));
    m_duration = new QSpinBox;
    m_duration->setFixedWidth(spinBoxWidth);
    m_duration->setRange(0, 100000);

    auto continuousL = new QLabel(tr("Continuous"));
    continuousL->setToolTip(tr("Sets the animation to loop indefinitely."));
    m_continuous = new QCheckBox;

    auto loopsL = new QLabel(tr("Loops:"));
    loopsL->setToolTip(tr("Number of times the animation runs before it stops."));
    m_loops = new QSpinBox;
    m_loops->setFixedWidth(spinBoxWidth);
    m_loops->setRange(-1, 1000);

    auto pingPongL = new QLabel(tr("Ping pong"));
    pingPongL->setToolTip(
        tr("Runs the animation backwards to the beginning when it reaches the end."));
    m_pingPong = new QCheckBox;

    auto transitionToStateL = new QLabel(tr("Finished:"));
    transitionToStateL->setToolTip(tr("State to activate when the animation finishes."));
    m_transitionToState = new QComboBox;
    m_transitionToState->addItem(tr("none"));

    auto str = new QWidget;
    QSizePolicy sp(QSizePolicy::MinimumExpanding, QSizePolicy::Ignored);
    sp.setHorizontalStretch(2);
    str->setSizePolicy(sp);

    using namespace Layouting;
    Grid {
        Span(4, mainL), br,
        empty(), br,
        idL, Span(2, m_idLineEdit), Span(2, Row{ runningL, m_running }), br,
        empty(), startFrameL, m_startFrame, endFrameL, m_endFrame, durationL, m_duration, br,
        empty(), continuousL, m_continuous, loopsL, m_loops, pingPongL, m_pingPong, str, br,
        tr("Transition to state:"), transitionToStateL, m_transitionToState, br,
    }.attachTo(this);

    connectSpinBox(m_duration, "duration");
    connectSpinBox(m_loops, "loops");

    connectSpinBox(m_startFrame, "from");
    connectSpinBox(m_endFrame, "to");

    connect(m_loops, &QSpinBox::valueChanged, this, [this] {
        m_continuous->setChecked(m_loops->value() == -1);
    });

    connect(m_continuous, &QCheckBox::toggled, this, [this](bool checked) {
        if (checked) {
            setProperty("loops", -1);
            m_loops->setValue(-1);
        } else {
            setProperty("loops", 1);
            m_loops->setValue(1);
        }
    });

    connect(m_idLineEdit, &QLineEdit::editingFinished, this, [this] {
        QTC_ASSERT(m_timeline.isValid(), return );

        static QString lastString;

        const QString newId = m_idLineEdit->text();

        if (lastString == newId)
            return;

        lastString = newId;

        if (newId == animation().id())
            return;

        bool error = false;

        if (!ModelNode::isValidId(newId)) {
            Core::AsynchronousMessageBox::warning(tr("Invalid Id"),
                                                  tr("%1 is an invalid id.").arg(newId));
            error = true;
        } else if (animation().view()->hasId(newId)) {
            Core::AsynchronousMessageBox::warning(tr("Invalid Id"),
                                                  tr("%1 already exists.").arg(newId));
        } else {
            animation().setIdWithRefactoring(newId);
            error = true;
        }

        if (error) {
            lastString.clear();
            m_idLineEdit->setText(animation().id());
        }
    });

    connect(m_running, &QCheckBox::clicked, this, [this](bool checked) {
        if (checked) {
            setProperty("running", true);
        } else {
            setProperty("running", false);
        }
    });

    connect(m_pingPong, &QCheckBox::clicked, this, [this](bool checked) {
        if (checked) {
            setProperty("pingPong", true);
        } else {
            setProperty("pingPong", false);
        }
    });

    connect(m_transitionToState, &QComboBox::activated, this, [this](int index) {
                if (!m_animation.isValid())
                    return;
                if (!m_animation.view()->rootModelNode().hasId())
                    return;

                ModelNode rootNode = m_animation.view()->rootModelNode();

                if (index == 0) {
                    if (m_animation.signalHandlerProperty("onFinished").isValid())
                        m_animation.removeProperty("onFinished");
                } else if (index == 1) {
                    m_animation.signalHandlerProperty("onFinished")
                        .setSource(rootNode.id() + ".state = \"" + "\"");
                } else {
                    m_animation.signalHandlerProperty("onFinished")
                        .setSource(rootNode.id() + ".state = \""
                                   + m_transitionToState->currentText() + "\"");
                }
            });
}

void TimelineAnimationForm::setup(const ModelNode &animation)
{
    m_timeline = QmlTimeline(animation.parentProperty().parentModelNode());
    setAnimation(animation);
    setupAnimation();
}

ModelNode TimelineAnimationForm::animation() const
{
    return m_animation;
}

void TimelineAnimationForm::setAnimation(const ModelNode &animation)
{
    m_animation = animation;
}

void TimelineAnimationForm::setupAnimation()
{
    if (!m_animation.isValid())
        setEnabled(false);

    if (m_animation.isValid()) {
        setEnabled(true);

        m_idLineEdit->setText(m_animation.id());

        if (m_animation.hasVariantProperty("duration"))
            m_duration->setValue(m_animation.variantProperty("duration").value().toInt());
        else
            m_duration->setValue(0);

        m_startFrame->setValue(m_animation.variantProperty("from").value().toInt());
        m_endFrame->setValue(m_animation.variantProperty("to").value().toInt());

        if (m_animation.hasVariantProperty("loops"))
            m_loops->setValue(m_animation.variantProperty("loops").value().toInt());
        else
            m_loops->setValue(0);

        if (m_animation.hasVariantProperty("running"))
            m_running->setChecked(m_animation.variantProperty("running").value().toBool());
        else
            m_running->setChecked(false);

        if (m_animation.hasVariantProperty("pingPong"))
            m_pingPong->setChecked(m_animation.variantProperty("pingPong").value().toBool());
        else
            m_pingPong->setChecked(false);

        m_continuous->setChecked(m_loops->value() == -1);
    }

    populateStateComboBox();

    m_duration->setEnabled(m_animation.isValid());
    m_running->setEnabled(m_animation.isValid());
    m_continuous->setEnabled(m_animation.isValid());
    m_loops->setEnabled(m_animation.isValid());
}

void TimelineAnimationForm::setProperty(const PropertyName &propertyName, const QVariant &value)
{
    QTC_ASSERT(m_animation.isValid(), return );

    try {
        m_animation.variantProperty(propertyName).setValue(value);
    } catch (const Exception &e) {
        e.showException();
    }
}

void TimelineAnimationForm::connectSpinBox(QSpinBox *spinBox, const PropertyName &propertyName)
{
    connect(spinBox, &QSpinBox::editingFinished, [this, propertyName, spinBox]() {
        setProperty(propertyName, spinBox->value());
    });
}

void TimelineAnimationForm::populateStateComboBox()
{
    m_transitionToState->clear();
    m_transitionToState->addItem(tr("none"));
    m_transitionToState->addItem(tr("Base State"));
    if (!m_animation.isValid())
        return;
    QmlObjectNode rootNode = QmlObjectNode(m_animation.view()->rootModelNode());
    if (rootNode.isValid() && rootNode.modelNode().hasId()) {
        for (const QmlModelState &state : QmlVisualNode(rootNode).states().allStates()) {
            m_transitionToState
                ->addItem(state.modelNode().variantProperty("name").value().toString(),
                          QVariant::fromValue<ModelNode>(state.modelNode()));
        }
        if (m_animation.signalHandlerProperty("onFinished").isValid()) {
            const QString source = m_animation.signalHandlerProperty("onFinished").source();
            const QStringList list = source.split("=");
            if (list.size() == 2) {
                QString name = list.last().trimmed();
                name.chop(1);
                name.remove(0, 1);
                if (name.isEmpty())
                    m_transitionToState->setCurrentIndex(1);
                else
                    m_transitionToState->setCurrentText(name);
            }
        }
    }
}

} // namespace QmlDesigner
