// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlprofilertextmark.h"

#include "qmlprofilerconstants.h"
#include "qmlprofilerviewmanager.h"
#include "qmlprofilerstatisticsview.h"
#include "qmlprofilertr.h"

#include <QLabel>
#include <QLayout>
#include <QPainter>

using namespace Utils;

namespace QmlProfiler {
namespace Internal {

QmlProfilerTextMark::QmlProfilerTextMark(QmlProfilerViewManager *viewManager, int typeId,
                                         const FilePath &fileName, int lineNumber)
    : TextMark(fileName, lineNumber, {Tr::tr("QML Profiler"), Constants::TEXT_MARK_CATEGORY})
    , m_viewManager(viewManager)
{
    addTypeId(typeId);
}

void QmlProfilerTextMark::addTypeId(int typeId)
{
    m_typeIds.append(typeId);

    const QmlProfilerStatisticsView *statisticsView = m_viewManager->statisticsView();
    QTC_ASSERT(statisticsView, return);

    setLineAnnotation(statisticsView->summary(m_typeIds));
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
    for (const auto &id : ids) {
        if (id.lineNumber == lineNumber) {
            m_marks.last()->addTypeId(id.typeId);
        } else {
            lineNumber = id.lineNumber;
            m_marks << new QmlProfilerTextMark(viewManager,
                                               id.typeId,
                                               FilePath::fromString(fileName),
                                               id.lineNumber);
        }
    }
}

void QmlProfilerTextMarkModel::showTextMarks()
{
    for (QmlProfilerTextMark *mark : std::as_const(m_marks))
        mark->setVisible(true);
}

void QmlProfilerTextMarkModel::hideTextMarks()
{
    for (QmlProfilerTextMark *mark : std::as_const(m_marks))
        mark->setVisible(false);
}

bool QmlProfilerTextMark::addToolTipContent(QLayout *target) const
{
    const QmlProfilerStatisticsView *statisticsView = m_viewManager->statisticsView();
    QTC_ASSERT(statisticsView, return false);

    auto layout = new QGridLayout;
    layout->setHorizontalSpacing(10);
    for (int row = 0, rowEnd = m_typeIds.length(); row != rowEnd; ++row) {
        int typeId = m_typeIds[row];
        const QStringList typeDetails = statisticsView->details(m_typeIds[row]);
        for (int column = 0, columnEnd = typeDetails.length(); column != columnEnd; ++column) {
            QLabel *label = new QLabel;
            label->setAlignment(column == columnEnd - 1 ? Qt::AlignRight : Qt::AlignLeft);
            if (column == 0) {
                label->setTextFormat(Qt::RichText);
                label->setTextInteractionFlags(Qt::LinksAccessibleByMouse
                                               | Qt::LinksAccessibleByKeyboard);
                label->setText(QString("<a href='selectType' style='text-decoration:none'>%1</a>")
                                   .arg(typeDetails[column]));
                QObject::connect(label,
                                 &QLabel::linkActivated,
                                 m_viewManager,
                                 [this, typeId]() {
                                     emit m_viewManager->typeSelected(typeId);
                                 });
            } else {
                label->setTextFormat(Qt::PlainText);
                label->setText(typeDetails[column]);
            }
            layout->addWidget(label, row, column);
        }
    }

    target->addItem(layout);
    return true;
}

} // namespace Internal
} // namespace QmlProfiler


