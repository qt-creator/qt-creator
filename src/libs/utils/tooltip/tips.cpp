/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "tips.h"
#include "tipcontents.h"
#include "reuse.h"

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

namespace Utils {
    namespace Internal {

namespace {
    // @todo: Reuse...
    QPixmap tilePixMap(int size)
    {
        const int checkerbordSize= size;
        QPixmap tilePixmap(checkerbordSize * 2, checkerbordSize * 2);
        tilePixmap.fill(Qt::white);
        QPainter tilePainter(&tilePixmap);
        QColor color(220, 220, 220);
        tilePainter.fillRect(0, 0, checkerbordSize, checkerbordSize, color);
        tilePainter.fillRect(checkerbordSize, checkerbordSize, checkerbordSize, checkerbordSize, color);
        return tilePixmap;
    }
}

QTipLabel::QTipLabel(QWidget *parent) :
    QLabel(parent, Qt::ToolTip | Qt::BypassGraphicsProxyWidget),
    m_tipContent(0)
{}

QTipLabel::~QTipLabel()
{
    Utils::TipContent *tmpTipContent = m_tipContent;
    m_tipContent = 0;
    delete tmpTipContent;
}

bool QTipLabel::isInteractive() const
{
    return m_tipContent && m_tipContent->isInteractive();
}

void QTipLabel::setContent(const TipContent &content)
{
    Utils::TipContent *tmpTipContent = m_tipContent;
    m_tipContent = content.clone();
    delete tmpTipContent;
}

const TipContent &QTipLabel::content() const
{ return *m_tipContent; }

ColorTip::ColorTip(QWidget *parent) : QTipLabel(parent)
{
    resize(QSize(40, 40));
    m_tilePixMap = tilePixMap(10);
}

ColorTip::~ColorTip()
{}

void ColorTip::configure(const QPoint &pos, QWidget *w)
{
    Q_UNUSED(pos)
    Q_UNUSED(w)

    update();
}

bool ColorTip::canHandleContentReplacement(const TipContent &content) const
{
    if (content.typeId() == ColorContent::COLOR_CONTENT_ID)
        return true;
    return false;
}

void ColorTip::paintEvent(QPaintEvent *event)
{
    QTipLabel::paintEvent(event);

    const QColor &color = static_cast<const ColorContent &>(content()).color();

    QPen pen;
    pen.setWidth(1);
    if (color.value() > 100)
        pen.setColor(color.darker());
    else
        pen.setColor(color.lighter());

    QPainter painter(this);
    painter.setPen(pen);
    painter.setBrush(color);
    QRect r(1, 1, rect().width() - 2, rect().height() - 2);
    painter.drawTiledPixmap(r, m_tilePixMap);
    painter.drawRect(r);
}

TextTip::TextTip(QWidget *parent) : QTipLabel(parent)
{
    setForegroundRole(QPalette::ToolTipText);
    setBackgroundRole(QPalette::ToolTipBase);
    ensurePolished();
    setMargin(1 + style()->pixelMetric(QStyle::PM_ToolTipLabelFrameWidth, 0, this));
    setFrameStyle(QFrame::NoFrame);
    setAlignment(Qt::AlignLeft);
    setIndent(1);
    setWindowOpacity(style()->styleHint(QStyle::SH_ToolTipLabel_Opacity, 0, this) / 255.0);
}

TextTip::~TextTip()
{}

void TextTip::configure(const QPoint &pos, QWidget *w)
{
    const QString &text = static_cast<const TextContent &>(content()).text();
    setText(text);

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
        tipWidth = sizeHint().width();
        // If the width is still too large (maybe due to some extremely long word which prevents
        // wrapping), the tip is truncated according to the screen.
        if (tipWidth > screenWidth)
            tipWidth = screenWidth - 10;
    }

    resize(tipWidth, heightForWidth(tipWidth) + extraHeight);
}

bool TextTip::canHandleContentReplacement(const TipContent &content) const
{
    if (content.typeId() == TextContent::TEXT_CONTENT_ID)
        return true;
    return false;
}

void TextTip::paintEvent(QPaintEvent *event)
{
    QStylePainter p(this);
    QStyleOptionFrame opt;
    opt.init(this);
    p.drawPrimitive(QStyle::PE_PanelTipLabel, opt);
    p.end();

    QLabel::paintEvent(event);
}

void TextTip::resizeEvent(QResizeEvent *event)
{
    QStyleHintReturnMask frameMask;
    QStyleOption option;
    option.init(this);
    if (style()->styleHint(QStyle::SH_ToolTip_Mask, &option, this, &frameMask))
        setMask(frameMask.region);

    QLabel::resizeEvent(event);
}

WidgetTip::WidgetTip(QWidget *parent) :
    QTipLabel(parent), m_layout(new QVBoxLayout)
{
    m_layout->setContentsMargins(0, 0, 0, 0);
    setLayout(m_layout);
}

QWidget *WidgetTip::takeWidget(Qt::WindowFlags wf)
{
    // Remove widget from layout
    if (!m_layout->count())
        return 0;
    QLayoutItem *item = m_layout->takeAt(0);
    QWidget *widget = item->widget();
    delete item;
    if (!widget)
        return 0;
    widget->setParent(0, wf);
    return widget;
}

void WidgetTip::configure(const QPoint &pos, QWidget *)
{
    const WidgetContent &anyContent = static_cast<const WidgetContent &>(content());
    QWidget *widget = anyContent.widget();

    QTC_ASSERT(widget && m_layout->count() == 0, return);

    move(pos);
    m_layout->addWidget(widget);
    m_layout->setSizeConstraint(QLayout::SetFixedSize);
    adjustSize();
}

void WidgetTip::pinToolTipWidget()
{
    QTC_ASSERT(m_layout->count(), return);

    // Pin the content widget: Rip the widget out of the layout
    // and re-show as a tooltip, with delete on close.
    const QPoint screenPos = mapToGlobal(QPoint(0, 0));
    QWidget *widget = takeWidget(Qt::ToolTip);
    QTC_ASSERT(widget, return);

    widget->move(screenPos);
    widget->show();
    widget->setAttribute(Qt::WA_DeleteOnClose);
}

bool WidgetTip::canHandleContentReplacement(const TipContent & ) const
{
    // Always create a new widget.
    return false;
}

// need to include it here to force it to be inside the namespaces
#include "moc_tips.cpp"

} // namespace Internal
} // namespace Utils
