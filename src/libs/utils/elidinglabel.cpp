// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "elidinglabel.h"

#include "utilstr.h"

#include <QAbstractTextDocumentLayout>
#include <QDesktopServices>
#include <QFontMetrics>
#include <QHelpEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QStyle>
#include <QStyleOption>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextDocumentFragment>
#include <QToolTip>
#include <QUrl>
#include <QtMath>

#include <memory>

/*!
    \class Utils::ElidingLabel
    \inmodule QtCreator

    \brief The ElidingLabel class is a label suitable for displaying elided
    text.

    Rich text is elided as the plain text it shows, and keeps its formatting.
    Links stay clickable where they are shown while elided.

    While the text is elided, the tool tip shows it in full, followed by the
    target of each link whose text does not already say where it leads. The
    tool tip is always shown as plain text. Hovering a link that leads to a URL
    shows that URL as tool tip instead, whether the text is elided or not.
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

class Anchor
{
public:
    int start = 0;
    int end = 0;
    QString href;
    QString text;
};

static QList<Anchor> documentAnchors(const QTextDocument &document)
{
    QList<Anchor> anchors;
    Anchor anchor;
    const auto flush = [&] {
        if (!anchor.href.isEmpty())
            anchors.append(anchor);
        anchor = {};
    };
    for (QTextBlock block = document.begin(); block.isValid(); block = block.next()) {
        for (auto it = block.begin(); !it.atEnd(); ++it) {
            const QTextFragment fragment = it.fragment();
            const QTextCharFormat format = fragment.charFormat();
            const QString href = format.isAnchor() ? format.anchorHref() : QString();
            if (href != anchor.href) {
                flush();
                anchor.href = href;
                anchor.start = fragment.position();
            }
            anchor.text += fragment.text();
            anchor.end = fragment.position() + fragment.length();
        }
        flush();
    }
    return anchors;
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
    for (const Anchor &anchor : documentAnchors(document)) {
        if (anchor.href != anchor.text.trimmed() && !targets.contains(anchor.href))
            targets.append(anchor.href);
    }
    return targets;
}

// How a label with an elide mode other than Qt::ElideNone is painted.
class PaintedText
{
public:
    QRect contents;
    QString text;
    bool elide = false;
    bool byQLabel = true;
};

static PaintedText paintedText(const ElidingLabel *label)
{
    PaintedText painted;
    if (label->elideMode() == Qt::ElideNone)
        return painted;

    const int m = label->margin();
    painted.contents = label->contentsRect().adjusted(m, m, -m, -m);
    const QFontMetrics fm = label->fontMetrics();
    painted.text = shownText(label);
    painted.elide = painted.text.size() > 4
                    && fm.horizontalAdvance(painted.text) > painted.contents.width();

    // QLabelPrivate::layoutRect() clamps the vertical offset at 0, so a label
    // shorter than its text loses Qt::AlignBottom, and the descender with it.
    // Equality is the normal case: it is what sizeHint() asks for.
    const bool fitsVertically = fm.height() <= painted.contents.height();

    // Nothing to truncate: let QLabel paint so that features handled by the
    // base class, such as text selection, are rendered normally.
    painted.byQLabel = !painted.elide && fitsVertically;
    return painted;
}

static bool paintsRichTextItself(const ElidingLabel *label)
{
    return isRichText(label) && !paintedText(label).byQLabel;
}

// The rich text in one line, with the part that does not fit replaced by an
// ellipsis in the format of the first character it replaces.
static std::unique_ptr<QTextDocument> elidedDocument(const ElidingLabel *label,
                                                     const PaintedText &painted)
{
    const QFontMetrics fm = label->fontMetrics();
    // QTextLayout draws the right edge of the focus indicator of a link just
    // beyond the text, and only if the line reaches there.
    const int focusIndicatorRoom = (label->textInteractionFlags() & Qt::LinksAccessibleByKeyboard)
                                       ? 2
                                       : 0;
    const int available = painted.contents.width() - focusIndicatorRoom;
    // Bold or larger parts are wider than the font metrics say, so shorten
    // until the formatted text fits.
    for (int width = available;; width -= fm.averageCharWidth()) {
        auto document = std::make_unique<QTextDocument>();
        document->setDefaultFont(label->font());
        document->setDocumentMargin(0);
        QTextOption option = document->defaultTextOption();
        option.setWrapMode(QTextOption::NoWrap);
        document->setDefaultTextOption(option);
        document->setHtml(label->text());
        if (!painted.elide) {
            document->setTextWidth(painted.contents.width());
            return document;
        }

        const QString plain = document->toPlainText();
        const QString elided = fm.elidedText(plain, label->elideMode(), qMax(width, 0));
        // The text may contain an ellipsis of its own, and elidedText() may use
        // "..." instead of one, so cut where the elided text differs.
        qsizetype head = 0;
        while (head < elided.size() && head < plain.size() && elided.at(head) == plain.at(head))
            ++head;
        qsizetype tail = 0;
        while (tail < elided.size() - head && tail < plain.size() - head
               && elided.at(elided.size() - 1 - tail) == plain.at(plain.size() - 1 - tail)) {
            ++tail;
        }
        if (head + tail < plain.size()) {
            QTextCursor cursor(document.get());
            cursor.setPosition(head + 1);
            const QTextCharFormat format = cursor.charFormat();
            cursor.setPosition(head);
            cursor.setPosition(plain.size() - tail, QTextCursor::KeepAnchor);
            cursor.insertText(elided.mid(head, elided.size() - head - tail), format);
        }
        if (width <= 0 || document->idealWidth() <= available) {
            document->setTextWidth(painted.contents.width());
            return document;
        }
    }
}

static QRect documentRect(const QLabel *label, const QTextDocument &document,
                          const QRect &contents)
{
    const QSize size(qCeil(document.idealWidth()), qCeil(document.size().height()));
    return QStyle::alignedRect(label->layoutDirection(), label->alignment(), size, contents);
}

static QString elidedAnchorAt(const ElidingLabel *label, const QPoint &pos)
{
    const PaintedText painted = paintedText(label);
    const std::unique_ptr<QTextDocument> document = elidedDocument(label, painted);
    const QRect rect = documentRect(label, *document, painted.contents);
    return document->documentLayout()->anchorAt(pos - rect.topLeft());
}

static QString asToolTip(const QString &text)
{
    // QToolTip renders anything that looks like markup as rich text.
    if (Qt::mightBeRichText(text))
        return Qt::convertFromPlainText(text, Qt::WhiteSpaceNormal);
    return text;
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
    connect(this, &QLabel::linkHovered, this, [this](const QString &link) {
        m_hoveredLink = link;
    });
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

    const PaintedText painted = paintedText(this);
    if (painted.byQLabel) {
        QLabel::paintEvent(nullptr);
        updateToolTip({});
        return;
    }

    updateToolTip(painted.elide ? elidedToolTip(this, painted.text) : QString());

    QPainter painter(this);
    drawFrame(&painter);

    if (!isRichText(this)) {
        const QString shown = painted.elide ? fontMetrics().elidedText(painted.text, m_elideMode,
                                                                       painted.contents.width())
                                            : painted.text;
        const int flags = QStyle::visualAlignment(layoutDirection(), alignment())
                          | Qt::TextSingleLine;
        painter.drawText(painted.contents, flags, shown);
        return;
    }

    const std::unique_ptr<QTextDocument> document = elidedDocument(this, painted);
    const QRect rect = documentRect(this, *document, painted.contents);
    // Clip horizontally only, the descender may reach below a short label.
    painter.setClipRect(painted.contents.left(), 0, painted.contents.width(), height());
    painter.translate(rect.topLeft());
    QAbstractTextDocumentLayout::PaintContext context;
    context.palette = palette();
    context.palette.setColor(QPalette::Text, palette().color(foregroundRole()));
    const QList<Anchor> anchors = documentAnchors(*document);
    if (m_focusedAnchor >= anchors.size())
        m_focusedAnchor = -1;
    if (hasFocus() && m_focusedAnchor >= 0 && handlesLinkFocusItself()) {
        // As QWidgetTextControl draws the link that has the focus.
        QAbstractTextDocumentLayout::Selection selection;
        selection.cursor = QTextCursor(document.get());
        selection.cursor.setPosition(anchors.at(m_focusedAnchor).start);
        selection.cursor.setPosition(anchors.at(m_focusedAnchor).end, QTextCursor::KeepAnchor);
        QStyleOption option;
        option.palette = context.palette;
        QStyleHintReturnVariant format;
        style()->styleHint(QStyle::SH_TextControl_FocusIndicatorTextCharFormat, &option, this,
                           &format);
        selection.format = qvariant_cast<QTextFormat>(format.variant).toCharFormat();
        context.selections.append(selection);
    }
    document->documentLayout()->draw(&painter, context);
}

void ElidingLabel::activateLink(const QString &link)
{
    if (openExternalLinks())
        QDesktopServices::openUrl(QUrl(link));
    else
        emit linkActivated(link);
}

// QLabel and this label do not know which link the other one hovers, presses
// or has the focus on, so forget it when the other one takes over.
void ElidingLabel::followLinkHandler()
{
    const bool paintsItself = paintsRichTextItself(this);
    if (paintsItself == m_paintsRichTextItself)
        return;
    m_paintsRichTextItself = paintsItself;
    m_pressedLink.clear();
    m_focusedAnchor = -1;
    if (paintsItself) {
        // QLabel reports a link only when the mouse reaches another one, so
        // without this it would not report the one it hovers now when it takes
        // over again. A move to where no link is resets it, cursor included;
        // before Qt 6.12, leaving the label does not.
        const QPointF outside(-1, -1);
        QMouseEvent move(QEvent::MouseMove, outside, mapToGlobal(outside), Qt::NoButton,
                         Qt::NoButton, Qt::NoModifier);
        QLabel::mouseMoveEvent(&move);
    } else if (!m_hoveredLink.isEmpty()) {
        unsetCursor();
        emit linkHovered({});
    }
}

// While the label paints the rich text itself, QLabel would find links in the
// text as it lays it out, unelided, and so under other positions.
bool ElidingLabel::handlesLinksItself() const
{
    return (textInteractionFlags() & Qt::LinksAccessibleByMouse) && paintsRichTextItself(this);
}

void ElidingLabel::mousePressEvent(QMouseEvent *event)
{
    if (!handlesLinksItself()) {
        QLabel::mousePressEvent(event);
        return;
    }
    m_pressedLink = elidedAnchorAt(this, event->position().toPoint());
    event->setAccepted(!m_pressedLink.isEmpty());
}

void ElidingLabel::mouseMoveEvent(QMouseEvent *event)
{
    if (!handlesLinksItself()) {
        QLabel::mouseMoveEvent(event);
        return;
    }
    const QString link = elidedAnchorAt(this, event->position().toPoint());
    if (link == m_hoveredLink)
        return;
    if (link.isEmpty())
        unsetCursor();
    else
        setCursor(Qt::PointingHandCursor);
    emit linkHovered(link);
}

void ElidingLabel::mouseReleaseEvent(QMouseEvent *event)
{
    if (!handlesLinksItself()) {
        QLabel::mouseReleaseEvent(event);
        return;
    }
    const QString link = elidedAnchorAt(this, event->position().toPoint());
    const bool activated = event->button() == Qt::LeftButton && !link.isEmpty()
                           && link == m_pressedLink;
    m_pressedLink.clear();
    event->setAccepted(!link.isEmpty());
    if (activated)
        activateLink(link);
}

// While the label paints the rich text itself, QLabel would move the focus over
// the links of the unelided text, including those elided away.
bool ElidingLabel::handlesLinkFocusItself() const
{
    return (textInteractionFlags() & Qt::LinksAccessibleByKeyboard)
           && paintsRichTextItself(this);
}

static QList<Anchor> elidedAnchors(const ElidingLabel *label)
{
    return documentAnchors(*elidedDocument(label, paintedText(label)));
}

bool ElidingLabel::focusNextPrevChild(bool next)
{
    followLinkHandler();
    if (!handlesLinkFocusItself())
        return QLabel::focusNextPrevChild(next);

    const qsizetype count = elidedAnchors(this).size();
    if (m_focusedAnchor < 0 || m_focusedAnchor >= count)
        m_focusedAnchor = next ? 0 : count - 1;
    else
        m_focusedAnchor += next ? 1 : -1;
    if (m_focusedAnchor < 0 || m_focusedAnchor >= count)
        m_focusedAnchor = -1;
    update();
    if (m_focusedAnchor >= 0)
        return true;
    return QFrame::focusNextPrevChild(next);
}

void ElidingLabel::keyPressEvent(QKeyEvent *event)
{
    const bool activates = event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter;
    if (!activates || !handlesLinkFocusItself()) {
        QLabel::keyPressEvent(event);
        return;
    }
    const QList<Anchor> anchors = elidedAnchors(this);
    if (m_focusedAnchor < 0 || m_focusedAnchor >= anchors.size()) {
        event->ignore();
        return;
    }
    event->accept();
    activateLink(anchors.at(m_focusedAnchor).href);
}

void ElidingLabel::focusOutEvent(QFocusEvent *event)
{
    if (event->reason() != Qt::ActiveWindowFocusReason
        && event->reason() != Qt::PopupFocusReason) {
        m_focusedAnchor = -1;
    }
    QLabel::focusOutEvent(event);
}

void ElidingLabel::leaveEvent(QEvent *event)
{
    if (handlesLinksItself() && !m_hoveredLink.isEmpty()) {
        unsetCursor();
        emit linkHovered({});
    }
    QLabel::leaveEvent(event);
}

bool ElidingLabel::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::Paint:
    case QEvent::MouseMove:
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::KeyPress:
    case QEvent::Leave:
    case QEvent::ToolTip:
        followLinkHandler();
        break;
    default:
        break;
    }

    if (event->type() == QEvent::ToolTip) {
        const auto helpEvent = static_cast<QHelpEvent *>(event);
        const QString link = handlesLinksItself() ? elidedAnchorAt(this, helpEvent->pos())
                                                  : m_hoveredLink;
        if (!QUrl(link).scheme().isEmpty()) {
            QToolTip::showText(helpEvent->globalPos(), asToolTip(link), this);
            return true;
        }
        // Without a tool tip of its own, the label would leave the URL of a
        // link it hovered before open.
        if (toolTip().isEmpty())
            QToolTip::hideText();
    }
    return QLabel::event(event);
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
    setToolTip(asToolTip(toolTip));
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
