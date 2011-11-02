/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "qmljsinspectortoolbar.h"

#include "qmljsinspectorconstants.h"
#include "qmljstoolbarcolorbox.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/id.h>
#include <coreplugin/icore.h>
#include <debugger/debuggerconstants.h>
#include <debugger/debuggermainwindow.h>
#include <debugger/debuggerplugin.h>
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <utils/styledbar.h>

#include <QtGui/QAction>
#include <QtGui/QActionGroup>
#include <QtGui/QHBoxLayout>
#include <QtGui/QMenu>
#include <QtGui/QToolButton>
#include <QtGui/QLineEdit>

namespace QmlJSInspector {
namespace Internal {

static QToolButton *toolButton(QAction *action)
{
    QToolButton *button = new QToolButton;
    button->setDefaultAction(action);
    return button;
}

QmlJsInspectorToolBar::QmlJsInspectorToolBar(QObject *parent) :
    QObject(parent),
    m_fromQmlAction(0),
    m_playAction(0),
    m_selectAction(0),
    m_zoomAction(0),
    m_colorPickerAction(0),
    m_showAppOnTopAction(0),
    m_playSpeedMenuActions(0),
    m_playIcon(QIcon(QLatin1String(":/qml/images/play-small.png"))),
    m_pauseIcon(QIcon(QLatin1String(":/qml/images/pause-small.png"))),
    m_colorBox(0),
    m_emitSignals(true),
    m_paused(false),
    m_animationSpeed(1.0f),
    m_designModeActive(false),
    m_activeTool(NoTool),
    m_barWidget(0)
{
}

void QmlJsInspectorToolBar::setEnabled(bool value)
{
    m_fromQmlAction->setEnabled(value);
    m_showAppOnTopAction->setEnabled(value);
    m_playAction->setEnabled(value);
    m_selectAction->setEnabled(value);
    m_zoomAction->setEnabled(value);
    m_colorPickerAction->setEnabled(value);
    m_colorBox->setEnabled(value);
}

void QmlJsInspectorToolBar::enable()
{
    setEnabled(true);
    m_emitSignals = false;
    m_showAppOnTopAction->setChecked(false);
    setAnimationSpeed(1.0f);
    m_designModeActive = false;
    updateDesignModeActions(NoTool);
    m_emitSignals = true;
}

void QmlJsInspectorToolBar::disable()
{
    setAnimationSpeed(1.0f);
    m_designModeActive = false;
    updateDesignModeActions(NoTool);
    setEnabled(false);
}

void QmlJsInspectorToolBar::activateColorPicker()
{
    updateDesignModeActions(ColorPickerMode);
}

void QmlJsInspectorToolBar::activateSelectTool()
{
    updateDesignModeActions(SelectionToolMode);
}

void QmlJsInspectorToolBar::activateZoomTool()
{
    updateDesignModeActions(ZoomMode);
}

void QmlJsInspectorToolBar::setAnimationSpeed(qreal slowDownFactor)
{
    if (m_animationSpeed == slowDownFactor)
        return;

    m_emitSignals = false;
    m_animationSpeed = slowDownFactor;

    foreach (QAction *action, m_playSpeedMenuActions->actions()) {
        if (action->data().toReal() == slowDownFactor) {
            action->setChecked(true);
            break;
        }
    }

    m_emitSignals = true;
}

void QmlJsInspectorToolBar::setAnimationPaused(bool paused)
{
    if (m_paused == paused)
        return;

    m_paused = paused;
    updatePlayAction();
}

void QmlJsInspectorToolBar::setDesignModeBehavior(bool inDesignMode)
{
    m_emitSignals = false;
    m_designModeActive = inDesignMode;
    updateDesignModeActions(m_activeTool);
    m_emitSignals = true;
}

void QmlJsInspectorToolBar::setShowAppOnTop(bool showAppOnTop)
{
    m_emitSignals = false;
    m_showAppOnTopAction->setChecked(showAppOnTop);
    m_emitSignals = true;
}

void QmlJsInspectorToolBar::createActions()
{
    Core::Context context(Debugger::Constants::C_QMLDEBUGGER);
    Core::ICore *core = Core::ICore::instance();
    Core::ActionManager *am = core->actionManager();

    m_fromQmlAction =
            new QAction(QIcon(QLatin1String(":/qml/images/from-qml-small.png")),
                        tr("Apply Changes on Save"), this);
    m_showAppOnTopAction =
            new QAction(QIcon(QLatin1String(":/qml/images/app-on-top.png")),
                        tr("Show application on top"), this);
    m_playAction =
            new QAction(m_pauseIcon, tr("Play/Pause Animations"), this);
    m_selectAction =
            new QAction(QIcon(QLatin1String(":/qml/images/select-small.png")),
                        tr("Select"), this);
    m_zoomAction =
            new QAction(QIcon(QLatin1String(":/qml/images/zoom-small.png")),
                        tr("Zoom"), this);
    m_colorPickerAction =
            new QAction(QIcon(QLatin1String(":/qml/images/color-picker-small.png")),
                        tr("Color Picker"), this);

    m_fromQmlAction->setCheckable(true);
    m_fromQmlAction->setChecked(true);
    m_showAppOnTopAction->setCheckable(true);
    m_showAppOnTopAction->setChecked(false);
    m_selectAction->setCheckable(true);
    m_zoomAction->setCheckable(true);
    m_colorPickerAction->setCheckable(true);

    Core::Command *command = am->registerAction(m_playAction, Constants::PLAY_ACTION, context);
    command->setAttribute(Core::Command::CA_UpdateIcon);
    am->registerAction(m_selectAction, Constants::SELECT_ACTION, context);
    am->registerAction(m_zoomAction, Constants::ZOOM_ACTION, context);
    am->registerAction(m_colorPickerAction, Constants::COLOR_PICKER_ACTION, context);
    am->registerAction(m_fromQmlAction, Constants::FROM_QML_ACTION, context);
    am->registerAction(m_showAppOnTopAction, Constants::SHOW_APP_ON_TOP_ACTION, context);

    m_barWidget = new Utils::StyledBar;
    m_barWidget->setSingleRow(true);
    m_barWidget->setProperty("topBorder", true);

    QMenu *playSpeedMenu = new QMenu(m_barWidget);
    m_playSpeedMenuActions = new QActionGroup(this);
    m_playSpeedMenuActions->setExclusive(true);
    QAction *speedAction = playSpeedMenu->addAction(tr("1x"), this, SLOT(changeAnimationSpeed()));
    speedAction->setCheckable(true);
    speedAction->setChecked(true);
    speedAction->setData(1.0f);
    m_playSpeedMenuActions->addAction(speedAction);

    speedAction = playSpeedMenu->addAction(tr("0.5x"), this, SLOT(changeAnimationSpeed()));
    speedAction->setCheckable(true);
    speedAction->setData(2.0f);
    m_playSpeedMenuActions->addAction(speedAction);

    speedAction = playSpeedMenu->addAction(tr("0.25x"), this, SLOT(changeAnimationSpeed()));
    speedAction->setCheckable(true);
    speedAction->setData(4.0f);
    m_playSpeedMenuActions->addAction(speedAction);

    speedAction = playSpeedMenu->addAction(tr("0.125x"), this, SLOT(changeAnimationSpeed()));
    speedAction->setCheckable(true);
    speedAction->setData(8.0f);
    m_playSpeedMenuActions->addAction(speedAction);

    speedAction = playSpeedMenu->addAction(tr("0.1x"), this, SLOT(changeAnimationSpeed()));
    speedAction->setCheckable(true);
    speedAction->setData(10.0f);
    m_playSpeedMenuActions->addAction(speedAction);

    QHBoxLayout *toolBarLayout = new QHBoxLayout(m_barWidget);
    toolBarLayout->setMargin(0);
    toolBarLayout->setSpacing(5);

    m_operateByInstructionButton = toolButton(am->command(Debugger::Constants::OPERATE_BY_INSTRUCTION)->action());

    // Add generic debugging controls
    toolBarLayout->addWidget(toolButton(Debugger::DebuggerPlugin::visibleDebugAction()));
    toolBarLayout->addWidget(toolButton(am->command(Debugger::Constants::STOP)->action()));
    toolBarLayout->addWidget(toolButton(am->command(Debugger::Constants::NEXT)->action()));
    toolBarLayout->addWidget(toolButton(am->command(Debugger::Constants::STEP)->action()));
    toolBarLayout->addWidget(toolButton(am->command(Debugger::Constants::STEPOUT)->action()));
    toolBarLayout->addWidget(m_operateByInstructionButton);
    toolBarLayout->addStretch(1);

    // QML Helpers
    toolBarLayout->addWidget(new Utils::StyledSeparator);
    toolBarLayout->addWidget(toolButton(am->command(Constants::FROM_QML_ACTION)->action()));
    toolBarLayout->addWidget(toolButton(am->command(Constants::SHOW_APP_ON_TOP_ACTION)->action()));
    m_playButton = toolButton(am->command(Constants::PLAY_ACTION)->action());
    m_playButton->setMenu(playSpeedMenu);
    toolBarLayout->addWidget(m_playButton);

    // Inspector
    toolBarLayout->addWidget(new Utils::StyledSeparator);
    toolBarLayout->addWidget(toolButton(am->command(Constants::SELECT_ACTION)->action()));
    toolBarLayout->addWidget(toolButton(am->command(Constants::ZOOM_ACTION)->action()));
    toolBarLayout->addWidget(toolButton(am->command(Constants::COLOR_PICKER_ACTION)->action()));

    m_colorBox = new ToolBarColorBox(m_barWidget);
    m_colorBox->setMinimumSize(20, 20);
    m_colorBox->setMaximumSize(20, 20);
    m_colorBox->setInnerBorderColor(QColor(192, 192, 192));
    m_colorBox->setOuterBorderColor(QColor(58, 58, 58));
    toolBarLayout->addWidget(m_colorBox);

    connect(m_fromQmlAction, SIGNAL(triggered()), SLOT(activateFromQml()));
    connect(m_showAppOnTopAction, SIGNAL(triggered()), SLOT(showAppOnTopClick()));
    connect(m_playAction, SIGNAL(triggered()), SLOT(activatePlayOnClick()));
    connect(m_colorPickerAction, SIGNAL(triggered(bool)), SLOT(colorPickerTriggered(bool)));
    connect(m_selectAction, SIGNAL(triggered(bool)), SLOT(selectToolTriggered(bool)));
    connect(m_zoomAction, SIGNAL(triggered(bool)), SLOT(zoomToolTriggered(bool)));

    Debugger::DebuggerMainWindow *mw = Debugger::DebuggerPlugin::mainWindow();
    activeDebugLanguagesChanged(mw->activeDebugLanguages());
    connect(mw, SIGNAL(activeDebugLanguagesChanged(Debugger::DebuggerLanguages)),
            this, SLOT(activeDebugLanguagesChanged(Debugger::DebuggerLanguages)));
}

QWidget *QmlJsInspectorToolBar::widget() const
{
    return m_barWidget;
}

void QmlJsInspectorToolBar::changeAnimationSpeed()
{
    QAction *action = static_cast<QAction*>(sender());

    m_animationSpeed = action->data().toReal();
    emit animationSpeedChanged(m_animationSpeed);

    updatePlayAction();
}

void QmlJsInspectorToolBar::activatePlayOnClick()
{
    m_paused = !m_paused;
    emit animationPausedChanged(m_paused);
    updatePlayAction();
}

void QmlJsInspectorToolBar::updatePlayAction()
{
    m_playAction->setIcon(m_paused ? m_playIcon : m_pauseIcon);
}

void QmlJsInspectorToolBar::colorPickerTriggered(bool checked)
{
    updateDesignModeActions(ColorPickerMode);

    if (m_designModeActive != checked) {
        m_designModeActive = checked;
        emit designModeSelected(checked);
    }

    if (checked)
        emit colorPickerSelected();
}

void QmlJsInspectorToolBar::selectToolTriggered(bool checked)
{
    updateDesignModeActions(SelectionToolMode);

    if (m_designModeActive != checked) {
        m_designModeActive = checked;
        emit designModeSelected(checked);
    }

    if (checked)
        emit selectToolSelected();
}

void QmlJsInspectorToolBar::zoomToolTriggered(bool checked)
{
    updateDesignModeActions(ZoomMode);

    if (m_designModeActive != checked) {
        m_designModeActive = checked;
        emit designModeSelected(checked);
    }

    if (checked)
        emit zoomToolSelected();
}

void QmlJsInspectorToolBar::showAppOnTopClick()
{
    if (m_emitSignals)
        emit showAppOnTopSelected(m_showAppOnTopAction->isChecked());
}

void QmlJsInspectorToolBar::setSelectedColor(const QColor &color)
{
    m_colorBox->setColor(color);
}

void QmlJsInspectorToolBar::activateFromQml()
{
    if (m_emitSignals)
        emit applyChangesFromQmlFileTriggered(m_fromQmlAction->isChecked());
}

void QmlJsInspectorToolBar::activeDebugLanguagesChanged(Debugger::DebuggerLanguages languages)
{
    m_operateByInstructionButton->setVisible(languages & Debugger::CppLanguage);
}

void QmlJsInspectorToolBar::updateDesignModeActions(DesignTool activeTool)
{
    m_activeTool = activeTool;
    m_selectAction->setChecked(m_designModeActive && (m_activeTool == SelectionToolMode));
    m_zoomAction->setChecked(m_designModeActive && (m_activeTool == ZoomMode));
    m_colorPickerAction->setChecked(m_designModeActive && (m_activeTool == ColorPickerMode));
}

} // namespace Internal
} // namespace QmlJSInspector
