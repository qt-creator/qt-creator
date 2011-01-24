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

#ifndef QMLINSPECTORTOOLBAR_H
#define QMLINSPECTORTOOLBAR_H

#include <QtCore/QObject>
#include <QtGui/QIcon>

QT_FORWARD_DECLARE_CLASS(QAction)
QT_FORWARD_DECLARE_CLASS(QColor)
QT_FORWARD_DECLARE_CLASS(QToolButton)
QT_FORWARD_DECLARE_CLASS(QLineEdit)

namespace Core {
    class Context;
}

namespace Utils {
    class StyledBar;
    class FilterLineEdit;
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
    QWidget *widget() const;

public slots:
    void setEnabled(bool value);
    void enable();
    void disable();

    void activateColorPicker();
    void activateSelectTool();
    void activateZoomTool();
    void setAnimationSpeed(qreal slowdownFactor);
    void setDesignModeBehavior(bool inDesignMode);
    void setShowAppOnTop(bool showAppOnTop);
    void setSelectedColor(const QColor &color);

signals:
    void applyChangesFromQmlFileTriggered(bool isChecked);

    void designModeSelected(bool checked);
    void reloadSelected();
    void colorPickerSelected();
    void selectToolSelected();
    void zoomToolSelected();

    void showAppOnTopSelected(bool isChecked);
    void animationSpeedChanged(qreal slowdownFactor = 1.0f);
    void filterTextChanged(const QString &);

private slots:
    void activateDesignModeOnClick();
    void activatePlayOnClick();
    void activateColorPickerOnClick();
    void activateSelectToolOnClick();
    void activateZoomOnClick();

    void showAppOnTopClick();

    void changeToDefaultAnimSpeed();
    void changeToHalfAnimSpeed();
    void changeToFourthAnimSpeed();
    void changeToEighthAnimSpeed();
    void changeToTenthAnimSpeed();

    void activateFromQml();

    void updatePlayAction();
    void updatePauseAction();

private:
    QAction *m_fromQmlAction;
    QAction *m_observerModeAction;
    QAction *m_playAction;
    QAction *m_selectAction;
    QAction *m_zoomAction;
    QAction *m_colorPickerAction;

    QAction *m_showAppOnTopAction;

    QAction *m_defaultAnimSpeedAction;
    QAction *m_halfAnimSpeedAction;
    QAction *m_fourthAnimSpeedAction;
    QAction *m_eighthAnimSpeedAction;
    QAction *m_tenthAnimSpeedAction;
    QAction *m_menuPauseAction;

    QToolButton *m_playButton;
    QIcon m_playIcon;
    QIcon m_pauseIcon;

    QLineEdit *m_filterExp;

    ToolBarColorBox *m_colorBox;

    bool m_emitSignals;
    bool m_isRunning;
    qreal m_animationSpeed;
    qreal m_previousAnimationSpeed;

    DesignTool m_activeTool;

    Utils::StyledBar *m_barWidget;
};

} // namespace Internal
} // namespace QmlJSInspector

#endif // QMLINSPECTORTOOLBAR_H
