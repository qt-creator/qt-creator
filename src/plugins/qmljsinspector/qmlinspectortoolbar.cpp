#include "qmlinspectortoolbar.h"
#include "qmljsinspectorconstants.h"
#include "qmljstoolbarcolorbox.h"

#include <extensionsystem/pluginmanager.h>
#include <coreplugin/icore.h>
#include <debugger/debuggeruiswitcher.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>

#include <QHBoxLayout>
#include <QAction>
#include <QToolButton>
#include <QMenu>
#include <QActionGroup>

namespace QmlJSInspector {
namespace Internal {

static QToolButton *createToolButton(QAction *action)
{
    QToolButton *button = new QToolButton;
    button->setDefaultAction(action);
    return button;
}

QmlInspectorToolbar::QmlInspectorToolbar(QObject *parent) :
    QObject(parent),
    m_designmodeAction(0),
    m_reloadAction(0),
    m_playAction(0),
    m_pauseAction(0),
    m_selectAction(0),
    m_selectMarqueeAction(0),
    m_zoomAction(0),
    m_colorPickerAction(0),
    m_toQmlAction(0),
    m_fromQmlAction(0),
    m_defaultAnimSpeedAction(0),
    m_halfAnimSpeedAction(0),
    m_fourthAnimSpeedAction(0),
    m_eighthAnimSpeedAction(0),
    m_tenthAnimSpeedAction(0),
    m_menuPauseAction(0),
    m_colorBox(0),
    m_emitSignals(true),
    m_isRunning(false),
    m_animationSpeed(1.0f),
    m_previousAnimationSpeed(0.0f),
    m_activeTool(NoTool)
{

}

void QmlInspectorToolbar::setEnabled(bool value)
{
    m_designmodeAction->setEnabled(value);
    //m_toQmlAction->setEnabled(value);
    m_fromQmlAction->setEnabled(value);

    m_reloadAction->setEnabled(value);
    m_playAction->setEnabled(value);
    m_pauseAction->setEnabled(value);
    m_selectAction->setEnabled(value);
    m_selectMarqueeAction->setEnabled(value);
    m_zoomAction->setEnabled(value);
    m_colorPickerAction->setEnabled(value);
    //m_toQmlAction->setEnabled(value);
    m_fromQmlAction->setEnabled(value);
    m_colorBox->setEnabled(value);
}

void QmlInspectorToolbar::enable()
{
    setEnabled(true);
    m_emitSignals = false;
    m_designmodeAction->setChecked(false);
    changeAnimationSpeed(1.0f);
    activateDesignModeOnClick();
    m_emitSignals = true;
}

void QmlInspectorToolbar::disable()
{
    changeAnimationSpeed(1.0f);
    activateSelectTool();
    setEnabled(false);
}

void QmlInspectorToolbar::activateColorPicker()
{
    m_emitSignals = false;
    activateColorPickerOnClick();
    m_emitSignals = true;
}

void QmlInspectorToolbar::activateSelectTool()
{
    m_emitSignals = false;
    activateSelectToolOnClick();
    m_emitSignals = true;
}

void QmlInspectorToolbar::activateMarqueeSelectTool()
{
    m_emitSignals = false;
    activateMarqueeSelectToolOnClick();
    m_emitSignals = true;
}

void QmlInspectorToolbar::activateZoomTool()
{
    m_emitSignals = false;
    activateZoomOnClick();
    m_emitSignals = true;
}

void QmlInspectorToolbar::changeAnimationSpeed(qreal slowdownFactor)
{
    m_emitSignals = false;
    if (slowdownFactor == 0) {
        activatePauseOnClick();
    } else {
        m_animationSpeed = slowdownFactor;

        if (slowdownFactor == 1.0f) {
            m_defaultAnimSpeedAction->setChecked(true);
        } else if (slowdownFactor == 2.0f) {
            m_halfAnimSpeedAction->setChecked(true);
        } else if (slowdownFactor == 4.0f) {
            m_fourthAnimSpeedAction->setChecked(true);
        } else if (slowdownFactor == 8.0f) {
            m_eighthAnimSpeedAction->setChecked(true);
        } else if (slowdownFactor == 10.0f) {
            m_tenthAnimSpeedAction->setChecked(true);
        }

        activatePlayOnClick();
    }
    m_emitSignals = true;
}

void QmlInspectorToolbar::setDesignModeBehavior(bool inDesignMode)
{
    m_emitSignals = false;
    m_designmodeAction->setChecked(inDesignMode);
    activateDesignModeOnClick();
    m_emitSignals = true;
}

void QmlInspectorToolbar::createActions(const Core::Context &context)
{
    Core::ICore *core = Core::ICore::instance();
    Core::ActionManager *am = core->actionManager();
    ExtensionSystem::PluginManager *pluginManager = ExtensionSystem::PluginManager::instance();
    Debugger::DebuggerUISwitcher *uiSwitcher = pluginManager->getObject<Debugger::DebuggerUISwitcher>();

    m_fromQmlAction = new QAction(QIcon(QLatin1String(":/qml/images/from-qml-small.png")), tr("Apply Changes to Document"), this);
    m_designmodeAction = new QAction(QIcon(QLatin1String(":/qml/images/designmode.png")), QLatin1String("Design Mode"), this); // TODO: tr?

    m_reloadAction = new QAction(QIcon(QLatin1String(":/qml/images/reload.png")), QLatin1String("Reload"), this); // TODO: tr?
    m_playAction = new QAction(QIcon(QLatin1String(":/qml/images/play-small.png")), tr("Play animations"), this);
    m_pauseAction = new QAction(QIcon(QLatin1String(":/qml/images/pause-small.png")), tr("Pause animations"), this);
    m_selectAction = new QAction(QIcon(QLatin1String(":/qml/images/select-small.png")), tr("Select"), this);
    m_selectMarqueeAction = new QAction(QIcon(QLatin1String(":/qml/images/select-marquee-small.png")), tr("Select (Marquee)"), this);
    m_zoomAction = new QAction(QIcon(QLatin1String(":/qml/images/zoom-small.png")), tr("Zoom"), this);
    m_colorPickerAction = new QAction(QIcon(QLatin1String(":/qml/images/color-picker-small.png")), tr("Color Picker"), this);
    m_toQmlAction = new QAction(QIcon(QLatin1String(":/qml/images/to-qml-small.png")), tr("Live Preview Changes in QML Viewer"), this);

    m_designmodeAction->setCheckable(true);
    m_designmodeAction->setChecked(false);
    m_playAction->setCheckable(true);
    m_playAction->setChecked(true);
    m_pauseAction->setCheckable(true);
    m_selectAction->setCheckable(true);
    m_selectMarqueeAction->setCheckable(true);
    m_zoomAction->setCheckable(true);
    m_colorPickerAction->setCheckable(true);

    m_fromQmlAction->setCheckable(true);
    m_fromQmlAction->setChecked(true);

    am->registerAction(m_designmodeAction, QmlJSInspector::Constants::DESIGNMODE_ACTION, context);
    am->registerAction(m_reloadAction, QmlJSInspector::Constants::RELOAD_ACTION, context);
    am->registerAction(m_playAction, QmlJSInspector::Constants::PLAY_ACTION, context);
    am->registerAction(m_pauseAction, QmlJSInspector::Constants::PAUSE_ACTION, context);
    am->registerAction(m_selectAction, QmlJSInspector::Constants::SELECT_ACTION, context);
    am->registerAction(m_selectMarqueeAction, QmlJSInspector::Constants::SELECT_MARQUEE_ACTION, context);
    am->registerAction(m_zoomAction, QmlJSInspector::Constants::ZOOM_ACTION, context);
    am->registerAction(m_colorPickerAction, QmlJSInspector::Constants::COLOR_PICKER_ACTION, context);
    am->registerAction(m_toQmlAction, QmlJSInspector::Constants::TO_QML_ACTION, context);
    am->registerAction(m_fromQmlAction, QmlJSInspector::Constants::FROM_QML_ACTION, context);

    QWidget *configBar = new QWidget;
    configBar->setProperty("topBorder", true);

    QHBoxLayout *configBarLayout = new QHBoxLayout(configBar);
    configBarLayout->setMargin(0);
    configBarLayout->setSpacing(5);

    QMenu *playSpeedMenu = new QMenu(configBar);
    QActionGroup *playSpeedMenuActions = new QActionGroup(this);
    playSpeedMenuActions->setExclusive(true);
    playSpeedMenu->addAction(tr("Animation Speed"));
    playSpeedMenu->addSeparator();
    m_defaultAnimSpeedAction = playSpeedMenu->addAction(tr("1x"), this, SLOT(changeToDefaultAnimSpeed()));
    m_defaultAnimSpeedAction->setCheckable(true);
    m_defaultAnimSpeedAction->setChecked(true);
    playSpeedMenuActions->addAction(m_defaultAnimSpeedAction);

    m_halfAnimSpeedAction = playSpeedMenu->addAction(tr("0.5x"), this, SLOT(changeToHalfAnimSpeed()));
    m_halfAnimSpeedAction->setCheckable(true);
    playSpeedMenuActions->addAction(m_halfAnimSpeedAction);

    m_fourthAnimSpeedAction = playSpeedMenu->addAction(tr("0.25x"), this, SLOT(changeToFourthAnimSpeed()));
    m_fourthAnimSpeedAction->setCheckable(true);
    playSpeedMenuActions->addAction(m_fourthAnimSpeedAction);

    m_eighthAnimSpeedAction = playSpeedMenu->addAction(tr("0.125x"), this, SLOT(changeToEighthAnimSpeed()));
    m_eighthAnimSpeedAction->setCheckable(true);
    playSpeedMenuActions->addAction(m_eighthAnimSpeedAction);

    m_tenthAnimSpeedAction = playSpeedMenu->addAction(tr("0.1x"), this, SLOT(changeToTenthAnimSpeed()));
    m_tenthAnimSpeedAction->setCheckable(true);
    playSpeedMenuActions->addAction(m_tenthAnimSpeedAction);

    m_menuPauseAction = playSpeedMenu->addAction(tr("Pause"), this, SLOT(activatePauseOnClick()));
    m_menuPauseAction->setCheckable(true);
    playSpeedMenuActions->addAction(m_menuPauseAction);

    configBarLayout->addWidget(createToolButton(am->command(ProjectExplorer::Constants::DEBUG)->action()));
    configBarLayout->addWidget(createToolButton(am->command(ProjectExplorer::Constants::STOP)->action()));
    configBarLayout->addWidget(createToolButton(am->command(QmlJSInspector::Constants::FROM_QML_ACTION)->action()));
    configBarLayout->addWidget(createToolButton(am->command(QmlJSInspector::Constants::DESIGNMODE_ACTION)->action()));
    configBarLayout->addWidget(createToolButton(am->command(QmlJSInspector::Constants::RELOAD_ACTION)->action()));

    QToolButton *playButton = createToolButton(am->command(QmlJSInspector::Constants::PLAY_ACTION)->action());
    playButton->setMenu(playSpeedMenu);
    configBarLayout->addWidget(playButton);
    configBarLayout->addWidget(createToolButton(am->command(QmlJSInspector::Constants::PAUSE_ACTION)->action()));

    configBarLayout->addWidget(createToolButton(am->command(QmlJSInspector::Constants::SELECT_ACTION)->action()));
    configBarLayout->addWidget(createToolButton(am->command(QmlJSInspector::Constants::SELECT_MARQUEE_ACTION)->action()));

    configBarLayout->addWidget(createToolButton(am->command(QmlJSInspector::Constants::ZOOM_ACTION)->action()));
    configBarLayout->addWidget(createToolButton(am->command(QmlJSInspector::Constants::COLOR_PICKER_ACTION)->action()));

    m_colorBox = new ToolBarColorBox(configBar);
    m_colorBox->setMinimumSize(20, 20);
    m_colorBox->setMaximumSize(20, 20);
    m_colorBox->setInnerBorderColor(QColor(192,192,192));
    m_colorBox->setOuterBorderColor(QColor(58,58,58));
    configBarLayout->addWidget(m_colorBox);
    //configBarLayout->addWidget(createToolButton(am->command(QmlJSInspector::Constants::TO_QML_ACTION)->action()));

    configBarLayout->addStretch();

    uiSwitcher->setToolbar(QmlJSInspector::Constants::LANG_QML, configBar);
    setEnabled(false);

    connect(m_designmodeAction, SIGNAL(triggered()), SLOT(activateDesignModeOnClick()));
    connect(m_reloadAction, SIGNAL(triggered()), SIGNAL(reloadSelected()));

    connect(m_colorPickerAction, SIGNAL(triggered()), SLOT(activateColorPickerOnClick()));

    connect(m_playAction, SIGNAL(triggered()), SLOT(activatePlayOnClick()));
    connect(m_pauseAction, SIGNAL(triggered()), SLOT(activatePauseOnClick()));

    connect(m_zoomAction, SIGNAL(triggered()), SLOT(activateZoomOnClick()));
    connect(m_colorPickerAction, SIGNAL(triggered()), SLOT(activateColorPickerOnClick()));
    connect(m_selectAction, SIGNAL(triggered()), SLOT(activateSelectToolOnClick()));
    connect(m_selectMarqueeAction, SIGNAL(triggered()), SLOT(activateMarqueeSelectToolOnClick()));

    //connect(m_toQmlAction, SIGNAL(triggered()), SLOT(activateToQml()));
    connect(m_fromQmlAction, SIGNAL(triggered()), SLOT(activateFromQml()));
}

void QmlInspectorToolbar::changeToDefaultAnimSpeed()
{
    m_animationSpeed = 1.0f;
    activatePlayOnClick();
}

void QmlInspectorToolbar::changeToHalfAnimSpeed()
{
    m_animationSpeed = 2.0f;
    activatePlayOnClick();
}

void QmlInspectorToolbar::changeToFourthAnimSpeed()
{
    m_animationSpeed = 4.0f;
    activatePlayOnClick();
}

void QmlInspectorToolbar::changeToEighthAnimSpeed()
{
    m_animationSpeed = 8.0f;
    activatePlayOnClick();
}

void QmlInspectorToolbar::changeToTenthAnimSpeed()
{
    m_animationSpeed = 10.0f;
    activatePlayOnClick();
}

void QmlInspectorToolbar::activateDesignModeOnClick()
{
    bool checked = m_designmodeAction->isChecked();

    m_reloadAction->setEnabled(checked);
    m_playAction->setEnabled(checked);
    m_pauseAction->setEnabled(checked);
    m_selectAction->setEnabled(checked);
    m_selectMarqueeAction->setEnabled(checked);
    m_zoomAction->setEnabled(checked);
    m_colorPickerAction->setEnabled(checked);

    if (m_emitSignals)
        emit designModeSelected(checked);
}

void QmlInspectorToolbar::activatePlayOnClick()
{
    m_pauseAction->setChecked(false);
    if (!m_isRunning || m_animationSpeed != m_previousAnimationSpeed) {
        m_playAction->setChecked(true);
        m_isRunning = true;
        m_previousAnimationSpeed = m_animationSpeed;
        if (m_emitSignals)
            emit animationSpeedChanged(m_animationSpeed);
    }
}

void QmlInspectorToolbar::activatePauseOnClick()
{
    m_playAction->setChecked(false);
    if (m_isRunning) {
        m_isRunning = false;
        m_pauseAction->setChecked(true);
        if (m_emitSignals)
            emit animationSpeedChanged(0.0f);
    }
}

void QmlInspectorToolbar::activateColorPickerOnClick()
{
    m_zoomAction->setChecked(false);
    m_selectAction->setChecked(false);
    m_selectMarqueeAction->setChecked(false);

    m_colorPickerAction->setChecked(true);
    if (m_activeTool != ColorPickerMode) {
        m_activeTool = ColorPickerMode;
        if (m_emitSignals)
            emit colorPickerSelected();
    }
}

void QmlInspectorToolbar::activateSelectToolOnClick()
{
    m_zoomAction->setChecked(false);
    m_selectMarqueeAction->setChecked(false);
    m_colorPickerAction->setChecked(false);

    m_selectAction->setChecked(true);
    if (m_activeTool != SelectionToolMode) {
        m_activeTool = SelectionToolMode;
        if (m_emitSignals)
            emit selectToolSelected();
    }
}

void QmlInspectorToolbar::activateMarqueeSelectToolOnClick()
{
    m_zoomAction->setChecked(false);
    m_selectAction->setChecked(false);
    m_colorPickerAction->setChecked(false);

    m_selectMarqueeAction->setChecked(true);
    if (m_activeTool != MarqueeSelectionToolMode) {
        m_activeTool = MarqueeSelectionToolMode;
        if (m_emitSignals)
            emit marqueeSelectToolSelected();
    }
}

void QmlInspectorToolbar::activateZoomOnClick()
{
    m_selectAction->setChecked(false);
    m_selectMarqueeAction->setChecked(false);
    m_colorPickerAction->setChecked(false);

    m_zoomAction->setChecked(true);
    if (m_activeTool != ZoomMode) {
        m_activeTool = ZoomMode;
        if (m_emitSignals)
            emit zoomToolSelected();
    }
}

void QmlInspectorToolbar::setLivePreviewChecked(bool value)
{
    m_fromQmlAction->setChecked(value);
}

void QmlInspectorToolbar::setSelectedColor(const QColor &color)
{
    m_colorBox->setColor(color);
}

void QmlInspectorToolbar::activateFromQml()
{
    if (m_emitSignals)
        emit applyChangesFromQmlFileTriggered(m_fromQmlAction->isChecked());
}

void QmlInspectorToolbar::activateToQml()
{
    if (m_emitSignals)
        emit applyChangesToQmlFileSelected();
}

} // namespace Internal
} // namespace QmlJSInspector
