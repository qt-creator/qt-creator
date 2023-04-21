// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "transitioneditortoolbar.h"
#include "transitioneditorgraphicsscene.h"

#include "transitioneditorconstants.h"

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
#include <utils/stylehelper.h>

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

static QAction *createAction(const Utils::Id &id,
                             const QIcon &icon,
                             const QString &name,
                             const QKeySequence &shortcut)
{

    Core::Context context(TransitionEditorConstants::C_QMLTRANSITIONS);

    auto *action = new QAction(icon, name);
    auto *command = Core::ActionManager::registerAction(action, id, context);
    command->setDefaultKeySequence(shortcut);
    command->augmentActionWithShortcutToolTip(action);

    return action;
}

TransitionEditorToolBar::TransitionEditorToolBar(QWidget *parent)
    : QToolBar(parent)
    , m_grp()
{
    setFixedHeight(Theme::toolbarSize());
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

void TransitionEditorToolBar::updateComboBox(const ModelNode &root)
{
    if (NodeAbstractProperty transitions = root.nodeAbstractProperty("transitions")) {
        for (const ModelNode &transition : transitions.directSubNodes())
            m_transitionComboBox->addItem(transition.id());
    }
}

void TransitionEditorToolBar::setCurrentTransition(const ModelNode &transition)
{
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

    auto *settingsAction = createAction(TransitionEditorConstants::C_SETTINGS,
                                        Theme::iconFromName(Theme::Icon::settings_medium),
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

    auto *curvePicker = createAction(TransitionEditorConstants::C_CURVE_PICKER,
                                     Theme::iconFromName(Theme::Icon::curveDesigner_medium),
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

    auto *zoomOut = createAction(TransitionEditorConstants::C_ZOOM_OUT,
                                 Theme::iconFromName(Theme::Icon::zoomOut_medium),
                                 tr("Zoom Out"),
                                 QKeySequence(QKeySequence::ZoomOut));

    connect(zoomOut, &QAction::triggered, [this]() {
        m_scale->setValue(m_scale->value() - m_scale->pageStep());
    });
    addAction(zoomOut);

    addSpacing(10);

    m_scale = new QSlider(this);
    Utils::StyleHelper::setPanelWidget(m_scale);
    Utils::StyleHelper::setPanelWidgetSingleRow(m_scale);
    m_scale->setOrientation(Qt::Horizontal);
    m_scale->setMaximumWidth(200);
    m_scale->setMinimumWidth(100);
    m_scale->setMinimum(0);
    m_scale->setMaximum(100);
    m_scale->setValue(0);

    connect(m_scale, &QSlider::valueChanged, this, &TransitionEditorToolBar::scaleFactorChanged);
    addWidget(m_scale);

    addSpacing(10);

    auto *zoomIn = createAction(TransitionEditorConstants::C_ZOOM_IN,
                                Theme::iconFromName(Theme::Icon::zoomIn_medium),
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

    addSpacing(5);
}

void TransitionEditorToolBar::addSpacing(int width)
{
    auto *widget = new QWidget;
    widget->setFixedWidth(width);
    addWidget(widget);
}

void TransitionEditorToolBar::resizeEvent([[maybe_unused]] QResizeEvent *event)
{
    int width = 0;
    QWidget *spacer = nullptr;
    for (auto *object : std::as_const(m_grp)) {
        if (isSpacer(object))
            spacer = qobject_cast<QWidget *>(object);
        else
            width += controlWidth(this, object);
    }

    if (spacer) {
        int spacerWidth = TransitionEditorConstants::sectionWidth - width - 12;
        spacer->setFixedWidth(spacerWidth > 0 ? spacerWidth : 0);
    }
}

} // namespace QmlDesigner
