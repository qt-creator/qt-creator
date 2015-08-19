/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd
** All rights reserved.
** For any questions to The Qt Company, please use contact form at
** http://www.qt.io/contact-us
**
** This file is part of the Qt Creator Enterprise Auto Test Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.
**
** If you have questions regarding the use of this file, please use
** contact form at http://www.qt.io/contact-us
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

TestResultDelegate::~TestResultDelegate()
{

}

void TestResultDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItemV4 opt = option;
    initStyleOption(&opt, index);
    // make sure we paint the complete delegate instead of keeping an offset
    opt.rect.adjust(-opt.rect.x(), 0, 0, 0);
    painter->save();

    QFontMetrics fm(opt.font);
    QColor background;
    QColor foreground;

    const QAbstractItemView *view = qobject_cast<const QAbstractItemView *>(opt.widget);
    const bool selected = view->selectionModel()->currentIndex() == index;

    if (selected) {
        painter->setBrush(opt.palette.highlight().color());
        background = opt.palette.highlight().color();
        foreground = opt.palette.highlightedText().color();
    } else {
        painter->setBrush(opt.palette.background().color());
        background = opt.palette.background().color();
        foreground = opt.palette.text().color();
    }

    painter->setPen(Qt::NoPen);
    painter->drawRect(opt.rect);

    painter->setPen(foreground);
    TestResultFilterModel *resultFilterModel = static_cast<TestResultFilterModel *>(view->model());
    TestResultModel *resultModel = static_cast<TestResultModel *>(resultFilterModel->sourceModel());
    LayoutPositions positions(opt, resultModel);
    TestResult testResult = resultModel->testResult(resultFilterModel->mapToSource(index));
    Result::Type type = testResult.result();

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

    const QString desc = testResult.description();
    QString output;
    switch (type) {
    case Result::PASS:
    case Result::FAIL:
    case Result::EXPECTED_FAIL:
    case Result::UNEXPECTED_PASS:
    case Result::BLACKLISTED_FAIL:
    case Result::BLACKLISTED_PASS:
        output = testResult.className() + QLatin1String("::") + testResult.testCase();
        if (!testResult.dataTag().isEmpty())
            output.append(QString::fromLatin1(" (%1)").arg(testResult.dataTag()));
        if (selected && !desc.isEmpty()) {
            output.append(QLatin1Char('\n')).append(desc);
        }
        break;
    case Result::BENCHMARK:
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
    painter->setPen(QColor::fromRgb(150, 150, 150));
    painter->drawLine(0, opt.rect.bottom(), opt.rect.right(), opt.rect.bottom());
    painter->restore();
}

QSize TestResultDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItemV4 opt = option;
    // make sure opt.rect is initialized correctly - otherwise we might get a width of 0
    opt.initFrom(opt.widget);
    initStyleOption(&opt, index);

    const QAbstractItemView *view = qobject_cast<const QAbstractItemView *>(opt.widget);
    const bool selected = view->selectionModel()->currentIndex() == index;

    QFontMetrics fm(opt.font);
    int fontHeight = fm.height();
    TestResultFilterModel *resultFilterModel = static_cast<TestResultFilterModel *>(view->model());
    TestResultModel *resultModel = static_cast<TestResultModel *>(resultFilterModel->sourceModel());
    LayoutPositions positions(opt, resultModel);
    QSize s;
    s.setWidth(opt.rect.width());

    if (selected) {
        TestResult testResult = resultModel->testResult(resultFilterModel->mapToSource(index));

        QString desc = testResult.description();
        QString output;
        switch (testResult.result()) {
        case Result::PASS:
        case Result::FAIL:
        case Result::EXPECTED_FAIL:
        case Result::UNEXPECTED_PASS:
        case Result::BLACKLISTED_FAIL:
        case Result::BLACKLISTED_PASS:
            output = testResult.className() + QLatin1String("::") + testResult.testCase();
            if (!testResult.dataTag().isEmpty())
                output.append(QString::fromLatin1(" (%1)").arg(testResult.dataTag()));
            if (!desc.isEmpty()) {
                output.append(QLatin1Char('\n')).append(desc);
            }
            break;
        case Result::BENCHMARK:
            output = testResult.className() + QLatin1String("::") + testResult.testCase();
            if (!testResult.dataTag().isEmpty())
                output.append(QString::fromLatin1(" (%1)").arg(testResult.dataTag()));
            if (!desc.isEmpty()) {
                int breakPos = desc.indexOf(QLatin1Char('('));
                output.append(QLatin1String(" - ")).append(desc.left(breakPos));
            if (selected)
                output.append(QLatin1Char('\n')).append(desc.mid(breakPos));
            }
            break;
        default:
            output = desc;
        }

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
