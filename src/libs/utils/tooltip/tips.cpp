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

#include "tips.h"
#include "tooltip.h"

#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>

#include <QRect>
#include <QColor>
#include <QPainter>
#include <QPen>
#include <QPixmap>
#include <QStyle>
#include <QFontMetrics>
#include <QTextDocument>
#include <QStylePainter>
#include <QStyleOptionFrame>
#include <QResizeEvent>
#include <QPaintEvent>
#include <QVBoxLayout>

#include <QPoint>
#include <QRect>
#include <QWidget>
#include <QApplication>
#include <QDesktopWidget>

#include <memory>

namespace Utils {
namespace Internal {

TipLabel::TipLabel(QWidget *parent) :
    QLabel(parent, Qt::ToolTip | Qt::BypassGraphicsProxyWidget)
{
}

void TipLabel::setContextHelp(const QVariant &help)
{
    m_contextHelp = help;
    update();
}

QVariant TipLabel::contextHelp() const
{
    return m_contextHelp;
}

const QMetaObject *TipLabel::metaObject() const
{
    // CSS Tooltip styling depends on a the name of this class.
    // So set up a minimalist QMetaObject to fake a class name "QTipLabel":
    static const uint tip_label_meta_data[15] = { 8 /* moc revision */ };
    static const QMetaObject tipMetaObject {
         &QLabel::staticMetaObject,
         QByteArrayLiteral("QTipLabel").data_ptr(),
         tip_label_meta_data,
         nullptr,
         nullptr,
         nullptr
    };

    return &tipMetaObject;
}

ColorTip::ColorTip(QWidget *parent)
    : TipLabel(parent)
{
    resize(40, 40);
}

void ColorTip::setContent(const QVariant &content)
{
    m_color = content.value<QColor>();

    const int size = 10;
    m_tilePixmap = QPixmap(size * 2, size * 2);
    m_tilePixmap.fill(Qt::white);
    QPainter tilePainter(&m_tilePixmap);
    QColor col(220, 220, 220);
    tilePainter.fillRect(0, 0, size, size, col);
    tilePainter.fillRect(size, size, size, size, col);
}

void ColorTip::configure(const QPoint &pos, QWidget *w)
{
    Q_UNUSED(pos)
    Q_UNUSED(w)

    update();
}

bool ColorTip::canHandleContentReplacement(int typeId) const
{
    return typeId == ToolTip::ColorContent;
}

bool ColorTip::equals(int typeId, const QVariant &other, const QVariant &otherContextHelp) const
{
    return typeId == ToolTip::ColorContent && otherContextHelp == contextHelp() && other == m_color;
}

void ColorTip::paintEvent(QPaintEvent *event)
{
    TipLabel::paintEvent(event);

    QPainter painter(this);
    painter.setBrush(m_color);
    painter.drawTiledPixmap(rect(), m_tilePixmap);

    QPen pen;
    pen.setColor(m_color.value() > 100 ? m_color.darker() : m_color.lighter());
    pen.setJoinStyle(Qt::MiterJoin);
    const QRectF borderRect = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    painter.setPen(pen);
    painter.drawRect(borderRect);
}

TextTip::TextTip(QWidget *parent) : TipLabel(parent)
{
    setForegroundRole(QPalette::ToolTipText);
    setBackgroundRole(QPalette::ToolTipBase);
    ensurePolished();
    setMargin(1 + style()->pixelMetric(QStyle::PM_ToolTipLabelFrameWidth, nullptr, this));
    setFrameStyle(QFrame::NoFrame);
    setAlignment(Qt::AlignLeft);
    setIndent(1);
    setWindowOpacity(style()->styleHint(QStyle::SH_ToolTipLabel_Opacity, nullptr, this) / 255.0);
}

static bool likelyContainsLink(const QString &s)
{
    return s.contains(QLatin1String("href"), Qt::CaseInsensitive);
}

void TextTip::setContent(const QVariant &content)
{
    if (content.canConvert<QString>()) {
        m_text = content.toString();
    } else if (content.canConvert<TextItem>()) {
        auto item = content.value<TextItem>();
        m_text = item.first;
        m_format = item.second;
    }

    bool containsLink = likelyContainsLink(m_text);
    setOpenExternalLinks(containsLink);
}

bool TextTip::isInteractive() const
{
    return likelyContainsLink(m_text);
}

void TextTip::configure(const QPoint &pos, QWidget *w)
{
    setTextFormat(m_format);
    setText(m_text);

    // Make it look good with the default ToolTip font on Mac, which has a small descent.
    QFontMetrics fm(font());
    int extraHeight = 0;
    if (fm.descent() == 2 && fm.ascent() >= 11)
        ++extraHeight;

    // Try to find a nice width without unnecessary wrapping.
    setWordWrap(false);
    int tipWidth = sizeHint().width();
    const int screenWidth = screenGeometry(pos, w).width();
    const int maxDesiredWidth = int(screenWidth * .5);
    if (tipWidth > maxDesiredWidth) {
        setWordWrap(true);
        tipWidth = maxDesiredWidth;
    }

    resize(tipWidth, heightForWidth(tipWidth) + extraHeight);
}

bool TextTip::canHandleContentReplacement(int typeId) const
{
    return typeId == ToolTip::TextContent;
}

int TextTip::showTime() const
{
    return 10000 + 40 * qMax(0, m_text.size() - 100);
}

bool TextTip::equals(int typeId, const QVariant &other, const QVariant &otherContextHelp) const
{
    return typeId == ToolTip::TextContent && otherContextHelp == contextHelp()
           && ((other.canConvert<QString>() && other.toString() == m_text)
               || (other.canConvert<TextItem>()
                   && other.value<TextItem>() == TextItem(m_text, m_format)));
}

void TextTip::paintEvent(QPaintEvent *event)
{
    QStylePainter p(this);
    QStyleOptionFrame opt;
    opt.initFrom(this);
    p.drawPrimitive(QStyle::PE_PanelTipLabel, opt);
    p.end();

    QLabel::paintEvent(event);
}

void TextTip::resizeEvent(QResizeEvent *event)
{
    QStyleHintReturnMask frameMask;
    QStyleOption option;
    option.initFrom(this);
    if (style()->styleHint(QStyle::SH_ToolTip_Mask, &option, this, &frameMask))
        setMask(frameMask.region);

    QLabel::resizeEvent(event);
}

WidgetTip::WidgetTip(QWidget *parent) :
    TipLabel(parent), m_layout(new QVBoxLayout)
{
    m_layout->setContentsMargins(0, 0, 0, 0);
    setLayout(m_layout);
}

void WidgetTip::setContent(const QVariant &content)
{
    m_widget = content.value<QWidget *>();
}

void WidgetTip::configure(const QPoint &pos, QWidget *)
{
    QTC_ASSERT(m_widget && m_layout->count() == 0, return);

    move(pos);
    m_layout->addWidget(m_widget);
    m_layout->setSizeConstraint(QLayout::SetFixedSize);
    adjustSize();
}

void WidgetTip::pinToolTipWidget(QWidget *parent)
{
    QTC_ASSERT(m_layout->count(), return);

    // Pin the content widget: Rip the widget out of the layout
    // and re-show as a tooltip, with delete on close.
    const QPoint screenPos = mapToGlobal(QPoint(0, 0));
    // Remove widget from layout
    if (!m_layout->count())
        return;

    QLayoutItem *item = m_layout->takeAt(0);
    QWidget *widget = item->widget();
    delete item;
    if (!widget)
        return;

    widget->setParent(parent, Qt::Tool|Qt::FramelessWindowHint);
    widget->move(screenPos);
    widget->show();
    widget->setAttribute(Qt::WA_DeleteOnClose);
}

bool WidgetTip::canHandleContentReplacement(int typeId) const
{
    // Always create a new widget.
    Q_UNUSED(typeId)
    return false;
}

bool WidgetTip::equals(int typeId, const QVariant &other, const QVariant &otherContextHelp) const
{
    return typeId == ToolTip::WidgetContent && otherContextHelp == contextHelp()
            && other.value<QWidget *>() == m_widget;
}


int screenNumber(const QPoint &pos, QWidget *w)
{
    if (QApplication::desktop()->isVirtualDesktop())
        return QApplication::desktop()->screenNumber(pos);
    else
        return QApplication::desktop()->screenNumber(w);
}

QRect screenGeometry(const QPoint &pos, QWidget *w)
{
    if (HostOsInfo::isMacHost())
        return QApplication::desktop()->availableGeometry(screenNumber(pos, w));
    return QApplication::desktop()->screenGeometry(screenNumber(pos, w));
}

} // namespace Internal
} // namespace Utils
