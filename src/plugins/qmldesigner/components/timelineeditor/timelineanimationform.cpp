/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "timelineanimationform.h"
#include "ui_timelineanimationform.h"

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
#include <utils/qtcassert.h>

namespace QmlDesigner {

TimelineAnimationForm::TimelineAnimationForm(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::TimelineAnimationForm)
{
    ui->setupUi(this);

    connectSpinBox(ui->duration, "duration");
    connectSpinBox(ui->loops, "loops");

    connectSpinBox(ui->startFrame, "from");
    connectSpinBox(ui->endFrame, "to");

    connect(ui->loops, QOverload<int>::of(&QSpinBox::valueChanged), [this]() {
        ui->continuous->setChecked(ui->loops->value() == -1);
    });

    connect(ui->continuous, &QCheckBox::toggled, [this](bool checked) {
        if (checked) {
            setProperty("loops", -1);
            ui->loops->setValue(-1);
        } else {
            setProperty("loops", 1);
            ui->loops->setValue(1);
        }
    });

    connect(ui->idLineEdit, &QLineEdit::editingFinished, [this]() {
        QTC_ASSERT(m_timeline.isValid(), return );

        static QString lastString;

        const QString newId = ui->idLineEdit->text();

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
            ui->idLineEdit->setText(animation().id());
        }
    });

    connect(ui->running, &QCheckBox::clicked, [this](bool checked) {
        if (checked) {
            setProperty("running", true);
        } else {
            setProperty("running", false);
        }
    });

    connect(ui->pingPong, &QCheckBox::clicked, [this](bool checked) {
        if (checked) {
            setProperty("pingPong", true);
        } else {
            setProperty("pingPong", false);
        }
    });

    connect(ui->transitionToState,
            QOverload<int>::of(&QComboBox::activated),
            [this](int index) {
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
                                   + ui->transitionToState->currentText() + "\"");
                }
            });
}

TimelineAnimationForm::~TimelineAnimationForm()
{
    delete ui;
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

        ui->idLineEdit->setText(m_animation.id());

        if (m_animation.hasVariantProperty("duration"))
            ui->duration->setValue(m_animation.variantProperty("duration").value().toInt());
        else
            ui->duration->setValue(0);

        ui->startFrame->setValue(m_animation.variantProperty("from").value().toInt());
        ui->endFrame->setValue(m_animation.variantProperty("to").value().toInt());

        if (m_animation.hasVariantProperty("loops"))
            ui->loops->setValue(m_animation.variantProperty("loops").value().toInt());
        else
            ui->loops->setValue(0);

        if (m_animation.hasVariantProperty("running"))
            ui->running->setChecked(m_animation.variantProperty("running").value().toBool());
        else
            ui->running->setChecked(false);

        if (m_animation.hasVariantProperty("pingPong"))
            ui->pingPong->setChecked(m_animation.variantProperty("pingPong").value().toBool());
        else
            ui->pingPong->setChecked(false);

        ui->continuous->setChecked(ui->loops->value() == -1);
    }

    populateStateComboBox();

    ui->duration->setEnabled(m_animation.isValid());
    ui->running->setEnabled(m_animation.isValid());
    ui->continuous->setEnabled(m_animation.isValid());
    ui->loops->setEnabled(m_animation.isValid());
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
    ui->transitionToState->clear();
    ui->transitionToState->addItem(tr("none"));
    ui->transitionToState->addItem(tr("Base State"));
    if (!m_animation.isValid())
        return;
    QmlObjectNode rootNode = QmlObjectNode(m_animation.view()->rootModelNode());
    if (rootNode.isValid() && rootNode.modelNode().hasId()) {
        for (const QmlModelState &state : QmlVisualNode(rootNode).states().allStates()) {
            ui->transitionToState
                ->addItem(state.modelNode().variantProperty("name").value().toString(),
                          QVariant::fromValue<ModelNode>(state.modelNode()));
        }
        if (m_animation.signalHandlerProperty("onFinished").isValid()) {
            const QString source = m_animation.signalHandlerProperty("onFinished").source();
            const QStringList list = source.split("=");
            if (list.count() == 2) {
                QString name = list.last().trimmed();
                name.chop(1);
                name.remove(0, 1);
                if (name.isEmpty())
                    ui->transitionToState->setCurrentIndex(1);
                else
                    ui->transitionToState->setCurrentText(name);
            }
        }
    }
}

} // namespace QmlDesigner
