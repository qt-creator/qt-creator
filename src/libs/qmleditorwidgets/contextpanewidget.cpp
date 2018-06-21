/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "contextpanewidget.h"

#include <utils/hostosinfo.h>

#include <QToolButton>
#include <QFontComboBox>
#include <QComboBox>
#include <QSpinBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QGridLayout>
#include <QGraphicsEffect>
#include <QAction>
#include "contextpanetextwidget.h"
#include "contextpanewidgetimage.h"
#include "contextpanewidgetrectangle.h"
#include "customcolordialog.h"
#include "colorbutton.h"
#include <qmleditorwidgets/easingpane/easingcontextpane.h>

using namespace Utils;

namespace QmlEditorWidgets {

/* XPM */
static const char * pin_xpm[] = {
"12 9 7 1",
" 	c None",
".	c #000000",
"+	c #515151",
"@	c #A8A8A8",
"#	c #A9A9A9",
"$	c #999999",
"%	c #696969",
"     .      ",
"     ......+",
"     .@@@@@.",
"     .#####.",
"+.....$$$$$.",
"     .%%%%%.",
"     .......",
"     ......+",
"     .      "};

DragWidget::DragWidget(QWidget *parent) : QFrame(parent)
{
    setFrameStyle(QFrame::NoFrame);
    setFrameShape(QFrame::StyledPanel);
    setFrameShadow(QFrame::Sunken);
    m_startPos = QPoint(-1, -1);
    m_pos = QPoint(-1, -1);
    m_dropShadowEffect = new QGraphicsDropShadowEffect;
    m_dropShadowEffect->setBlurRadius(6);
    m_dropShadowEffect->setOffset(2, 2);
    setGraphicsEffect(m_dropShadowEffect);
}

void DragWidget::mousePressEvent(QMouseEvent * event)
{
    if (event->button() ==  Qt::LeftButton) {
        m_startPos = event->globalPos() - parentWidget()->mapToGlobal((pos()));
        m_opacityEffect = new QGraphicsOpacityEffect;
        setGraphicsEffect(m_opacityEffect);
        event->accept();
    }
    QFrame::mousePressEvent(event);
}

void DragWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() ==  Qt::LeftButton) {
        m_startPos = QPoint(-1, -1);
        m_dropShadowEffect = new QGraphicsDropShadowEffect;
        m_dropShadowEffect->setBlurRadius(6);
        m_dropShadowEffect->setOffset(2, 2);
        setGraphicsEffect(m_dropShadowEffect);
    }
    QFrame::mouseReleaseEvent(event);
}

static inline int limit(int a, int min, int max)
{
    if (a < min)
        return min;
    if (a > max)
        return max;
    return a;
}

void DragWidget::mouseMoveEvent(QMouseEvent * event)
{
    if (event->buttons() &  Qt::LeftButton) {
        if (m_startPos != QPoint(-1, -1)) {
            QPoint newPos = parentWidget()->mapFromGlobal(event->globalPos() - m_startPos);

            newPos.setX(limit(newPos.x(), 20, parentWidget()->width() - 20 - width()));
            newPos.setY(limit(newPos.y(), 2, parentWidget()->height() - 20 - height()));

            QPoint diff = pos() - newPos;
            if (m_secondaryTarget)
                m_secondaryTarget->move(m_secondaryTarget->pos() - diff);
            move(newPos);
            if (m_pos != newPos)
                protectedMoved();
            m_pos = newPos;
        } else {
            m_opacityEffect = new QGraphicsOpacityEffect;
            setGraphicsEffect(m_opacityEffect);
        }
        event->accept();
    }
}

void DragWidget::protectedMoved()
{

}

void DragWidget::leaveEvent(QEvent *)
{
    if (HostOsInfo::isMacHost())
        unsetCursor();
}

void DragWidget::enterEvent(QEvent *)
{
    if (HostOsInfo::isMacHost())
        setCursor(Qt::ArrowCursor);
}

ContextPaneWidget::ContextPaneWidget(QWidget *parent) : DragWidget(parent), m_currentWidget(0)
{
    QGridLayout *layout = new QGridLayout(this);
    layout->setMargin(0);
    layout->setContentsMargins(1, 1, 1, 1);
    layout->setSpacing(0);
    m_toolButton = new QToolButton(this);
    m_toolButton->setAutoRaise(false);

    m_toolButton->setIcon(style()->standardIcon(QStyle::SP_DockWidgetCloseButton));
    m_toolButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    m_toolButton->setFixedSize(16, 16);

    m_toolButton->setToolTip(tr("Hides this toolbar."));
    connect(m_toolButton, &QToolButton::clicked, this, &ContextPaneWidget::onTogglePane);
    layout->addWidget(m_toolButton, 0, 0, 1, 1);
    colorDialog();

    QWidget *fontWidget = createFontWidget();
    m_currentWidget = fontWidget;
    QWidget *imageWidget = createImageWidget();
    QWidget *borderImageWidget = createBorderImageWidget();
    QWidget *rectangleWidget = createRectangleWidget();
    QWidget *easingWidget = createEasingWidget();
    layout->addWidget(fontWidget, 0, 1, 2, 1);
    layout->addWidget(easingWidget, 0, 1, 2, 1);
    layout->addWidget(imageWidget, 0, 1, 2, 1);
    layout->addWidget(borderImageWidget, 0, 1, 2, 1);
    layout->addWidget(rectangleWidget, 0, 1, 2, 1);

    setAutoFillBackground(true);
    setContextMenuPolicy(Qt::ActionsContextMenu);

    m_resetAction = new QAction(tr("Pin Toolbar"), this);
    m_resetAction->setCheckable(true);
    addAction(m_resetAction.data());
    connect(m_resetAction.data(), &QAction::triggered, this, &ContextPaneWidget::onResetPosition);

    m_disableAction = new QAction(tr("Show Always"), this);
    addAction(m_disableAction.data());
    m_disableAction->setCheckable(true);
    connect(m_disableAction.data(), &QAction::toggled, this, &ContextPaneWidget::onDisable);
    m_pinned = false;
    if (HostOsInfo::isMacHost())
        setCursor(Qt::ArrowCursor);
}

ContextPaneWidget::~ContextPaneWidget()
{
    //if the pane was never activated the widget is not in a widget tree
    if (!m_bauhausColorDialog.isNull()) {
        delete m_bauhausColorDialog.data();
        m_bauhausColorDialog = 0;
    }
}

void ContextPaneWidget::activate(const QPoint &pos, const QPoint &alternative, const QPoint &alternative2, bool pinned)
{
    //uncheck all color buttons
    foreach (ColorButton *colorButton, findChildren<ColorButton*>()) {
            colorButton->setChecked(false);
    }
    show();
    update();
    resize(sizeHint());
    show();
    rePosition(pos, alternative, alternative2, pinned);
    raise();
}

void ContextPaneWidget::rePosition(const QPoint &position, const QPoint &alternative, const QPoint &alternative2, bool pinned)
{
    if ((position.x()  + width()) < parentWidget()->width())
        move(position);
    else
        move(alternative);

    if (pos().y() < 0)
        move(alternative2);
    if ((pos().y() + height()) > parentWidget()->height())
        move(x(), parentWidget()->height() - height() - 10);

    m_originalPos = pos();

    if (m_pos.x() > 0 && pinned) {
        move(m_pos);
        show();
        setPinButton();
    } else {
        setLineButton();
    }
}

void ContextPaneWidget::deactivate()
{
    hide();
    if (m_bauhausColorDialog)
        m_bauhausColorDialog->hide();
}

void ContextPaneWidget::setOptions(bool enabled, bool pinned)
{

    m_disableAction->setChecked(enabled);
    m_resetAction->setChecked(pinned);
}

CustomColorDialog *ContextPaneWidget::colorDialog()
{
    if (m_bauhausColorDialog.isNull()) {
        m_bauhausColorDialog = new CustomColorDialog(parentWidget());
        m_bauhausColorDialog->hide();
        setSecondaryTarget(m_bauhausColorDialog.data());
    }

    return m_bauhausColorDialog.data();
}

void ContextPaneWidget::setProperties(::QmlJS::PropertyReader *propertyReader)
{
    ContextPaneTextWidget *textWidget = qobject_cast<ContextPaneTextWidget*>(m_currentWidget);
    if (textWidget)
        textWidget->setProperties(propertyReader);

    EasingContextPane *easingWidget = qobject_cast<EasingContextPane*>(m_currentWidget);
    if (easingWidget)
        easingWidget->setProperties(propertyReader);

    ContextPaneWidgetImage *imageWidget = qobject_cast<ContextPaneWidgetImage*>(m_currentWidget);
    if (imageWidget)
        imageWidget->setProperties(propertyReader);

    ContextPaneWidgetRectangle *rectangleWidget = qobject_cast<ContextPaneWidgetRectangle*>(m_currentWidget);
    if (rectangleWidget)
        rectangleWidget->setProperties(propertyReader);
}

void ContextPaneWidget::setPath(const QString &path)
{
    ContextPaneWidgetImage *imageWidget = qobject_cast<ContextPaneWidgetImage*>(m_currentWidget);
    if (imageWidget)
        imageWidget->setPath(path);

}

bool ContextPaneWidget::setType(const QStringList &types)
{
    m_imageWidget->hide();
    m_borderImageWidget->hide();
    m_textWidget->hide();
    m_rectangleWidget->hide();
    m_easingWidget->hide();

    if (types.contains(QLatin1String("Text")) ||
        types.contains(QLatin1String("TextEdit")) ||
        types.contains(QLatin1String("TextInput"))) {
        m_currentWidget = m_textWidget;
        m_textWidget->show();
        m_textWidget->setStyleVisible(true);
        m_textWidget->setVerticalAlignmentVisible(true);
        if (types.contains(QLatin1String("TextInput"))) {
            m_textWidget->setVerticalAlignmentVisible(false);
            m_textWidget->setStyleVisible(false);
        } else if (types.contains(QLatin1String("TextEdit"))) {
            m_textWidget->setStyleVisible(false);
        }
        resize(sizeHint());
        return true;
    }

    if (m_easingWidget->acceptsType(types)) {
        m_currentWidget = m_easingWidget;
        m_easingWidget->show();
        resize(sizeHint());
        return true;
    }
    if (types.contains(QLatin1String("Rectangle"))) {
        m_currentWidget = m_rectangleWidget;
        m_rectangleWidget->enabableGradientEditing(!isPropertyChanges());
        m_rectangleWidget->show();
        resize(sizeHint());
        return true;
    }

    if (types.contains(QLatin1String("BorderImage"))) {
        m_currentWidget = m_borderImageWidget;
        m_borderImageWidget->show();
        resize(sizeHint());
        return true;
    }

    if (types.contains(QLatin1String("Image"))) {
        m_currentWidget = m_imageWidget;
        m_imageWidget->show();
        resize(sizeHint());
        return true;
    }
    return false;
}

bool ContextPaneWidget::acceptsType(const QStringList &types)
{
    return types.contains(QLatin1String("Text")) || types.contains(QLatin1String("TextEdit")) ||
            types.contains(QLatin1String("TextInput")) || m_easingWidget->acceptsType(types) ||
            types.contains(QLatin1String("Rectangle")) || types.contains(QLatin1String("Image")) ||
            types.contains(QLatin1String("BorderImage"));
}

void ContextPaneWidget::onTogglePane()
{
    if (!m_currentWidget)
        return;
    if (m_pinned) {
        m_pos = QPoint(-1,-1);
        move(m_originalPos);
        setLineButton();
    } else {
        deactivate();
        emit closed();
    }
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

void ContextPaneWidget::onDisable(bool b)
{
    enabledChanged(b);
    if (!b) {
        hide();
        colorDialog()->hide();
    }
}

void  ContextPaneWidget::onResetPosition(bool toggle)
{
    if (!toggle) {
        setLineButton();
        m_pos = QPoint(-1,-1);
        move(m_originalPos);
    } else {
        setPinButton();
    }
}

void ContextPaneWidget::protectedMoved()
{
    setPinButton();
}

QWidget* ContextPaneWidget::createFontWidget()
{
    m_textWidget = new ContextPaneTextWidget(this);
    connect(m_textWidget, &ContextPaneTextWidget::propertyChanged,
            this, &ContextPaneWidget::propertyChanged);
    connect(m_textWidget, &ContextPaneTextWidget::removeProperty,
            this, &ContextPaneWidget::removeProperty);
    connect(m_textWidget, &ContextPaneTextWidget::removeAndChangeProperty,
            this, &ContextPaneWidget::removeAndChangeProperty);

    return m_textWidget;
}

QWidget* ContextPaneWidget::createEasingWidget()
{
    m_easingWidget = new EasingContextPane(this);

    connect(m_easingWidget, &EasingContextPane::propertyChanged,
            this, &ContextPaneWidget::propertyChanged);
    connect(m_easingWidget, &EasingContextPane::removeProperty,
            this, &ContextPaneWidget::removeProperty);
    connect(m_easingWidget, &EasingContextPane::removeAndChangeProperty,
            this, &ContextPaneWidget::removeAndChangeProperty);

    return m_easingWidget;
}

QWidget *ContextPaneWidget::createImageWidget()
{
    m_imageWidget = new ContextPaneWidgetImage(this);
    connect(m_imageWidget, &ContextPaneWidgetImage::propertyChanged,
            this, &ContextPaneWidget::propertyChanged);
    connect(m_imageWidget, &ContextPaneWidgetImage::removeProperty,
            this, &ContextPaneWidget::removeProperty);
    connect(m_imageWidget, &ContextPaneWidgetImage::removeAndChangeProperty,
            this, &ContextPaneWidget::removeAndChangeProperty);

    return m_imageWidget;
}

QWidget *ContextPaneWidget::createBorderImageWidget()
{
    m_borderImageWidget = new ContextPaneWidgetImage(this, true);
    connect(m_borderImageWidget, &ContextPaneWidgetImage::propertyChanged,
            this, &ContextPaneWidget::propertyChanged);
    connect(m_borderImageWidget, &ContextPaneWidgetImage::removeProperty,
            this, &ContextPaneWidget::removeProperty);
    connect(m_borderImageWidget, &ContextPaneWidgetImage::removeAndChangeProperty,
            this, &ContextPaneWidget::removeAndChangeProperty);

    return m_borderImageWidget;

}

QWidget *ContextPaneWidget::createRectangleWidget()
{
    m_rectangleWidget = new ContextPaneWidgetRectangle(this);
    connect(m_rectangleWidget, &ContextPaneWidgetRectangle::propertyChanged,
            this, &ContextPaneWidget::propertyChanged);
    connect(m_rectangleWidget, &ContextPaneWidgetRectangle::removeProperty,
            this, &ContextPaneWidget::removeProperty);
    connect(m_rectangleWidget, &ContextPaneWidgetRectangle::removeAndChangeProperty,
            this, &ContextPaneWidget::removeAndChangeProperty);

    return m_rectangleWidget;
}

void ContextPaneWidget::setPinButton()
{
    m_toolButton->setAutoRaise(true);
    m_pinned = true;

    m_toolButton->setIcon(QPixmap::fromImage(QImage(pin_xpm)));
    m_toolButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    m_toolButton->setFixedSize(20, 20);
    m_toolButton->setToolTip(tr("Unpins the toolbar and moves it to the default position."));

    pinnedChanged(true);
    if (m_resetAction) {
        QSignalBlocker blocker(m_resetAction);
        m_resetAction->setChecked(true);
    }
}

void ContextPaneWidget::setLineButton()
{
    m_pinned = false;
    m_toolButton->setAutoRaise(true);
    m_toolButton->setIcon(style()->standardIcon(QStyle::SP_DockWidgetCloseButton));
    m_toolButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    m_toolButton->setFixedSize(20, 20);
    m_toolButton->setToolTip(tr("Hides this toolbar. This toolbar can be"
                                " permanently disabled in the options page or in the context menu."));

    pinnedChanged(false);
    if (m_resetAction) {
        QSignalBlocker blocker(m_resetAction);
        m_resetAction->setChecked(false);
    }
}

} //QmlDesigner


