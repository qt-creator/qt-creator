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

namespace Autotest::Internal {

constexpr int outputLimit = 100000;

enum class FontType
{
    App,
    Mono
};

static const QFont &fontForType(FontType type)
{
    static QFont appFont; // default initialized app font
    static QFont monoFont("Source Code Pro", appFont.pointSize() - 1);
    return type == FontType::Mono ? monoFont : appFont;
}

static const QFontMetricsF &fontMetrics(FontType type)
{
    static QFontMetricsF appFontMetrics(fontForType(FontType::App));
    static QFontMetricsF monoFontMetrics(fontForType(FontType::Mono));
    return type == FontType::Mono ? monoFontMetrics : appFontMetrics;
}

TestResultDelegate::TestResultDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

void TestResultDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

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

    const LayoutPositions positions(opt, resultFilterModel, m_showDuration);
    const TestResult testResult = resultFilterModel->testResult(index);
    QTC_ASSERT(testResult.isValid(), painter->restore(); return);

    const QIcon icon = index.data(Qt::DecorationRole).value<QIcon>();
    if (!icon.isNull()) {
        painter->drawPixmap(positions.left(), positions.top(),
                            icon.pixmap(QSize(positions.iconSize(), positions.iconSize()),
                                        painter->device()->devicePixelRatio()));
    }

    TestResultItem *item = resultFilterModel->itemForIndex(index);
    QTC_ASSERT(item, painter->restore(); return);
    const QString typeStr = item->resultString();
    if (selected) {
        painter->drawText(
            positions.typeAreaLeft(),
            positions.top() + fontMetrics(FontType::App).ascent(),
            typeStr);
    } else {
        QPen tmp = painter->pen();
        if (testResult.result() == ResultType::TestStart)
            painter->setPen(opt.palette.mid().color());
        else
            painter->setPen(TestResult::colorForType(testResult.result()));
        painter->drawText(
            positions.typeAreaLeft(),
            positions.top() + fontMetrics(FontType::App).ascent(),
            typeStr);
        painter->setPen(tmp);
    }

    QString output = testResult.outputString(selected);

    if (selected) {
        limitTextOutput(output);
        output.replace('\n', QChar::LineSeparator);
        recalculateTextLayouts(index, item->testResult().result(),
                               output, positions.textAreaWidth());

        m_lastCalculatedLayout.draw(painter, QPoint(positions.textAreaLeft(), positions.top()));
        m_lastCalculatedMSLayout.draw(painter, QPoint(positions.textAreaLeft(), positions.top()));
    } else {
        painter->setClipRect(positions.textArea());
        // cut output before generating elided text as this takes quite long for exhaustive output
        painter->drawText(
            positions.textAreaLeft(),
            positions.top() + fontMetrics(FontType::App).ascent(),
            fontMetrics(FontType::App)
                .elidedText(output.left(2000), Qt::ElideRight, positions.textAreaWidth()));
    }

    if (testResult.result() == ResultType::TestStart && m_showDuration && testResult.duration()) {
        const QString txt = *testResult.duration() + " ms";
        QPen tmp = painter->pen();
        painter->setPen(opt.palette.mid().color());
        painter->setClipRect(positions.durationArea());
        option.widget->style()->drawItemText(painter, positions.durationArea(), Qt::AlignRight,
                                             opt.palette, true, txt);
        painter->setPen(tmp);
    }

    const QString file = testResult.fileName().fileName();
    painter->setClipRect(positions.fileArea());
    painter->drawText(
        positions.fileAreaLeft(), positions.top() + fontMetrics(FontType::App).ascent(), file);

    if (testResult.line()) {
        QString line = QString::number(testResult.line());
        painter->setClipRect(positions.lineArea());
        painter->drawText(
            positions.lineAreaLeft(), positions.top() + fontMetrics(FontType::App).ascent(), line);
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

    // correct width only important for selected item to calculate layout correctly, other items
    // get layouted directly inside paint() based on the LayoutPositions, so width of the rect may
    // be wrong but is good enough as nothing important depends on it
    // height is always needed to be correct to update the view correctly
    QSize s;
    s.setWidth(opt.rect.width());

    if (selected) {
        auto *resultFilterModel = static_cast<TestResultFilterModel *>(view->model());
        const LayoutPositions positions(opt, resultFilterModel, m_showDuration);
        const int depth = resultFilterModel->itemForIndex(index)->level() + 1;
        const int indentation = depth * view->style()->pixelMetric(QStyle::PM_TreeViewIndentation, &opt);
        const TestResult testResult = resultFilterModel->testResult(index);
        QTC_ASSERT(testResult.isValid(), return {});
        QString output = testResult.outputString(selected);
        limitTextOutput(output);
        output.replace('\n', QChar::LineSeparator);
        recalculateTextLayouts(index, testResult.result(),
                               output, positions.textAreaWidth() - indentation);

        s.setHeight(m_lastCalculatedHeight + 3);
    } else {
        s.setHeight(fontMetrics(FontType::App).height() + 3);
    }

    if (s.height() < LayoutPositions::minimumHeight())
        s.setHeight(LayoutPositions::minimumHeight());

    return s;
}

void TestResultDelegate::currentChanged(const QModelIndex &current, const QModelIndex &/*previous*/)
{
    // UniformRowHeights == false, so emitting once will trigger sizeHint() request on all anyhow
    emit sizeHintChanged(current);
}

void TestResultDelegate::clearCache()
{
    m_lastProcessedIndex = QModelIndex();
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

    if (testSettings().limitResultOutput() && output.size() > outputLimit) {
        output = output.left(outputLimit);
        limited = true;
    }

    if (limited)
        output.append("...");
}

void TestResultDelegate::recalculateTextLayouts(const QModelIndex &index, ResultType type,
                                                const QString &output, int width) const
{
    if (m_lastWidth == width && m_lastProcessedIndex == index)
        return;

    m_lastWidth = width;
    m_lastProcessedIndex = index;
    m_lastCalculatedHeight = 0;
    m_lastCalculatedLayout.clearLayout();

    const int leading = fontMetrics(FontType::App).leading();
    const int fontHeight = fontMetrics(FontType::App).height();

    const int firstNL = type == ResultType::MessageInternal ? -1
                                                            : output.indexOf(QChar::LineSeparator);
    m_lastCalculatedLayout.setText(output.left(firstNL));

    m_lastCalculatedLayout.setFont(fontForType(FontType::App));
    QTextOption txtOption;
    txtOption.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    m_lastCalculatedLayout.setTextOption(txtOption);

    auto doLayout = [this](QTextLayout *layout, int width, int leading, int height) {
        while (true) {
            QTextLine line = layout->createLine();
            if (!line.isValid())
                break;
            line.setLineWidth(width);
            m_lastCalculatedHeight += leading;
            line.setPosition(QPoint(0, m_lastCalculatedHeight));
            m_lastCalculatedHeight += height;
        }
    };

    m_lastCalculatedLayout.beginLayout();
    doLayout(&m_lastCalculatedLayout, width, leading, fontHeight);
    m_lastCalculatedLayout.endLayout();

    m_lastCalculatedMSLayout.clearLayout();
    if (firstNL == -1)
        return;

    const int monoLeading = fontMetrics(FontType::Mono).leading();
    const int monoFontHeight = fontMetrics(FontType::Mono).height();
    m_lastCalculatedMSLayout.setText(output.mid(firstNL + 1));
    m_lastCalculatedMSLayout.setFont(fontForType(FontType::Mono));
    m_lastCalculatedMSLayout.beginLayout();
    doLayout(&m_lastCalculatedMSLayout, width, monoLeading, monoFontHeight);
    m_lastCalculatedMSLayout.endLayout();
}

} // namespace Autotest::Internal
