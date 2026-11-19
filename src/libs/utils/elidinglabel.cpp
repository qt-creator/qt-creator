// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "elidinglabel.h"

#include "utilstr.h"

#include <QFontMetrics>
#include <QPainter>
#include <QStyle>
#include <QTextBlock>
#include <QTextDocument>
#include <QTextDocumentFragment>

/*!
    \class Utils::ElidingLabel
    \inmodule QtCreator

    \brief The ElidingLabel class is a label suitable for displaying elided
    text.

    Rich text is elided as the plain text it shows. A link spanning the whole
    text stays clickable while elided.

    While the text is elided, the tool tip shows it in full, followed by the
    target of each link whose text does not already say where it leads. The
    tool tip is always shown as plain text.
*/

namespace Utils {

static bool isRichText(const QLabel *label)
{
    const Qt::TextFormat format = label->textFormat();
    return format == Qt::RichText
           || (format == Qt::AutoText && Qt::mightBeRichText(label->text()));
}

static QString shownText(const QLabel *label)
{
    if (isRichText(label))
        return QTextDocumentFragment::fromHtml(label->text()).toPlainText();
    return label->text();
}

// The targets of the links whose text differs from where they lead, so that
// the text cannot pretend to be a different target.
static QStringList hiddenLinkTargets(const QLabel *label)
{
    if (!isRichText(label))
        return {};

    QTextDocument document;
    document.setHtml(label->text());
    QStringList targets;
    QString href;
    QString anchorText;
    const auto flush = [&] {
        if (!href.isEmpty() && href != anchorText.trimmed() && !targets.contains(href))
            targets.append(href);
        href.clear();
        anchorText.clear();
    };
    for (QTextBlock block = document.begin(); block.isValid(); block = block.next()) {
        for (auto it = block.begin(); !it.atEnd(); ++it) {
            const QTextFragment fragment = it.fragment();
            const QTextCharFormat format = fragment.charFormat();
            const QString fragmentHref = format.isAnchor() ? format.anchorHref() : QString();
            if (fragmentHref != href)
                flush();
            href = fragmentHref;
            if (!href.isEmpty())
                anchorText += fragment.text();
        }
        flush();
    }
    return targets;
}

static QString elidedToolTip(const QLabel *label, const QString &shown)
{
    QString toolTip = shown;
    for (const QString &target : hiddenLinkTargets(label))
        toolTip += '\n' + Tr::tr("Link: %1").arg(target);
    return toolTip;
}

ElidingLabel::ElidingLabel(QWidget *parent)
    : ElidingLabel({}, parent)
{
}

ElidingLabel::ElidingLabel(const QString &text, QWidget *parent)
    : QLabel(text, parent)
{
    setElideMode(Qt::ElideRight);
}

Qt::TextElideMode ElidingLabel::elideMode() const
{
    return m_elideMode;
}

void ElidingLabel::setElideMode(const Qt::TextElideMode &elideMode)
{
    m_elideMode = elideMode;
    if (elideMode == Qt::ElideNone)
        updateToolTip({});

    setSizePolicy(QSizePolicy(
                      m_elideMode == Qt::ElideNone ? QSizePolicy::Preferred : QSizePolicy::Ignored,
                      QSizePolicy::Preferred,
                      QSizePolicy::Label));
    update();
}

QString ElidingLabel::additionalToolTip()
{
    return m_additionalToolTip;
}

void ElidingLabel::setAdditionalToolTip(const QString &additionalToolTip)
{
    m_additionalToolTip = additionalToolTip;
}

void ElidingLabel::paintEvent(QPaintEvent *)
{
    if (m_elideMode == Qt::ElideNone) {
        QLabel::paintEvent(nullptr);
        updateToolTip({});
        return;
    }

    const int m = margin();
    const QRect contents = contentsRect().adjusted(m, m, -m, -m);
    const QFontMetrics fm = fontMetrics();
    const QString txt = shownText(this);
    const bool elide = txt.size() > 4 && fm.horizontalAdvance(txt) > contents.width();

    // QLabelPrivate::layoutRect() clamps the vertical offset at 0, so a label
    // shorter than its text loses Qt::AlignBottom, and the descender with it.
    // Equality is the normal case: it is what sizeHint() asks for.
    const bool fitsVertically = fm.height() <= contents.height();

    if (!elide && fitsVertically) {
        // Nothing to truncate: let QLabel paint so that features handled by the
        // base class, such as text selection, are rendered normally.
        QLabel::paintEvent(nullptr);
        updateToolTip({});
        return;
    }

    updateToolTip(elide ? elidedToolTip(this, txt) : QString());
    const QString shown = elide ? fm.elidedText(txt, m_elideMode, contents.width()) : txt;
    const int flags = QStyle::visualAlignment(layoutDirection(), alignment()) | Qt::TextSingleLine;

    QPainter painter(this);
    drawFrame(&painter);
    painter.drawText(contents, flags, shown);
}

void ElidingLabel::updateToolTip(const QString &elidedText)
{
    QString toolTip;
    if (m_additionalToolTip.isEmpty())
        toolTip = elidedText;
    else if (elidedText.isEmpty())
        toolTip = m_additionalToolTip;
    else
        toolTip = elidedText + m_additionalToolTipSeparator + m_additionalToolTip;

    // QToolTip renders anything that looks like markup as rich text.
    if (Qt::mightBeRichText(toolTip))
        toolTip = Qt::convertFromPlainText(toolTip, Qt::WhiteSpaceNormal);
    setToolTip(toolTip);
}

QString ElidingLabel::additionalToolTipSeparator() const
{
    return m_additionalToolTipSeparator;
}

void ElidingLabel::setAdditionalToolTipSeparator(const QString &newAdditionalToolTipSeparator)
{
    m_additionalToolTipSeparator = newAdditionalToolTipSeparator;
}

static int singleLineHeight(const QLabel *label)
{
    const QMargins margins = label->contentsMargins();
    return label->fontMetrics().height() + 2 * label->margin() + margins.top() + margins.bottom();
}

QSize ElidingLabel::sizeHint() const
{
    if (m_elideMode == Qt::ElideNone)
        return QLabel::sizeHint();
    return {QLabel::sizeHint().width(), singleLineHeight(this)};
}

QSize ElidingLabel::minimumSizeHint() const
{
    if (m_elideMode == Qt::ElideNone)
        return QLabel::minimumSizeHint();
    return {QLabel::minimumSizeHint().width(), singleLineHeight(this)};
}

} // namespace Utils
