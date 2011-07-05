/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "qmltoolbar.h"
#include "toolbarcolorbox.h"

#include <QtGui/QLabel>
#include <QtGui/QIcon>
#include <QtGui/QAction>
#include <QtGui/QMenu>

#include <QtCore/QDebug>

namespace QmlJSDebugger {

QmlToolBar::QmlToolBar(QWidget *parent)
    : QToolBar(parent)
    , m_emitSignals(true)
    , m_paused(false)
    , m_animationSpeed(1.0f)
    , ui(new Ui)
{
    ui->playIcon = QIcon(QLatin1String(":/qml/images/play-24.png"));
    ui->pauseIcon = QIcon(QLatin1String(":/qml/images/pause-24.png"));

    ui->designmode = new QAction(QIcon(QLatin1String(":/qml/images/inspectormode-24.png")),
                                 tr("Inspector Mode"), this);
    ui->play = new QAction(ui->pauseIcon, tr("Play/Pause Animations"), this);
    ui->select = new QAction(QIcon(QLatin1String(":/qml/images/select-24.png")), tr("Select"), this);
    ui->selectMarquee = new QAction(QIcon(QLatin1String(":/qml/images/select-marquee-24.png")),
                                    tr("Select (Marquee)"), this);
    ui->zoom = new QAction(QIcon(QLatin1String(":/qml/images/zoom-24.png")), tr("Zoom"), this);
    ui->colorPicker = new QAction(QIcon(QLatin1String(":/qml/images/color-picker-24.png")),
                                  tr("Color Picker"), this);
    ui->toQml = new QAction(QIcon(QLatin1String(":/qml/images/to-qml-24.png")),
                            tr("Apply Changes to QML Viewer"), this);
    ui->fromQml = new QAction(QIcon(QLatin1String(":/qml/images/from-qml-24.png")),
                              tr("Apply Changes to Document"), this);
    ui->designmode->setCheckable(true);
    ui->designmode->setChecked(false);

    ui->play->setCheckable(false);
    ui->select->setCheckable(true);
    ui->selectMarquee->setCheckable(true);
    ui->zoom->setCheckable(true);
    ui->colorPicker->setCheckable(true);

    setWindowTitle(tr("Tools"));

    addAction(ui->designmode);
    addAction(ui->play);
    addSeparator();

    addAction(ui->select);
    // disabled because multi selection does not do anything useful without design mode
    //addAction(ui->selectMarquee);
    addSeparator();
    addAction(ui->zoom);
    addAction(ui->colorPicker);
    //addAction(ui->fromQml);

    ui->colorBox = new ToolBarColorBox(this);
    ui->colorBox->setMinimumSize(24, 24);
    ui->colorBox->setMaximumSize(28, 28);
    ui->colorBox->setColor(Qt::black);
    addWidget(ui->colorBox);

    setWindowFlags(Qt::Tool);

    QMenu *playSpeedMenu = new QMenu(this);
    ui->playSpeedMenuActions = new QActionGroup(this);
    ui->playSpeedMenuActions->setExclusive(true);

    QAction *speedAction = playSpeedMenu->addAction(tr("1x"), this, SLOT(changeAnimationSpeed()));
    speedAction->setCheckable(true);
    speedAction->setChecked(true);
    speedAction->setData(1.0f);
    ui->playSpeedMenuActions->addAction(speedAction);

    speedAction = playSpeedMenu->addAction(tr("0.5x"), this, SLOT(changeAnimationSpeed()));
    speedAction->setCheckable(true);
    speedAction->setData(2.0f);
    ui->playSpeedMenuActions->addAction(speedAction);

    speedAction = playSpeedMenu->addAction(tr("0.25x"), this, SLOT(changeAnimationSpeed()));
    speedAction->setCheckable(true);
    speedAction->setData(4.0f);
    ui->playSpeedMenuActions->addAction(speedAction);

    speedAction = playSpeedMenu->addAction(tr("0.125x"), this, SLOT(changeAnimationSpeed()));
    speedAction->setCheckable(true);
    speedAction->setData(8.0f);
    ui->playSpeedMenuActions->addAction(speedAction);

    speedAction = playSpeedMenu->addAction(tr("0.1x"), this, SLOT(changeAnimationSpeed()));
    speedAction->setCheckable(true);
    speedAction->setData(10.0f);
    ui->playSpeedMenuActions->addAction(speedAction);

    ui->play->setMenu(playSpeedMenu);

    connect(ui->designmode, SIGNAL(toggled(bool)), SLOT(setDesignModeBehaviorOnClick(bool)));

    connect(ui->colorPicker, SIGNAL(triggered()), SLOT(activateColorPickerOnClick()));

    connect(ui->play, SIGNAL(triggered()), SLOT(activatePlayOnClick()));

    connect(ui->zoom, SIGNAL(triggered()), SLOT(activateZoomOnClick()));
    connect(ui->colorPicker, SIGNAL(triggered()), SLOT(activateColorPickerOnClick()));
    connect(ui->select, SIGNAL(triggered()), SLOT(activateSelectToolOnClick()));
    connect(ui->selectMarquee, SIGNAL(triggered()), SLOT(activateMarqueeSelectToolOnClick()));

    connect(ui->toQml, SIGNAL(triggered()), SLOT(activateToQml()));
    connect(ui->fromQml, SIGNAL(triggered()), SLOT(activateFromQml()));
}

QmlToolBar::~QmlToolBar()
{
    delete ui;
}

void QmlToolBar::activateColorPicker()
{
    m_emitSignals = false;
    activateColorPickerOnClick();
    m_emitSignals = true;
}

void QmlToolBar::activateSelectTool()
{
    m_emitSignals = false;
    activateSelectToolOnClick();
    m_emitSignals = true;
}

void QmlToolBar::activateMarqueeSelectTool()
{
    m_emitSignals = false;
    activateMarqueeSelectToolOnClick();
    m_emitSignals = true;
}

void QmlToolBar::activateZoom()
{
    m_emitSignals = false;
    activateZoomOnClick();
    m_emitSignals = true;
}

void QmlToolBar::setAnimationSpeed(qreal slowDownFactor)
{
    if (m_animationSpeed == slowDownFactor)
        return;

    m_emitSignals = false;
    m_animationSpeed = slowDownFactor;

    foreach (QAction *action, ui->playSpeedMenuActions->actions()) {
        if (action->data().toReal() == slowDownFactor) {
            action->setChecked(true);
            break;
        }
    }

    m_emitSignals = true;
}

void QmlToolBar::setAnimationPaused(bool paused)
{
    if (m_paused == paused)
        return;

    m_paused = paused;
    updatePlayAction();
}

void QmlToolBar::changeAnimationSpeed()
{
    QAction *action = qobject_cast<QAction*>(sender());
    m_animationSpeed = action->data().toReal();
    emit animationSpeedChanged(m_animationSpeed);
}

void QmlToolBar::setDesignModeBehavior(bool inDesignMode)
{
    m_emitSignals = false;
    ui->designmode->setChecked(inDesignMode);
    setDesignModeBehaviorOnClick(inDesignMode);
    m_emitSignals = true;
}

void QmlToolBar::setDesignModeBehaviorOnClick(bool checked)
{
    ui->select->setEnabled(checked);
    ui->selectMarquee->setEnabled(checked);
    ui->zoom->setEnabled(checked);
    ui->colorPicker->setEnabled(checked);
    ui->toQml->setEnabled(checked);
    ui->fromQml->setEnabled(checked);

    if (m_emitSignals)
        emit designModeBehaviorChanged(checked);
}

void QmlToolBar::setColorBoxColor(const QColor &color)
{
    ui->colorBox->setColor(color);
}

void QmlToolBar::activatePlayOnClick()
{
    m_paused = !m_paused;
    emit animationPausedChanged(m_paused);
    updatePlayAction();
}

void QmlToolBar::updatePlayAction()
{
    ui->play->setIcon(m_paused ? ui->playIcon : ui->pauseIcon);
}

void QmlToolBar::activateColorPickerOnClick()
{
    ui->zoom->setChecked(false);
    ui->select->setChecked(false);
    ui->selectMarquee->setChecked(false);

    ui->colorPicker->setChecked(true);
    if (m_activeTool != Constants::ColorPickerMode) {
        m_activeTool = Constants::ColorPickerMode;
        if (m_emitSignals)
            emit colorPickerSelected();
    }
}

void QmlToolBar::activateSelectToolOnClick()
{
    ui->zoom->setChecked(false);
    ui->selectMarquee->setChecked(false);
    ui->colorPicker->setChecked(false);

    ui->select->setChecked(true);
    if (m_activeTool != Constants::SelectionToolMode) {
        m_activeTool = Constants::SelectionToolMode;
        if (m_emitSignals)
            emit selectToolSelected();
    }
}

void QmlToolBar::activateMarqueeSelectToolOnClick()
{
    ui->zoom->setChecked(false);
    ui->select->setChecked(false);
    ui->colorPicker->setChecked(false);

    ui->selectMarquee->setChecked(true);
    if (m_activeTool != Constants::MarqueeSelectionToolMode) {
        m_activeTool = Constants::MarqueeSelectionToolMode;
        if (m_emitSignals)
            emit marqueeSelectToolSelected();
    }
}

void QmlToolBar::activateZoomOnClick()
{
    ui->select->setChecked(false);
    ui->selectMarquee->setChecked(false);
    ui->colorPicker->setChecked(false);

    ui->zoom->setChecked(true);
    if (m_activeTool != Constants::ZoomMode) {
        m_activeTool = Constants::ZoomMode;
        if (m_emitSignals)
            emit zoomToolSelected();
    }
}

void QmlToolBar::activateFromQml()
{
    if (m_emitSignals)
        emit applyChangesFromQmlFileSelected();
}

void QmlToolBar::activateToQml()
{
    if (m_emitSignals)
        emit applyChangesToQmlFileSelected();
}

} // namespace QmlJSDebugger
