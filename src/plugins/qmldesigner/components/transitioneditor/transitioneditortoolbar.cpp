/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "transitioneditortoolbar.h"
#include "transitioneditorgraphicsscene.h"

#include "timelineconstants.h"

#include "timelineicons.h"

#include "timelineview.h"
#include "timelinewidget.h"

#include <designeractionmanager.h>
#include <nodelistproperty.h>
#include <theme.h>
#include <variantproperty.h>
#include <qmlstate.h>
#include <qmltimeline.h>

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/icore.h>

#include <utils/algorithm.h>

#include <QApplication>
#include <QComboBox>
#include <QIntValidator>
#include <QLineEdit>
#include <QResizeEvent>
#include <QSlider>

#include <cmath>

namespace QmlDesigner {

static bool isSpacer(QObject *object)
{
    return object->property("spacer_widget").toBool();
}

static QWidget *createSpacer()
{
    QWidget *spacer = new QWidget();
    spacer->setProperty("spacer_widget", true);
    return spacer;
}

static int controlWidth(QToolBar *bar, QObject *control)
{
    QWidget *widget = nullptr;

    if (auto *action = qobject_cast<QAction *>(control))
        widget = bar->widgetForAction(action);

    if (widget == nullptr)
        widget = qobject_cast<QWidget *>(control);

    if (widget)
        return widget->width();

    return 0;
}

static QAction *createAction(const Core::Id &id,
                             const QIcon &icon,
                             const QString &name,
                             const QKeySequence &shortcut)
{
    QString text = QString("%1 (%2)").arg(name).arg(shortcut.toString());

    Core::Context context(TimelineConstants::C_QMLTIMELINE);

    auto *action = new QAction(icon, text);
    auto *command = Core::ActionManager::registerAction(action, id, context);
    command->setDefaultKeySequence(shortcut);

    return action;
}

TransitionEditorToolBar::TransitionEditorToolBar(QWidget *parent)
    : QToolBar(parent)
    , m_grp()
{
    setContentsMargins(0, 0, 0, 0);
    createLeftControls();
    createCenterControls();
    createRightControls();
}

void TransitionEditorToolBar::reset() {}

int TransitionEditorToolBar::scaleFactor() const
{
    if (m_scale)
        return m_scale->value();
    return 0;
}

QString TransitionEditorToolBar::currentTransitionId() const
{
    return m_transitionComboBox->currentText();
}

void TransitionEditorToolBar::setBlockReflection(bool block)
{
    m_blockReflection = block;
}

void TransitionEditorToolBar::updateComboBox(const ModelNode &root)
{
    if (root.isValid() && root.hasProperty("transitions")) {
        NodeAbstractProperty transitions = root.nodeAbstractProperty("transitions");
        if (transitions.isValid())
            for (const ModelNode &transition : transitions.directSubNodes())
                m_transitionComboBox->addItem(transition.id());
    }
}

void TransitionEditorToolBar::setCurrentTransition(const ModelNode &transition)
{
    if (m_blockReflection)
        return;

    if (transition.isValid()) {
        m_transitionComboBox->clear();
        const ModelNode root = transition.view()->rootModelNode();
        updateComboBox(root);
        m_transitionComboBox->setCurrentText(transition.id());
    } else {
        m_transitionComboBox->clear();
        m_transitionComboBox->setCurrentText("");
    }
}

void TransitionEditorToolBar::setDuration(qreal frame)
{
    auto text = QString::number(frame, 'f', 0);
    m_duration->setText(text);
}

void TransitionEditorToolBar::setScaleFactor(int factor)
{
    const QSignalBlocker blocker(m_scale);
    m_scale->setValue(factor);
}

void TransitionEditorToolBar::setActionEnabled(const QString &name, bool enabled)
{
    for (auto *action : actions())
        if (action->objectName() == name)
            action->setEnabled(enabled);
}

void TransitionEditorToolBar::createLeftControls()
{
    auto addActionToGroup = [&](QAction *action) {
        addAction(action);
        m_grp << action;
    };

    auto addWidgetToGroup = [&](QWidget *widget) {
        addWidget(widget);
        m_grp << widget;
    };

    auto addSpacingToGroup = [&](int width) {
        auto *widget = new QWidget;
        widget->setFixedWidth(width);
        addWidget(widget);
        m_grp << widget;
    };

    addSpacingToGroup(5);

    auto *settingsAction = createAction(TimelineConstants::C_SETTINGS,
                                        TimelineIcons::ANIMATION.icon(),
                                        tr("Transition Settings"),
                                        QKeySequence(Qt::Key_S));
    connect(settingsAction,
            &QAction::triggered,
            this,
            &TransitionEditorToolBar::settingDialogClicked);

    addActionToGroup(settingsAction);

    addWidgetToGroup(createSpacer());

    m_transitionComboBox = new QComboBox(this);
    addWidgetToGroup(m_transitionComboBox);

    connect(m_transitionComboBox, &QComboBox::currentTextChanged, this, [this]() {
        emit currentTransitionChanged(m_transitionComboBox->currentText());
    });
}

static QLineEdit *createToolBarLineEdit(QWidget *parent)
{
    auto lineEdit = new QLineEdit(parent);
    lineEdit->setStyleSheet("* { background-color: rgba(0, 0, 0, 0); }");
    lineEdit->setFixedWidth(48);
    lineEdit->setAlignment(Qt::AlignCenter);

    QPalette pal = parent->palette();
    pal.setColor(QPalette::Text, Theme::instance()->color(Utils::Theme::PanelTextColorLight));
    lineEdit->setPalette(pal);
    QValidator *validator = new QIntValidator(-100000, 100000, lineEdit);
    lineEdit->setValidator(validator);

    return lineEdit;
}

void TransitionEditorToolBar::createCenterControls()
{
    addSpacing(10);

    auto *curvePicker = createAction(TimelineConstants::C_CURVE_PICKER,
                                     TimelineIcons::CURVE_EDITOR.icon(),
                                     tr("Easing Curve Editor"),
                                     QKeySequence(Qt::Key_C));

    curvePicker->setObjectName("Easing Curve Editor");
    connect(curvePicker, &QAction::triggered, this, &TransitionEditorToolBar::openEasingCurveEditor);
    addAction(curvePicker);

    addSpacing(10);

#if 0
    addSeparator();

    addSpacing(10);

    auto *curveEditor = new QAction(TimelineIcons::CURVE_PICKER.icon(), tr("Curve Editor"));
    addAction(curveEditor);
#endif
}

void TransitionEditorToolBar::createRightControls()
{
    auto *spacer = createSpacer();
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    addWidget(spacer);

    addSeparator();
    addSpacing(10);

    auto *zoomOut = createAction(TimelineConstants::C_ZOOM_OUT,
                                 TimelineIcons::ZOOM_SMALL.icon(),
                                 tr("Zoom Out"),
                                 QKeySequence(QKeySequence::ZoomOut));

    connect(zoomOut, &QAction::triggered, [this]() {
        m_scale->setValue(m_scale->value() - m_scale->pageStep());
    });
    addAction(zoomOut);

    addSpacing(10);

    m_scale = new QSlider(this);
    m_scale->setOrientation(Qt::Horizontal);
    m_scale->setMaximumWidth(200);
    m_scale->setMinimumWidth(100);
    m_scale->setMinimum(0);
    m_scale->setMaximum(100);
    m_scale->setValue(0);

    connect(m_scale, &QSlider::valueChanged, this, &TransitionEditorToolBar::scaleFactorChanged);
    addWidget(m_scale);

    addSpacing(10);

    auto *zoomIn = createAction(TimelineConstants::C_ZOOM_IN,
                                TimelineIcons::ZOOM_BIG.icon(),
                                tr("Zoom In"),
                                QKeySequence(QKeySequence::ZoomIn));

    connect(zoomIn, &QAction::triggered, [this]() {
        m_scale->setValue(m_scale->value() + m_scale->pageStep());
    });
    addAction(zoomIn);

    addSpacing(10);

    addSeparator();

    m_duration = createToolBarLineEdit(this);
    addWidget(m_duration);

    auto emitEndChanged = [this]() { emit durationChanged(m_duration->text().toInt()); };
    connect(m_duration, &QLineEdit::editingFinished, emitEndChanged);
}

void TransitionEditorToolBar::addSpacing(int width)
{
    auto *widget = new QWidget;
    widget->setFixedWidth(width);
    addWidget(widget);
}

void TransitionEditorToolBar::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event)

    int width = 0;
    QWidget *spacer = nullptr;
    for (auto *object : qAsConst(m_grp)) {
        if (isSpacer(object))
            spacer = qobject_cast<QWidget *>(object);
        else
            width += controlWidth(this, object);
    }

    if (spacer) {
        int spacerWidth = TimelineConstants::sectionWidth - width - 12;
        spacer->setFixedWidth(spacerWidth > 0 ? spacerWidth : 0);
    }
}

} // namespace QmlDesigner
