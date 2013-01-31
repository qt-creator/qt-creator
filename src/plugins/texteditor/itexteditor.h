/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#ifndef ITEXTEDITOR_H
#define ITEXTEDITOR_H

#include "texteditor_global.h"

#include "itextmark.h"

#include <coreplugin/editormanager/ieditor.h>

#include <QObject>
#include <QList>
#include <QMap>
#include <QIcon>

QT_BEGIN_NAMESPACE
class QIcon;
class QMenu;
class QPainter;
class QPoint;
class QRect;
class QTextBlock;
QT_END_NAMESPACE

namespace TextEditor {

class TEXTEDITOR_EXPORT ITextEditor : public Core::IEditor
{
    Q_OBJECT
public:
    enum PositionOperation {
        Current = 1,
        EndOfLine = 2,
        StartOfLine = 3,
        Anchor = 4,
        EndOfDoc = 5
    };

    ITextEditor() {}

    virtual int find(const QString &string) const = 0;
    virtual int position(PositionOperation posOp = Current, int at = -1) const = 0;
    virtual void convertPosition(int pos, int *line, int *column) const = 0;
    virtual QRect cursorRect(int pos = -1) const = 0;
    virtual int columnCount() const = 0;
    virtual int rowCount() const = 0;

    virtual QString contents() const = 0;
    virtual QString selectedText() const = 0;
    virtual QString textAt(int pos, int length) const = 0;
    virtual QChar characterAt(int pos) const = 0;

    /*! Removes \a length characters to the right of the cursor. */
    virtual void remove(int length) = 0;
    /*! Inserts the given string to the right of the cursor. */
    virtual void insert(const QString &string) = 0;
    /*! Replaces \a length characters to the right of the cursor with the given string. */
    virtual void replace(int length, const QString &string) = 0;
    /*! Sets current cursor position to \a pos. */
    virtual void setCursorPosition(int pos) = 0;
    /*! Selects text between current cursor position and \a toPos. */
    virtual void select(int toPos) = 0;

    virtual ITextMarkable *markableInterface() = 0;

    enum TextCodecReason {
        TextCodecOtherReason,
        TextCodecFromSystemSetting,
        TextCodecFromProjectSetting
    };

    virtual void setTextCodec(QTextCodec *, TextCodecReason reason = TextCodecOtherReason) = 0;
    virtual QTextCodec *textCodec() const = 0;

    static QMap<QString, QString> openedTextEditorsContents();
    static QMap<QString, QTextCodec *> openedTextEditorsEncodings();

    enum MarkRequestKind {
        BreakpointRequest,
        BookmarkRequest,
        TaskMarkRequest
    };

signals:
    void contentsChanged();
    void contentsChangedBecauseOfUndo();
    void markRequested(TextEditor::ITextEditor *editor, int line, TextEditor::ITextEditor::MarkRequestKind kind);
    void markContextMenuRequested(TextEditor::ITextEditor *editor, int line, QMenu *menu);
    void tooltipOverrideRequested(TextEditor::ITextEditor *editor, const QPoint &globalPos, int position, bool *handled);
    void tooltipRequested(TextEditor::ITextEditor *editor, const QPoint &globalPos, int position);
    void markTooltipRequested(TextEditor::ITextEditor *editor, const QPoint &globalPos, int line);
    void contextHelpIdRequested(TextEditor::ITextEditor *editor, int position);
};

} // namespace TextEditor

#endif // ITEXTEDITOR_H
