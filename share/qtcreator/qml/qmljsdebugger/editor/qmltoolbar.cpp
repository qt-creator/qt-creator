/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
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
    , m_isRunning(false)
    , m_animationSpeed(1.0f)
    , m_previousAnimationSpeed(0.0f)
    , ui(new Ui)
{
    ui->playIcon = QIcon(QLatin1String(":/qml/images/play-24.png"));
    ui->pauseIcon = QIcon(QLatin1String(":/qml/images/pause-24.png"));

    ui->designmode = new QAction(QIcon(QLatin1String(":/qml/images/observermode-24.png")),
                                 tr("Observer Mode"), this);
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
    QActionGroup *playSpeedMenuActions = new QActionGroup(this);
    playSpeedMenuActions->setExclusive(true);
    playSpeedMenu->addAction(tr("Animation Speed"));
    playSpeedMenu->addSeparator();
    ui->defaultAnimSpeedAction = playSpeedMenu->addAction(tr("1x"), this,
                                                          SLOT(changeToDefaultAnimSpeed()));
    ui->defaultAnimSpeedAction->setCheckable(true);
    ui->defaultAnimSpeedAction->setChecked(true);
    playSpeedMenuActions->addAction(ui->defaultAnimSpeedAction);

    ui->halfAnimSpeedAction = playSpeedMenu->addAction(tr("0.5x"), this,
                                                       SLOT(changeToHalfAnimSpeed()));
    ui->halfAnimSpeedAction->setCheckable(true);
    playSpeedMenuActions->addAction(ui->halfAnimSpeedAction);

    ui->fourthAnimSpeedAction = playSpeedMenu->addAction(tr("0.25x"), this,
                                                         SLOT(changeToFourthAnimSpeed()));
    ui->fourthAnimSpeedAction->setCheckable(true);
    playSpeedMenuActions->addAction(ui->fourthAnimSpeedAction);

    ui->eighthAnimSpeedAction = playSpeedMenu->addAction(tr("0.125x"), this,
                                                         SLOT(changeToEighthAnimSpeed()));
    ui->eighthAnimSpeedAction->setCheckable(true);
    playSpeedMenuActions->addAction(ui->eighthAnimSpeedAction);

    ui->tenthAnimSpeedAction = playSpeedMenu->addAction(tr("0.1x"), this,
                                                        SLOT(changeToTenthAnimSpeed()));
    ui->tenthAnimSpeedAction->setCheckable(true);
    playSpeedMenuActions->addAction(ui->tenthAnimSpeedAction);

    ui->menuPauseAction = playSpeedMenu->addAction(tr("Pause"), this, SLOT(updatePauseAction()));
    ui->menuPauseAction->setCheckable(true);
    ui->menuPauseAction->setIcon(ui->pauseIcon);
    playSpeedMenuActions->addAction(ui->menuPauseAction);
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

void QmlToolBar::setAnimationSpeed(qreal slowdownFactor)
{
    m_emitSignals = false;
    if (slowdownFactor != 0) {
        m_animationSpeed = slowdownFactor;

        if (slowdownFactor == 1.0f) {
            ui->defaultAnimSpeedAction->setChecked(true);
        } else if (slowdownFactor == 2.0f) {
            ui->halfAnimSpeedAction->setChecked(true);
        } else if (slowdownFactor == 4.0f) {
            ui->fourthAnimSpeedAction->setChecked(true);
        } else if (slowdownFactor == 8.0f) {
            ui->eighthAnimSpeedAction->setChecked(true);
        } else if (slowdownFactor == 10.0f) {
            ui->tenthAnimSpeedAction->setChecked(true);
        }
        updatePlayAction();
    } else {
        ui->menuPauseAction->setChecked(true);
        updatePauseAction();
    }

    m_emitSignals = true;
}

void QmlToolBar::changeToDefaultAnimSpeed()
{
    m_animationSpeed = 1.0f;
    updatePlayAction();
}

void QmlToolBar::changeToHalfAnimSpeed()
{
    m_animationSpeed = 2.0f;
    updatePlayAction();
}

void QmlToolBar::changeToFourthAnimSpeed()
{
    m_animationSpeed = 4.0f;
    updatePlayAction();
}

void QmlToolBar::changeToEighthAnimSpeed()
{
    m_animationSpeed = 8.0f;
    updatePlayAction();
}

void QmlToolBar::changeToTenthAnimSpeed()
{
    m_animationSpeed = 10.0f;
    updatePlayAction();
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
    if (m_isRunning) {
        updatePauseAction();
    } else {
        updatePlayAction();
    }
}

void QmlToolBar::updatePlayAction()
{
    m_isRunning = true;
    ui->play->setIcon(ui->pauseIcon);
    if (m_animationSpeed != m_previousAnimationSpeed)
        m_previousAnimationSpeed = m_animationSpeed;

    if (m_emitSignals)
        emit animationSpeedChanged(m_animationSpeed);
}

void QmlToolBar::updatePauseAction()
{
    m_isRunning = false;
    ui->play->setIcon(ui->playIcon);
    if (m_emitSignals)
        emit animationSpeedChanged(0.0f);
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
