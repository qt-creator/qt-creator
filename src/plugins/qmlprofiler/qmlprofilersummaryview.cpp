/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "qmlprofilersummaryview.h"

#include <QtCore/QUrl>
#include <QtCore/QHash>

#include <QtGui/QHeaderView>
#include <QtGui/QStandardItemModel>

using namespace QmlProfiler::Internal;

struct BindingData {
    QString displayname;
    QString filename;
    int line;
    qint64 duration;
    qint64 calls;
    qint64 minTime;
    qint64 maxTime;
    double tpc;
    double percent;
};

class QmlProfilerSummaryView::QmlProfilerSummaryViewPrivate
{
public:
    QmlProfilerSummaryViewPrivate(QmlProfilerSummaryView *qq):q(qq) {}
    ~QmlProfilerSummaryViewPrivate() {}

    QmlProfilerSummaryView *q;

    QStandardItemModel *m_model;
    QHash<QString, BindingData *> m_bindingHash;

    enum RangeType {
        Painting,
        Compiling,
        Creating,
        Binding,
        HandlingSignal,

        MaximumRangeType
    };
};

class ProfilerItem : public QStandardItem
{
public:
    ProfilerItem(const QString &text):QStandardItem ( text ) {}

    virtual bool operator< ( const QStandardItem & other ) const
    {
        if (data().type() == QVariant::String) {
            // first column
            return data(Qt::UserRole+2).toString() == other.data(Qt::UserRole+2).toString() ?
                        data(Qt::UserRole+3).toInt() < other.data(Qt::UserRole+3).toInt() :
                        data(Qt::UserRole+2).toString() < other.data(Qt::UserRole+2).toString();
        }

        return data().toDouble() < other.data().toDouble();
    }
};

QmlProfilerSummaryView::QmlProfilerSummaryView(QWidget *parent) :
    QTreeView(parent), d(new QmlProfilerSummaryViewPrivate(this))
{
    setRootIsDecorated(false);
    header()->setResizeMode(QHeaderView::Interactive);
    header()->setMinimumSectionSize(100);
    setSortingEnabled(false);

    d->m_model = new QStandardItemModel(this);

    setModel(d->m_model);

    d->m_model->setColumnCount(7);
    setHeaderLabels();

    connect(this,SIGNAL(clicked(QModelIndex)), this,SLOT(jumpToItem(QModelIndex)));
}

QmlProfilerSummaryView::~QmlProfilerSummaryView()
{
    delete d->m_model;
}

void QmlProfilerSummaryView::clean()
{
    d->m_model->clear();
    d->m_model->setColumnCount(7);

    // clean the hash
    QHashIterator<QString, BindingData *>it(d->m_bindingHash);
    while (it.hasNext()) {
        it.next();
        delete it.value();
    }
    d->m_bindingHash.clear();

    setHeaderLabels();
    setSortingEnabled(false);
}

void QmlProfilerSummaryView::addRangedEvent(int type, qint64 startTime, qint64 length, const QStringList &data, const QString &fileName, int line)
{
    Q_UNUSED(startTime);
    Q_UNUSED(data);

    if (type != QmlProfilerSummaryViewPrivate::Binding && type != QmlProfilerSummaryViewPrivate::HandlingSignal)
        return;

    if (fileName.isEmpty())
        return;
    const QChar colon = QLatin1Char(':');
    QString localName = QUrl(fileName).toLocalFile();
    QString displayName = localName.mid(localName.lastIndexOf(QChar('/'))+1)+colon+QString::number(line);
    QString location = fileName+colon+QString::number(line);

    QHash<QString, BindingData *>::iterator it = d->m_bindingHash.find(location);
    if (it != d->m_bindingHash.end()) {
        BindingData *bindingInfo = it.value();
        bindingInfo->duration += length;
        bindingInfo->calls++;
        if (bindingInfo->maxTime < length)
            bindingInfo->maxTime = length;
        if (bindingInfo->minTime > length)
            bindingInfo->minTime = length;
    } else {
        BindingData *newBinding = new BindingData;
        newBinding->calls = 1;
        newBinding->duration = length;
        newBinding->displayname = displayName;
        newBinding->filename = fileName;
        newBinding->line = line;
        newBinding->minTime = length;
        newBinding->maxTime = length;

        d->m_bindingHash.insert(location, newBinding);
    }
}

void QmlProfilerSummaryView::complete()
{
    // compute percentages
    double totalTime = 0;

    QHashIterator<QString, BindingData *> it(d->m_bindingHash);

    while (it.hasNext()) {
        it.next();
        totalTime += it.value()->duration;
    }

    it.toFront();

    while (it.hasNext()) {
        it.next();
        BindingData *binding = it.value();
        binding->percent = binding->duration * 100.0 / totalTime;
        binding->tpc = binding->calls>0? (double)binding->duration / binding->calls : 0;

        appendRow(binding->displayname,
                  binding->filename,
                  binding->line,
                  binding->percent,
                  binding->duration,
                  binding->calls,
                  binding->tpc,
                  binding->maxTime,
                  binding->minTime);

    }
    setSortingEnabled(true);
    sortByColumn(1,Qt::DescendingOrder);
    resizeColumnToContents(0);
}

void QmlProfilerSummaryView::jumpToItem(const QModelIndex &index)
{
    int line = d->m_model->item(index.row(),0)->data(Qt::UserRole+3).toInt();
    if (line == -1)
        return;
    QString fileName = d->m_model->item(index.row(),0)->data(Qt::UserRole+2).toString();
    emit gotoSourceLocation(fileName, line);
}

void QmlProfilerSummaryView::appendRow(const QString &displayName,
                const QString &fileName,
                int line,
                double percentTime,
                double totalTime,
                int nCalls,
                double timePerCall,
                double maxTime,
                double minTime)
{
    QString location =fileName+QLatin1Char(':')+QString::number(line);
    ProfilerItem *locationColumn = new ProfilerItem(displayName);
    locationColumn->setData(QVariant(location),Qt::UserRole+1);
    locationColumn->setData(QVariant(fileName),Qt::UserRole+2);
    locationColumn->setData(QVariant(line),Qt::UserRole+3);
    locationColumn->setEditable(false);
    ProfilerItem *percentColumn = new ProfilerItem(QString::number(percentTime,'f',2)+QLatin1String(" %"));
    percentColumn->setData(QVariant(percentTime));
    percentColumn->setEditable(false);
    ProfilerItem *timeColumn = new ProfilerItem(displayTime(totalTime));
    timeColumn->setData(QVariant(totalTime));
    timeColumn->setEditable(false);
    ProfilerItem *callsColumn = new ProfilerItem(QString::number(nCalls));
    callsColumn->setData(QVariant(nCalls));
    callsColumn->setEditable(false);
    ProfilerItem *tpcColumn = new ProfilerItem(displayTime(timePerCall));
    tpcColumn->setData(QVariant(timePerCall));
    tpcColumn->setEditable(false);
    ProfilerItem *maxTimeColumn = new ProfilerItem(displayTime(maxTime));
    maxTimeColumn->setData(QVariant(maxTime));
    maxTimeColumn->setEditable(false);
    ProfilerItem *minTimeColumn = new ProfilerItem(displayTime(minTime));
    minTimeColumn->setData(QVariant(minTime));
    minTimeColumn->setEditable(false);

    QList<QStandardItem *> newRow;
    newRow << locationColumn << percentColumn << timeColumn << callsColumn << tpcColumn << maxTimeColumn << minTimeColumn;
    d->m_model->appendRow(newRow);
}

QString QmlProfilerSummaryView::displayTime(double time) const
{
    if (time<1e6)
        return QString::number(time/1e3,'f',3) + QString::fromWCharArray(L" \u03BCs");
    if (time<1e9)
        return QString::number(time/1e6,'f',3) + QLatin1String(" ms");

    return QString::number(time/1e9,'f',3) + QLatin1String(" s");
}

void QmlProfilerSummaryView::setHeaderLabels()
{
    d->m_model->setHeaderData(0,Qt::Horizontal,QVariant(tr("location")));
    d->m_model->setHeaderData(1,Qt::Horizontal,QVariant(tr("% time")));
    d->m_model->setHeaderData(2,Qt::Horizontal,QVariant(tr("total time")));
    d->m_model->setHeaderData(3,Qt::Horizontal,QVariant(tr("calls")));
    d->m_model->setHeaderData(4,Qt::Horizontal,QVariant(tr("time per call")));
    d->m_model->setHeaderData(5,Qt::Horizontal,QVariant(tr("longest time")));
    d->m_model->setHeaderData(6,Qt::Horizontal,QVariant(tr("shortest time")));
}
