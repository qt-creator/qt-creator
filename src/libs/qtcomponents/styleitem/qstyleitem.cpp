/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** All rights reserved.
** Contact: http://www.qt-project.org/
**
** This file is part of the examples of the Qt Toolkit.
**
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOTgall
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qstyleitem.h"

#include <QPainter>
#include <QStyle>
#include <QStyleOption>
#include <QApplication>
#include <QMainWindow>
#include <QGroupBox>
#include <QToolBar>
#include <QMenu>
#include <QStringBuilder>


QStyleItem::QStyleItem(QDeclarativeItem *parent)
    : QDeclarativeItem(parent),
    m_dummywidget(0),
    m_styleoption(0),
    m_type(Undefined),
    m_sunken(false),
    m_raised(false),
    m_active(true),
    m_selected(false),
    m_focus(false),
    m_on(false),
    m_horizontal(true),
    m_sharedWidget(false),
    m_minimum(0),
    m_maximum(100),
    m_value(0),
    m_paintMargins(0)
{
    setFlag(QGraphicsItem::ItemHasNoContents, false);
    setCacheMode(QGraphicsItem::DeviceCoordinateCache);
    setSmooth(true);

    connect(this, SIGNAL(infoChanged()), this, SLOT(updateItem()));
    connect(this, SIGNAL(onChanged()), this, SLOT(updateItem()));
    connect(this, SIGNAL(selectedChanged()), this, SLOT(updateItem()));
    connect(this, SIGNAL(activeChanged()), this, SLOT(updateItem()));
    connect(this, SIGNAL(textChanged()), this, SLOT(updateItem()));
    connect(this, SIGNAL(activeChanged()), this, SLOT(updateItem()));
    connect(this, SIGNAL(raisedChanged()), this, SLOT(updateItem()));
    connect(this, SIGNAL(sunkenChanged()), this, SLOT(updateItem()));
    connect(this, SIGNAL(hoverChanged()), this, SLOT(updateItem()));
    connect(this, SIGNAL(maximumChanged()), this, SLOT(updateItem()));
    connect(this, SIGNAL(minimumChanged()), this, SLOT(updateItem()));
    connect(this, SIGNAL(valueChanged()), this, SLOT(updateItem()));
    connect(this, SIGNAL(horizontalChanged()), this, SLOT(updateItem()));
    connect(this, SIGNAL(activeControlChanged()), this, SLOT(updateItem()));
    connect(this, SIGNAL(focusChanged()), this, SLOT(updateItem()));
    connect(this, SIGNAL(activeControlChanged()), this, SLOT(updateItem()));
    connect(this, SIGNAL(elementTypeChanged()), this, SLOT(updateItem()));
}

QStyleItem::~QStyleItem()
{
    delete m_styleoption;
    m_styleoption = 0;

    if (!m_sharedWidget) {
        delete m_dummywidget;
        m_dummywidget = 0;
    }
}

void QStyleItem::initStyleOption()
{
    QString type = elementType();
    if (m_styleoption)
        m_styleoption->state = 0;

    switch (m_itemType) {
    case Button: {
            if (!m_styleoption)
                m_styleoption = new QStyleOptionButton();

            QStyleOptionButton *opt = qstyleoption_cast<QStyleOptionButton*>(m_styleoption);
            opt->text = text();
            opt->features = (activeControl() == QLatin1String("default")) ?
                            QStyleOptionButton::DefaultButton :
                            QStyleOptionButton::None;
        }
        break;
    case ItemRow: {
            if (!m_styleoption)
                m_styleoption = new QStyleOptionViewItemV4();

            QStyleOptionViewItemV4 *opt = qstyleoption_cast<QStyleOptionViewItemV4*>(m_styleoption);
            opt->features = 0;
            if (activeControl() == QLatin1String("alternate"))
                opt->features |= QStyleOptionViewItemV2::Alternate;
        }
        break;

    case Splitter: {
            if (!m_styleoption)
                m_styleoption = new QStyleOption;
        }
        break;

    case Item: {
            if (!m_styleoption)
                m_styleoption = new QStyleOptionViewItemV4();
            QStyleOptionViewItemV4 *opt = qstyleoption_cast<QStyleOptionViewItemV4*>(m_styleoption);
            opt->features = QStyleOptionViewItemV4::HasDisplay;
            opt->text = text();
            opt->textElideMode = Qt::ElideRight;
            QPalette pal = m_styleoption->palette;
            pal.setBrush(QPalette::Base, Qt::NoBrush);
            m_styleoption->palette = pal;
        }
        break;
    case Header: {
            if (!m_styleoption)
                m_styleoption = new QStyleOptionHeader();

            QStyleOptionHeader *opt = qstyleoption_cast<QStyleOptionHeader*>(m_styleoption);
            opt->text = text();
            opt->sortIndicator = activeControl() == QLatin1String("down") ?
                                 QStyleOptionHeader::SortDown
                                     : activeControl() == QLatin1String("up") ?
                                     QStyleOptionHeader::SortUp : QStyleOptionHeader::None;
            if (activeControl() == QLatin1String("beginning"))
                opt->position = QStyleOptionHeader::Beginning;
            else if (activeControl() == QLatin1String("end"))
                opt->position = QStyleOptionHeader::End;
            else if (activeControl() == QLatin1String("only"))
                opt->position = QStyleOptionHeader::OnlyOneSection;
            else
                opt->position = QStyleOptionHeader::Middle;

        }
        break;
    case ToolButton :{
            if (!m_styleoption)
                m_styleoption = new QStyleOptionToolButton();

            QStyleOptionToolButton *opt =
                    qstyleoption_cast<QStyleOptionToolButton*>(m_styleoption);
            opt->subControls = QStyle::SC_ToolButton;
            opt->state |= QStyle::State_AutoRaise;
            opt->activeSubControls = QStyle::SC_ToolButton;
        }
        break;
    case ToolBar: {
            if (!m_styleoption)
                m_styleoption = new QStyleOptionToolBar();
        }
        break;
    case Tab: {
            if (!m_styleoption)
                m_styleoption = new QStyleOptionTabV3();

            QStyleOptionTabV3 *opt =
                    qstyleoption_cast<QStyleOptionTabV3*>(m_styleoption);
            opt->text = text();
            opt->shape = info() == QLatin1String("South") ? QTabBar::RoundedSouth : QTabBar::RoundedNorth;
            if (activeControl() == QLatin1String("beginning"))
                opt->position = QStyleOptionTabV3::Beginning;
            else if (activeControl() == QLatin1String("end"))
                opt->position = QStyleOptionTabV3::End;
            else if (activeControl() == QLatin1String("only"))
                opt->position = QStyleOptionTabV3::OnlyOneTab;
            else
                opt->position = QStyleOptionTabV3::Middle;

        } break;

    case Menu: {
            if (!m_styleoption)
                m_styleoption = new QStyleOptionMenuItem();
        }
        break;
    case Frame: {
            if (!m_styleoption)
                m_styleoption = new QStyleOptionFrameV3();

            QStyleOptionFrameV3 *opt = qstyleoption_cast<QStyleOptionFrameV3*>(m_styleoption);
            opt->frameShape = QFrame::StyledPanel;
            opt->lineWidth = 1;
            opt->midLineWidth = 1;
        }
        break;
    case TabFrame: {
            if (!m_styleoption)
                m_styleoption = new QStyleOptionTabWidgetFrameV2();
            QStyleOptionTabWidgetFrameV2 *opt = qstyleoption_cast<QStyleOptionTabWidgetFrameV2*>(m_styleoption);
            opt->shape = (info() == QLatin1String("South")) ? QTabBar::RoundedSouth : QTabBar::RoundedNorth;
            if (minimum())
                opt->selectedTabRect = QRect(value(), 0, minimum(), height());
            opt->tabBarSize = QSize(minimum() , height());
            // oxygen style needs this hack
            opt->leftCornerWidgetSize = QSize(value(), 0);
        }
        break;
    case MenuItem:
    case ComboBoxItem:
        {
            if (!m_styleoption)
                m_styleoption = new QStyleOptionMenuItem();

            QStyleOptionMenuItem *opt = qstyleoption_cast<QStyleOptionMenuItem*>(m_styleoption);
            opt->checked = false;
            opt->text = text();
            opt->palette = widget()->palette();
        }
        break;
    case CheckBox:
    case RadioButton:
        {
            if (!m_styleoption)
                m_styleoption = new QStyleOptionButton();

            QStyleOptionButton *opt = qstyleoption_cast<QStyleOptionButton*>(m_styleoption);
            if (!on())
                opt->state |= QStyle::State_Off;
            opt->text = text();
        }
        break;
    case Edit: {
            if (!m_styleoption)
                m_styleoption = new QStyleOptionFrameV3();

            QStyleOptionFrameV3 *opt = qstyleoption_cast<QStyleOptionFrameV3*>(m_styleoption);
            opt->lineWidth = 1; // this must be non-zero
        }
        break;
    case ComboBox :{
            if (!m_styleoption)
                m_styleoption = new QStyleOptionComboBox();
            QStyleOptionComboBox *opt = qstyleoption_cast<QStyleOptionComboBox*>(m_styleoption);
            opt->currentText = text();
        }
        break;
    case SpinBox: {
            if (!m_styleoption)
                m_styleoption = new QStyleOptionSpinBox();

            QStyleOptionSpinBox *opt = qstyleoption_cast<QStyleOptionSpinBox*>(m_styleoption);
            opt->frame = true;
            if (value() & 0x1)
                opt->activeSubControls = QStyle::SC_SpinBoxUp;
            else if (value() & (1<<1))
                opt->activeSubControls = QStyle::SC_SpinBoxDown;
            opt->subControls = QStyle::SC_All;
            opt->stepEnabled = 0;
            if (value() & (1<<2))
                opt->stepEnabled |= QAbstractSpinBox::StepUpEnabled;
            if (value() & (1<<3))
                opt->stepEnabled |= QAbstractSpinBox::StepDownEnabled;
        }
        break;
    case Slider:
    case Dial:
        {
            if (!m_styleoption)
                m_styleoption = new QStyleOptionSlider();

            QStyleOptionSlider *opt = qstyleoption_cast<QStyleOptionSlider*>(m_styleoption);
            opt->minimum = minimum();
            opt->maximum = maximum();
            // ### fixme - workaround for KDE inverted dial
            opt->sliderPosition = value();
            opt->singleStep = step();

            if (opt->singleStep)
            {
                qreal numOfSteps = (opt->maximum - opt->minimum) / opt->singleStep;

                // at least 5 pixels between tick marks
                if (numOfSteps && (width() / numOfSteps < 5))
                    opt->tickInterval = qRound((5*numOfSteps / width()) + 0.5)*step();
                else
                    opt->tickInterval = opt->singleStep;
            }
            else // default Qt-components implementation
                opt->tickInterval = opt->maximum != opt->minimum ? 1200 / (opt->maximum - opt->minimum) : 0;

            if (style() == QLatin1String("oxygen") && type == QLatin1String("dial"))
                opt->sliderValue  = maximum() - value();
            else
                opt->sliderValue = value();
            opt->subControls = QStyle::SC_SliderGroove | QStyle::SC_SliderHandle;
            opt->tickPosition = (activeControl() == QLatin1String("below")) ?
                                QSlider::TicksBelow : (activeControl() == QLatin1String("above") ?
                                                       QSlider::TicksAbove:
                                                       QSlider::NoTicks);
            if (opt->tickPosition != QSlider::NoTicks)
                opt->subControls |= QStyle::SC_SliderTickmarks;

            opt->activeSubControls = QStyle::SC_None;
        }
        break;
    case ProgressBar: {
            if (QProgressBar *bar= qobject_cast<QProgressBar*>(widget())){
                bar->setMaximum(maximum());
                bar->setMinimum(minimum());
                if (maximum() != minimum())
                    bar->setValue(1);
            }
            if (!m_styleoption)
                m_styleoption = new QStyleOptionProgressBarV2();

            QStyleOptionProgressBarV2 *opt = qstyleoption_cast<QStyleOptionProgressBarV2*>(m_styleoption);
            opt->orientation = horizontal() ? Qt::Horizontal : Qt::Vertical;
            opt->minimum = minimum();
            opt->maximum = maximum();
            opt->progress = value();
        }
        break;
    case GroupBox: {
            if (!m_styleoption)
                m_styleoption = new QStyleOptionGroupBox();

            QStyleOptionGroupBox *opt = qstyleoption_cast<QStyleOptionGroupBox*>(m_styleoption);
            opt->text = text();
            opt->lineWidth = 1;
            opt->subControls = QStyle::SC_GroupBoxLabel;
            if (sunken()) // Qt draws an ugly line here so I ignore it
                opt->subControls |= QStyle::SC_GroupBoxFrame;
            else
                opt->features |= QStyleOptionFrameV2::Flat;
            if (activeControl() == QLatin1String("checkbox"))
                opt->subControls |= QStyle::SC_GroupBoxCheckBox;

            if (QGroupBox *group= qobject_cast<QGroupBox*>(widget())) {
                group->setTitle(text());
                group->setCheckable(opt->subControls & QStyle::SC_GroupBoxCheckBox);
            }
        }
        break;
    case ScrollBar: {
            if (!m_styleoption)
                m_styleoption = new QStyleOptionSlider();

            QStyleOptionSlider *opt = qstyleoption_cast<QStyleOptionSlider*>(m_styleoption);
            opt->minimum = minimum();
            opt->maximum = maximum();
            opt->pageStep = horizontal() ? width() : height();
            opt->orientation = horizontal() ? Qt::Horizontal : Qt::Vertical;
            opt->sliderPosition = value();
            opt->sliderValue = value();
            opt->activeSubControls = (activeControl() == QLatin1String("up"))
                                     ? QStyle::SC_ScrollBarSubLine :
                                     (activeControl() == QLatin1String("down")) ?
                                     QStyle::SC_ScrollBarAddLine:
                                     QStyle::SC_ScrollBarSlider;

            opt->sliderValue = value();
            opt->subControls = QStyle::SC_All;

            QScrollBar *bar = qobject_cast<QScrollBar *>(widget());
            bar->setMaximum(maximum());
            bar->setMinimum(minimum());
            bar->setValue(value());
        }
        break;
    default:
        break;
    }

    if (!m_styleoption)
        m_styleoption = new QStyleOption();

    m_styleoption->rect = QRect(m_paintMargins, m_paintMargins, width() - 2* m_paintMargins, height() - 2 * m_paintMargins);
#if QT_VERSION >= 0x050000
    m_styleoption->styleObject = this;
#endif

    if (isEnabled())
        m_styleoption->state |= QStyle::State_Enabled;
    if (m_active)
        m_styleoption->state |= QStyle::State_Active;
    if (m_sunken)
        m_styleoption->state |= QStyle::State_Sunken;
    if (m_raised)
        m_styleoption->state |= QStyle::State_Raised;
    if (m_selected)
        m_styleoption->state |= QStyle::State_Selected;
    if (m_focus)
        m_styleoption->state |= QStyle::State_HasFocus;
    if (m_on)
        m_styleoption->state |= QStyle::State_On;
    if (m_hover)
        m_styleoption->state |= QStyle::State_MouseOver;
    if (m_horizontal)
        m_styleoption->state |= QStyle::State_Horizontal;

    if (widget()) {
        widget()->ensurePolished();
        if (type == QLatin1String("tab") && style() != QLatin1String("mac")) {
            // Some styles actually check the beginning and end position
            // using widget geometry, so we have to trick it
            widget()->setGeometry(0, 0, width(), height());
            if (activeControl() != QLatin1String("beginning"))
                m_styleoption->rect.translate(1, 0); // Don't position at start of widget
            if (activeControl() != QLatin1String("end"))
                widget()->resize(200, height());
        }
#ifdef Q_WS_WIN
        else widget()->resize(width(), height());
#endif

        widget()->setEnabled(isEnabled());
        m_styleoption->fontMetrics = widget()->fontMetrics();
        if (!m_styleoption->palette.resolve())
            m_styleoption->palette = widget()->palette();
        if (m_hint.contains(QLatin1String("mac.mini")))
            widget()->setAttribute(Qt::WA_MacMiniSize);
        else if (m_hint.contains(QLatin1String("mac.small")))
            widget()->setAttribute(Qt::WA_MacSmallSize);
    }
}

/*
 *   Property style
 *
 *   Returns a simplified style name.
 *
 *   QMacStyle = "mac"
 *   QWindowsXPStyle = "windowsxp"
 *   QPlastiqueStyle = "plastique"
 */

QString QStyleItem::style() const
{
    QByteArray style = qApp->style()->metaObject()->className();
    style = style.toLower();
    if (style.contains("oxygen"))
        return QLatin1String("oxygen");
    if (style.startsWith('q'))
        style.remove(0, 1);
    if (style.endsWith("style"))
        style.chop(5);
    return QLatin1String(style);
}

QString QStyleItem::hitTest(int px, int py)
{
    QStyle::SubControl subcontrol = QStyle::SC_All;
    initStyleOption();
    switch (m_itemType) {
    case SpinBox :{
            subcontrol = qApp->style()->hitTestComplexControl(QStyle::CC_SpinBox,
                                                              qstyleoption_cast<QStyleOptionComplex*>(m_styleoption),
                                                              QPoint(px,py), 0);
            if (subcontrol == QStyle::SC_SpinBoxUp)
                return QLatin1String("up");
            else if (subcontrol == QStyle::SC_SpinBoxDown)
                return QLatin1String("down");

        }
        break;

    case Slider: {
            subcontrol = qApp->style()->hitTestComplexControl(QStyle::CC_Slider,
                                                              qstyleoption_cast<QStyleOptionComplex*>(m_styleoption),
                                                              QPoint(px,py), 0);
            if (subcontrol == QStyle::SC_SliderHandle)
                return QLatin1String("handle");

        }
        break;
    case ScrollBar: {
        subcontrol = qApp->style()->hitTestComplexControl(QStyle::CC_ScrollBar,
                                                          qstyleoption_cast<QStyleOptionComplex*>(m_styleoption),
                                                          QPoint(px,py), 0);
        if (subcontrol == QStyle::SC_ScrollBarSlider)
            return QLatin1String("handle");

        if (subcontrol == QStyle::SC_ScrollBarSubLine)
            return QLatin1String("up");
        else if (subcontrol == QStyle::SC_ScrollBarSubPage)
            return QLatin1String("upPage");

        if (subcontrol == QStyle::SC_ScrollBarAddLine)
            return QLatin1String("down");
        else if (subcontrol == QStyle::SC_ScrollBarAddPage)
            return QLatin1String("downPage");
        }
        break;
    default:
        break;
    }
    return QLatin1String("none");
}

QSize QStyleItem::sizeFromContents(int width, int height)
{
    initStyleOption();

    QSize size;
    switch (m_itemType) {
    case CheckBox:
        size =  qApp->style()->sizeFromContents(QStyle::CT_CheckBox, m_styleoption, QSize(width,height), widget());
        break;
    case ToolButton:
        size = qApp->style()->sizeFromContents(QStyle::CT_ToolButton, m_styleoption, QSize(width,height), widget());
        break;
    case Button:
        size = qApp->style()->sizeFromContents(QStyle::CT_PushButton, m_styleoption, QSize(width,height), widget());
        break;
    case Tab:
        size = qApp->style()->sizeFromContents(QStyle::CT_TabBarTab, m_styleoption, QSize(width,height), widget());
        break;
    case ComboBox:
        size = qApp->style()->sizeFromContents(QStyle::CT_ComboBox, m_styleoption, QSize(width,height), widget());
        break;
    case SpinBox:
        size = qApp->style()->sizeFromContents(QStyle::CT_SpinBox, m_styleoption, QSize(width,height), widget());
        break;
    case Slider:
        size = qApp->style()->sizeFromContents(QStyle::CT_Slider, m_styleoption, QSize(width,height), widget());
        break;
    case ProgressBar:
        size = qApp->style()->sizeFromContents(QStyle::CT_ProgressBar, m_styleoption, QSize(width,height), widget());
        break;
    case Edit:
        size = qApp->style()->sizeFromContents(QStyle::CT_LineEdit, m_styleoption, QSize(width,height), widget());
        break;
    case GroupBox:
        size = qApp->style()->sizeFromContents(QStyle::CT_GroupBox, m_styleoption, QSize(width,height), widget());
        break;
    case Header:
        size = qApp->style()->sizeFromContents(QStyle::CT_HeaderSection, m_styleoption, QSize(width,height), widget());
#ifdef Q_OS_MAC
        if (style() == QLatin1String("mac"))
            size.setHeight(15);
#endif
        break;
    case ItemRow:
    case Item: //fall through
        size = qApp->style()->sizeFromContents(QStyle::CT_ItemViewItem, m_styleoption, QSize(width,height), widget());
        break;
    default:
        break;
    }

#ifdef Q_OS_MAC
    // ### hack - With even heights, the text baseline is off on mac
    if (size.height() %2 == 0)
        size.setHeight(size.height() + 1);
#endif
    return size;
}


int QStyleItem::pixelMetric(const QString &metric)
{

    if (metric == QLatin1String("scrollbarExtent"))
        return qApp->style()->pixelMetric(QStyle::PM_ScrollBarExtent, 0, widget());
    else if (metric == QLatin1String("defaultframewidth"))
        return qApp->style()->pixelMetric(QStyle::PM_DefaultFrameWidth, 0, widget());
    else if (metric == QLatin1String("taboverlap"))
        return qApp->style()->pixelMetric(QStyle::PM_TabBarTabOverlap, 0 , widget());
    else if (metric == QLatin1String("tabbaseoverlap"))
#ifdef Q_WS_WIN
        // On windows the tabbar paintmargin extends the overlap by one pixels
        return 1 + qApp->style()->pixelMetric(QStyle::PM_TabBarBaseOverlap, 0 , widget());
#else
    return qApp->style()->pixelMetric(QStyle::PM_TabBarBaseOverlap, 0 , widget());
#endif
    else if (metric == QLatin1String("tabhspace"))
        return qApp->style()->pixelMetric(QStyle::PM_TabBarTabHSpace, 0 , widget());
    else if (metric == QLatin1String("tabvspace"))
        return qApp->style()->pixelMetric(QStyle::PM_TabBarTabVSpace, 0 , widget());
    else if (metric == QLatin1String("tabbaseheight"))
        return qApp->style()->pixelMetric(QStyle::PM_TabBarBaseHeight, 0 , widget());
    else if (metric == QLatin1String("tabvshift"))
        return qApp->style()->pixelMetric(QStyle::PM_TabBarTabShiftVertical, 0 , widget());
    else if (metric == QLatin1String("menuhmargin"))
        return qApp->style()->pixelMetric(QStyle::PM_MenuHMargin, 0 , widget());
    else if (metric == QLatin1String("menuvmargin"))
        return qApp->style()->pixelMetric(QStyle::PM_MenuVMargin, 0 , widget());
    else if (metric == QLatin1String("menupanelwidth"))
        return qApp->style()->pixelMetric(QStyle::PM_MenuPanelWidth, 0 , widget());
    else if (metric == QLatin1String("splitterwidth"))
        return qApp->style()->pixelMetric(QStyle::PM_SplitterWidth, 0 , widget());
    // This metric is incorrectly negative on oxygen
    else if (metric == QLatin1String("scrollbarspacing"))
        return abs(qApp->style()->pixelMetric(QStyle::PM_ScrollView_ScrollBarSpacing, 0 , widget()));
    return 0;
}

QVariant QStyleItem::styleHint(const QString &metric)
{
    initStyleOption();
    if (metric == QLatin1String("comboboxpopup")) {
        return qApp->style()->styleHint(QStyle::SH_ComboBox_Popup, m_styleoption);
    } else if (metric == QLatin1String("highlightedTextColor")) {
        if (widget())
            return widget()->palette().highlightedText().color().name();
        return qApp->palette().highlightedText().color().name();
    } else if (metric == QLatin1String("textColor")) {
        if (widget())
            return widget()->palette().text().color().name();
        return qApp->palette().text().color().name();
    } else if (metric == QLatin1String("focuswidget")) {
        return qApp->style()->styleHint(QStyle::SH_FocusFrame_AboveWidget);
    } else if (metric == QLatin1String("tabbaralignment")) {
        int result = qApp->style()->styleHint(QStyle::SH_TabBar_Alignment);
        if (result == Qt::AlignCenter)
            return QLatin1String("center");
        return QLatin1String("left");
    } else if (metric == QLatin1String("framearoundcontents")) {
        return qApp->style()->styleHint(QStyle::SH_ScrollView_FrameOnlyAroundContents);
    } else if (metric == QLatin1String("scrollToClickPosition"))
        return qApp->style()->styleHint(QStyle::SH_ScrollBar_LeftClickAbsolutePosition);
    return 0;
}

void QStyleItem::setCursor(const QString &str)
{
    if (m_cursor != str) {
        m_cursor = str;
        if (m_cursor == QLatin1String("sizehorcursor"))
            QDeclarativeItem::setCursor(Qt::SizeHorCursor);
        else if (m_cursor == QLatin1String("sizevercursor"))
            QDeclarativeItem::setCursor(Qt::SizeVerCursor);
        else if (m_cursor == QLatin1String("sizeallcursor"))
            QDeclarativeItem::setCursor(Qt::SizeAllCursor);
        else if (m_cursor == QLatin1String("splithcursor"))
            QDeclarativeItem::setCursor(Qt::SplitHCursor);
        else if (m_cursor == QLatin1String("splitvcursor"))
            QDeclarativeItem::setCursor(Qt::SplitVCursor);
        else if (m_cursor == QLatin1String("wait"))
            QDeclarativeItem::setCursor(Qt::WaitCursor);
        else if (m_cursor == QLatin1String("pointinghandcursor"))
            QDeclarativeItem::setCursor(Qt::PointingHandCursor);
        emit cursorChanged();
    }
}

void QStyleItem::setElementType(const QString &str)
{
    if (m_type == str)
        return;

    m_type = str;

    emit elementTypeChanged();

    if (m_dummywidget && !m_sharedWidget) {
        delete m_dummywidget;
        m_dummywidget = 0;
    }

    if (m_styleoption) {
        delete m_styleoption;
        m_styleoption = 0;
    }

    // Only enable visible if the widget can animate
    bool visible = false;
    if (str == QLatin1String("menu") || str == QLatin1String("menuitem")) {
        // Since these are used by the delegate, it makes no
        // sense to re-create them per item
        static QWidget *menu = new QMenu();
        m_sharedWidget = true;
        m_dummywidget = menu;
        m_itemType = (str == QLatin1String("menu")) ? Menu : MenuItem;
    } else if (str == QLatin1String("item") || str == QLatin1String("itemrow") || str == QLatin1String("header")) {
        // Since these are used by the delegate, it makes no
        // sense to re-create them per item
        static QTreeView *menu = new QTreeView();
        menu->setAttribute(Qt::WA_MacMiniSize);
        m_sharedWidget = true;
        if (str == QLatin1String("header")) {
            m_dummywidget = menu->header();
            if (style() == QLatin1String("mac")) { // The default qt font seems to big
                QFont font = m_dummywidget->font();
                font.setPointSize(11);
                m_dummywidget->setFont(font);
            }
            m_itemType = Header;
        } else {
            m_dummywidget = menu;
            m_itemType = (str == QLatin1String("item")) ? Item : ItemRow;
        }
    } else if (str == QLatin1String("groupbox")) {
        // Since these are used by the delegate, it makes no
        // sense to re-create them per item
        static QGroupBox *group = new QGroupBox();
        m_sharedWidget = true;
        m_dummywidget = group;
        m_itemType = GroupBox;
    } else if (str == QLatin1String("tabframe") || str == QLatin1String("tab")) {
        static QTabWidget *tabframe = new QTabWidget();
        m_sharedWidget = true;
        if (str == QLatin1String("tab")) {
            m_dummywidget = tabframe->findChild<QTabBar*>();
            m_itemType = Tab;
        } else {
            m_dummywidget = tabframe;
            m_itemType = TabFrame;
        }
    } else if (str == QLatin1String("comboboxitem"))  {
        // Gtk uses qobject cast, hence we need to separate this from menuitem
        // On mac, we temporarily use the menu item because it has more accurate
        // palette.
#ifdef Q_OS_MAC
        static QMenu *combo = new QMenu();
#else
        static QComboBox *combo = new QComboBox();
#endif
        m_sharedWidget = true;
        m_dummywidget = combo;
        m_itemType = ComboBoxItem;
    } else if (str == QLatin1String("toolbar")) {
        static QToolBar *tb = 0;
        if (!tb) {
            QMainWindow *mw = new QMainWindow();
            tb = new QToolBar(mw);
        }
        m_dummywidget = tb;
        m_itemType = ToolBar;
    } else if (str == QLatin1String("toolbutton")) {
        static QToolButton *tb = 0;
        static QToolBar *bar = 0;
        // KDE animations are too broken with these widgets
        if (style() != QLatin1String("oxygen")) {
            if (!tb) {
                bar = new QToolBar(0);
                tb = new QToolButton(bar);
            }
        }
        m_sharedWidget = true;
        m_dummywidget = tb;
        m_itemType = ToolButton;
    } else if (str == QLatin1String("slider")) {
        static QSlider *slider = new QSlider();
        m_sharedWidget = true;
        m_dummywidget = slider;
        m_itemType = Slider;
    } else if (str == QLatin1String("frame")) {
        static QFrame *frame = new QFrame();
        m_sharedWidget = true;
        m_dummywidget = frame;
        m_itemType = Frame;
    } else if (str == QLatin1String("combobox")) {
        m_dummywidget = new QComboBox();
        visible = true;
        m_itemType = ComboBox;
    } else if (str == QLatin1String("splitter")) {
        visible = true;
        m_itemType = Splitter;
    } else if (str == QLatin1String("progressbar")) {
        m_dummywidget = new QProgressBar();
        visible = true;
        m_itemType = ProgressBar;
    } else if (str == QLatin1String("button")) {
        m_dummywidget = new QPushButton();
        visible = true;
        m_itemType = Button;
    } else if (str == QLatin1String("checkbox")) {
        m_dummywidget = new QCheckBox();
        visible = true;
        m_itemType = CheckBox;
    } else if (str == QLatin1String("radiobutton")) {
        m_dummywidget = new QRadioButton();
        visible = true;
        m_itemType = RadioButton;
    } else if (str == QLatin1String("edit")) {
        m_dummywidget = new QLineEdit();
        visible = true;
        m_itemType = Edit;
    } else if (str == QLatin1String("spinbox")) {
#ifndef Q_WS_WIN // Vista spinbox is currently not working due to grabwidget
        m_dummywidget = new QSpinBox();
        visible = true;
#endif
        m_itemType = SpinBox;
    } else if (str == QLatin1String("scrollbar")) {
        m_dummywidget = new QScrollBar();
        visible = true;
        m_itemType = ScrollBar;
    } else if (str == QLatin1String("widget")) {
        m_itemType = Widget;
    } else if (str == QLatin1String("focusframe")) {
        m_itemType = FocusFrame;
    } else if (str == QLatin1String("dial")) {
        m_itemType = Dial;
    }
    if (m_dummywidget) {
        m_dummywidget->installEventFilter(this);
        m_dummywidget->setAttribute(Qt::WA_QuitOnClose, false); // dont keep app open
        m_dummywidget->setAttribute(Qt::WA_LayoutUsesWidgetRect);
        m_dummywidget->winId();
#ifdef Q_OS_MAC
        m_dummywidget->setGeometry(-1000, 0, 10,10);
        m_dummywidget->setVisible(visible); // Mac require us to set the visibility before this
#endif
        m_dummywidget->setAttribute(Qt::WA_DontShowOnScreen);
        m_dummywidget->setVisible(visible);
    }
}

bool QStyleItem::eventFilter(QObject *o, QEvent *e) {
    if (e->type() == QEvent::Paint) {
        updateItem();
        return true;
    }
    return QObject::eventFilter(o, e);
}

void QStyleItem::showToolTip(const QString &str)
{
    QToolTip::showText(QCursor::pos(), str);
}

QRect QStyleItem::subControlRect(const QString &subcontrolString)
{
    QStyle::SubControl subcontrol = QStyle::SC_None;
    initStyleOption();
    switch (m_itemType) {
    case SpinBox:
        {
            QStyle::ComplexControl control = QStyle::CC_SpinBox;
            if (subcontrolString == QLatin1String("down"))
                subcontrol = QStyle::SC_SpinBoxDown;
            else if (subcontrolString == QLatin1String("up"))
                subcontrol = QStyle::SC_SpinBoxUp;
            else if (subcontrolString == QLatin1String("edit")){
                subcontrol = QStyle::SC_SpinBoxEditField;
            }
            return qApp->style()->subControlRect(control,
                                                 qstyleoption_cast<QStyleOptionComplex*>(m_styleoption),
                                                 subcontrol, widget());

        }
        break;
    case Slider:
        {
            QStyle::ComplexControl control = QStyle::CC_Slider;
            if (subcontrolString == QLatin1String("handle"))
                subcontrol = QStyle::SC_SliderHandle;
            else if (subcontrolString == QLatin1String("groove"))
                subcontrol = QStyle::SC_SliderGroove;
            return qApp->style()->subControlRect(control,
                                                 qstyleoption_cast<QStyleOptionComplex*>(m_styleoption),
                                                 subcontrol, widget());

        }
        break;
    case ScrollBar:
        {
            QStyle::ComplexControl control = QStyle::CC_ScrollBar;
            if (subcontrolString == QLatin1String("slider"))
                subcontrol = QStyle::SC_ScrollBarSlider;
            if (subcontrolString == QLatin1String("groove"))
                subcontrol = QStyle::SC_ScrollBarGroove;
            else if (subcontrolString == QLatin1String("handle"))
                subcontrol = QStyle::SC_ScrollBarSlider;
            else if (subcontrolString == QLatin1String("add"))
                subcontrol = QStyle::SC_ScrollBarAddPage;
            else if (subcontrolString == QLatin1String("sub"))
                subcontrol = QStyle::SC_ScrollBarSubPage;
            return qApp->style()->subControlRect(control,
                                                 qstyleoption_cast<QStyleOptionComplex*>(m_styleoption),
                                                 subcontrol, widget());
        }
        break;
    default:
        break;
    }
    return QRect();
}

void QStyleItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    if (width() < 1 || height() <1)
        return;

    initStyleOption();

    if (widget()) {
        painter->save();
        painter->setFont(widget()->font());
        painter->translate(-m_styleoption->rect.left() + m_paintMargins, 0);
    }

    switch (m_itemType) {
    case Button:
        qApp->style()->drawControl(QStyle::CE_PushButton, m_styleoption, painter, widget());
        break;
    case ItemRow :{
            QPixmap pixmap;
            // Only draw through style once
            const QString pmKey = QLatin1Literal("itemrow") % QString::number(m_styleoption->state,16) % activeControl();
            if (!QPixmapCache::find(pmKey, pixmap) || pixmap.width() < width() || height() != pixmap.height()) {
                int newSize = width();
                pixmap = QPixmap(newSize, height());
                pixmap.fill(Qt::transparent);
                QPainter pixpainter(&pixmap);
                qApp->style()->drawPrimitive(QStyle::PE_PanelItemViewRow, m_styleoption, &pixpainter, widget());
                if (!qApp->style()->styleHint(QStyle::SH_ItemView_ShowDecorationSelected) && selected())
                    pixpainter.fillRect(m_styleoption->rect, m_styleoption->palette.highlight());
                QPixmapCache::insert(pmKey, pixmap);
            }
            painter->drawPixmap(0, 0, pixmap);
        }
        break;
    case Item:
        qApp->style()->drawControl(QStyle::CE_ItemViewItem, m_styleoption, painter, widget());
        break;
    case Header:
        widget()->resize(m_styleoption->rect.size()); // macstyle explicitly uses the widget height
        qApp->style()->drawControl(QStyle::CE_Header, m_styleoption, painter, widget());
        break;
    case ToolButton:
        qApp->style()->drawComplexControl(QStyle::CC_ToolButton, qstyleoption_cast<QStyleOptionComplex*>(m_styleoption), painter, widget());
        break;
    case Tab:
        qApp->style()->drawControl(QStyle::CE_TabBarTab, m_styleoption, painter, widget());
        break;
    case Frame:
        qApp->style()->drawControl(QStyle::CE_ShapedFrame, m_styleoption, painter, widget());
        break;
    case FocusFrame:
        qApp->style()->drawControl(QStyle::CE_FocusFrame, m_styleoption, painter, widget());
        break;
    case TabFrame:
        qApp->style()->drawPrimitive(QStyle::PE_FrameTabWidget, m_styleoption, painter, widget());
        break;
    case MenuItem:
    case ComboBoxItem: // fall through
        qApp->style()->drawControl(QStyle::CE_MenuItem, m_styleoption, painter, widget());
        break;
    case CheckBox:
        qApp->style()->drawControl(QStyle::CE_CheckBox, m_styleoption, painter, widget());
        break;
    case RadioButton:
        qApp->style()->drawControl(QStyle::CE_RadioButton, m_styleoption, painter, widget());
        break;
    case Edit:
        qApp->style()->drawPrimitive(QStyle::PE_PanelLineEdit, m_styleoption, painter, widget());
        break;
    case Widget:
        qApp->style()->drawPrimitive(QStyle::PE_Widget, m_styleoption, painter, widget());
        break;
    case Splitter:
        qApp->style()->drawControl(QStyle::CE_Splitter, m_styleoption, painter, widget());
        break;
    case ComboBox:
        qApp->style()->drawComplexControl(QStyle::CC_ComboBox,
                                          qstyleoption_cast<QStyleOptionComplex*>(m_styleoption),
                                          painter, widget());
        qApp->style()->drawControl(QStyle::CE_ComboBoxLabel, m_styleoption, painter, widget());
        break;
    case SpinBox:
        qApp->style()->drawComplexControl(QStyle::CC_SpinBox,
                                          qstyleoption_cast<QStyleOptionComplex*>(m_styleoption),
                                          painter, widget());
        break;
    case Slider:
        qApp->style()->drawComplexControl(QStyle::CC_Slider,
                                          qstyleoption_cast<QStyleOptionComplex*>(m_styleoption),
                                          painter, widget());
        break;
    case Dial:
        qApp->style()->drawComplexControl(QStyle::CC_Dial,
                                          qstyleoption_cast<QStyleOptionComplex*>(m_styleoption),
                                          painter, widget());
        break;
    case ProgressBar:
        qApp->style()->drawControl(QStyle::CE_ProgressBar, m_styleoption, painter, widget());
        break;
    case ToolBar:
        qApp->style()->drawControl(QStyle::CE_ToolBar, m_styleoption, painter, widget());
        break;
    case GroupBox:
        qApp->style()->drawComplexControl(QStyle::CC_GroupBox, qstyleoption_cast<QStyleOptionComplex*>(m_styleoption), painter, widget());
        break;
    case ScrollBar:
        qApp->style()->drawComplexControl(QStyle::CC_ScrollBar, qstyleoption_cast<QStyleOptionComplex*>(m_styleoption), painter, widget());
        break;
    case Menu: {
            if (QMenu *menu = qobject_cast<QMenu*>(widget()))
                m_styleoption->palette = menu->palette();
            QStyleHintReturnMask val;
            qApp->style()->styleHint(QStyle::SH_Menu_Mask, m_styleoption, widget(), &val);
            painter->save();
            painter->setClipRegion(val.region);
            painter->fillRect(m_styleoption->rect, m_styleoption->palette.window());
            painter->restore();
            qApp->style()->drawPrimitive(QStyle::PE_PanelMenu, m_styleoption, painter, widget());

            QStyleOptionFrame frame;
            frame.lineWidth = qApp->style()->pixelMetric(QStyle::PM_MenuPanelWidth);
            frame.midLineWidth = 0;
            frame.rect = m_styleoption->rect;
            qApp->style()->drawPrimitive(QStyle::PE_FrameMenu, &frame, painter, widget());
        }
        break;
    default:
        break;
    }
    if (widget())
        painter->restore();
}

int QStyleItem::textWidth(const QString &text)
{
    if (widget())
        return widget()->fontMetrics().boundingRect(text).width();
    return qApp->fontMetrics().boundingRect(text).width();
}

int QStyleItem::fontHeight()
{
    if (widget())
        return widget()->fontMetrics().height();
    return qApp->fontMetrics().height();
}

QString QStyleItem::fontFamily()
{
    if (widget())
        return widget()->font().family();
    return qApp->font().family();
}

double QStyleItem::fontPointSize()
{
    if (widget())
        return widget()->font().pointSizeF();
    return qApp->font().pointSizeF();
}
