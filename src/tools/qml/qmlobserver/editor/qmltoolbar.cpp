#include <QLabel>
#include <QIcon>
#include <QAction>

#include "qmltoolbar.h"
#include "toolbarcolorbox.h"

#include <QDebug>

namespace QmlViewer {

QmlToolbar::QmlToolbar(QWidget *parent) :
    QToolBar(parent),
    m_emitSignals(true),
    ui(new Ui)
{
    ui->designmode = new QAction(QIcon(":/qml/images/designmode.png"), tr("Design Mode"), this);
    ui->play = new QAction(QIcon(":/qml/images/play.png"), tr("Play"), this);
    ui->pause = new QAction(QIcon(":/qml/images/pause.png"), tr("Pause"), this);
    ui->select = new QAction(QIcon(":/qml/images/select.png"), tr("Select"), this);
    ui->selectMarquee = new QAction(QIcon(":/qml/images/select-marquee.png"), tr("Select (Marquee)"), this);
    ui->zoom = new QAction(QIcon(":/qml/images/zoom.png"), tr("Zoom"), this);
    ui->colorPicker = new QAction(QIcon(":/qml/images/color-picker.png"), tr("Color Picker"), this);
    ui->toQml = new QAction(QIcon(":/qml/images/to-qml.png"), tr("Apply Changes to QML Viewer"), this);
    ui->fromQml = new QAction(QIcon(":/qml/images/from-qml.png"), tr("Apply Changes to Document"), this);
    ui->designmode->setCheckable(true);
    ui->designmode->setChecked(false);

    ui->play->setCheckable(true);
    ui->play->setChecked(true);
    ui->pause->setCheckable(true);
    ui->select->setCheckable(true);
    ui->selectMarquee->setCheckable(true);
    ui->zoom->setCheckable(true);
    ui->colorPicker->setCheckable(true);

    setWindowTitle(tr("Tools"));

    addAction(ui->designmode);
    addAction(ui->play);
    addAction(ui->pause);
    addSeparator();

    addAction(ui->select);
    addAction(ui->selectMarquee);
    addSeparator();
    addAction(ui->zoom);
    addAction(ui->colorPicker);
    addAction(ui->fromQml);

    ui->colorBox = new ToolBarColorBox(this);
    ui->colorBox->setMinimumSize(24, 24);
    ui->colorBox->setMaximumSize(28, 28);
    ui->colorBox->setColor(Qt::black);
    addWidget(ui->colorBox);

    setWindowFlags(Qt::Tool);

    connect(ui->designmode, SIGNAL(toggled(bool)), SLOT(setDesignModeBehaviorOnClick(bool)));

    connect(ui->colorPicker, SIGNAL(triggered()), SLOT(activateColorPickerOnClick()));

    connect(ui->play, SIGNAL(triggered()), SLOT(activatePlayOnClick()));
    connect(ui->pause, SIGNAL(triggered()), SLOT(activatePauseOnClick()));

    connect(ui->zoom, SIGNAL(triggered()), SLOT(activateZoomOnClick()));
    connect(ui->colorPicker, SIGNAL(triggered()), SLOT(activateColorPickerOnClick()));
    connect(ui->select, SIGNAL(triggered()), SLOT(activateSelectToolOnClick()));
    connect(ui->selectMarquee, SIGNAL(triggered()), SLOT(activateMarqueeSelectToolOnClick()));

    connect(ui->toQml, SIGNAL(triggered()), SLOT(activateToQml()));
    connect(ui->fromQml, SIGNAL(triggered()), SLOT(activateFromQml()));
}

QmlToolbar::~QmlToolbar()
{
    delete ui;
}

void QmlToolbar::startExecution()
{
    m_emitSignals = false;
    activatePlayOnClick();
    m_emitSignals = true;
}

void QmlToolbar::pauseExecution()
{
    m_emitSignals = false;
    activatePauseOnClick();
    m_emitSignals = true;
}

void QmlToolbar::activateColorPicker()
{
    m_emitSignals = false;
    activateColorPickerOnClick();
    m_emitSignals = true;
}

void QmlToolbar::activateSelectTool()
{
    m_emitSignals = false;
    activateSelectToolOnClick();
    m_emitSignals = true;
}

void QmlToolbar::activateMarqueeSelectTool()
{
    m_emitSignals = false;
    activateMarqueeSelectToolOnClick();
    m_emitSignals = true;
}

void QmlToolbar::activateZoom()
{
    m_emitSignals = false;
    activateZoomOnClick();
    m_emitSignals = true;
}

void QmlToolbar::setDesignModeBehavior(bool inDesignMode)
{
    m_emitSignals = false;
    ui->designmode->setChecked(inDesignMode);
    setDesignModeBehaviorOnClick(inDesignMode);
    m_emitSignals = true;
}

void QmlToolbar::setDesignModeBehaviorOnClick(bool checked)
{
    ui->play->setEnabled(checked);
    ui->pause->setEnabled(checked);
    ui->select->setEnabled(checked);
    ui->selectMarquee->setEnabled(checked);
    ui->zoom->setEnabled(checked);
    ui->colorPicker->setEnabled(checked);
    ui->toQml->setEnabled(checked);
    ui->fromQml->setEnabled(checked);

    if (m_emitSignals)
        emit designModeBehaviorChanged(checked);
}

void QmlToolbar::setColorBoxColor(const QColor &color)
{
    ui->colorBox->setColor(color);
}

void QmlToolbar::activatePlayOnClick()
{
    ui->pause->setChecked(false);
    ui->play->setChecked(true);
    if (!m_isRunning) {
        m_isRunning = true;
        if (m_emitSignals)
            emit executionStarted();
    }
}

void QmlToolbar::activatePauseOnClick()
{
    ui->play->setChecked(false);
    ui->pause->setChecked(true);
    if (m_isRunning) {
        m_isRunning = false;
        if (m_emitSignals)
            emit executionPaused();
    }
}

void QmlToolbar::activateColorPickerOnClick()
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

void QmlToolbar::activateSelectToolOnClick()
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

void QmlToolbar::activateMarqueeSelectToolOnClick()
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

void QmlToolbar::activateZoomOnClick()
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

void QmlToolbar::activateFromQml()
{
    if (m_emitSignals)
        emit applyChangesFromQmlFileSelected();
}

void QmlToolbar::activateToQml()
{
    if (m_emitSignals)
        emit applyChangesToQmlFileSelected();
}

}
