// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <theme.h>

#include <QFontComboBox>
#include <QList>
#include <QPointer>
#include <QTextCharFormat>
#include <QTextEdit>
#include <QTextList>
#include <QToolBar>
#include <QWidgetAction>

namespace QmlDesigner {

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

    void setImageActionVisible(bool change);

    void setDocumentBaseUrl(const QUrl &url);

signals:
    void insertingImage(QString &filePath);
    void textChanged(QString text);

private slots:
    void currentCharFormatChanged(const QTextCharFormat &format);
    void cursorPositionChanged();
    void onTextChanged();
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
    void setupImageActions();
    void setupHyperlinkActions();
    void setupAlignActions();
    void setupListActions();
    void setupFontActions();
    void setupTableActions();

    void textStyle(QTextListFormat::Style style);

    void setTableActionsActive(bool active); //switches between "has table/has no table" ui setup

private:
    QTextEdit *m_textEdit;
    QToolBar *m_toolBar;
    QToolBar *m_tableBar;

    QPointer<HyperlinkDialog> m_linkDialog;

    QAction *m_actionTextBold;
    QAction *m_actionTextItalic;
    QAction *m_actionTextUnderline;

    QAction *m_actionImage;
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
