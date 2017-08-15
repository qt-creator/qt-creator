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

#include "qmlprofilertextmark.h"

#include "qmlprofilerconstants.h"
#include "qmlprofilerviewmanager.h"
#include "qmlprofilerstatisticsview.h"

#include <QLabel>
#include <QLayout>
#include <QPainter>

namespace QmlProfiler {
namespace Internal {

QmlProfilerTextMark::QmlProfilerTextMark(QmlProfilerViewManager *viewManager, int typeId,
                                         const QString &fileName, int lineNumber) :
    TextMark(fileName, lineNumber, Constants::TEXT_MARK_CATEGORY, 3.5), m_viewManager(viewManager),
    m_typeIds(1, typeId)
{
}

void QmlProfilerTextMark::addTypeId(int typeId)
{
    m_typeIds.append(typeId);
}

void QmlProfilerTextMark::paintIcon(QPainter *painter, const QRect &paintRect) const
{
    painter->save();
    painter->setPen(Qt::black);
    painter->fillRect(paintRect, Qt::white);
    painter->drawRect(paintRect);
    painter->drawText(paintRect, m_viewManager->statisticsView()->summary(m_typeIds),
                      Qt::AlignRight | Qt::AlignVCenter);
    painter->restore();
}

void QmlProfilerTextMark::clicked()
{
    int typeId = m_typeIds.takeFirst();
    m_typeIds.append(typeId);
    m_viewManager->typeSelected(typeId);
}

QmlProfilerTextMarkModel::QmlProfilerTextMarkModel(QObject *parent) : QObject(parent)
{
}

QmlProfilerTextMarkModel::~QmlProfilerTextMarkModel()
{
    qDeleteAll(m_marks);
}

void QmlProfilerTextMarkModel::clear()
{
    qDeleteAll(m_marks);
    m_marks.clear();
    m_ids.clear();
}

void QmlProfilerTextMarkModel::addTextMarkId(int typeId, const QmlEventLocation &location)
{
    m_ids.insert(location.filename(), {typeId, location.line(), location.column()});
}

void QmlProfilerTextMarkModel::createMarks(QmlProfilerViewManager *viewManager,
                                           const QString &fileName)
{
    auto first = m_ids.find(fileName);
    QVarLengthArray<TextMarkId> ids;

    for (auto it = first; it != m_ids.end() && it.key() == fileName;) {
        ids.append({it->typeId, it->lineNumber > 0 ? it->lineNumber : 1, it->columnNumber});
        it = m_ids.erase(it);
    }

    std::sort(ids.begin(), ids.end(), [](const TextMarkId &a, const TextMarkId &b) {
        return (a.lineNumber == b.lineNumber) ? (a.columnNumber < b.columnNumber)
                                              : (a.lineNumber < b.lineNumber);
    });

    int lineNumber = -1;
    for (auto it = ids.begin(), end = ids.end(); it != end; ++it) {
        if (it->lineNumber == lineNumber) {
            m_marks.last()->addTypeId(it->typeId);
        } else {
            lineNumber = it->lineNumber;
            m_marks << new QmlProfilerTextMark(viewManager, it->typeId, fileName, it->lineNumber);
        }
    }
}

bool QmlProfilerTextMark::addToolTipContent(QLayout *target) const
{
    QGridLayout *layout = new QGridLayout;
    layout->setHorizontalSpacing(10);
    for (int row = 0, rowEnd = m_typeIds.length(); row != rowEnd; ++row) {
        const QStringList typeDetails = m_viewManager->statisticsView()->details(m_typeIds[row]);
        for (int column = 0, columnEnd = typeDetails.length(); column != columnEnd; ++column) {
            QLabel *label = new QLabel;
            label->setAlignment(column == columnEnd - 1 ? Qt::AlignRight : Qt::AlignLeft);
            label->setTextFormat(Qt::PlainText);
            label->setText(typeDetails[column]);
            layout->addWidget(label, row, column);
        }
    }

    target->addItem(layout);
    return true;
}

} // namespace Internal
} // namespace QmlProfiler


