/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/
/****************************************************************************
**
** Copyright (C) 2005-$THISYEAR$ Trolltech AS. All rights reserved.
**
** This file is part of the $MODULE$ of the Qt Toolkit.
**
** $LICENSE$
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include <QtGui>

#include "qdockarrows.h"
#include "mainwindow.h"

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
#include "mainwindow.moc"

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
