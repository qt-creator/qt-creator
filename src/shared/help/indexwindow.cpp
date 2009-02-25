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

#include "indexwindow.h"
#include "centralwidget.h"
#include "topicchooser.h"

#include <QtGui/QLayout>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QKeyEvent>
#include <QtGui/QMenu>
#include <QtGui/QContextMenuEvent>
#include <QtGui/QListWidgetItem>

#include <QtHelp/QHelpEngine>
#include <QtHelp/QHelpIndexWidget>

QT_BEGIN_NAMESPACE

IndexWindow::IndexWindow(QHelpEngine *helpEngine, QWidget *parent)
    : QWidget(parent)
    , m_searchLineEdit(0)
    , m_indexWidget(0)
    , m_helpEngine(helpEngine)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    QLabel *l = new QLabel(tr("&Look for:"));
    layout->addWidget(l);

    m_searchLineEdit = new QLineEdit();
    l->setBuddy(m_searchLineEdit);
    connect(m_searchLineEdit, SIGNAL(textChanged(QString)), this,
        SLOT(filterIndices(QString)));
    m_searchLineEdit->installEventFilter(this);
    layout->setMargin(4);
    layout->addWidget(m_searchLineEdit);

    m_indexWidget = m_helpEngine->indexWidget();
    m_indexWidget->installEventFilter(this);
    connect(m_helpEngine->indexModel(), SIGNAL(indexCreationStarted()), this,
        SLOT(disableSearchLineEdit()));
    connect(m_helpEngine->indexModel(), SIGNAL(indexCreated()), this,
        SLOT(enableSearchLineEdit()));
    connect(m_indexWidget, SIGNAL(linkActivated(QUrl, QString)), this,
        SIGNAL(linkActivated(QUrl)));
    connect(m_indexWidget, SIGNAL(linksActivated(QMap<QString, QUrl>, QString)),
        this, SIGNAL(linksActivated(QMap<QString, QUrl>, QString)));
    connect(m_searchLineEdit, SIGNAL(returnPressed()), m_indexWidget,
        SLOT(activateCurrentItem()));
    layout->addWidget(m_indexWidget);

    m_indexWidget->viewport()->installEventFilter(this);
}

IndexWindow::~IndexWindow()
{
}

void IndexWindow::filterIndices(const QString &filter)
{
    if (filter.contains(QLatin1Char('*')))
        m_indexWidget->filterIndices(filter, filter);
    else
        m_indexWidget->filterIndices(filter, QString());
}

bool IndexWindow::eventFilter(QObject *obj, QEvent *e)
{
    if (obj == m_searchLineEdit && e->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent*>(e);
        QModelIndex idx = m_indexWidget->currentIndex();
        switch (ke->key()) {
        case Qt::Key_Up:
            idx = m_indexWidget->model()->index(idx.row()-1,
                idx.column(), idx.parent());
            if (idx.isValid())
                m_indexWidget->setCurrentIndex(idx);
            break;
        case Qt::Key_Down:
            idx = m_indexWidget->model()->index(idx.row()+1,
                idx.column(), idx.parent());
            if (idx.isValid())
                m_indexWidget->setCurrentIndex(idx);
            break;
        case Qt::Key_Escape:
            emit escapePressed();            
            break;
        default:
            ;
        }
    } else if (obj == m_indexWidget && e->type() == QEvent::ContextMenu) {
        QContextMenuEvent *ctxtEvent = static_cast<QContextMenuEvent*>(e);
        QModelIndex idx = m_indexWidget->indexAt(ctxtEvent->pos());
        if (idx.isValid()) {
            QMenu menu;
            QAction *curTab = menu.addAction(tr("Open Link"));
            QAction *newTab = menu.addAction(tr("Open Link in New Tab"));
            menu.move(m_indexWidget->mapToGlobal(ctxtEvent->pos()));

            QAction *action = menu.exec();
            if (curTab == action)
                m_indexWidget->activateCurrentItem();
            else if (newTab == action) {
                QHelpIndexModel *model =
                    qobject_cast<QHelpIndexModel*>(m_indexWidget->model());
                QString keyword = model->data(idx, Qt::DisplayRole).toString();
                if (model) {
                    QMap<QString, QUrl> links = model->linksForKeyword(keyword);
                    if (links.count() == 1) {
                        CentralWidget::instance()->
                            setSourceInNewTab(links.constBegin().value());
                    } else {
                        TopicChooser tc(this, keyword, links);
                        if (tc.exec() == QDialog::Accepted) {
                            CentralWidget::instance()->setSourceInNewTab(tc.link());
                        }
                    }
                }
            }
        }
    } else if (m_indexWidget && obj == m_indexWidget->viewport()
        && e->type() == QEvent::MouseButtonRelease) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(e);
        QModelIndex idx = m_indexWidget->indexAt(mouseEvent->pos());
        if (idx.isValid() && mouseEvent->button()==Qt::MidButton) {
            QHelpIndexModel *model =
                qobject_cast<QHelpIndexModel*>(m_indexWidget->model());
            QString keyword = model->data(idx, Qt::DisplayRole).toString();
            if (model) {
                QMap<QString, QUrl> links = model->linksForKeyword(keyword);
                if (links.count() > 1) {
                    TopicChooser tc(this, keyword, links);
                    if (tc.exec() == QDialog::Accepted) {
                        CentralWidget::instance()->setSourceInNewTab(tc.link());
                    }
                } else if (links.count() == 1) {
                    CentralWidget::instance()->
                        setSourceInNewTab(links.constBegin().value());
                }
            }
        }
    }
#ifdef Q_OS_MAC
    else if (obj == m_indexWidget && e->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent*>(e);
        if (ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter)
           m_indexWidget->activateCurrentItem();
    }
#endif
    return QWidget::eventFilter(obj, e);
}

void IndexWindow::enableSearchLineEdit()
{
    m_searchLineEdit->setDisabled(false);
    filterIndices(m_searchLineEdit->text());
}

void IndexWindow::disableSearchLineEdit()
{
    m_searchLineEdit->setDisabled(true);
}

void IndexWindow::setSearchLineEditText(const QString &text)
{
    m_searchLineEdit->setText(text);
}

void IndexWindow::focusInEvent(QFocusEvent *e)
{
    if (e->reason() != Qt::MouseFocusReason) {
        m_searchLineEdit->selectAll();
        m_searchLineEdit->setFocus();
    }
}

QT_END_NAMESPACE
