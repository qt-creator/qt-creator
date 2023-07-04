// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "annotationtableview.h"

#include "defaultannotations.h"

#include <utils/qtcolorbutton.h>

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QCompleter>
#include <QDoubleSpinBox>
#include <QHeaderView>
#include <QItemEditorFactory>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QPushButton>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QStringListModel>
#include <QStyle>
#include <QTextEdit>

namespace QmlDesigner {

struct ColumnId
{
    enum Column {
        Title = 0,
        Author = 1,
        Value = 2,
    };
};

CommentDelegate::CommentDelegate(QObject *parent)
    : QItemDelegate(parent)
    , m_completer(std::make_unique<QCompleter>())
{}

CommentDelegate::~CommentDelegate() {}

DefaultAnnotationsModel *CommentDelegate::defaultAnnotations() const
{
    return m_defaults;
}

void CommentDelegate::setDefaultAnnotations(DefaultAnnotationsModel *defaults)
{
    m_defaults = defaults;
    m_completer->setModel(m_defaults);
}

QCompleter *CommentDelegate::completer() const
{
    return m_completer.get();
}

void CommentDelegate::updateEditorGeometry(QWidget *editor,
                                           const QStyleOptionViewItem &option,
                                           [[maybe_unused]] const QModelIndex &index) const
{
    editor->setGeometry(option.rect);
}

Comment CommentDelegate::comment(const QModelIndex &index)
{
    auto *model = index.model();
    return model->data(model->index(index.row(), ColumnId::Title), CommentRole).value<Comment>();
}

CommentTitleDelegate::CommentTitleDelegate(QObject *parent)
    : CommentDelegate(parent)
{}

CommentTitleDelegate::~CommentTitleDelegate() {}

QWidget *CommentTitleDelegate::createEditor(QWidget *parent,
                                            [[maybe_unused]] const QStyleOptionViewItem &option,
                                            [[maybe_unused]] const QModelIndex &index) const
{
    auto *editor = new QComboBox(parent);
    editor->setEditable(true);
    editor->setCompleter(completer());
    editor->setFrame(false);
    editor->setFocusPolicy(Qt::StrongFocus);

    return editor;
}

void CommentTitleDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QString text = index.model()->data(index, Qt::DisplayRole).toString();
    auto *comboBox = qobject_cast<QComboBox *>(editor);
    comboBox->setModel(defaultAnnotations());
    comboBox->setCurrentText(text);
}

void CommentTitleDelegate::setModelData(QWidget *editor,
                                        QAbstractItemModel *model,
                                        const QModelIndex &index) const
{
    auto *comboBox = qobject_cast<QComboBox *>(editor);
    auto oldText = model->data(index, Qt::EditRole).toString();
    auto newText = comboBox->currentText();

    if (oldText != newText) {
        model->setData(index, comboBox->currentText(), Qt::EditRole);
        auto comment = model->data(index, CommentRole).value<Comment>();
        comment.setTitle(newText);
        model->setData(index, QVariant::fromValue(comment), CommentRole);

        // Set default value to data item
        auto colIdx = model->index(index.row(), ColumnId::Value);
        if (defaultAnnotations()->hasDefault(comment))
            model->setData(colIdx, defaultAnnotations()->defaultValue(comment), Qt::DisplayRole);
        else
            // Reset to rich text when there is no default item
            model->setData(colIdx,
                           QVariant::fromValue<RichTextProxy>({comment.text()}),
                           Qt::DisplayRole);
    }
}

CommentValueDelegate::CommentValueDelegate(QObject *parent)
    : CommentDelegate(parent)
{}

CommentValueDelegate::~CommentValueDelegate() {}

void CommentValueDelegate::paint(QPainter *painter,
                                 const QStyleOptionViewItem &option,
                                 const QModelIndex &index) const
{
    auto data = index.model()->data(index, Qt::DisplayRole);
    if (data.typeId() == qMetaTypeId<RichTextProxy>())
        drawDisplay(painter, option, option.rect, data.value<RichTextProxy>().plainText());
    else if (data.typeId() == QMetaType::QColor)
        painter->fillRect(option.rect, data.value<QColor>());
    else
        QItemDelegate::paint(painter, option, index);
}

void CommentValueDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    auto data = index.model()->data(index, Qt::DisplayRole);
    if (data.typeId() == qMetaTypeId<RichTextProxy>()) {
        auto richText = data.value<RichTextProxy>();
        auto *e = qobject_cast<RichTextCellEditor *>(editor);
        e->setText(richText.plainText());
        e->setupSignal(index.row(), comment(index).title());
        connect(e,
                &RichTextCellEditor::richTextClicked,
                this,
                &CommentValueDelegate::richTextEditorRequested,
                Qt::UniqueConnection);
    } else if (data.typeId() == QMetaType::QString) {
        auto *e = qobject_cast<QLineEdit *>(editor);
        e->setText(data.toString());
    } else if (data.typeId() == QMetaType::QColor) {
        auto *e = qobject_cast<AnnotationTableColorButton *>(editor);
        e->setColor(data.value<QColor>());
        e->installEventFilter(e);
        connect(e,
                &AnnotationTableColorButton::editorFinished,
                this,
                &CommentValueDelegate::slotEditorFinished,
                Qt::UniqueConnection);
        connect(e,
                &AnnotationTableColorButton::editorCanceled,
                this,
                &CommentValueDelegate::slotEditorCanceled,
                Qt::UniqueConnection);
    } else
        QItemDelegate::setEditorData(editor, index);
}

bool AnnotationTableColorButton::eventFilter(QObject *object, QEvent *event)
{
    AnnotationTableColorButton *editor = qobject_cast<AnnotationTableColorButton*>(object);
    if (editor && event->type() == QEvent::FocusOut && editor->isDialogOpen())
        return true;

    return QObject::eventFilter(object, event);
}

void CommentValueDelegate::slotEditorCanceled(QWidget *editor)
{
    emit closeEditor(editor);
}

void CommentValueDelegate::slotEditorFinished(QWidget *editor)
{
    AnnotationTableColorButton* e = qobject_cast<AnnotationTableColorButton *>(editor);
    if (e) {
        emit commitData(editor);
        emit closeEditor(editor, QAbstractItemDelegate::SubmitModelCache);
    }
}

void CommentValueDelegate::setModelData(QWidget *editor,
                                        QAbstractItemModel *model,
                                        const QModelIndex &index) const
{
    auto data = model->data(index, Qt::EditRole);
    if (data.typeId() == qMetaTypeId<RichTextProxy>())
        return;
    else if (data.typeId() == QMetaType::QColor) {
        model->setData(index,
                       qobject_cast<AnnotationTableColorButton *>(editor)->color(),
                       Qt::DisplayRole);
    } else if (data.typeId() == QMetaType::QString)
        model->setData(index, qobject_cast<QLineEdit *>(editor)->text(), Qt::DisplayRole);
    else
        QItemDelegate::setModelData(editor, model, index);
}

RichTextCellEditor::RichTextCellEditor(QWidget *parent)
    : QLabel(parent)
{}

RichTextCellEditor::~RichTextCellEditor() {}

RichTextProxy RichTextCellEditor::richText() const
{
    return m_richText;
}

void RichTextCellEditor::setRichText(const RichTextProxy &richText)
{
    if (richText.text == m_richText.text)
        return;

    m_richText = richText;
    setText(richText.plainText());

    emit richTextChanged();
}

void RichTextCellEditor::setupSignal(int index, const QString &commentTitle)
{
    if (m_connection)
        disconnect(m_connection);

    m_connection = connect(this, &RichTextCellEditor::clicked, this, [=]() {
        emit richTextClicked(index, commentTitle);
    });
}

void RichTextCellEditor::mouseReleaseEvent(QMouseEvent *)
{
    emit clicked();
}

AnnotationTableColorButton::AnnotationTableColorButton(QWidget *parent)
    : Utils::QtColorButton(parent)
{
    connect(this, &Utils::QtColorButton::colorChangeStarted, this, [this](){emit editorStarted(this);});
    connect(this, &Utils::QtColorButton::colorChanged, this, [this](QColor){emit editorFinished(this);});
    connect(this, &Utils::QtColorButton::colorUnchanged, this, [this](){emit editorCanceled(this);});
}

AnnotationTableColorButton::~AnnotationTableColorButton() {}

AnnotationTableView::AnnotationTableView(QWidget *parent)
    : QTableView(parent)
    , m_model(std::make_unique<QStandardItemModel>())
    , m_editorFactory(std::make_unique<QItemEditorFactory>())
{
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::ContiguousSelection);

    setModel(m_model.get());
    connect(m_model.get(), &QStandardItemModel::itemChanged, this, [this](QStandardItem *item) {
        if (item->isCheckable())
            m_model->setData(item->index(), item->checkState() == Qt::Checked);

        if (this->m_modelUpdating)
            return;

        auto *valueItem = m_model->item(item->row(), ColumnId::Value);

        // When comment title was edited, make value item editable
        if (item->column() == ColumnId::Title && valueItem) {
            valueItem->setEditable(!item->text().isEmpty());
            valueItem->setCheckable(valueItem->data(Qt::DisplayRole).typeId() == QMetaType::Bool);
        }

        m_modelUpdating = true;
        if (!rowIsEmpty(m_model->rowCount() - 1))
            addEmptyRow();
        m_modelUpdating = false;
    });

    horizontalHeader()->setStretchLastSection(true);
    horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);

    m_editorFactory->registerEditor(qMetaTypeId<RichTextProxy>(),
                                    new QItemEditorCreator<RichTextCellEditor>("richText"));
    m_editorFactory->registerEditor(QMetaType::QColor,
                                    new QItemEditorCreator<AnnotationTableColorButton>("color"));

    m_valueDelegate.setItemEditorFactory(m_editorFactory.get());
    connect(&m_valueDelegate,
            &CommentValueDelegate::richTextEditorRequested,
            this,
            &AnnotationTableView::richTextEditorRequested);

    verticalHeader()->hide();
}

AnnotationTableView::~AnnotationTableView() {}

QVector<Comment> AnnotationTableView::fetchComments() const
{
    QVector<Comment> comments;

    for (int i = 0; i < m_model->rowCount(); ++i) {
        Comment comment = fetchComment(i);
        if (!comment.isEmpty())
            comments.push_back(comment);
    }

    return comments;
}

Comment AnnotationTableView::fetchComment(int row) const
{
    const auto *item = m_model->item(row, ColumnId::Title);
    Comment comment = item->data().value<Comment>();

    if (comment.isEmpty())
        return {};

    comment.setTitle(item->text());
    comment.setAuthor(m_model->item(row, ColumnId::Author)->text());
    comment.setText(dataToCommentText(m_model->item(row, ColumnId::Value)->data(Qt::DisplayRole)));

    return comment;
}

void AnnotationTableView::setupComments(const QVector<Comment> &comments)
{
    m_model->clear();
    m_modelUpdating = true;
    m_model->setColumnCount(3);
    m_model->setHeaderData(ColumnId::Title, Qt::Horizontal, tr("Title"));
    m_model->setHeaderData(ColumnId::Author, Qt::Horizontal, tr("Author"));
    m_model->setHeaderData(ColumnId::Value, Qt::Horizontal, tr("Value"));
    setItemDelegateForColumn(ColumnId::Title, &m_titleDelegate);
    setItemDelegateForColumn(ColumnId::Value, &m_valueDelegate);

    for (auto &comment : comments) {
        if (comment.isEmpty())
            continue;

        addEmptyRow();
        changeRow(m_model->rowCount() - 1, comment);
    }

    addEmptyRow();
    m_modelUpdating = false;
}

DefaultAnnotationsModel *AnnotationTableView::defaultAnnotations() const
{
    return m_defaults;
}

void AnnotationTableView::setDefaultAnnotations(DefaultAnnotationsModel *defaults)
{
    m_defaults = defaults;
    m_titleDelegate.setDefaultAnnotations(defaults);
    m_valueDelegate.setDefaultAnnotations(defaults);
}

void AnnotationTableView::changeRow(int index, const Comment &comment)
{
    auto *titleItem = m_model->item(index, ColumnId::Title);
    auto *authorItem = m_model->item(index, ColumnId::Author);
    auto *textItem = m_model->item(index, ColumnId::Value);

    titleItem->setText(comment.title());
    titleItem->setData(QVariant::fromValue<Comment>(comment));

    authorItem->setText(comment.author());

    QVariant data = commentToData(comment,
                                  m_defaults ? m_defaults->defaultType(comment)
                                  : QMetaType::UnknownType);

    textItem->setEditable(data.isValid());
    textItem->setCheckable(data.typeId() == QMetaType::Bool);
    textItem->setData(data, Qt::DisplayRole);
}

void AnnotationTableView::removeRow(int index)
{
    m_model->removeRow(index);
}

void AnnotationTableView::removeSelectedRows()
{
    const auto selRows = selectionModel()->selectedRows();
    for (auto it = selRows.rbegin(); it != selRows.rend(); ++it)
        removeRow(it->row());
}

void AnnotationTableView::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Delete)
        removeSelectedRows();
}

void AnnotationTableView::addEmptyRow()
{
    auto *valueItem = new QStandardItem;
    valueItem->setEditable(false);
    m_model->appendRow({new QStandardItem, new QStandardItem, valueItem});
}

bool AnnotationTableView::rowIsEmpty(int row) const
{
    auto itemText = [&](int col) {
        return m_model->item(row, col) ? m_model->item(row, col)->text() : QString();
    };

    return QString(itemText(0) + itemText(1) + itemText(2)).isEmpty();
}

QString AnnotationTableView::dataToCommentText(const QVariant &data)
{
    auto type = data.typeId();
    if (type == qMetaTypeId<RichTextProxy>())
        return data.value<RichTextProxy>().text;

    switch (type) {
    case QMetaType::QColor:
        return data.value<QColor>().name();
    case QMetaType::Bool:
        return data.toBool() ? QStringLiteral("true") : QStringLiteral("false");
    case QMetaType::QString:
        return data.toString();
    }

    return {};
}

QVariant AnnotationTableView::commentToData(const Comment &comment, QMetaType::Type type)
{
    switch (type) {
    case QMetaType::Bool:
        return QVariant::fromValue(comment.deescapedText().toLower().trimmed() == "true");
    case QMetaType::QColor:
        return QVariant::fromValue(QColor(comment.deescapedText().toLower().trimmed()));
        break;
    case QMetaType::QString:
        return QVariant::fromValue(comment.text());
        break;
    default:
        if (type == qMetaTypeId<RichTextProxy>() || type == QMetaType::UnknownType)
            return QVariant::fromValue<RichTextProxy>({comment.text()});
    }
    return {};
}

} // namespace QmlDesigner
