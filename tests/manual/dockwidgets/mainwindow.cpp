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

#include "qdockarrows.h"
#include "mainwindow.h"

#include <QtWidgets>

class TestTool : public QWidget
{
    Q_OBJECT

public:
    TestTool(QWidget *parent = 0) : QWidget(parent) {
        m_widget = new QLabel("Test", this);
        qDebug() << m_tmp.width() << m_tmp.height();
    }

    QSize sizeHint() const
    {
        return QSize(20, 100);
    }

protected:
    void paintEvent(QPaintEvent *event)
    {
        m_widget->setVisible(false);

        QStyleOptionTabV2 opt;
        opt.initFrom(this);
        opt.shape = QTabBar::RoundedWest;
        opt.text = tr("Hello");

        QStylePainter p(this);
        p.drawControl(QStyle::CE_TabBarTab, opt);

        m_tmp = QPixmap::grabWidget(m_widget, 10, 0, 10, 10);

        QPainter pn(this);
        pn.drawPixmap(0,0,m_tmp);

        QWidget::paintEvent(event);
    }

private:
    QPixmap m_tmp;
    QLabel *m_widget;
};

MainWindow::MainWindow()
{
    centralWidget = new QLabel(tr("Central Widget"));
    setCentralWidget(centralWidget);

    QToolBar *tb = this->addToolBar("Normal Toolbar");
    tb->addAction("Test");

    tb = this->addToolBar("Test Toolbar");
    tb->setMovable(false);
    tb->addWidget(new TestTool(this));
    this->addToolBar(Qt::RightToolBarArea, tb);

    createDockWindows();

    setWindowTitle(tr("Dock Widgets"));
}

void MainWindow::createDockWindows()
{
    QDockArrowManager *manager = new QDockArrowManager(this);

    for (int i=0; i<5; ++i) {
        QArrowManagedDockWidget *dock = new QArrowManagedDockWidget(manager);
        QLabel *label = new QLabel(tr("Widget %1").arg(i), dock);
        label->setWindowTitle(tr("Widget %1").arg(i));
        label->setObjectName(tr("widget_%1").arg(i));
        dock->setObjectName(tr("dock_%1").arg(i));
        dock->setWidget(label);
        addDockWidget(Qt::RightDockWidgetArea, dock);
    }
}

#include "mainwindow.moc"
