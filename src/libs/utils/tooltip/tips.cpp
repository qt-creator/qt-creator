// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "tips.h"
#include "tooltip.h"

#include "../qtcassert.h"
#include "../hostosinfo.h"

#include <QColor>
#include <QFontMetrics>
#include <QGuiApplication>
#include <QPaintEvent>
#include <QPainter>
#include <QPen>
#include <QPoint>
#include <QRect>
#include <QResizeEvent>
#include <QStyle>
#include <QStylePainter>
#include <QStyleOptionFrame>
#include <QTextDocument>
#include <QScreen>
#include <QVBoxLayout>
#include <QWidget>

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

    static const uint tip_label_meta_data[15] = { 9 /* moc revision */ };

    static const struct {
        const uint offsetsAndSize[2];
        char stringdata0[24];
    } qt_meta_stringdata = { {8, sizeof("QTipLabel")}, "QTipLabel" };

    static const QMetaObject tipMetaObject {
        &QLabel::staticMetaObject,                    // SuperData superdata
        qt_meta_stringdata.offsetsAndSize,            // const uint *stringdata;
        tip_label_meta_data,                          // const uint *data;
        nullptr,                                      // StaticMetacallFunction static_metacall;
        nullptr,                                      // const SuperData *relatedMetaObjects;
        nullptr,                                      // QtPrivate::QMetaTypeInterface *const *metaTypes;
        nullptr,                                      // void *extradata;
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

void ColorTip::configure(const QPoint &pos)
{
    Q_UNUSED(pos)

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

static bool likelyContainsLink(const QString &s, const Qt::TextFormat &format)
{
    if (s.contains(QLatin1String("href"), Qt::CaseInsensitive))
        return true;
    if (format == Qt::MarkdownText)
        return s.contains("](");
    return false;
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

    bool containsLink = likelyContainsLink(m_text, m_format);
    setOpenExternalLinks(containsLink);
}

bool TextTip::isInteractive() const
{
    return likelyContainsLink(m_text, m_format);
}

void TextTip::configure(const QPoint &pos)
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

    QScreen *screen = QGuiApplication::screenAt(pos);
    if (!screen)
        screen = QGuiApplication::primaryScreen();

    const int screenWidth = screen->availableGeometry().width();
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

void WidgetTip::configure(const QPoint &pos)
{
    QTC_ASSERT(m_widget && m_layout->count() == 0, return);

    move(pos);
    m_layout->addWidget(m_widget);
    m_layout->setSizeConstraint(QLayout::SetFixedSize);
    if (!HostOsInfo::isMacHost()) // work around QTBUG-131479
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

} // namespace Internal
} // namespace Utils
