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

#include <QAbstractItemView>
#include <QDebug>
#include <QPainter>
#include <QTextLayout>

namespace Autotest {
namespace Internal {

const static int outputLimit = 100000;

TestResultDelegate::TestResultDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

QString TestResultDelegate::outputString(const TestResult &testResult, bool selected)
{
    const QString desc = testResult.description();
    QString output;
    switch (testResult.result()) {
    case Result::Pass:
    case Result::Fail:
    case Result::ExpectedFail:
    case Result::UnexpectedPass:
    case Result::BlacklistedFail:
    case Result::BlacklistedPass:
        if (testResult.type() == TestTypeQt)
            output = testResult.className() + QLatin1String("::") + testResult.testCase();
        else // TestTypeGTest
            output = testResult.testCase();
        if (!testResult.dataTag().isEmpty())
            output.append(QString::fromLatin1(" (%1)").arg(testResult.dataTag()));
        if (selected && !desc.isEmpty()) {
            output.append(QLatin1Char('\n')).append(desc);
        }
        break;
    case Result::Benchmark:
        output = testResult.className() + QLatin1String("::") + testResult.testCase();
        if (!testResult.dataTag().isEmpty())
            output.append(QString::fromLatin1(" (%1)").arg(testResult.dataTag()));
        if (!desc.isEmpty()) {
            int breakPos = desc.indexOf(QLatin1Char('('));
            output.append(QLatin1String(": ")).append(desc.left(breakPos));
            if (selected)
                output.append(QLatin1Char('\n')).append(desc.mid(breakPos));
        }
        break;
    default:
        output = desc;
        if (!selected)
            output = output.split(QLatin1Char('\n')).first();
    }
    return output;
}

void TestResultDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);
    // make sure we paint the complete delegate instead of keeping an offset
    opt.rect.adjust(-opt.rect.x(), 0, 0, 0);
    painter->save();

    QFontMetrics fm(opt.font);
    QColor foreground;

    const QAbstractItemView *view = qobject_cast<const QAbstractItemView *>(opt.widget);
    const bool selected = view->selectionModel()->currentIndex() == index;

    if (selected) {
        painter->setBrush(opt.palette.highlight().color());
        foreground = opt.palette.highlightedText().color();
    } else {
        painter->setBrush(opt.palette.background().color());
        foreground = opt.palette.text().color();
    }
    painter->setPen(Qt::NoPen);
    painter->drawRect(opt.rect);
    painter->setPen(foreground);

    TestResultFilterModel *resultFilterModel = static_cast<TestResultFilterModel *>(view->model());
    LayoutPositions positions(opt, resultFilterModel);
    const TestResult &testResult = resultFilterModel->testResult(index);

    // draw the indicator by ourself as we paint across it with the delegate
    QStyleOptionViewItem indicatorOpt = option;
    indicatorOpt.rect = QRect(0, opt.rect.y(), positions.indentation(), opt.rect.height());
    opt.widget->style()->drawPrimitive(QStyle::PE_IndicatorBranch, &indicatorOpt, painter);

    QIcon icon = index.data(Qt::DecorationRole).value<QIcon>();
    if (!icon.isNull())
        painter->drawPixmap(positions.left(), positions.top(),
                            icon.pixmap(positions.iconSize(), positions.iconSize()));

    QString typeStr = TestResult::resultToString(testResult.result());
    if (selected) {
        painter->drawText(positions.typeAreaLeft(), positions.top() + fm.ascent(), typeStr);
    } else {
        QPen tmp = painter->pen();
        painter->setPen(TestResult::colorForType(testResult.result()));
        painter->drawText(positions.typeAreaLeft(), positions.top() + fm.ascent(), typeStr);
        painter->setPen(tmp);
    }

    QString output = outputString(testResult, selected);

    if (selected) {
        output.replace(QLatin1Char('\n'), QChar::LineSeparator);

        if (AutotestPlugin::instance()->settings()->limitResultOutput
                && output.length() > outputLimit)
            output = output.left(outputLimit).append(QLatin1String("..."));

        recalculateTextLayout(index, output, painter->font(), positions.textAreaWidth());

        m_lastCalculatedLayout.draw(painter, QPoint(positions.textAreaLeft(), positions.top()));
    } else {
        painter->setClipRect(positions.textArea());
        // cut output before generating elided text as this takes quite long for exhaustive output
        painter->drawText(positions.textAreaLeft(), positions.top() + fm.ascent(),
                          fm.elidedText(output.left(2000), Qt::ElideRight, positions.textAreaWidth()));
    }

    QString file = testResult.fileName();
    const int pos = file.lastIndexOf(QLatin1Char('/'));
    if (pos != -1)
        file = file.mid(pos + 1);

    painter->setClipRect(positions.fileArea());
    painter->drawText(positions.fileAreaLeft(), positions.top() + fm.ascent(), file);


    if (testResult.line()) {
        QString line = QString::number(testResult.line());
        painter->setClipRect(positions.lineArea());
        painter->drawText(positions.lineAreaLeft(), positions.top() + fm.ascent(), line);
    }

    painter->setClipRect(opt.rect);
    painter->setPen(opt.palette.midlight().color());
    painter->drawLine(0, opt.rect.bottom(), opt.rect.right(), opt.rect.bottom());
    painter->restore();
}

QSize TestResultDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem opt = option;
    // make sure opt.rect is initialized correctly - otherwise we might get a width of 0
    opt.initFrom(opt.widget);
    initStyleOption(&opt, index);

    const QAbstractItemView *view = qobject_cast<const QAbstractItemView *>(opt.widget);
    const bool selected = view->selectionModel()->currentIndex() == index;

    QFontMetrics fm(opt.font);
    int fontHeight = fm.height();
    TestResultFilterModel *resultFilterModel = static_cast<TestResultFilterModel *>(view->model());
    LayoutPositions positions(opt, resultFilterModel);
    QSize s;
    s.setWidth(opt.rect.width());

    if (selected) {
        const TestResult &testResult = resultFilterModel->testResult(index);

        QString output = outputString(testResult, selected);
        output.replace(QLatin1Char('\n'), QChar::LineSeparator);

        if (AutotestPlugin::instance()->settings()->limitResultOutput
                && output.length() > outputLimit)
            output = output.left(outputLimit).append(QLatin1String("..."));

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
