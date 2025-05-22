// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QApplication>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QPushButton>
#include <QScrollArea>
#include <QStandardItemModel>
#include <QTreeView>
#include <QTableView>
#include <QPainter>
#include <QItemDelegate>
#include <QListView>

class Delegate : public QItemDelegate
{
public:

   // QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
   //  {
   //      // const int height = index.data(Qt::SizeHintRole).value<QSize>().height();
   //      // // get text width, see QItemDelegatePrivate::displayRect
   //      // const QString text = index.data(Qt::DisplayRole).toString();
   //      // const QRect textMaxRect(0, 0, INT_MAX / 256, height);
   //      // const QRect textLayoutRect = textRectangle(nullptr, textMaxRect, info.option.font, text);
   //      // const QRect textRect(info.textRect.x(), info.textRect.y(), textLayoutRect.width(), height);
   //      // const QRect layoutRect = info.checkRect | textRect;

   //      return QSize(100, 14);
   //  }

    // void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
    // {
    //     painter->save();

    //     painter->drawText(QPoint(0, 0), index.data(Qt::DisplayRole).toString());
    //     // painter->setFont(info.option.font);

    //     // drawBackground(painter, info.option, index);
    //     // drawText(painter, info.option, info.textRect, index);
    //     // drawCheck(painter, info.option, info.checkRect, info.checkState);

    //     painter->restore();
    // }
};

#define KIND 1
#if KIND == 0
#define USE_TABLE 1
using Base = QTableView;
#elif KIND == 1
#define USE_TREE 1
using Base = QTreeView;
#elif KIND == 2
#define USE_LIST 1
using Base = QListView;
#endif

class View : public Base
{
public:
    View()
    {
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        // setItemDelegate(&m_delegate);
        // headerView()->hide();
    }

    QSize minimumSizeHint() const final
    {
        return QSize(10, 10);
    }

    // Delegate m_delegate;
};

View *createView(QAbstractItemModel *model)
{
    auto view = new View;
    view->setSizeAdjustPolicy(QAbstractItemView::SizeAdjustPolicy::AdjustToContents);
    view->setModel(model);
#if USE_TABLE
    view->verticalHeader()->hide();
    view->horizontalHeader()->hide();
#endif
#if USE_TREE
    view->header()->hide();
    view->resizeColumnToContents(0);
#endif

    return view;
}

class MainWindow : public QWidget
{
public:
    MainWindow()
    {
        for (int i = 0; i != 1; ++i)
            model1.appendRow(QList{new QStandardItem(QString("1-%1").arg(i))});

        for (int i = 0; i != 10; ++i)
            model2.appendRow(QList{new QStandardItem(QString("2-%1").arg(i))});

        for (int i = 0; i != 4; ++i)
            model3.appendRow(QList{new QStandardItem(QString("3-%1").arg(i))});

        view1 = createView(&model1);
        view2 = createView(&model2);
        view3 = createView(&model3);

        auto scrolledWidget = new QWidget;
        auto scrolledLayout = new QVBoxLayout(scrolledWidget);
        scrolledLayout->addWidget(view1);
        scrolledLayout->addWidget(view2);
        scrolledLayout->addWidget(view3);
        scrolledLayout->setSizeConstraint(QLayout::SetFixedSize);

        auto scrollArea = new QScrollArea(this);
        scrollArea->setSizeAdjustPolicy(QAbstractItemView::SizeAdjustPolicy::AdjustToContents);
        scrollArea->setWidget(scrolledWidget);

        auto button = new QPushButton("More");
        connect(button, &QPushButton::clicked, this, [this, scrollArea] {
            model2.appendRow(QList{new QStandardItem(QString("xxx"))});
#if USE_TREE
            // Doesn't have any effect?
            view1->resizeColumnToContents(0);
            view2->resizeColumnToContents(0);
            view3->resizeColumnToContents(0);
#endif
        });

        auto layout = new QVBoxLayout(this);
        layout->addWidget(button);
        layout->addWidget(scrollArea);
    }

    QStandardItemModel model1;
    QStandardItemModel model2;
    QStandardItemModel model3;

    View *view1;
    View *view2;
    View *view3;
};

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.resize(300, 500);
    w.show();
    return a.exec();
}
