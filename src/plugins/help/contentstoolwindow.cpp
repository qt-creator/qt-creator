/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "contentstoolwindow.h"
#include "helpengine.h"

#include <QtCore/QDebug>
#include <QtCore/QStack>
#include <QtGui/QFocusEvent>
#include <QtGui/QKeyEvent>

using namespace Help::Internal;

ContentsToolWidget::ContentsToolWidget()
{
    wasInitialized = false;

    setRootIsDecorated(true);
    setItemHidden(headerItem(), true);
    setUniformRowHeights(true);
    setColumnCount(1);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
   
    setWindowTitle(tr("Contents"));
    setWindowIcon(QIcon(":/help/images/book.png"));
}

void ContentsToolWidget::focusInEvent(QFocusEvent *e)
{
    if (wasInitialized) {
        if (e && e->reason() != Qt::MouseFocusReason
            && !currentItem() && topLevelItemCount())
            setCurrentItem(topLevelItem(0));
        return;
    }
    wasInitialized = true;
    setCursor(QCursor(Qt::WaitCursor));
    emit buildRequested();
}

void ContentsToolWidget::showEvent(QShowEvent *)
{
    if (wasInitialized)
        return;
    wasInitialized = true;
    setCursor(QCursor(Qt::WaitCursor));
    emit buildRequested();
}

void ContentsToolWidget::keyPressEvent(QKeyEvent *e)
{
    if (e && e->key() == Qt::Key_Escape) {
        emit escapePressed();
        e->accept();
        return;
    }
    QTreeWidget::keyPressEvent(e);
}


enum
{
    LinkRole = Qt::UserRole + 1000
};

ContentsToolWindow::ContentsToolWindow(const QList<int> &context, HelpEngine *help)
{
    m_widget = new ContentsToolWidget;
    helpEngine = help;
    connect(helpEngine, SIGNAL(contentsInitialized()), this, SLOT(contentsDone()));
    connect(m_widget, SIGNAL(buildRequested()), helpEngine, SLOT(buildContents()));

    m_context = context;
    m_context << 0;

    connect(m_widget, SIGNAL(itemActivated(QTreeWidgetItem*,int)), this, SLOT(indexRequested()));
    connect(m_widget, SIGNAL(escapePressed()), this, SIGNAL(escapePressed()));
}

ContentsToolWindow::~ContentsToolWindow()
{
    delete m_widget;
}

const QList<int> &ContentsToolWindow::context() const
{
    return m_context;
}

QWidget *ContentsToolWindow::widget()
{
    return m_widget;
}

void ContentsToolWindow::contentsDone()
{    
    m_widget->setCursor(QCursor(Qt::WaitCursor));
    QList<QPair<QString, ContentList> > contentList = helpEngine->contents();
    for (QList<QPair<QString, ContentList> >::Iterator it = contentList.begin(); it != contentList.end(); ++it) {
        QTreeWidgetItem *newEntry;
        QTreeWidgetItem *contentEntry;
        QStack<QTreeWidgetItem*> stack;
        stack.clear();
        int depth = 0;
        bool root = false;

        QTreeWidgetItem *lastItem[64];
        for (int j = 0; j < 64; ++j)
            lastItem[j] = 0;

        ContentList lst = (*it).second;
        for (ContentList::ConstIterator it = lst.begin(); it != lst.end(); ++it) {
            ContentItem item = *it;
            if (item.depth == 0) {
                newEntry = new QTreeWidgetItem(m_widget, 0);
                newEntry->setIcon(0, QIcon(QString::fromUtf8(":/help/images/book.png")));
                newEntry->setText(0, item.title);
                newEntry->setData(0, LinkRole, item.reference);
                stack.push(newEntry);
                depth = 1;
                root = true;
            } else {
                if (item.depth > depth && root) {
                    depth = item.depth;
                    stack.push(contentEntry);
                }
                if (item.depth == depth) {
                    contentEntry = new QTreeWidgetItem(stack.top(), lastItem[ depth ]);
                    lastItem[ depth ] = contentEntry;
                    contentEntry->setText(0, item.title);
                    contentEntry->setData(0, LinkRole, item.reference);
                }
                else if (item.depth < depth) {
                    stack.pop();
                    depth--;
                    item = *(--it);
                }
            }
        }        
    }
    m_widget->setCursor(QCursor(Qt::ArrowCursor));
}

void ContentsToolWindow::indexRequested()
{
    QTreeWidgetItem *itm = m_widget->currentItem();
    if (!itm)
        return;
    emit showLinkRequested(itm->data(0, LinkRole).toString(), false);
}
