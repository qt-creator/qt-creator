/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef QMLTOOLBAR_H
#define QMLTOOLBAR_H

#include <QToolBar>
#include <QIcon>
#include "qmlobserverconstants.h"

namespace QmlJSDebugger {

class ToolBarColorBox;

class QmlToolbar : public QToolBar
{
    Q_OBJECT

public:
    explicit QmlToolbar(QWidget *parent = 0);
    ~QmlToolbar();

public slots:
    void setDesignModeBehavior(bool inDesignMode);
    void setColorBoxColor(const QColor &color);
    void activateColorPicker();
    void activateSelectTool();
    void activateMarqueeSelectTool();
    void activateZoom();
    void setAnimationSpeed(qreal slowdownFactor = 0.0f);

signals:
    void animationSpeedChanged(qreal slowdownFactor = 1.0f);

    void designModeBehaviorChanged(bool inDesignMode);
    void colorPickerSelected();
    void selectToolSelected();
    void marqueeSelectToolSelected();
    void zoomToolSelected();

    void applyChangesToQmlFileSelected();
    void applyChangesFromQmlFileSelected();

private slots:
    void setDesignModeBehaviorOnClick(bool inDesignMode);
    void activatePlayOnClick();
    void activateColorPickerOnClick();
    void activateSelectToolOnClick();
    void activateMarqueeSelectToolOnClick();
    void activateZoomOnClick();

    void activateFromQml();
    void activateToQml();

    void changeToDefaultAnimSpeed();
    void changeToHalfAnimSpeed();
    void changeToFourthAnimSpeed();
    void changeToEighthAnimSpeed();
    void changeToTenthAnimSpeed();

    void updatePlayAction();
    void updatePauseAction();

private:
    class Ui {
    public:
        QAction *designmode;
        QAction *play;
        QAction *select;
        QAction *selectMarquee;
        QAction *zoom;
        QAction *colorPicker;
        QAction *toQml;
        QAction *fromQml;
        QIcon playIcon;
        QIcon pauseIcon;
        ToolBarColorBox *colorBox;

        QAction *defaultAnimSpeedAction;
        QAction *halfAnimSpeedAction;
        QAction *fourthAnimSpeedAction;
        QAction *eighthAnimSpeedAction;
        QAction *tenthAnimSpeedAction;
        QAction *menuPauseAction;
    };

    bool m_emitSignals;
    bool m_isRunning;
    qreal m_animationSpeed;
    qreal m_previousAnimationSpeed;

    Constants::DesignTool m_activeTool;

    Ui *ui;
};

}

#endif // QMLTOOLBAR_H
