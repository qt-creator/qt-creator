#ifndef QMLINSPECTORTOOLBAR_H
#define QMLINSPECTORTOOLBAR_H

#include <QObject>

QT_FORWARD_DECLARE_CLASS(QAction);

namespace Core {
    class Context;
}

namespace QmlJSInspector {
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

signals:
    void animationSpeedChanged(qreal slowdownFactor = 1.0f);

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
    QAction *m_reloadAction;
    QAction *m_playAction;
    QAction *m_pauseAction;
    QAction *m_selectAction;
    QAction *m_selectMarqueeAction;
    QAction *m_zoomAction;
    QAction *m_colorPickerAction;
    QAction *m_toQmlAction;
    QAction *m_fromQmlAction;

    bool m_emitSignals;
    bool m_isRunning;
    DesignTool m_activeTool;

};

} // namespace Internal
} // namespace QmlJSInspector

#endif // QMLINSPECTORTOOLBAR_H
