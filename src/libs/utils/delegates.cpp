// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "delegates.h"

#include "completinglineedit.h"

#include <QPainter>
#include <QApplication>
#include <QCompleter>

namespace Utils {

AnnotatedItemDelegate::AnnotatedItemDelegate(QObject *parent) : QStyledItemDelegate(parent)
{}

AnnotatedItemDelegate::~AnnotatedItemDelegate() = default;

void AnnotatedItemDelegate::setAnnotationRole(int role)
{
    m_annotationRole = role;
}

int AnnotatedItemDelegate::annotationRole() const
{
    return m_annotationRole;
}

void AnnotatedItemDelegate::setDelimiter(const QString &delimiter)
{
    m_delimiter = delimiter;
}

const QString &AnnotatedItemDelegate::delimiter() const
{
    return m_delimiter;
}

void AnnotatedItemDelegate::paint(QPainter *painter,
                                  const QStyleOptionViewItem &option,
                                  const QModelIndex &index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    QStyle *style = QApplication::style();
    style->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, painter, opt.widget);
    style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, opt.widget);

    QString annotation = index.data(m_annotationRole).toString();
    if (!annotation.isEmpty()) {

        int newlinePos = annotation.indexOf(QLatin1Char('\n'));
        if (newlinePos != -1) {
            // print first line with '...' at end
            const QChar ellipsisChar(0x2026);
            annotation = annotation.left(newlinePos) + ellipsisChar;
        }

        QPalette disabled(opt.palette);
        disabled.setCurrentColorGroup(QPalette::Disabled);

        painter->save();
        painter->setPen(disabled.color(QPalette::WindowText));

        static int extra = opt.fontMetrics.horizontalAdvance(m_delimiter) + 10;
        const QPixmap &pixmap = opt.icon.pixmap(opt.decorationSize);
        const QRect &iconRect = style->itemPixmapRect(opt.rect, opt.decorationAlignment, pixmap);
        const QRect &displayRect = style->itemTextRect(opt.fontMetrics, opt.rect,
            opt.displayAlignment, true, index.data(Qt::DisplayRole).toString());
        QRect annotationRect = style->itemTextRect(opt.fontMetrics, opt.rect,
            opt.displayAlignment, true, annotation);
        annotationRect.adjust(iconRect.width() + displayRect.width() + extra, 0,
                              iconRect.width() + displayRect.width() + extra, 0);

        QApplication::style()->drawItemText(painter, annotationRect,
            Qt::AlignLeft | Qt::AlignBottom, disabled, true, annotation);

        painter->restore();
    }
}

QSize AnnotatedItemDelegate::sizeHint(const QStyleOptionViewItem &option,
                                      const QModelIndex &index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    const QString &annotation = index.data(m_annotationRole).toString();
    if (!annotation.isEmpty())
        opt.text += m_delimiter + annotation;

    return QApplication::style()->sizeFromContents(QStyle::CT_ItemViewItem, &opt, QSize(), nullptr);
}

PathChooserDelegate::PathChooserDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

void PathChooserDelegate::setExpectedKind(PathChooser::Kind kind)
{
    m_kind = kind;
}

void PathChooserDelegate::setPromptDialogFilter(const QString &filter)
{
    m_filter = filter;
}

QWidget *PathChooserDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option)
    Q_UNUSED(index)

    auto editor = new PathChooser(parent);

    editor->setHistoryCompleter(m_historyKey);
    editor->setAutoFillBackground(true); // To hide the text beneath the editor widget
    editor->lineEdit()->setMinimumWidth(0);

    connect(editor, &PathChooser::browsingFinished, this, [this, editor] {
        emit const_cast<PathChooserDelegate*>(this)->commitData(editor);
    });

    return editor;
}

void PathChooserDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    if (auto pathChooser = qobject_cast<PathChooser *>(editor)) {
        pathChooser->setExpectedKind(m_kind);
        pathChooser->setPromptDialogFilter(m_filter);
        pathChooser->setFilePath(FilePath::fromVariant(index.model()->data(index, Qt::EditRole)));
    }
}

void PathChooserDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    auto pathChooser = qobject_cast<PathChooser *>(editor);
    if (!pathChooser)
        return;

    model->setData(index, pathChooser->filePath().toVariant(), Qt::EditRole);
}

void PathChooserDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(index)

    editor->setGeometry(option.rect);
}

void PathChooserDelegate::setHistoryCompleter(const Key &key)
{
    m_historyKey = key;
}

CompleterDelegate::CompleterDelegate(const QStringList &candidates, QObject *parent)
    : CompleterDelegate(new QCompleter(candidates, parent))
{ }

CompleterDelegate::CompleterDelegate(QAbstractItemModel *model, QObject *parent)
    : CompleterDelegate(new QCompleter(model, parent))
{ }

CompleterDelegate::CompleterDelegate(QCompleter *completer, QObject *parent)
    : QStyledItemDelegate(parent)
    , m_completer(completer)
{ }

CompleterDelegate::~CompleterDelegate()
{
    if (m_completer)
        delete m_completer;
}

QWidget *CompleterDelegate::createEditor(QWidget *parent,
                                         const QStyleOptionViewItem &option,
                                         const QModelIndex &index) const
{
    Q_UNUSED(option)
    Q_UNUSED(index)

    auto edit = new CompletingLineEdit(parent);

    edit->setCompleter(m_completer);

    return edit;
}

void CompleterDelegate::setEditorData(QWidget *editor,
                                      const QModelIndex &index) const
{
    if (auto *edit = qobject_cast<CompletingLineEdit *>(editor))
        edit->setText(index.model()->data(index, Qt::EditRole).toString());
}

void CompleterDelegate::setModelData(QWidget *editor,
                                     QAbstractItemModel *model,
                                     const QModelIndex &index) const
{
    if (auto edit = qobject_cast<CompletingLineEdit *>(editor))
        model->setData(index, edit->text(), Qt::EditRole);
}

void CompleterDelegate::updateEditorGeometry(QWidget *editor,
                                             const QStyleOptionViewItem &option,
                                             const QModelIndex &index) const
{
    Q_UNUSED(index)

    editor->setGeometry(option.rect);
}

} // Utils
