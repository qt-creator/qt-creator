/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <theme.h>

#include <QWidget>
#include <QToolBar>
#include <QList>
#include <QTextCharFormat>
#include <QTextList>
#include <QFontComboBox>
#include <QWidgetAction>
#include <QPointer>

namespace QmlDesigner {

namespace Ui {
class RichTextEditor;
}

template <class>
class FontWidgetActions;

class HyperlinkDialog;

class RichTextEditor : public QWidget
{
    Q_OBJECT

public:
    explicit RichTextEditor(QWidget *parent = nullptr);
    ~RichTextEditor();

    void setPlainText(const QString &text);
    QString plainText() const;

    void setRichText(const QString &text);
    QString richText() const;

    void setTabChangesFocus(bool change);

private slots:
    void currentCharFormatChanged(const QTextCharFormat &format);
    void cursorPositionChanged();

private:
    QIcon getIcon(Theme::Icon icon);
    void mergeFormatOnWordOrSelection(const QTextCharFormat &format);

    void fontChanged(const QFont &f);
    void colorChanged(const QColor &c);
    void alignmentChanged(Qt::Alignment a);
    void styleChanged(const QTextCursor &cursor);
    void tableChanged(const QTextCursor &cursor);

    void setupEditActions();
    void setupTextActions();
    void setupHyperlinkActions();
    void setupAlignActions();
    void setupListActions();
    void setupFontActions();
    void setupTableActions();

    void textStyle(QTextListFormat::Style style);

    void setTableActionsActive(bool active); //switches between "has table/has no table" ui setup

private:
    QScopedPointer<Ui::RichTextEditor> ui;
    QPointer<HyperlinkDialog> m_linkDialog;

    QAction *m_actionTextBold;
    QAction *m_actionTextItalic;
    QAction *m_actionTextUnderline;

    QAction *m_actionHyperlink;

    QAction *m_actionAlignLeft;
    QAction *m_actionAlignCenter;
    QAction *m_actionAlignRight;
    QAction *m_actionAlignJustify;

    QAction *m_actionTextColor;

    QAction *m_actionBulletList;
    QAction *m_actionNumberedList;

    QAction *m_actionTableSettings;

    QAction *m_actionCreateTable;
    QAction *m_actionRemoveTable;

    QAction *m_actionAddRow;
    QAction *m_actionAddColumn;
    QAction *m_actionRemoveRow;
    QAction *m_actionRemoveColumn;

    QAction *m_actionMergeCells;
    QAction *m_actionSplitRow;
    QAction *m_actionSplitColumn;

    QPointer<FontWidgetActions<QFontComboBox>> m_fontNameAction;
    QPointer<FontWidgetActions<QComboBox>> m_fontSizeAction;
};

} //namespace QmlDesigner
