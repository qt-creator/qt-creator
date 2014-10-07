/****************************************************************************
**
** Copyright (C) 2014 Digia Plc
** All rights reserved.
** For any questions to Digia, please use contact form at http://qt.digia.com
**
** This file is part of the Qt Creator Enterprise Auto Test Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.
**
** If you have questions regarding the use of this file, please use
** contact form at http://qt.digia.com
**
****************************************************************************/

#include "testresultdelegate.h"
#include "testresultmodel.h"

#include <QAbstractItemView>
#include <QDebug>
#include <QPainter>
#include <QTextLayout>

namespace Autotest {
namespace Internal {

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
    TestResultModel *resultModel = static_cast<TestResultModel *>(view->model());
    LayoutPositions positions(opt, resultModel);
    TestResult testResult = resultModel->testResult(index);
    ResultType type = testResult.result();

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

    QString output;
    switch (type) {
    case ResultType::PASS:
    case ResultType::FAIL:
    case ResultType::EXPECTED_FAIL:
    case ResultType::UNEXPECTED_PASS:
        output = testResult.className() + QLatin1String("::") + testResult.testCase();
        if (!testResult.dataTag().isEmpty())
            output.append(QString::fromLatin1(" (%1)").arg(testResult.dataTag()));
        if (selected && !testResult.description().isEmpty()) {
            output.append(QLatin1Char('\n'));
            output.append(testResult.description());
        }
        break;
    default:
        output = testResult.description();
        if (!selected)
            output = output.split(QLatin1Char('\n')).first();
    }

    QColor mix;
    mix.setRgb( static_cast<int>(0.7 * foreground.red()   + 0.3 * background.red()),
                static_cast<int>(0.7 * foreground.green() + 0.3 * background.green()),
                static_cast<int>(0.7 * foreground.blue()  + 0.3 * background.blue()));

    if (selected) {
        int height = 0;
        int leading = fm.leading();
        int firstLineBreak = output.indexOf(QLatin1Char('\n'));
        output.replace(QLatin1Char('\n'), QChar::LineSeparator);
        QTextLayout tl(output);
        if (firstLineBreak != -1) {
            QTextLayout::FormatRange fr;
            fr.start = firstLineBreak;
            fr.length = output.length() - firstLineBreak;
            fr.format.setFontStyleHint(QFont::Monospace);
            fr.format.setForeground(mix);
            tl.setAdditionalFormats(QList<QTextLayout::FormatRange>() << fr);
        }
        tl.beginLayout();
        while (true) {
            QTextLine tLine = tl.createLine();
            if (!tLine.isValid())
                break;
            tLine.setLineWidth(positions.textAreaWidth());
            height += leading;
            tLine.setPosition(QPoint(0, height));
            height += fm.ascent() + fm.descent();
        }
        tl.endLayout();
        tl.draw(painter, QPoint(positions.textAreaLeft(), positions.top()));

        painter->setPen(mix);

        int bottomLine = positions.top() + fm.ascent() + height + leading;
        painter->drawText(positions.textAreaLeft(), bottomLine, testResult.fileName());
    } else {
        painter->setClipRect(positions.textArea());
        painter->drawText(positions.textAreaLeft(), positions.top() + fm.ascent(), output);
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
    initStyleOption(&opt, index);

    const QAbstractItemView *view = qobject_cast<const QAbstractItemView *>(opt.widget);
    const bool selected = view->selectionModel()->currentIndex() == index;

    QFontMetrics fm(opt.font);
    int fontHeight = fm.height();
    TestResultModel *resultModel = static_cast<TestResultModel *>(view->model());
    LayoutPositions positions(opt, resultModel);
    QSize s;
    s.setWidth(opt.rect.width());

    if (selected) {
        TestResult testResult = resultModel->testResult(index);

        QString output;
        switch (testResult.result()) {
        case ResultType::PASS:
        case ResultType::FAIL:
        case ResultType::EXPECTED_FAIL:
        case ResultType::UNEXPECTED_PASS:
            output = testResult.className() + QLatin1String("::") + testResult.testCase();
            if (!testResult.dataTag().isEmpty())
                output.append(QString::fromLatin1(" (%1)").arg(testResult.dataTag()));
            if (!testResult.description().isEmpty()) {
                output.append(QLatin1Char('\n'));
                output.append(testResult.description());
            }
            break;
        default:
            output = testResult.description();
        }

        output.replace(QLatin1Char('\n'), QChar::LineSeparator);

        int height = 0;
        int leading = fm.leading();
        QTextLayout tl(output);
        tl.beginLayout();
        while (true) {
            QTextLine line = tl.createLine();
            if (!line.isValid())
                break;
            height += leading;
            line.setPosition(QPoint(0, height));
            height += fm.ascent() + fm.descent();
        }
        tl.endLayout();

        s.setHeight(height + leading + 3 + (testResult.fileName().isEmpty() ? 0 : fontHeight));
    } else {
        s.setHeight(fontHeight + 3);
    }

    if (s.height() < positions.minimumHeight())
        s.setHeight(positions.minimumHeight());

    return s;
}

void TestResultDelegate::emitSizeHintChanged(const QModelIndex &index)
{
    emit sizeHintChanged(index);
}

void TestResultDelegate::currentChanged(const QModelIndex &current, const QModelIndex &previous)
{
    emit sizeHintChanged(current);
    emit sizeHintChanged(previous);
}

} // namespace Internal
} // namespace Autotest
