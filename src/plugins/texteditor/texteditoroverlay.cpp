#include <QtGui/QPainter>
#include "texteditoroverlay.h"
#include <QDebug>

using namespace TextEditor;
using namespace TextEditor::Internal;



TextEditorOverlay::TextEditorOverlay(BaseTextEditor *editor)
    :QObject(editor) {
    m_visible = false;
    m_borderWidth = 1;
    m_editor = editor;
    m_viewport = editor->viewport();
}

void TextEditorOverlay::update()
{
    if (m_visible)
        m_viewport->update();
}


void TextEditorOverlay::setVisible(bool b)
{
    if (m_visible == b)
        return;
    m_visible = b;
    update();
}

void TextEditorOverlay::clear()
{
    m_selections.clear();
    update();
}

void TextEditorOverlay::addOverlaySelection(int begin, int end,
                                            const QColor &fg, const QColor &bg,
                                            bool lockSize,
                                            bool dropShadow)
{
    if (end < begin)
        return;

    QTextDocument *document = m_editor->document();

    OverlaySelection selection;
    selection.m_fg = fg;
    selection.m_bg = bg;

    selection.m_cursor_begin = QTextCursor(document);
    selection.m_cursor_begin.setPosition(begin);

    selection.m_cursor_end = QTextCursor(document);
    selection.m_cursor_end.setPosition(end);

    if (lockSize)
        selection.m_fixedLength = (end - begin);

    selection.m_dropShadow = dropShadow;

    m_selections += selection;
    update();
}


void TextEditorOverlay::addOverlaySelection(const QTextCursor &cursor,
                                            const QColor &fg, const QColor &bg, bool lockSize)
{
    addOverlaySelection(cursor.selectionStart(), cursor.selectionEnd(), fg, bg, lockSize);
}

QRect TextEditorOverlay::rect() const
{
    return m_viewport->rect();
}

QPainterPath TextEditorOverlay::createSelectionPath(const QTextCursor &begin, const QTextCursor &end,
                                                    const QRect &clip)
{
    if (begin.isNull() || end.isNull() || begin.position() > end.position())
        return QPainterPath();


    QPointF offset = m_editor->contentOffset();
    QRect viewportRect = rect();
    QTextDocument *document = m_editor->document();

    if (m_editor->blockBoundingGeometry(begin.block()).translated(offset).top() > clip.bottom() + 10
        || m_editor->blockBoundingGeometry(end.block()).translated(offset).bottom() < clip.top() - 10
        )
        return QPainterPath(); // nothing of the selection is visible

    QTextBlock block = begin.block();

    bool inSelection = false;

    QVector<QRectF> selection;

    for (; block.isValid() && block.blockNumber() <= end.blockNumber(); block = block.next()) {
        if (! block.isVisible())
            continue;

        const QRectF blockGeometry = m_editor->blockBoundingGeometry(block);
        QTextLayout *blockLayout = block.layout();

        QTextLine line = blockLayout->lineAt(0);

        int beginChar = 0;
        if (!inSelection) {
            beginChar = begin.position() - begin.block().position();
            line = blockLayout->lineForTextPosition(beginChar);
            inSelection = true;
        } else {
            while (beginChar < block.length() && document->characterAt(block.position() + beginChar).isSpace())
                ++beginChar;
            if (beginChar == block.length())
                beginChar = 0;
        }

        int lastLine = blockLayout->lineCount()-1;
        int endChar = -1;
        if (block == end.block()) {
            endChar = end.position()  - end.block().position();
            lastLine = blockLayout->lineForTextPosition(endChar).lineNumber();
            inSelection = false;
        } else {
            endChar = block.length();
            while (endChar > beginChar && document->characterAt(block.position() + endChar - 1).isSpace())
                --endChar;
        }

        QRectF lineRect = line.naturalTextRect();
        if (beginChar < endChar) {
            lineRect.setLeft(line.cursorToX(beginChar));
            if (line.lineNumber() == lastLine)
                lineRect.setRight(line.cursorToX(endChar));
            selection += lineRect.translated(blockGeometry.topLeft());

            for (int lineIndex = line.lineNumber()+1; lineIndex <= lastLine; ++lineIndex) {
                line = blockLayout->lineAt(lineIndex);
                lineRect = line.naturalTextRect();
                if (lineIndex == lastLine)
                    lineRect.setRight(line.cursorToX(endChar));
                selection += lineRect.translated(blockGeometry.topLeft());
            }
        } else { // empty lines
            if (!selection.isEmpty())
                lineRect.setLeft(selection.last().left());
            lineRect.setRight(lineRect.left() + 16);
            selection += lineRect.translated(blockGeometry.topLeft());
        }

        if (!inSelection)
            break;

        if (blockGeometry.translated(offset).y() > 2*viewportRect.height())
            break;
    }


    if (selection.isEmpty())
        return QPainterPath();

    QVector<QPointF> points;

    const int margin = m_borderWidth/2;
    const int extra = 0;

    points += (selection.at(0).topLeft() + selection.at(0).topRight()) / 2 + QPointF(0, -margin);
    points += selection.at(0).topRight() + QPointF(margin+1, -margin);
    points += selection.at(0).bottomRight() + QPointF(margin+1, 0);


    for(int i = 1; i < selection.count()-1; ++i) {

#define MAX3(a,b,c) qMax(a, qMax(b,c))
        qreal x = MAX3(selection.at(i-1).right(),
                       selection.at(i).right(),
                       selection.at(i+1).right()) + margin;

        points += QPointF(x+1, selection.at(i).top());
        points += QPointF(x+1, selection.at(i).bottom()+1);

    }

    points += selection.at(selection.count()-1).topRight() + QPointF(margin+1, 0);
    points += selection.at(selection.count()-1).bottomRight() + QPointF(margin+1, margin+extra);
    points += selection.at(selection.count()-1).bottomLeft() + QPointF(-margin, margin+extra);
    points += selection.at(selection.count()-1).topLeft() + QPointF(-margin, 0);

    for(int i = selection.count()-2; i > 0; --i) {
#define MIN3(a,b,c) qMin(a, qMin(b,c))
        qreal x = MIN3(selection.at(i-1).left(),
                       selection.at(i).left(),
                       selection.at(i+1).left()) - margin;

        points += QPointF(x, selection.at(i).bottom()+extra);
        points += QPointF(x, selection.at(i).top());
    }

    points += selection.at(0).bottomLeft() + QPointF(-margin, extra);
    points += selection.at(0).topLeft() + QPointF(-margin, -margin);


    QPainterPath path;
    const int corner = 4;
    path.moveTo(points.at(0));
    points += points.at(0);
    QPointF previous = points.at(0);
    for (int i = 1; i < points.size(); ++i) {
        QPointF point = points.at(i);
        if (point.y() == previous.y() && qAbs(point.x() - previous.x()) > 2*corner) {
            QPointF tmp = QPointF(previous.x() + corner * ((point.x() > previous.x())?1:-1), previous.y());
            path.quadTo(previous, tmp);
            previous = tmp;
            i--;
            continue;
        } else if (point.x() == previous.x() && qAbs(point.y() - previous.y()) > 2*corner) {
            QPointF tmp = QPointF(previous.x(), previous.y() + corner * ((point.y() > previous.y())?1:-1));
            path.quadTo(previous, tmp);
            previous = tmp;
            i--;
            continue;
        }


        QPointF target = (previous + point) / 2;
        path.quadTo(previous, target);
        previous = points.at(i);
    }
    path.closeSubpath();
    path.translate(offset);
    return path;
}

void TextEditorOverlay::paintSelection(QPainter *painter,
                                       const OverlaySelection &selection)
{

    const QTextCursor &begin = selection.m_cursor_begin;
    const QTextCursor &end= selection.m_cursor_end;
    const QColor &fg = selection.m_fg;
    const QColor &bg = selection.m_bg;


    if (begin.isNull() || end.isNull() || begin.position() > end.position())
        return;

    QPainterPath path = createSelectionPath(begin, end, m_editor->viewport()->rect());

    painter->save();
    QColor penColor = fg;
    penColor.setAlpha(220);
    QPen pen(penColor, m_borderWidth);
    painter->translate(-.5, -.5);

    QRectF pathRect = path.controlPointRect();

    if (bg.isValid()) {
        QLinearGradient linearGrad(pathRect.topLeft(), pathRect.bottomLeft());
        QColor col1 = fg.lighter(150);
        col1.setAlpha(20);
        QColor col2 = fg;
        col2.setAlpha(80);
        linearGrad.setColorAt(0, col1);
        linearGrad.setColorAt(1, col2);
        painter->setBrush(QBrush(linearGrad));
    } else {
        painter->setBrush(QBrush());
    }

    painter->setRenderHint(QPainter::Antialiasing);
    pen.setJoinStyle(Qt::RoundJoin);
    painter->setPen(pen);
    painter->drawPath(path);
    painter->restore();
}

void TextEditorOverlay::fillSelection(QPainter *painter,
                                      const OverlaySelection &selection,
                                      const QColor &color)
{
    const QTextCursor &begin = selection.m_cursor_begin;
    const QTextCursor &end= selection.m_cursor_end;
    if (begin.isNull() || end.isNull() || begin.position() > end.position())
        return;

    QPainterPath path = createSelectionPath(begin, end, m_editor->viewport()->rect());

    painter->save();
    painter->translate(-.5, -.5);
    painter->setRenderHint(QPainter::Antialiasing);
    if (selection.m_dropShadow) {
        painter->translate(2, 2);
        painter->fillPath(path, QColor(0, 0, 0, 100));
        painter->translate(-2, -2);
    }
    painter->fillPath(path, color);
    painter->restore();
}

void TextEditorOverlay::paint(QPainter *painter, const QRect &clip)
{
    Q_UNUSED(clip);
    for (int i = 0; i < m_selections.size(); ++i) {
        const OverlaySelection &selection = m_selections.at(i);
        if (selection.m_fixedLength >= 0
            && selection.m_cursor_end.position() - selection.m_cursor_begin.position()
            != selection.m_fixedLength)
            continue;

        paintSelection(painter, selection);
    }
}

void TextEditorOverlay::fill(QPainter *painter, const QColor &color, const QRect &clip)
{
    Q_UNUSED(clip);
    for (int i = 0; i < m_selections.size(); ++i) {
        const OverlaySelection &selection = m_selections.at(i);
        if (selection.m_fixedLength >= 0
            && selection.m_cursor_end.position() - selection.m_cursor_begin.position()
            != selection.m_fixedLength)
            continue;

        fillSelection(painter, selection, color);
    }
}

void TextEditorOverlay::paintInverted(QPainter *painter, const QRect &clip, const QColor &color)
{
    QPainterPath path;
    for (int i = 0; i < m_selections.size(); ++i) {
        const OverlaySelection &selection = m_selections.at(i);
        if (selection.m_fixedLength >= 0
            && selection.m_cursor_end.position() - selection.m_cursor_begin.position()
            != selection.m_fixedLength)
            continue;
        path.addPath(createSelectionPath(selection.m_cursor_begin, selection.m_cursor_end, clip));
    }

    QRect viewportRect = m_editor->viewport()->rect();
    QColor background = Qt::black;
    background.setAlpha(30);

    if (path.isEmpty()) {
        painter->fillRect(viewportRect, background);
        return;
    }

//    QPainterPath all;
//    all.addRect(viewportRect);
//    QPainterPath inversion = all.subtracted(path);

    painter->save();
    QColor penColor = color;
    penColor.setAlpha(220);
    QPen pen(penColor, m_borderWidth);
    QColor brush = color;
    brush.setAlpha(30);
    painter->translate(-.5, -.5);

//    painter->setRenderHint(QPainter::Antialiasing);
    //pen.setJoinStyle(Qt::RoundJoin);
    painter->setPen(pen);
    painter->setBrush(QBrush());
    painter->drawPath(path);

    painter->translate(.5, .5);

    QPixmap shadow(clip.size());
    shadow.fill(background);
    QPainter pmp(&shadow);
    pmp.translate(-.5, -.5);
    pmp.setRenderHint(QPainter::Antialiasing);
    pmp.setCompositionMode(QPainter::CompositionMode_Source);
    path.translate(-clip.topLeft());
    pen.setColor(Qt::transparent);
    pmp.setPen(pen);
    pmp.setBrush(Qt::transparent);
    pmp.drawPath(path);
    pmp.end();

    painter->drawPixmap(clip.topLeft(), shadow);

//    painter->fillPath(inversion, background);
    painter->restore();
}



