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

#include "modeltest.h"

#include <QStringList>
#include <QFile>
#include <QTextStream>
#include <QDebug>

#include <QApplication>
#include <QTableView>
#include <QDesktopWidget>
#include <QVBoxLayout>
#include <QMenu>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QPushButton>
#include <QSpinBox>
#include <QComboBox>
#include <QMainWindow>
#include <QDockWidget>
#include <QToolButton>

#include <callgrindcostview.h>
#include <callgrindcostdelegate.h>
#include <callgrindvisualisation.h>

#include <valgrind/callgrind/callgrindparsedata.h>
#include <valgrind/callgrind/callgrindparser.h>
#include <valgrind/callgrind/callgrinddatamodel.h>
#include <valgrind/callgrind/callgrindfunction.h>
#include <valgrind/callgrind/callgrindcallmodel.h>
#include <valgrind/callgrind/callgrindcostitem.h>
#include <valgrind/callgrind/callgrindfunctioncall.h>
#include <valgrind/callgrind/callgrindproxymodel.h>


using namespace Valgrind::Callgrind;
using namespace Callgrind::Internal;

QTextStream qerr(stderr);

void usage() {
    qerr << "modeltest CALLGRINDFILE ..." << endl;
}

ModelTestWidget::ModelTestWidget(CallgrindWidgetHandler *handler)
    : m_format(0)
    , m_event(0)
    , m_handler(handler)
{
    QVBoxLayout *l = new QVBoxLayout;
    setLayout(l);

    QHBoxLayout *h = new QHBoxLayout;
    l->addLayout(h);
    l->addWidget(handler->flatView());

    m_handler->populateActions(h);

    m_format = new QComboBox;
    m_format->addItem("absolute", CostDelegate::FormatAbsolute);
    m_format->addItem("relative", CostDelegate::FormatRelative);
    m_format->addItem("rel. to parent", CostDelegate::FormatRelativeToParent);
    connect(m_format, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated),
            this, &ModelTestWidget::formatChanged);
    h->addWidget(m_format);

    QDoubleSpinBox *minimumCost = new QDoubleSpinBox;
    minimumCost->setToolTip("Minimum cost");
    minimumCost->setRange(0, 1);
    minimumCost->setDecimals(4);
    minimumCost->setSingleStep(0.01);
    connect(minimumCost, &QDoubleSpinBox::valueChanged,
            m_handler->proxyModel(), &DataProxyModel::setMinimumInclusiveCostRatio);
    minimumCost->setValue(0.0001);
    h->addWidget(minimumCost);

    m_handler->flatView()->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_handler->flatView(), SIGNAL(customContextMenuRequested(QPoint)),
            this, SLOT(showViewContextMenu(QPoint)));

    resize(qMin(qApp->desktop()->width(), 1024), 600);
}

ModelTestWidget::~ModelTestWidget()
{
}

void ModelTestWidget::formatChanged(int index)
{
    CostDelegate::CostFormat format = static_cast<CostDelegate::CostFormat>(m_format->itemData(index).toInt());
    m_handler->setCostFormat(format);
}

void ModelTestWidget::showViewContextMenu(const QPoint &pos)
{
    const QModelIndex idx = m_handler->flatView()->indexAt(pos);
    if (!idx.isValid())
        return;

    QMenu menu;
    QAction *showCostItems = menu.addAction("show cost items");

    const ParseData* data = m_handler->dataModel()->parseData();

    QAction *ret = menu.exec(m_handler->flatView()->mapToGlobal(pos));
    if (ret == showCostItems) {
        ///TODO: put this into a reusable class?
        const Function *func = idx.data(DataModel::FunctionRole).value<const Function *>();
        Q_ASSERT(func);

        QTableView *view = new QTableView();
        view->setAttribute(Qt::WA_DeleteOnClose, true);
        const int rows = func->costItems().size();
        const int columns = data->events().size() + data->positions().size() + 2;
        QStandardItemModel *model = new QStandardItemModel(rows, columns, view);
        int headerColumn = 0;
        foreach (const QString &event, data->events()) {
            model->setHeaderData(headerColumn++, Qt::Horizontal, event);
        }
        const int lastEventColumn = headerColumn;
        foreach (const QString &pos, data->positions()) {
            model->setHeaderData(headerColumn++, Qt::Horizontal, pos);
        }
        const int lastPosColumn = headerColumn;
        model->setHeaderData(headerColumn++, Qt::Horizontal, "Call");
        model->setHeaderData(headerColumn++, Qt::Horizontal, "Differring File");
        Q_ASSERT(headerColumn == columns);

        QVector<quint64> totalCosts;
        totalCosts.fill(0, data->events().size());
        for (int row = 0; row < rows; ++row) {
            const CostItem *item = func->costItems().at(row);
            for (int column = 0; column < columns; ++column) {
                QVariant value;
                if (column < lastEventColumn) {
                    value = item->cost(column);
                    totalCosts[column] += item->cost(column);
                } else if (column < lastPosColumn) {
                    value = item->position(column - lastEventColumn);
                } else if (column == lastPosColumn) {
                    if (item->call())
                        value = item->call()->callee()->name();
                } else {
                    value = item->differingFile();
                }
                model->setData(model->index(row, column), value);
            }
        }
        QStringList totalCostsStrings;
        for (int i = 0; i < totalCosts.size(); ++i) {
            totalCostsStrings << QString("%1: %2").arg(totalCosts.at(i)).arg(data->events().at(i));
        }
        view->setWindowTitle(totalCostsStrings.join(QLatin1String(", ")));
        view->setModel(model);
        view->show();
    }
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    if (app.arguments().count() < 2) {
        usage();
        return 1;
    }

    ///TODO: multi-part callgrind files
    QFile file(app.arguments().at(1));
    if (!file.open(QIODevice::ReadOnly)) {
        qerr << "could not open callgrind file for reading:" << file.fileName() << file.errorString();
        return 3;
    }

    Parser p;
    p.parse(&file);

    ParseData *data = p.takeData();
    if (!data) {
        qerr << "invalid callgrind file:" << file.fileName() << endl;
        return 2;
    }

    QMainWindow window;

    CallgrindWidgetHandler *handler = new CallgrindWidgetHandler(&window);

    ModelTestWidget *widget = new ModelTestWidget(handler);
    widget->setWindowTitle(file.fileName());
    window.setCentralWidget(widget);

    QDockWidget *callerDock = new QDockWidget("callers", &window);
    callerDock->setWidget(handler->callersView());
    window.addDockWidget(Qt::RightDockWidgetArea, callerDock);

    QDockWidget *calleeDock = new QDockWidget("callees", &window);
    calleeDock->setWidget(handler->calleesView());
    window.addDockWidget(Qt::RightDockWidgetArea, calleeDock);

    QDockWidget *widgetDock = new QDockWidget("vizualisation", &window);
    widgetDock->setWidget(handler->visualisation());
    window.addDockWidget(Qt::BottomDockWidgetArea, widgetDock);

    handler->setParseData(data);

    window.show();
    return app.exec();
}
