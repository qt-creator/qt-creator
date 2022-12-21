// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include <QItemDelegate>
#include <QLabel>
#include <QPointer>
#include <QTableView>

#include <memory>

#include "annotation.h"
#include "defaultannotations.h"

#include <utils/qtcolorbutton.h>

QT_BEGIN_NAMESPACE
class QStandardItemModel;
class QCompleter;
QT_END_NAMESPACE

namespace QmlDesigner {

class CommentDelegate : public QItemDelegate
{
    Q_OBJECT
public:
    enum Role { CommentRole = Qt::UserRole + 1 };

    CommentDelegate(QObject *parent = nullptr);
    ~CommentDelegate() override;

    DefaultAnnotationsModel *defaultAnnotations() const;
    void setDefaultAnnotations(DefaultAnnotationsModel *);

    QCompleter *completer() const;

    void updateEditorGeometry(QWidget *editor,
                              const QStyleOptionViewItem &option,
                              const QModelIndex &index) const override;

    static Comment comment(QModelIndex const &);

private:
    std::unique_ptr<QCompleter> m_completer;
    QPointer<DefaultAnnotationsModel> m_defaults;
};

class CommentTitleDelegate : public CommentDelegate
{
    Q_OBJECT
public:
    CommentTitleDelegate(QObject *parent = nullptr);
    ~CommentTitleDelegate() override;

    QWidget *createEditor(QWidget *parent,
                          const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override;

    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor,
                      QAbstractItemModel *model,
                      const QModelIndex &index) const override;
signals:
    void commentChanged(int row, const QmlDesigner::Comment &comment);
};

class CommentValueDelegate : public CommentDelegate
{
    Q_OBJECT
public:
    CommentValueDelegate(QObject *parent = nullptr);
    ~CommentValueDelegate();

    void paint(QPainter *painter,
               const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;

    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor,
                      QAbstractItemModel *model,
                      const QModelIndex &index) const override;

public slots:
    void slotEditorFinished(QWidget* editor);
    void slotEditorCanceled(QWidget* editor);

signals:
    void richTextEditorRequested(int index, QString const &richText);
};

class RichTextCellEditor : public QLabel
{
    Q_OBJECT
    Q_PROPERTY(QmlDesigner::RichTextProxy richText READ richText WRITE setRichText NOTIFY
                   richTextChanged USER true)
public:
    RichTextCellEditor(QWidget *parent = nullptr);
    ~RichTextCellEditor() override;

    RichTextProxy richText() const;
    void setRichText(RichTextProxy const &);

    void setupSignal(int row, QString const &commentTitle);

signals:
    void clicked();
    void richTextChanged();
    void richTextClicked(int index, QString const &text);

protected:
    void mouseReleaseEvent(QMouseEvent *) override;

private:
    RichTextProxy m_richText;
    QMetaObject::Connection m_connection;
};

class AnnotationTableColorButton : public Utils::QtColorButton
{
    Q_OBJECT
public:
    AnnotationTableColorButton(QWidget* parent);
    ~AnnotationTableColorButton();

    bool eventFilter(QObject *object, QEvent *event) override;

signals:
    void editorStarted(QWidget* editor);
    void editorFinished(QWidget* editor);
    void editorCanceled(QWidget* editor);
};

class AnnotationTableView : public QTableView
{
    Q_OBJECT
public:
    AnnotationTableView(QWidget *parent = nullptr);
    ~AnnotationTableView();

    QVector<Comment> fetchComments() const;
    Comment fetchComment(int row) const;
    void setupComments(QVector<Comment> const &comments);

    DefaultAnnotationsModel *defaultAnnotations() const;
    void setDefaultAnnotations(DefaultAnnotationsModel *);

    void changeRow(int index, Comment const &comment);
    void removeRow(int index);
    void removeSelectedRows();

signals:
    void richTextEditorRequested(int index, QString const &commentTitle);

protected:
    void keyPressEvent(QKeyEvent *) override;

private:
    void addEmptyRow();
    bool rowIsEmpty(int row) const;
    static QString dataToCommentText(QVariant const &);
    static QVariant commentToData(Comment const&, QMetaType::Type type);

    CommentTitleDelegate m_titleDelegate;
    CommentValueDelegate m_valueDelegate;

    bool m_modelUpdating = false;
    std::unique_ptr<QStandardItemModel> m_model;
    std::unique_ptr<QItemEditorFactory> m_editorFactory;
    QPointer<DefaultAnnotationsModel> m_defaults;
};
} // namespace QmlDesigner
