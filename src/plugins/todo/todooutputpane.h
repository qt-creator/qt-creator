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

#ifndef TODOOUTPUTPANE_H
#define TODOOUTPUTPANE_H

#include "keyword.h"
#include <coreplugin/ioutputpane.h>
#include <QObject>
#include <QListWidget>

class TodoOutputPane : public Core::IOutputPane
{
public:
    TodoOutputPane(QObject *parent);
    ~TodoOutputPane();

    QWidget *outputWidget(QWidget *parent);
    QList<QWidget*> toolBarWidgets() const;
    QString name() const;
    QString displayName() const;

    int priorityInStatusBar() const;

    void clearContents();
    void clearContents(QString filename);
    void visibilityChanged(bool visible);

    void setFocus();
    bool hasFocus();
    bool canFocus();

    bool canNavigate();
    bool canNext();
    bool canPrevious();
    void goToNext();
    void goToPrev();

    void sort();

    void addItem(const QString &text, const QString &file, const int rowNumber, const QIcon &icon, const QColor &color);
    QListWidget *getTodoList() const;

private:
    QListWidget *todoList;
    int lastCurrentRow;
};

#endif // TODOOUTPUTPANE_H
