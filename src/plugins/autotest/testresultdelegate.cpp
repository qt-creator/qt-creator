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

#include "autotestplugin.h"
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

const static int outputLimit = 100000;

static bool isSummaryItem(Result::Type type)
{
    return type == Result::MessageTestCaseSuccess || type == Result::MessageTestCaseSuccessWarn
            || type == Result::MessageTestCaseFail || type == Result::MessageTestCaseFailWarn;
}

TestResultDelegate::TestResultDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

void TestResultDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);
    painter->save();

    QFontMetrics fm(opt.font);
    QBrush background;
    QColor foreground;

    const QAbstractItemView *view = qobject_cast<const QAbstractItemView *>(opt.widget);
    const bool selected = opt.state & QStyle::State_Selected;

    if (selected) {
        background = opt.palette.highlight().color();
        foreground = opt.palette.highlightedText().color();
    } else {
        background = opt.palette.window().color();
        foreground = opt.palette.text().color();
    }
    painter->fillRect(opt.rect, background);
    painter->setPen(foreground);

    TestResultFilterModel *resultFilterModel = static_cast<TestResultFilterModel *>(view->model());
    LayoutPositions positions(opt, resultFilterModel);
    const TestResult *testResult = resultFilterModel->testResult(index);
    QTC_ASSERT(testResult, painter->restore();return);

    const QWidget *widget = dynamic_cast<const QWidget*>(painter->device());
    QWindow *window = widget ? widget->window()->windowHandle() : nullptr;

    QIcon icon = index.data(Qt::DecorationRole).value<QIcon>();
    if (!icon.isNull())
        painter->drawPixmap(positions.left(), positions.top(),
                            icon.pixmap(window, QSize(positions.iconSize(), positions.iconSize())));

    QString typeStr = TestResult::resultToString(testResult->result());
    if (selected) {
        painter->drawText(positions.typeAreaLeft(), positions.top() + fm.ascent(), typeStr);
    } else {
        QPen tmp = painter->pen();
        if (isSummaryItem(testResult->result()))
            painter->setPen(opt.palette.mid().color());
        else
            painter->setPen(TestResult::colorForType(testResult->result()));
        painter->drawText(positions.typeAreaLeft(), positions.top() + fm.ascent(), typeStr);
        painter->setPen(tmp);
    }

    QString output = testResult->outputString(selected);

    if (selected) {
        output.replace('\n', QChar::LineSeparator);

        if (AutotestPlugin::instance()->settings()->limitResultOutput
                && output.length() > outputLimit)
            output = output.left(outputLimit).append("...");

        recalculateTextLayout(index, output, painter->font(), positions.textAreaWidth());

        m_lastCalculatedLayout.draw(painter, QPoint(positions.textAreaLeft(), positions.top()));
    } else {
        painter->setClipRect(positions.textArea());
        // cut output before generating elided text as this takes quite long for exhaustive output
        painter->drawText(positions.textAreaLeft(), positions.top() + fm.ascent(),
                          fm.elidedText(output.left(2000), Qt::ElideRight, positions.textAreaWidth()));
    }

    QString file = testResult->fileName();
    const int pos = file.lastIndexOf('/');
    if (pos != -1)
        file = file.mid(pos + 1);

    painter->setClipRect(positions.fileArea());
    painter->drawText(positions.fileAreaLeft(), positions.top() + fm.ascent(), file);


    if (testResult->line()) {
        QString line = QString::number(testResult->line());
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
    QSize s;
    s.setWidth(opt.rect.width());

    if (selected) {
        const TestResult *testResult = resultFilterModel->testResult(index);
        QTC_ASSERT(testResult, return QSize());
        QString output = testResult->outputString(selected);
        output.replace('\n', QChar::LineSeparator);

        if (AutotestPlugin::instance()->settings()->limitResultOutput
                && output.length() > outputLimit)
            output = output.left(outputLimit).append("...");

        recalculateTextLayout(index, output, opt.font, positions.textAreaWidth());

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
}

void TestResultDelegate::recalculateTextLayout(const QModelIndex &index, const QString &output,
                                               const QFont &font, int width) const
{
    if (m_lastProcessedIndex == index && m_lastProcessedFont == font)
        return;

    const QFontMetrics fm(font);
    const int leading = fm.leading();
    const int fontHeight = fm.height();

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
