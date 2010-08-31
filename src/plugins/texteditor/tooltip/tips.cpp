/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "tips.h"
#include "tipcontents.h"

#include <QtCore/QRect>
#include <QtGui/QColor>
#include <QtGui/QPainter>
#include <QtGui/QPen>
#include <QtGui/QPixmap>
#include <QtGui/QStyle>
#include <QtGui/QFontMetrics>
#include <QtGui/QTextDocument>
#include <QtGui/QStylePainter>
#include <QtGui/QStyleOptionFrame>

namespace TextEditor {
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
    if (m_tipContent)
        delete m_tipContent;
}

void QTipLabel::setContent(const TipContent &content)
{
    if (m_tipContent)
        delete m_tipContent;
    m_tipContent = content.clone();
    configure();
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

void ColorTip::configure()
{
    update();
}

bool ColorTip::handleContentReplacement(const TipContent &content) const
{
    if (content.typeId() == ColorContent::COLOR_CONTENT_ID)
        return true;
    return false;
}

void ColorTip::paintEvent(QPaintEvent *event)
{
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

    QLabel::paintEvent(event);
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

void TextTip::configure()
{
    const QString &text = static_cast<const TextContent &>(content()).text();
    setWordWrap(Qt::mightBeRichText(text));
    setText(text);
    QFontMetrics fm(font());
    QSize extra(1, 0);
    // Make it look good with the default ToolTip font on Mac, which has a small descent.
    if (fm.descent() == 2 && fm.ascent() >= 11)
        ++extra.rheight();
    resize(sizeHint() + extra);
}

bool TextTip::handleContentReplacement(const TipContent &content) const
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

// need to include it here to force it to be inside the namespaces
#include "moc_tips.cpp"

} // namespace Internal
} // namespace TextEditor
