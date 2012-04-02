/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef QMLJSINSPECTORTOOLBAR_H
#define QMLJSINSPECTORTOOLBAR_H

#include <debugger/debuggerconstants.h>

#include <QObject>
#include <QIcon>

QT_BEGIN_NAMESPACE
class QAction;
class QActionGroup;
class QToolButton;
QT_END_NAMESPACE

namespace Utils {
class StyledBar;
class SavedAction;
}

namespace QmlJSInspector {

namespace Internal {

class QmlJsInspectorToolBar : public QObject
{
    Q_OBJECT

public:
    enum DesignTool {
        NoTool = 0,
        SelectionToolMode = 1,
        MarqueeSelectionToolMode = 2,
        MoveToolMode = 3,
        ResizeToolMode = 4,
        ZoomMode = 6
    };

    explicit QmlJsInspectorToolBar(QObject *parent = 0);
    void createActions();
    QWidget *widget() const;
    void readSettings();
    void setZoomToolEnabled(bool enable);

public slots:
    void writeSettings() const;
    void setEnabled(bool value);
    void enable();
    void disable();

    void activateSelectTool();
    void activateZoomTool();

    void setAnimationSpeed(qreal slowDownFactor);
    void setAnimationPaused(bool paused);

    void setDesignModeBehavior(bool inDesignMode);
    void setShowAppOnTop(bool showAppOnTop);

signals:
    void applyChangesFromQmlFileTriggered(bool isChecked);

    void designModeSelected(bool);
    void reloadSelected();
    void selectToolSelected();
    void zoomToolSelected();

    void showAppOnTopSelected(bool isChecked);

    void animationSpeedChanged(qreal slowdownFactor);
    void animationPausedChanged(bool paused);

private slots:
    void activatePlayOnClick();
    void selectToolTriggered(bool checked);
    void zoomToolTriggered(bool checked);

    void showAppOnTopClick();

    void changeAnimationSpeed();

    void activateFromQml();

    void updatePlayAction();

private:
    void updateDesignModeActions(DesignTool activeTool);

    Utils::SavedAction *m_fromQmlAction;
    QAction *m_playAction;
    QAction *m_selectAction;
    QAction *m_zoomAction;

    Utils::SavedAction *m_showAppOnTopAction;

    QActionGroup *m_playSpeedMenuActions;

    QToolButton *m_playButton;
    QIcon m_playIcon;
    QIcon m_pauseIcon;

    bool m_emitSignals;
    bool m_paused;
    qreal m_animationSpeed;

    bool m_designModeActive;
    DesignTool m_activeTool;

    QWidget *m_barWidget;
    bool m_zoomActionEnable;
};

} // namespace Internal
} // namespace QmlJSInspector

#endif // QMLJSINSPECTORTOOLBAR_H
