#include "contextpanewidget.h"
#include <coreplugin/icore.h>
#include <QFontComboBox>
#include <QComboBox>
#include <QSpinBox>
#include <QToolButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QGridLayout>
#include <QToolButton>
#include <QAction>
#include <qmldesignerplugin.h>
#include "colorwidget.h"
#include "contextpanetextwidget.h"


namespace QmlDesigner {

ContextPaneWidget::ContextPaneWidget(QWidget *parent) : QFrame(parent), m_currentWidget(0), m_xPos(-1)
{
    setFrameStyle(QFrame::NoFrame);
    setFrameShape(QFrame::StyledPanel);
    setFrameShadow(QFrame::Sunken);
    m_oldPos = QPoint(-1, -1);

    m_dropShadowEffect = new QGraphicsDropShadowEffect;
    m_dropShadowEffect->setBlurRadius(6);
    m_dropShadowEffect->setOffset(2, 2);
    setGraphicsEffect(m_dropShadowEffect);

    QGridLayout *layout = new QGridLayout(this);
    layout->setMargin(2);
    layout->setContentsMargins(2, 4, 2, 2);
    QToolButton *toolButton = new QToolButton(this);    
    QIcon icon(style()->standardIcon(QStyle::SP_DockWidgetCloseButton));
    toolButton->setIcon(icon);
    toolButton->setToolButtonStyle(Qt::ToolButtonIconOnly);    
    toolButton->setFixedSize(icon.availableSizes().first() + QSize(4, 4));
    connect(toolButton, SIGNAL(clicked()), this, SLOT(onTogglePane()));
    layout->addWidget(toolButton, 0, 0, 1, 1);
    colorDialog();

    QWidget *fontWidget = createFontWidget();
    m_currentWidget = fontWidget;
    layout->addWidget(fontWidget, 0, 1, 2, 1);
    setAutoFillBackground(true);
    setContextMenuPolicy(Qt::ActionsContextMenu);

    QAction *disableAction = new QAction(tr("Disable permanently"), this);
    addAction(disableAction);
    connect(disableAction, SIGNAL(triggered()), this, SLOT(onDisable()));
}

ContextPaneWidget::~ContextPaneWidget()
{
    //if the pane was never activated the widget is not in a widget tree
    if (!m_bauhausColorDialog.isNull())
        delete m_bauhausColorDialog.data();
        m_bauhausColorDialog.clear();
}

void ContextPaneWidget::activate(const QPoint &pos, const QPoint &alternative)
{
    //uncheck all color buttons
    foreach (ColorButton *colorButton, findChildren<ColorButton*>()) {
            colorButton->setChecked(false);
    }
    resize(sizeHint());
    show();
    rePosition(pos, alternative);
    raise();
}

void ContextPaneWidget::rePosition(const QPoint &position, const QPoint &alternative)
{
    if (position.y() > 0)
        move(position);
    else
        move(alternative);

    if (m_xPos > 0)
        move(m_xPos, pos().y());
}

void ContextPaneWidget::deactivate()
{
    hide();
    if (m_bauhausColorDialog)
        m_bauhausColorDialog->hide();
}

BauhausColorDialog *ContextPaneWidget::colorDialog()
{
    if (m_bauhausColorDialog.isNull()) {
        m_bauhausColorDialog = new BauhausColorDialog(parentWidget());
        m_bauhausColorDialog->hide();
    }

    return m_bauhausColorDialog.data();
}

void ContextPaneWidget::setProperties(::QmlJS::PropertyReader *propertyReader)
{
    ContextPaneTextWidget *textWidget = qobject_cast<ContextPaneTextWidget*>(m_currentWidget);
    if (textWidget)
        textWidget->setProperties(propertyReader);
}

bool ContextPaneWidget::setType(const QString &typeName)
{
    if (typeName.contains("Text")) {
        m_currentWidget = m_textWidget;
        m_textWidget->show();
        m_textWidget->setStyleVisible(true);
        m_textWidget->setVerticalAlignmentVisible(true);
        if (typeName.contains("TextInput")) {
            m_textWidget->setVerticalAlignmentVisible(false);
            m_textWidget->setStyleVisible(false);
        } else if (typeName.contains("TextEdit")) {
            m_textWidget->setStyleVisible(false);
        }
        return true;
    }

    m_textWidget->hide();
    return false;
}

void ContextPaneWidget::onTogglePane()
{
    if (!m_currentWidget)
        return;
    deactivate();
}

void ContextPaneWidget::onShowColorDialog(bool checked, const QPoint &p)
{    
    if (checked) {
        colorDialog()->setParent(parentWidget());
        colorDialog()->move(p);
        colorDialog()->show();
        colorDialog()->raise();
    } else {
        colorDialog()->hide();
    }
}

void ContextPaneWidget::mousePressEvent(QMouseEvent * event)
{
    if (event->button() ==  Qt::LeftButton) {
        m_oldPos = event->globalPos();
        m_opacityEffect = new QGraphicsOpacityEffect;
        setGraphicsEffect(m_opacityEffect);
        event->accept();
    }
    QFrame::mousePressEvent(event);
}

void ContextPaneWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() ==  Qt::LeftButton) {
        m_oldPos = QPoint(-1, -1);
        m_dropShadowEffect = new QGraphicsDropShadowEffect;
        m_dropShadowEffect->setBlurRadius(6);
        m_dropShadowEffect->setOffset(2, 2);
        setGraphicsEffect(m_dropShadowEffect);
    }
    QFrame::mouseReleaseEvent(event);
}

void ContextPaneWidget::mouseMoveEvent(QMouseEvent * event)
{
    if (event->buttons() &&  Qt::LeftButton) {

        if (m_oldPos != QPoint(-1, -1)) {
            QPoint diff = event->globalPos() - m_oldPos;
            move(pos() + diff);
            if (m_bauhausColorDialog)
                m_bauhausColorDialog->move(m_bauhausColorDialog->pos() + diff);
            m_xPos = pos().x();
        } else {
            m_opacityEffect = new QGraphicsOpacityEffect;
            setGraphicsEffect(m_opacityEffect);
        }
        m_oldPos = event->globalPos();
        event->accept();
    }
}

void ContextPaneWidget::onDisable()
{       
    DesignerSettings designerSettings = Internal::BauhausPlugin::pluginInstance()->settings();
    designerSettings.enableContextPane = false;
    Internal::BauhausPlugin::pluginInstance()->setSettings(designerSettings);
    hide();
    colorDialog()->hide();
}

QWidget* ContextPaneWidget::createFontWidget()
{    
    m_textWidget = new ContextPaneTextWidget(this);
    connect(m_textWidget, SIGNAL(propertyChanged(QString,QVariant)), this, SIGNAL(propertyChanged(QString,QVariant)));
    connect(m_textWidget, SIGNAL(removeProperty(QString)), this, SIGNAL(removeProperty(QString)));
    connect(m_textWidget, SIGNAL(removeAndChangeProperty(QString,QString,QVariant)), this, SIGNAL(removeAndChangeProperty(QString,QString,QVariant)));

    return m_textWidget;
}



} //QmlDesigner


