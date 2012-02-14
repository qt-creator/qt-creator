/*
 *
 *  TODO plugin - Add pane with list all TODO, FIXME, etc. comments.
 *
 *  Copyright (C) 2010  VasiliySorokin
 *
 *  Authors: Vasiliy Sorokin <sorokin.vasiliy@gmail.com>
 *
 *  This file is part of TODO plugin for QtCreator.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 * * Neither the name of the vsorokin nor the names of its contributors may be used to endorse or
 * promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY
 * WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
*/
#include "todooutputpane.h"
#include <QListWidgetItem>
#include <QRegExp>
#include <QIcon>

// TODO: make fix
// NOTE: make note
// HACK: make hack
// BUG: make bug

TodoOutputPane::TodoOutputPane(QObject *parent) : IOutputPane(parent)
{
    todoList = new QListWidget();
    todoList->setFlow(QListView::TopToBottom);
    todoList->setFrameStyle(QFrame::NoFrame);
    lastCurrentRow = 0;
}

TodoOutputPane::~TodoOutputPane()
{
    delete todoList;
}

void TodoOutputPane::addItem(const QString &text, const QString &file, const int rowNumber, const QIcon &icon, const QColor &color)
{
    QListWidgetItem *newItem = new QListWidgetItem();
    newItem->setBackgroundColor(color);
    newItem->setIcon(icon);
    newItem->setData(Qt::UserRole + 1, file);
    newItem->setData(Qt::UserRole + 2, rowNumber);
    newItem->setToolTip(file + ":" + QString::number(rowNumber));

    newItem->setText(file.right(file.size() - file.lastIndexOf("/") - 1) + ":" + QString::number(rowNumber) + ": " + text);

    todoList->addItem(newItem);
}

QListWidget *TodoOutputPane::getTodoList() const
{
    return todoList;
}


QWidget *TodoOutputPane::outputWidget(QWidget */*parent*/)
{
    return todoList;
}

QList<QWidget*> TodoOutputPane::toolBarWidgets() const
{
    return QList<QWidget*>();
}

QString TodoOutputPane::name() const
{
    return tr("TODO Output");
}

QString TodoOutputPane::displayName() const
{
    return name();
}

int TodoOutputPane::priorityInStatusBar() const
{
     return 1;
}

void TodoOutputPane::clearContents()
{
    todoList->clear();
}


void TodoOutputPane::clearContents(QString filename)
{
    int i = 0;
    lastCurrentRow = 0;
    while (i < todoList->count())
    {
        if (!filename.compare(todoList->item(i)->data(Qt::UserRole + 1).toString()))
        {
            if (lastCurrentRow == 0)
                lastCurrentRow = todoList->currentRow();
            todoList->takeItem(i);
        }
        else
        {
            ++i;
        }
    }
}


void TodoOutputPane::visibilityChanged(bool visible)
{
    todoList->setVisible(visible);
}

void TodoOutputPane::setFocus()
{
    todoList->setFocus();
}

bool TodoOutputPane::hasFocus()
{
    return todoList->hasFocus();
}

bool TodoOutputPane::canFocus()
{
    return true;
}

bool TodoOutputPane::canNavigate()
{
    return todoList->count() > 1;
}

bool TodoOutputPane::canNext()
{
    return todoList->currentRow() < todoList->count() && todoList->count() > 1;
}

bool TodoOutputPane::canPrevious()
{
    return todoList->currentRow() > 0 && todoList->count() > 1;
}

void TodoOutputPane::goToNext()
{
    todoList->setCurrentRow(todoList->currentRow() + 1);
}

void TodoOutputPane::goToPrev()
{
    todoList->setCurrentRow(todoList->currentRow() - 1);
}

void TodoOutputPane::sort()
{
    todoList->sortItems(Qt::AscendingOrder);
    if (todoList->count() > 0)
        todoList->setCurrentRow(lastCurrentRow < todoList->count() ? lastCurrentRow : todoList->count() - 1);
}
