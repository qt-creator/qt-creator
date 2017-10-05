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

#include "textmark.h"
#include "textdocument.h"
#include "texteditor.h"
#include "texteditorplugin.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/documentmanager.h>
#include <utils/qtcassert.h>

#include <QGridLayout>
#include <QPainter>

using namespace Core;
using namespace Utils;
using namespace TextEditor::Internal;

namespace TextEditor {

class TextMarkRegistry : public QObject
{
    Q_OBJECT
public:
    static void add(TextMark *mark);
    static bool remove(TextMark *mark);

private:
    TextMarkRegistry(QObject *parent);
    static TextMarkRegistry* instance();
    void editorOpened(Core::IEditor *editor);
    void documentRenamed(Core::IDocument *document, const QString &oldName, const QString &newName);
    void allDocumentsRenamed(const QString &oldName, const QString &newName);

    QHash<Utils::FileName, QSet<TextMark *> > m_marks;
};

class AnnotationColors
{
public:
    static AnnotationColors &getAnnotationColors(const QColor &markColor,
                                                 const QColor &backgroundColor);

public:
    using SourceColors = QPair<QColor, QColor>;
    QColor rectColor;
    QColor textColor;

private:
    static QHash<SourceColors, AnnotationColors> m_colorCache;
};

TextMarkRegistry *m_instance = nullptr;

TextMark::TextMark(const QString &fileName, int lineNumber, Id category, double widthFactor)
    : m_fileName(fileName)
    , m_lineNumber(lineNumber)
    , m_visible(true)
    , m_category(category)
    , m_widthFactor(widthFactor)
{
    if (!m_fileName.isEmpty())
        TextMarkRegistry::add(this);
}

TextMark::~TextMark()
{
    if (!m_fileName.isEmpty())
        TextMarkRegistry::remove(this);
    if (m_baseTextDocument)
        m_baseTextDocument->removeMark(this);
    m_baseTextDocument = nullptr;
}

QString TextMark::fileName() const
{
    return m_fileName;
}

void TextMark::updateFileName(const QString &fileName)
{
    if (fileName == m_fileName)
        return;
    if (!m_fileName.isEmpty())
        TextMarkRegistry::remove(this);
    m_fileName = fileName;
    if (!m_fileName.isEmpty())
        TextMarkRegistry::add(this);
}

int TextMark::lineNumber() const
{
    return m_lineNumber;
}

void TextMark::paintIcon(QPainter *painter, const QRect &rect) const
{
    m_icon.paint(painter, rect, Qt::AlignCenter);
}

void TextMark::paintAnnotation(QPainter &painter, QRectF *annotationRect,
                               const qreal fadeInOffset, const qreal fadeOutOffset,
                               const QPointF &contentOffset) const
{
    QString text = lineAnnotation();
    if (text.isEmpty())
        return;

    const AnnotationRects &rects = annotationRects(*annotationRect, painter.fontMetrics(),
                                                   fadeInOffset, fadeOutOffset);
    const QColor &markColor = m_hasColor ? Utils::creatorTheme()->color(m_color).toHsl()
                                         : painter.pen().color();
    const AnnotationColors &colors = AnnotationColors::getAnnotationColors(
                markColor, painter.background().color());

    painter.save();
    QLinearGradient grad(rects.fadeInRect.topLeft() - contentOffset,
                         rects.fadeInRect.topRight() - contentOffset);
    grad.setColorAt(0.0, Qt::transparent);
    grad.setColorAt(1.0, colors.rectColor);
    painter.fillRect(rects.fadeInRect, grad);
    painter.fillRect(rects.annotationRect, colors.rectColor);
    painter.setPen(colors.textColor);
    paintIcon(&painter, rects.iconRect.toAlignedRect());
    painter.drawText(rects.textRect, Qt::AlignLeft, rects.text);
    if (rects.fadeOutRect.isValid()) {
        grad = QLinearGradient(rects.fadeOutRect.topLeft() - contentOffset,
                               rects.fadeOutRect.topRight() - contentOffset);
        grad.setColorAt(0.0, colors.rectColor);
        grad.setColorAt(1.0, Qt::transparent);
        painter.fillRect(rects.fadeOutRect, grad);
    }
    painter.restore();
    annotationRect->setRight(rects.fadeOutRect.right());
}

TextMark::AnnotationRects TextMark::annotationRects(const QRectF &boundingRect,
                                                    const QFontMetrics &fm,
                                                    const qreal fadeInOffset,
                                                    const qreal fadeOutOffset) const
{
    AnnotationRects rects;
    rects.text = lineAnnotation();
    if (rects.text.isEmpty())
        return rects;
    rects.fadeInRect = boundingRect;
    rects.fadeInRect.setWidth(fadeInOffset);
    rects.annotationRect = boundingRect;
    rects.annotationRect.setLeft(rects.fadeInRect.right());
    const bool drawIcon = !m_icon.isNull();
    constexpr qreal margin = 1;
    rects.iconRect = QRectF(rects.annotationRect.left(), boundingRect.top(),
                            0, boundingRect.height());
    if (drawIcon)
        rects.iconRect.setWidth(rects.iconRect.height() * m_widthFactor);
    rects.textRect = QRectF(rects.iconRect.right() + margin, boundingRect.top(),
                            qreal(fm.width(rects.text)), boundingRect.height());
    rects.annotationRect.setRight(rects.textRect.right() + margin);
    if (rects.annotationRect.right() > boundingRect.right()) {
        rects.textRect.setRight(boundingRect.right() - margin);
        rects.text = fm.elidedText(rects.text, Qt::ElideRight, int(rects.textRect.width()));
        rects.annotationRect.setRight(boundingRect.right());
        rects.fadeOutRect = QRectF(rects.annotationRect.topRight(),
                                   rects.annotationRect.bottomRight());
    } else {
        rects.fadeOutRect = boundingRect;
        rects.fadeOutRect.setLeft(rects.annotationRect.right());
        rects.fadeOutRect.setWidth(fadeOutOffset);
    }
    return rects;
}

void TextMark::updateLineNumber(int lineNumber)
{
    m_lineNumber = lineNumber;
}

void TextMark::move(int line)
{
    if (line == m_lineNumber)
        return;
    const int previousLine = m_lineNumber;
    m_lineNumber = line;
    if (m_baseTextDocument)
        m_baseTextDocument->moveMark(this, previousLine);
}

void TextMark::updateBlock(const QTextBlock &)
{}

void TextMark::removedFromEditor()
{}

void TextMark::updateMarker()
{
    if (m_baseTextDocument)
        m_baseTextDocument->updateMark(this);
}

void TextMark::setPriority(TextMark::Priority prioriy)
{
    m_priority = prioriy;
    updateMarker();
}

bool TextMark::isVisible() const
{
    return m_visible;
}

void TextMark::setVisible(bool visible)
{
    m_visible = visible;
    updateMarker();
}

double TextMark::widthFactor() const
{
    return m_widthFactor;
}

void TextMark::setWidthFactor(double factor)
{
    m_widthFactor = factor;
}

bool TextMark::isClickable() const
{
    return false;
}

void TextMark::clicked()
{}

bool TextMark::isDraggable() const
{
    return false;
}

void TextMark::dragToLine(int lineNumber)
{
    Q_UNUSED(lineNumber);
}

void TextMark::addToToolTipLayout(QGridLayout *target) const
{
    auto *contentLayout = new QVBoxLayout;
    addToolTipContent(contentLayout);
    if (contentLayout->count() > 0) {
        const int row = target->rowCount();
        if (!m_icon.isNull()) {
            auto iconLabel = new QLabel;
            iconLabel->setPixmap(m_icon.pixmap(16, 16));
            target->addWidget(iconLabel, row, 0, Qt::AlignTop | Qt::AlignHCenter);
        }
        target->addLayout(contentLayout, row, 1);
    }
}

bool TextMark::addToolTipContent(QLayout *target) const
{
    QString text = m_toolTip;
    if (text.isEmpty()) {
        text = m_defaultToolTip;
        if (text.isEmpty())
            return false;
    }

    auto textLabel = new QLabel;
    textLabel->setText(text);
    // Differentiate between tool tips that where explicitly set and default tool tips.
    textLabel->setEnabled(!m_toolTip.isEmpty());
    target->addWidget(textLabel);

    return true;
}

Theme::Color TextMark::color() const
{
    QTC_CHECK(m_hasColor);
    return m_color;
}

void TextMark::setColor(const Theme::Color &color)
{
    m_hasColor = true;
    m_color = color;
}

TextMarkRegistry::TextMarkRegistry(QObject *parent)
    : QObject(parent)
{
    connect(EditorManager::instance(), &EditorManager::editorOpened,
            this, &TextMarkRegistry::editorOpened);

    connect(DocumentManager::instance(), &DocumentManager::allDocumentsRenamed,
            this, &TextMarkRegistry::allDocumentsRenamed);
    connect(DocumentManager::instance(), &DocumentManager::documentRenamed,
            this, &TextMarkRegistry::documentRenamed);
}

void TextMarkRegistry::add(TextMark *mark)
{
    instance()->m_marks[FileName::fromString(mark->fileName())].insert(mark);
    auto document = qobject_cast<TextDocument*>(DocumentModel::documentForFilePath(mark->fileName()));
    if (!document)
        return;
    document->addMark(mark);
}

bool TextMarkRegistry::remove(TextMark *mark)
{
    return instance()->m_marks[FileName::fromString(mark->fileName())].remove(mark);
}

TextMarkRegistry *TextMarkRegistry::instance()
{
    if (!m_instance)
        m_instance = new TextMarkRegistry(TextEditorPlugin::instance());
    return m_instance;
}

void TextMarkRegistry::editorOpened(IEditor *editor)
{
    auto document = qobject_cast<TextDocument *>(editor ? editor->document() : 0);
    if (!document)
        return;
    if (!m_marks.contains(document->filePath()))
        return;

    foreach (TextMark *mark, m_marks.value(document->filePath()))
        document->addMark(mark);
}

void TextMarkRegistry::documentRenamed(IDocument *document, const
                                           QString &oldName, const QString &newName)
{
    TextDocument *baseTextDocument = qobject_cast<TextDocument *>(document);
    if (!baseTextDocument)
        return;
    FileName oldFileName = FileName::fromString(oldName);
    FileName newFileName = FileName::fromString(newName);
    if (!m_marks.contains(oldFileName))
        return;

    QSet<TextMark *> toBeMoved;
    foreach (TextMark *mark, baseTextDocument->marks())
        toBeMoved.insert(mark);

    m_marks[oldFileName].subtract(toBeMoved);
    m_marks[newFileName].unite(toBeMoved);

    foreach (TextMark *mark, toBeMoved)
        mark->updateFileName(newName);
}

void TextMarkRegistry::allDocumentsRenamed(const QString &oldName, const QString &newName)
{
    FileName oldFileName = FileName::fromString(oldName);
    FileName newFileName = FileName::fromString(newName);
    if (!m_marks.contains(oldFileName))
        return;

    QSet<TextMark *> oldFileNameMarks = m_marks.value(oldFileName);

    m_marks[newFileName].unite(oldFileNameMarks);
    m_marks[oldFileName].clear();

    foreach (TextMark *mark, oldFileNameMarks)
        mark->updateFileName(newName);
}

QHash<AnnotationColors::SourceColors, AnnotationColors> AnnotationColors::m_colorCache;

AnnotationColors &AnnotationColors::getAnnotationColors(const QColor &markColor,
                                                        const QColor &backgroundColor)
{
    auto highClipHsl = [](qreal value) {
        return std::max(0.7, std::min(0.9, value));
    };
    auto lowClipHsl = [](qreal value) {
        return std::max(0.1, std::min(0.3, value));
    };
    AnnotationColors &colors = m_colorCache[{markColor, backgroundColor}];
    if (!colors.rectColor.isValid() || !colors.textColor.isValid()) {
        const double backgroundLightness = backgroundColor.lightnessF();
        const double foregroundLightness = backgroundLightness > 0.5
                ? lowClipHsl(backgroundLightness - 0.5)
                : highClipHsl(backgroundLightness + 0.5);

        colors.rectColor = markColor;
        colors.rectColor.setAlphaF(0.15);

        colors.textColor.setHslF(markColor.hslHueF(),
                                 markColor.hslSaturationF(),
                                 foregroundLightness);
    }
    return colors;
}

} // namespace TextEditor

#include "textmark.moc"
