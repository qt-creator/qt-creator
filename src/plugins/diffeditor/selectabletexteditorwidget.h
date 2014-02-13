/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef SELECTABLETEXTEDITORWIDGET_H
#define SELECTABLETEXTEDITORWIDGET_H

#include "diffeditor_global.h"
#include <texteditor/basetexteditor.h>

namespace DiffEditor {

class DIFFEDITOR_EXPORT DiffSelection
{
public:
    DiffSelection() : start(-1), end(-1), format(0) {}
    DiffSelection(QTextCharFormat *f) : start(-1), end(-1), format(f) {}
    DiffSelection(int s, int e, QTextCharFormat *f) : start(s), end(e), format(f) {}

    int start;
    int end;
    QTextCharFormat *format;
};

class DIFFEDITOR_EXPORT SelectableTextEditorWidget
        : public TextEditor::BaseTextEditorWidget
{
    Q_OBJECT
public:
    SelectableTextEditorWidget(QWidget *parent = 0);
    ~SelectableTextEditorWidget();
    void setSelections(const QMap<int,
                       QList<DiffSelection> > &selections) {
        m_selections = selections;
    }

protected:
    virtual void paintEvent(QPaintEvent *e);

private:
    void paintSelections(QPaintEvent *e);
    void paintSelections(QPainter &painter,
                         const QList<DiffSelection> &selections,
                         const QTextBlock &block,
                         int top);

    // block number, list of ranges
    // DiffSelection.start - can be -1 (continues from the previous line)
    // DiffSelection.end - can be -1 (spans to the end of line, even after the last character in line)
    QMap<int, QList<DiffSelection> > m_selections;
};

} // namespace DiffEditor

#endif // SELECTABLETEXTEDITORWIDGET_H
