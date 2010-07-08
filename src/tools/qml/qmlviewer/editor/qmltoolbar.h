#ifndef QMLTOOLBAR_H
#define QMLTOOLBAR_H

#include <QToolBar>
#include "qmlviewerconstants.h"

namespace QmlViewer {

class ToolBarColorBox;

class QmlToolbar : public QToolBar
{
    Q_OBJECT

public:
    explicit QmlToolbar(QWidget *parent = 0);
    ~QmlToolbar();

public slots:
    void setColorBoxColor(const QColor &color);
    void startExecution();
    void pauseExecution();
    void activateColorPicker();
    void activateSelectTool();
    void activateMarqueeSelectTool();
    void activateZoom();

signals:
    void executionStarted();
    void executionPaused();

    void colorPickerSelected();
    void selectToolSelected();
    void marqueeSelectToolSelected();
    void zoomToolSelected();

    void applyChangesToQmlFileSelected();
    void applyChangesFromQmlFileSelected();

private slots:
    void activatePlayOnClick();
    void activatePauseOnClick();
    void activateColorPickerOnClick();
    void activateSelectToolOnClick();
    void activateMarqueeSelectToolOnClick();
    void activateZoomOnClick();

    void activateFromQml();
    void activateToQml();

private:
    class Ui {
    public:
        QAction *play;
        QAction *pause;
        QAction *select;
        QAction *selectMarquee;
        QAction *zoom;
        QAction *colorPicker;
        QAction *toQml;
        QAction *fromQml;
        ToolBarColorBox *colorBox;
    };

    bool m_emitSignals;
    bool m_isRunning;
    Constants::DesignTool m_activeTool;
    Ui *ui;
};

}

#endif // QMLTOOLBAR_H
