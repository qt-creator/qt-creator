/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef SELECTABLETEXTEDITORWIDGET_H
#define SELECTABLETEXTEDITORWIDGET_H

#include <texteditor/texteditor.h>

namespace DiffEditor {
namespace Internal {

class DiffSelection
{
public:
    DiffSelection() : start(-1), end(-1), format(0) {}
    DiffSelection(QTextCharFormat *f) : start(-1), end(-1), format(f) {}
    DiffSelection(int s, int e, QTextCharFormat *f) : start(s), end(e), format(f) {}

    int start;
    int end;
    QTextCharFormat *format;
};

class SelectableTextEditorWidget : public TextEditor::TextEditorWidget
{
    Q_OBJECT
public:
    SelectableTextEditorWidget(Core::Id id, QWidget *parent = 0);
    ~SelectableTextEditorWidget() override;
    void setSelections(const QMap<int, QList<DiffSelection> > &selections);

private:
    void paintBlock(QPainter *painter,
                    const QTextBlock &block,
                    const QPointF &offset,
                    const QVector<QTextLayout::FormatRange> &selections,
                    const QRect &clipRect) const override;

    // block number, list of ranges
    // DiffSelection.start - can be -1 (continues from the previous line)
    // DiffSelection.end - can be -1 (spans to the end of line, even after the last character in line)
    QMap<int, QList<DiffSelection> > m_diffSelections;
};

} // namespace Internal
} // namespace DiffEditor

#endif // SELECTABLETEXTEDITORWIDGET_H
