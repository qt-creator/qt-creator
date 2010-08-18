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

#ifndef QMLINSPECTORTOOLBAR_H
#define QMLINSPECTORTOOLBAR_H

#include <QObject>

QT_FORWARD_DECLARE_CLASS(QAction);
QT_FORWARD_DECLARE_CLASS(QColor);

namespace Core {
    class Context;
}

namespace QmlJSInspector {

class ToolBarColorBox;

namespace Internal {

class QmlInspectorToolbar : public QObject
{
    Q_OBJECT
public:
    enum DesignTool {
        NoTool = 0,
        SelectionToolMode = 1,
        MarqueeSelectionToolMode = 2,
        MoveToolMode = 3,
        ResizeToolMode = 4,
        ColorPickerMode = 5,
        ZoomMode = 6
    };

    explicit QmlInspectorToolbar(QObject *parent = 0);
    void createActions(const Core::Context &context);

public slots:
    void setEnabled(bool value);
    void enable();
    void disable();

    void activateColorPicker();
    void activateSelectTool();
    void activateMarqueeSelectTool();
    void activateZoomTool();
    void changeAnimationSpeed(qreal slowdownFactor);
    void setDesignModeBehavior(bool inDesignMode);
    void setSelectedColor(const QColor &color);

    void setLivePreviewChecked(bool value);

signals:
    void animationSpeedChanged(qreal slowdownFactor = 1.0f);

    void designModeSelected(bool checked);
    void reloadSelected();
    void colorPickerSelected();
    void selectToolSelected();
    void marqueeSelectToolSelected();
    void zoomToolSelected();

    void applyChangesToQmlFileSelected();
    void applyChangesFromQmlFileTriggered(bool isChecked);

private slots:
    void activateDesignModeOnClick();
    void activatePlayOnClick();
    void activatePauseOnClick();
    void activateColorPickerOnClick();
    void activateSelectToolOnClick();
    void activateMarqueeSelectToolOnClick();
    void activateZoomOnClick();

    void changeToDefaultAnimSpeed();
    void changeToHalfAnimSpeed();
    void changeToFourthAnimSpeed();
    void changeToEighthAnimSpeed();
    void changeToTenthAnimSpeed();

    void activateFromQml();
    void activateToQml();

private:
    QAction *m_designmodeAction;
    QAction *m_reloadAction;
    QAction *m_playAction;
    QAction *m_pauseAction;
    QAction *m_selectAction;
    QAction *m_selectMarqueeAction;
    QAction *m_zoomAction;
    QAction *m_colorPickerAction;
    QAction *m_toQmlAction;
    QAction *m_fromQmlAction;

    QAction *m_defaultAnimSpeedAction;
    QAction *m_halfAnimSpeedAction;
    QAction *m_fourthAnimSpeedAction;
    QAction *m_eighthAnimSpeedAction;
    QAction *m_tenthAnimSpeedAction;
    QAction *m_menuPauseAction;

    ToolBarColorBox *m_colorBox;

    bool m_emitSignals;
    bool m_isRunning;
    qreal m_animationSpeed;
    qreal m_previousAnimationSpeed;

    DesignTool m_activeTool;

};

} // namespace Internal
} // namespace QmlJSInspector

#endif // QMLINSPECTORTOOLBAR_H
