// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../utils_global.h"

#include <QAccessibleWidget>

QT_BEGIN_NAMESPACE
class QTextDocument;
QT_END_NAMESPACE

namespace Utils {

class PlainTextEdit;

class AccessibleTextWidget : public QAccessibleWidget,
                             public QAccessibleTextInterface,
                             public QAccessibleEditableTextInterface
{
public:
    AccessibleTextWidget(QWidget *o, QAccessible::Role r = QAccessible::EditableText, const QString &name = QString());

    QAccessible::State state() const override;

    // QAccessibleTextInterface
    //  selection
    void selection(int selectionIndex, int *startOffset, int *endOffset) const override;
    int selectionCount() const override;
    void addSelection(int startOffset, int endOffset) override;
    void removeSelection(int selectionIndex) override;
    void setSelection(int selectionIndex, int startOffset, int endOffset) override;

    // cursor
    int cursorPosition() const override;
    void setCursorPosition(int position) override;

    // text
    QString text(int startOffset, int endOffset) const override;
    QString textBeforeOffset(int offset, QAccessible::TextBoundaryType boundaryType,
                             int *startOffset, int *endOffset) const override;
    QString textAfterOffset(int offset, QAccessible::TextBoundaryType boundaryType,
                            int *startOffset, int *endOffset) const override;
    QString textAtOffset(int offset, QAccessible::TextBoundaryType boundaryType,
                         int *startOffset, int *endOffset) const override;
    int characterCount() const override;

    // character <-> geometry
    QRect characterRect(int offset) const override;
    int offsetAtPoint(const QPoint &point) const override;

    QString attributes(int offset, int *startOffset, int *endOffset) const override;

    // QAccessibleEditableTextInterface
    void deleteText(int startOffset, int endOffset) override;
    void insertText(int offset, const QString &text) override;
    void replaceText(int startOffset, int endOffset, const QString &text) override;

    using QAccessibleWidget::text;

protected:
    QTextCursor textCursorForRange(int startOffset, int endOffset) const;
    virtual QPoint scrollBarPosition() const;
    // return the current text cursor at the caret position including a potential selection
    virtual QTextCursor textCursor() const = 0;
    virtual void setTextCursor(const QTextCursor &) = 0;
    virtual QTextDocument *textDocument() const = 0;
    virtual QWidget *viewport() const = 0;
};

class AccessiblePlainTextEdit : public AccessibleTextWidget
{
public:
    explicit AccessiblePlainTextEdit(QWidget *o);

    QString text(QAccessible::Text t) const override;
    void setText(QAccessible::Text t, const QString &text) override;
    QAccessible::State state() const override;

    void *interface_cast(QAccessible::InterfaceType t) override;

    // QAccessibleTextInterface
    void scrollToSubstring(int startIndex, int endIndex) override;

    using AccessibleTextWidget::text;

protected:
    PlainTextEdit *plainTextEdit() const;

    QPoint scrollBarPosition() const override;
    QTextCursor textCursor() const override;
    void setTextCursor(const QTextCursor &textCursor) override;
    QTextDocument *textDocument() const override;
    QWidget *viewport() const override;
};

QTCREATOR_UTILS_EXPORT QAccessibleInterface *accessiblePlainTextEditFactory(const QString &classname, QObject *object);

} // namespace Utils
