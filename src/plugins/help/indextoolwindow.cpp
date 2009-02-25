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

#include "indextoolwindow.h"
#include "helpengine.h"
#include "topicchooser.h"

#include <QtCore/QDebug>
#include <QtGui/QKeyEvent>
#include <QtGui/QFocusEvent>
#include <QtGui/QLayout>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QListView>
#include <QtGui/QApplication>

using namespace Help::Internal;

IndexToolWidget::IndexToolWidget()
{
    wasInitialized = false;

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setMargin(0);

    QLabel *l = new QLabel(tr("Look for:"), this);
    layout->addWidget(l);

    findLineEdit = new QLineEdit(this);
    findLineEdit->installEventFilter(this);
    layout->addWidget(findLineEdit);

    indicesView = new QListView(this);
    indicesView->setLayoutMode(QListView::Batched);
    indicesView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    layout->addWidget(indicesView);

    setWindowTitle(tr("Index"));
    setWindowIcon(QIcon(":/help/images/find.png"));
}

void IndexToolWidget::focusInEvent(QFocusEvent *e)
{
    showEvent(0);
    if (e && e->reason() != Qt::MouseFocusReason) {
        findLineEdit->selectAll();
        findLineEdit->setFocus();
    }
}

void IndexToolWidget::showEvent(QShowEvent *)
{
    if (!wasInitialized) {
        wasInitialized = true;
        setCursor(QCursor(Qt::WaitCursor));
        emit buildRequested();
    }
}

bool IndexToolWidget::eventFilter(QObject * o, QEvent * e)
{
    if (o == findLineEdit && e->type() == QEvent::KeyPress) {
        switch (static_cast<QKeyEvent*>(e)->key()) {
            case Qt::Key_Up:
            case Qt::Key_Down:
            case Qt::Key_PageDown:
            case Qt::Key_PageUp:
                QApplication::sendEvent(indicesView, e);
                break;
            case Qt::Key_Escape:
                emit escapePressed();
                break;
            default:
                break;
        }
    }
    return QWidget::eventFilter(o, e);
}


IndexToolWindow::IndexToolWindow(const QList<int> &context, HelpEngine *help)
{
    m_context = context;
    m_context << 0;

    m_widget = new IndexToolWidget;

    helpEngine = help;
    connect(helpEngine, SIGNAL(indexInitialized()), this, SLOT(indexDone()));
    model = 0;

    connect(m_widget->findLineEdit, SIGNAL(textEdited(const QString&)),
        this, SLOT(searchInIndex(const QString &)));
    connect(m_widget->findLineEdit, SIGNAL(returnPressed()), this, SLOT(indexRequested()));    
    connect(m_widget, SIGNAL(buildRequested()), helpEngine, SLOT(buildIndex()));    
    
    connect(m_widget->indicesView, SIGNAL(activated(const QModelIndex&)),
        this, SLOT(indexRequested()));    
    connect(m_widget, SIGNAL(escapePressed()), this, SIGNAL(escapePressed()));    
    
}

IndexToolWindow::~IndexToolWindow()
{
    delete m_widget;
}

const QList<int> &IndexToolWindow::context() const
{
    return m_context;
}

QWidget *IndexToolWindow::widget()
{
    return m_widget;
}

void IndexToolWindow::indexDone()
{
    model = helpEngine->indices();
    m_widget->indicesView->setModel(model);
    m_widget->setCursor(QCursor(Qt::ArrowCursor));
}

void IndexToolWindow::searchInIndex(const QString &str)
{
    if (!model)
        return;
    QRegExp atoz("[A-Z]");
    int matches = str.count(atoz);
    if (matches > 0 && !str.contains(".*"))
    {
        int start = 0;
        QString newSearch;
        for (; matches > 0; --matches) {
            int match = str.indexOf(atoz, start+1);
            if (match <= start)
                continue;
            newSearch += str.mid(start, match-start);
            newSearch += ".*";
            start = match;
        }
        newSearch += str.mid(start);
        m_widget->indicesView->setCurrentIndex(model->filter(newSearch, str));
    }
    else
        m_widget->indicesView->setCurrentIndex(model->filter(str, str));
}

void IndexToolWindow::indexRequested()
{
    if (!model)
        return;
    int row = m_widget->indicesView->currentIndex().row();
    if (row == -1 || row >= model->rowCount())
        return;

    QString description = model->description(row);
    QStringList links = model->links(row);

    bool blocked = m_widget->findLineEdit->blockSignals(true);
    m_widget->findLineEdit->setText(description);
    m_widget->findLineEdit->blockSignals(blocked);

    if (links.count() == 1) {
        emit showLinkRequested(links.first(), false);
    } else {
        qSort(links);
        QStringList::Iterator it = links.begin();
        QStringList linkList;
        QStringList linkNames;
        for (; it != links.end(); ++it) {
            linkList << *it;
            linkNames << helpEngine->titleOfLink(*it);
        }
        QString link = TopicChooser::getLink(m_widget, linkNames, linkList, description);
        if (!link.isEmpty())
            emit showLinkRequested(link, false);
    }

    model->publish();
    m_widget->indicesView->setCurrentIndex(model->index(model->stringList().indexOf(description)));
    m_widget->indicesView->scrollTo(m_widget->indicesView->currentIndex(), QAbstractItemView::PositionAtTop);
}

