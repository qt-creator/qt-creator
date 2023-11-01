// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "testresultdelegate.h"

#include "testresultmodel.h"
#include "testsettings.h"

#include <utils/qtcassert.h>

#include <QAbstractItemView>
#include <QPainter>
#include <QTextLayout>
#include <QWindow>

namespace Autotest {
namespace Internal {

constexpr int outputLimit = 100000;

TestResultDelegate::TestResultDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

void TestResultDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    QFontMetrics fm(opt.font);
    QBrush background;
    QColor foreground;

    const bool selected = opt.state & QStyle::State_Selected;

    if (selected) {
        background = opt.palette.highlight().color();
        foreground = opt.palette.highlightedText().color();
    } else {
        background = opt.palette.window().color();
        foreground = opt.palette.text().color();
    }

    auto resultFilterModel = qobject_cast<const TestResultFilterModel *>(index.model());
    if (!resultFilterModel)
        return;
    painter->save();
    painter->fillRect(opt.rect, background);
    painter->setPen(foreground);

    const LayoutPositions positions(opt, resultFilterModel);
    const TestResult testResult = resultFilterModel->testResult(index);
    QTC_ASSERT(testResult.isValid(), painter->restore(); return);

    const QWidget *widget = dynamic_cast<const QWidget*>(painter->device());
    QWindow *window = widget ? widget->window()->windowHandle() : nullptr;

    QIcon icon = index.data(Qt::DecorationRole).value<QIcon>();
    if (!icon.isNull())
        painter->drawPixmap(positions.left(), positions.top(),
                            icon.pixmap(window, QSize(positions.iconSize(), positions.iconSize())));

    TestResultItem *item = resultFilterModel->itemForIndex(index);
    QTC_ASSERT(item, painter->restore(); return);
    const QString typeStr = item->resultString();
    if (selected) {
        painter->drawText(positions.typeAreaLeft(), positions.top() + fm.ascent(), typeStr);
    } else {
        QPen tmp = painter->pen();
        if (testResult.result() == ResultType::TestStart)
            painter->setPen(opt.palette.mid().color());
        else
            painter->setPen(TestResult::colorForType(testResult.result()));
        painter->drawText(positions.typeAreaLeft(), positions.top() + fm.ascent(), typeStr);
        painter->setPen(tmp);
    }

    QString output = testResult.outputString(selected);

    if (selected) {
        limitTextOutput(output);
        output.replace('\n', QChar::LineSeparator);
        recalculateTextLayout(index, output, painter->font(), positions.textAreaWidth());

        m_lastCalculatedLayout.draw(painter, QPoint(positions.textAreaLeft(), positions.top()));
    } else {
        painter->setClipRect(positions.textArea());
        // cut output before generating elided text as this takes quite long for exhaustive output
        painter->drawText(positions.textAreaLeft(), positions.top() + fm.ascent(),
                          fm.elidedText(output.left(2000), Qt::ElideRight, positions.textAreaWidth()));
    }

    const QString file = testResult.fileName().fileName();
    painter->setClipRect(positions.fileArea());
    painter->drawText(positions.fileAreaLeft(), positions.top() + fm.ascent(), file);

    if (testResult.line()) {
        QString line = QString::number(testResult.line());
        painter->setClipRect(positions.lineArea());
        painter->drawText(positions.lineAreaLeft(), positions.top() + fm.ascent(), line);
    }

    painter->setClipping(false);
    painter->setPen(opt.palette.mid().color());
    const QRectF adjustedRect(QRectF(opt.rect).adjusted(0.5, 0.5, -0.5, -0.5));
    painter->drawLine(adjustedRect.bottomLeft(), adjustedRect.bottomRight());
    painter->restore();
}

QSize TestResultDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem opt = option;
    // make sure opt.rect is initialized correctly - otherwise we might get a width of 0
    opt.initFrom(opt.widget);

    const QAbstractItemView *view = qobject_cast<const QAbstractItemView *>(opt.widget);
    const bool selected = view->selectionModel()->currentIndex() == index;

    QFontMetrics fm(opt.font);
    int fontHeight = fm.height();
    TestResultFilterModel *resultFilterModel = static_cast<TestResultFilterModel *>(view->model());
    LayoutPositions positions(opt, resultFilterModel);
    const int depth = resultFilterModel->itemForIndex(index)->level() + 1;
    const int indentation = depth * view->style()->pixelMetric(QStyle::PM_TreeViewIndentation, &opt);

    QSize s;
    s.setWidth(opt.rect.width() - indentation);

    if (selected) {
        const TestResult testResult = resultFilterModel->testResult(index);
        QTC_ASSERT(testResult.isValid(), return {});
        QString output = testResult.outputString(selected);
        limitTextOutput(output);
        output.replace('\n', QChar::LineSeparator);
        recalculateTextLayout(index, output, opt.font, positions.textAreaWidth() - indentation);

        s.setHeight(m_lastCalculatedHeight + 3);
    } else {
        s.setHeight(fontHeight + 3);
    }

    if (s.height() < positions.minimumHeight())
        s.setHeight(positions.minimumHeight());

    return s;
}

void TestResultDelegate::currentChanged(const QModelIndex &current, const QModelIndex &previous)
{
    emit sizeHintChanged(current);
    emit sizeHintChanged(previous);
}

void TestResultDelegate::clearCache()
{
    m_lastProcessedIndex = QModelIndex();
    m_lastProcessedFont = QFont();
    m_lastWidth = -1;
}

void TestResultDelegate::limitTextOutput(QString &output) const
{
    int maxLineCount = testSettings().resultDescriptionMaxSize();
    bool limited = false;

    if (testSettings().limitResultDescription() && maxLineCount > 0) {
        int index = -1;
        int lastChar = output.size() - 1;

        for (int i = 0; i < maxLineCount; i++) {
            index = output.indexOf('\n', index + 1);
            if (index == -1 || index == lastChar) {
                index = -1;
                break;
            }
        }

        if (index > 0) {
            output = output.left(index);
            limited = true;
        }
    }

    if (testSettings().limitResultOutput() && output.length() > outputLimit) {
        output = output.left(outputLimit);
        limited = true;
    }

    if (limited)
        output.append("...");
}

void TestResultDelegate::recalculateTextLayout(const QModelIndex &index, const QString &output,
                                               const QFont &font, int width) const
{
    if (m_lastWidth == width && m_lastProcessedIndex == index && m_lastProcessedFont == font)
        return;

    const QFontMetrics fm(font);
    const int leading = fm.leading();
    const int fontHeight = fm.height();

    m_lastWidth = width;
    m_lastProcessedIndex = index;
    m_lastProcessedFont = font;
    m_lastCalculatedHeight = 0;
    m_lastCalculatedLayout.clearLayout();
    m_lastCalculatedLayout.setText(output);
    m_lastCalculatedLayout.setFont(font);
    QTextOption txtOption;
    txtOption.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    m_lastCalculatedLayout.setTextOption(txtOption);
    m_lastCalculatedLayout.beginLayout();
    while (true) {
        QTextLine line = m_lastCalculatedLayout.createLine();
        if (!line.isValid())
            break;
        line.setLineWidth(width);
        m_lastCalculatedHeight += leading;
        line.setPosition(QPoint(0, m_lastCalculatedHeight));
        m_lastCalculatedHeight += fontHeight;
    }
    m_lastCalculatedLayout.endLayout();
}

} // namespace Internal
} // namespace Autotest
