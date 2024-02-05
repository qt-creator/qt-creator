// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "bookmarkmanager.h"

#include "bookmark.h"
#include "texteditorconstants.h"
#include "texteditortr.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <coreplugin/session.h>

#include <utils/algorithm.h>
#include <utils/icon.h>
#include <utils/qtcassert.h>
#include <utils/checkablemessagebox.h>
#include <utils/theme/theme.h>
#include <utils/dropsupport.h>

#include <QAction>
#include <QContextMenuEvent>
#include <QDebug>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QMenu>
#include <QPainter>
#include <QSpinBox>
#include <QToolButton>

Q_DECLARE_METATYPE(TextEditor::Internal::Bookmark*)

using namespace Core;
using namespace Utils;

namespace TextEditor::Internal {

const char BOOKMARKS_CONTEXT[]            = "Bookmarks";

class BookmarkDelegate : public QStyledItemDelegate
{
public:
    BookmarkDelegate(QObject *parent)
        : QStyledItemDelegate(parent)
    {}

private:
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const final;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const final;
    void generateGradientPixmap(int width, int height, const QColor &color, bool selected) const;

    mutable QPixmap m_normalPixmap;
    mutable QPixmap m_selectedPixmap;
};

QSize BookmarkDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    QFontMetrics fm(option.font);
    QSize s;
    s.setWidth(option.rect.width());
    s.setHeight(fm.height() * 2 + 10);
    return s;
}

void BookmarkDelegate::generateGradientPixmap(int width, int height, const QColor &color, bool selected) const
{
    QColor c = color;
    c.setAlpha(0);

    QPixmap pixmap(width+1, height);
    pixmap.fill(c);

    QPainter painter(&pixmap);
    painter.setPen(Qt::NoPen);

    QLinearGradient lg;
    lg.setCoordinateMode(QGradient::ObjectBoundingMode);
    lg.setFinalStop(1,0);

    lg.setColorAt(0, c);
    lg.setColorAt(0.4, color);

    painter.setBrush(lg);
    painter.drawRect(0, 0, width+1, height);

    if (selected)
        m_selectedPixmap = pixmap;
    else
        m_normalPixmap = pixmap;
}

void BookmarkDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);
    painter->save();

    QFontMetrics fm(opt.font);
    static int lwidth = fm.horizontalAdvance(QLatin1String("8888")) + 18;

    QColor backgroundColor;
    QColor textColor;

    bool selected = opt.state & QStyle::State_Selected;

    if (selected) {
        painter->setBrush(opt.palette.highlight().color());
        backgroundColor = opt.palette.highlight().color();
        if (!m_selectedPixmap)
            generateGradientPixmap(lwidth, fm.height()+1, backgroundColor, selected);
    } else {
        backgroundColor = opt.palette.window().color();
        painter->setBrush(backgroundColor);
        if (!m_normalPixmap)
            generateGradientPixmap(lwidth, fm.height(), backgroundColor, selected);
    }
    painter->setPen(Qt::NoPen);
    painter->drawRect(opt.rect);

    // Set Text Color
    if (opt.state & QStyle::State_Selected)
        textColor = opt.palette.highlightedText().color();
    else
        textColor = opt.palette.text().color();

    painter->setPen(textColor);


    // TopLeft
    QString topLeft = index.data(BookmarkManager::Filename).toString();
    painter->drawText(6, 2 + opt.rect.top() + fm.ascent(), topLeft);

    QString topRight = index.data(BookmarkManager::LineNumber).toString();
    // Check whether we need to be fancy and paint some background
    int fwidth = fm.horizontalAdvance(topLeft);
    if (fwidth + lwidth > opt.rect.width()) {
        int left = opt.rect.right() - lwidth;
        painter->drawPixmap(left, opt.rect.top(), selected ? m_selectedPixmap : m_normalPixmap);
    }
    // topRight
    painter->drawText(opt.rect.right() - fm.horizontalAdvance(topRight) - 6,
                      2 + opt.rect.top() + fm.ascent(), topRight);

    // Directory
    QColor mix;
    mix.setRgbF(0.7 * textColor.redF()   + 0.3 * backgroundColor.redF(),
                0.7 * textColor.greenF() + 0.3 * backgroundColor.greenF(),
                0.7 * textColor.blueF()  + 0.3 * backgroundColor.blueF());
    painter->setPen(mix);

    QString lineText = index.data(BookmarkManager::Note).toString().trimmed();
    if (lineText.isEmpty())
        lineText = index.data(BookmarkManager::LineText).toString();

    painter->drawText(6, opt.rect.top() + fm.ascent() + fm.height() + 6, lineText);

    // Separator lines
    const QRectF innerRect = QRectF(opt.rect).adjusted(0.5, 0.5, -0.5, -0.5);
    painter->setPen(QColor::fromRgb(150,150,150));
    painter->drawLine(innerRect.bottomLeft(), innerRect.bottomRight());
    painter->restore();
}

// BookmarkView

class BookmarkView final : public Utils::ListView
{
public:
    explicit BookmarkView(BookmarkManager *manager);

    QList<QToolButton *> createToolBarWidgets();

    void gotoBookmark(const QModelIndex &index);

    void removeFromContextMenu();
    void removeAll();

protected:
    void contextMenuEvent(QContextMenuEvent *event) final;
    void removeBookmark(const QModelIndex &index);
    void keyPressEvent(QKeyEvent *event) final;

private:
    Core::IContext *m_bookmarkContext;
    QModelIndex m_contextMenuIndex;
    BookmarkManager *m_manager;
};

BookmarkView::BookmarkView(BookmarkManager *manager)  :
    m_bookmarkContext(new IContext(this)),
    m_manager(manager)
{
    setWindowTitle(Tr::tr("Bookmarks"));

    m_bookmarkContext->setWidget(this);
    m_bookmarkContext->setContext(Context(BOOKMARKS_CONTEXT));

    ICore::addContextObject(m_bookmarkContext);

    ListView::setModel(manager);

    setItemDelegate(new BookmarkDelegate(this));
    setFrameStyle(QFrame::NoFrame);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setAttribute(Qt::WA_MacShowFocusRect, false);
    setSelectionModel(manager->selectionModel());
    setSelectionMode(QAbstractItemView::SingleSelection);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setDragEnabled(true);
    setDragDropMode(QAbstractItemView::DragDrop);

    connect(this, &QAbstractItemView::doubleClicked, this, &BookmarkView::gotoBookmark);
    connect(this, &QAbstractItemView::activated, this, &BookmarkView::gotoBookmark);
}

QList<QToolButton *> BookmarkView::createToolBarWidgets()
{
    Command *prevCmd = ActionManager::command(TextEditor::Constants::BOOKMARKS_PREV_ACTION);
    Command *nextCmd = ActionManager::command(TextEditor::Constants::BOOKMARKS_NEXT_ACTION);
    QTC_ASSERT(prevCmd && nextCmd, return {});
    auto prevButton = new QToolButton(this);
    prevButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    prevButton->setDefaultAction(prevCmd->action());
    auto nextButton = new QToolButton(this);
    nextButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    nextButton->setDefaultAction(nextCmd->action());
    return {prevButton, nextButton};
}

void BookmarkView::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu;
    QAction *moveUp = menu.addAction(Tr::tr("Move Up"));
    QAction *moveDown = menu.addAction(Tr::tr("Move Down"));
    QAction *edit = menu.addAction(Tr::tr("&Edit"));
    menu.addSeparator();
    QAction *remove = menu.addAction(Tr::tr("&Remove"));
    menu.addSeparator();
    QAction *removeAll = menu.addAction(Tr::tr("Remove All"));

    m_contextMenuIndex = indexAt(event->pos());
    if (!m_contextMenuIndex.isValid()) {
        moveUp->setEnabled(false);
        moveDown->setEnabled(false);
        remove->setEnabled(false);
        edit->setEnabled(false);
    }

    if (model()->rowCount() == 0)
        removeAll->setEnabled(false);

    connect(moveUp, &QAction::triggered, m_manager, &BookmarkManager::moveUp);
    connect(moveDown, &QAction::triggered, m_manager, &BookmarkManager::moveDown);
    connect(remove, &QAction::triggered, this, &BookmarkView::removeFromContextMenu);
    connect(removeAll, &QAction::triggered, this, &BookmarkView::removeAll);
    connect(edit, &QAction::triggered, m_manager, &BookmarkManager::edit);

    menu.exec(mapToGlobal(event->pos()));
}

void BookmarkView::removeFromContextMenu()
{
    removeBookmark(m_contextMenuIndex);
}

void BookmarkView::removeBookmark(const QModelIndex& index)
{
    Bookmark *bm = m_manager->bookmarkForIndex(index);
    m_manager->deleteBookmark(bm);
}

void BookmarkView::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
        removeBookmark(currentIndex());
        event->accept();
        return;
    }
    ListView::keyPressEvent(event);
}

void BookmarkView::removeAll()
{
    if (CheckableMessageBox::question(this,
                                      Tr::tr("Remove All Bookmarks"),
                                      Tr::tr("Are you sure you want to remove all bookmarks from "
                                             "all files in the current session?"),
                                      Key("RemoveAllBookmarks"))
        != QMessageBox::Yes)
        return;

    // The performance of this function could be greatly improved.
    while (m_manager->rowCount()) {
        QModelIndex index = m_manager->index(0, 0);
        removeBookmark(index);
    }
}

void BookmarkView::gotoBookmark(const QModelIndex &index)
{
    if (Bookmark *bk = m_manager->bookmarkForIndex(index)) {
        if (!m_manager->gotoBookmark(bk))
            m_manager->deleteBookmark(bk);
    }
}

////
// BookmarkManager
////

BookmarkManager::BookmarkManager() :
    m_selectionModel(new QItemSelectionModel(this, this))
{
    connect(ICore::instance(), &ICore::contextChanged,
            this, &BookmarkManager::updateActionStatus);

    connect(SessionManager::instance(), &SessionManager::sessionLoaded,
            this, &BookmarkManager::loadBookmarks);

    updateActionStatus();
}

BookmarkManager::~BookmarkManager()
{
    qDeleteAll(m_bookmarksList);
}

QItemSelectionModel *BookmarkManager::selectionModel() const
{
    return m_selectionModel;
}

bool BookmarkManager::hasBookmarkInPosition(const Utils::FilePath &fileName, int lineNumber)
{
    return findBookmark(fileName, lineNumber);
}

QModelIndex BookmarkManager::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid())
        return QModelIndex();
    else
        return createIndex(row, column);
}

QModelIndex BookmarkManager::parent(const QModelIndex &) const
{
    return QModelIndex();
}

int BookmarkManager::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    else
        return m_bookmarksList.count();
}

int BookmarkManager::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return 3;
}

QVariant BookmarkManager::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.column() !=0 || index.row() < 0 || index.row() >= m_bookmarksList.count())
        return QVariant();

    Bookmark *bookMark = m_bookmarksList.at(index.row());
    if (role == BookmarkManager::Filename)
        return bookMark->filePath().fileName();
    if (role == BookmarkManager::LineNumber)
        return bookMark->lineNumber();
    if (role == BookmarkManager::Directory)
        return bookMark->filePath().toFileInfo().path();
    if (role == BookmarkManager::LineText)
        return bookMark->lineText();
    if (role == BookmarkManager::Note)
        return bookMark->note();
    if (role == Qt::ToolTipRole)
        return bookMark->filePath().toUserOutput();
    return QVariant();
}

Qt::ItemFlags BookmarkManager::flags(const QModelIndex &index) const
{
    if (!index.isValid() || index.column() !=0 || index.row() < 0 || index.row() >= m_bookmarksList.count())
        return Qt::NoItemFlags;
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
}

Qt::DropActions BookmarkManager::supportedDragActions() const
{
    return Qt::MoveAction;
}

QStringList BookmarkManager::mimeTypes() const
{
    return DropSupport::mimeTypesForFilePaths();
}

QMimeData *BookmarkManager::mimeData(const QModelIndexList &indexes) const
{
    auto data = new DropMimeData;
    for (const QModelIndex &index : indexes) {
        if (!index.isValid() || index.column() != 0 || index.row() < 0 || index.row() >= m_bookmarksList.count())
            continue;
        Bookmark *bookMark = m_bookmarksList.at(index.row());
        data->addFile(bookMark->filePath(), bookMark->lineNumber());
        data->addValue(QVariant::fromValue(bookMark));
    }
    return data;
}

Qt::DropActions BookmarkManager::supportedDropActions() const
{
    return Qt::MoveAction;
}

bool BookmarkManager::canDropMimeData(const QMimeData *data, Qt::DropAction action,
                                      int row, int column,
                                      const QModelIndex &parent) const
{
    Q_UNUSED(row);
    Q_UNUSED(column);
    Q_UNUSED(parent);

    if (!(action & supportedDropActions()))
        return false;

    const DropMimeData* customData = qobject_cast<const DropMimeData*>(data);
    if (!customData)
        return false;

    return true;
}

bool BookmarkManager::dropMimeData(const QMimeData *data, Qt::DropAction action,
                                   int row, int column, const QModelIndex &parent)
{
    Q_UNUSED(column);

    if (!(action & supportedDropActions()))
        return false;

    const DropMimeData* customData = qobject_cast<const DropMimeData*>(data);
    if (!customData)
        return false;

    row = parent.row();
    if (row >= m_bookmarksList.size())
        row = m_bookmarksList.size() - 1;
    if (row == -1)
        row = m_bookmarksList.size() - 1;

    const QList<QVariant> values = customData->values();
    for (const QVariant &value : values) {
        auto mark = value.value<Bookmark*>();
        if (mark)
            move(mark, row);
    }

    return true;
}

void BookmarkManager::toggleBookmark(const FilePath &fileName, int lineNumber)
{
    if (lineNumber <= 0 || fileName.isEmpty())
        return;

    // Remove any existing bookmark on this line
    if (Bookmark *mark = findBookmark(fileName, lineNumber)) {
        // TODO check if the bookmark is really on the same markable Interface
        deleteBookmark(mark);
        return;
    }

    // Add a new bookmark if no bookmark existed on this line
    auto mark = new Bookmark(lineNumber, this);
    mark->updateFilePath(fileName);
    const QModelIndex currentIndex = selectionModel()->currentIndex();
    const int insertionIndex = currentIndex.isValid() ? currentIndex.row() + 1
                                                      : m_bookmarksList.size();
    insertBookmark(insertionIndex, mark);
}

void BookmarkManager::updateBookmark(Bookmark *bookmark)
{
    const int idx = m_bookmarksList.indexOf(bookmark);
    if (idx == -1)
        return;

    emit dataChanged(index(idx, 0, QModelIndex()), index(idx, 2, QModelIndex()));
    saveBookmarks();
}

void BookmarkManager::updateBookmarkFileName(Bookmark *bookmark, const FilePath &oldFilePath)
{
    if (oldFilePath == bookmark->filePath())
        return;

    m_bookmarksMap[oldFilePath].removeAll(bookmark);
    m_bookmarksMap[bookmark->filePath()].append(bookmark);
    updateBookmark(bookmark);
}

void BookmarkManager::removeAllBookmarks()
{
    if (m_bookmarksList.isEmpty())
        return;
    beginRemoveRows(QModelIndex(), 0, m_bookmarksList.size() - 1);

    qDeleteAll(m_bookmarksList);
    m_bookmarksMap.clear();
    m_bookmarksList.clear();
    endRemoveRows();
}

void BookmarkManager::deleteBookmark(Bookmark *bookmark)
{
    int idx = m_bookmarksList.indexOf(bookmark);
    beginRemoveRows(QModelIndex(), idx, idx);

    m_bookmarksMap[bookmark->filePath()].removeAll(bookmark);
    delete bookmark;

    m_bookmarksList.removeAt(idx);
    endRemoveRows();

    if (selectionModel()->currentIndex().isValid())
        selectionModel()->setCurrentIndex(selectionModel()->currentIndex(), QItemSelectionModel::Select | QItemSelectionModel::Clear);

    updateActionStatus();
    saveBookmarks();
}

Bookmark *BookmarkManager::bookmarkForIndex(const QModelIndex &index) const
{
    if (!index.isValid() || index.row() >= m_bookmarksList.size())
        return nullptr;
    return m_bookmarksList.at(index.row());
}

bool BookmarkManager::gotoBookmark(const Bookmark *bookmark) const
{
    QTC_ASSERT(bookmark, return false);
    if (IEditor *editor = EditorManager::openEditorAt(
            Utils::Link(bookmark->filePath(), bookmark->lineNumber()))) {
        return editor->currentLine() == bookmark->lineNumber();
    }
    return false;
}

void BookmarkManager::nextInDocument()
{
    documentPrevNext(true);
}

void BookmarkManager::prevInDocument()
{
    documentPrevNext(false);
}

void BookmarkManager::documentPrevNext(bool next)
{
    IEditor *editor = EditorManager::currentEditor();
    const int editorLine = editor->currentLine();
    if (editorLine <= 0)
        return;

    const FilePath filePath = editor->document()->filePath();
    if (!m_bookmarksMap.contains(filePath))
        return;

    int firstLine = -1;
    int lastLine = -1;
    int prevLine = -1;
    int nextLine = -1;
    const QVector<Bookmark *> marks = m_bookmarksMap[filePath];
    for (int i = 0; i < marks.count(); ++i) {
        int markLine = marks.at(i)->lineNumber();
        if (firstLine == -1 || firstLine > markLine)
            firstLine = markLine;
        if (lastLine < markLine)
            lastLine = markLine;
        if (markLine < editorLine && prevLine < markLine)
            prevLine = markLine;
        if (markLine > editorLine &&
            (nextLine == -1 || nextLine > markLine))
            nextLine = markLine;
    }

    EditorManager::addCurrentPositionToNavigationHistory();
    if (next) {
        if (nextLine == -1)
            editor->gotoLine(firstLine);
        else
            editor->gotoLine(nextLine);
    } else {
        if (prevLine == -1)
            editor->gotoLine(lastLine);
        else
            editor->gotoLine(prevLine);
    }
}

void BookmarkManager::next()
{
    QModelIndex current = selectionModel()->currentIndex();
    if (!current.isValid())
        return;
    int row = current.row();
    ++row;
    while (true) {
        if (row == m_bookmarksList.size())
            row = 0;

        Bookmark *bk = m_bookmarksList.at(row);
        if (gotoBookmark(bk)) {
            QModelIndex newIndex = current.sibling(row, current.column());
            selectionModel()->setCurrentIndex(newIndex, QItemSelectionModel::Select | QItemSelectionModel::Clear);
            return;
        }
        deleteBookmark(bk);
        if (m_bookmarksList.isEmpty()) // No bookmarks anymore ...
            return;
    }
}

void BookmarkManager::prev()
{
    QModelIndex current = selectionModel()->currentIndex();
    if (!current.isValid())
        return;
    if (!isAtCurrentBookmark() && gotoBookmark(bookmarkForIndex(current)))
        return;
    int row = current.row();
    while (true) {
        if (row == 0)
            row = m_bookmarksList.size();
        --row;
        Bookmark *bk = m_bookmarksList.at(row);
        if (gotoBookmark(bk)) {
            QModelIndex newIndex = current.sibling(row, current.column());
            selectionModel()->setCurrentIndex(newIndex, QItemSelectionModel::Select | QItemSelectionModel::Clear);
            return;
        }
        deleteBookmark(bk);
        if (m_bookmarksList.isEmpty())
            return;
    }
}

BookmarkManager::State BookmarkManager::state() const
{
    if (m_bookmarksList.empty())
        return NoBookMarks;

    IEditor *editor = EditorManager::currentEditor();
    if (!editor)
        return HasBookMarks;

    return m_bookmarksMap.value(editor->document()->filePath()).isEmpty() ? HasBookMarks
                                                                          : HasBookmarksInDocument;
}

void BookmarkManager::updateActionStatus()
{
    IEditor *editor = EditorManager::currentEditor();
    const bool enableToggle = editor && !editor->document()->isTemporary();

    emit updateActions(enableToggle, state());
}

void BookmarkManager::move(Bookmark* mark, int newRow)
{
    int currentRow = m_bookmarksList.indexOf(mark);
    if (newRow <= currentRow)
        ++currentRow;
    else
        ++newRow;
    m_bookmarksList.insert(newRow, mark);
    m_bookmarksList.removeAt(currentRow);

    QModelIndex current = selectionModel()->currentIndex();
    QModelIndex topLeft = current.sibling(std::min(newRow, currentRow), 0);
    QModelIndex bottomRight = current.sibling(std::max(newRow, currentRow), 2);
    emit dataChanged(topLeft, bottomRight);
    selectionModel()->setCurrentIndex(topLeft, QItemSelectionModel::Select | QItemSelectionModel::Clear);

    saveBookmarks();
}

void BookmarkManager::moveUp()
{
    QModelIndex current = selectionModel()->currentIndex();
    int row = current.row();
    if (row == 0)
        row = m_bookmarksList.size();
     --row;

    // swap current.row() and row

    Bookmark *b = m_bookmarksList.at(row);
    m_bookmarksList[row] = m_bookmarksList.at(current.row());
    m_bookmarksList[current.row()] = b;

    QModelIndex topLeft = current.sibling(row, 0);
    QModelIndex bottomRight = current.sibling(current.row(), 2);
    emit dataChanged(topLeft, bottomRight);
    selectionModel()->setCurrentIndex(current.sibling(row, 0), QItemSelectionModel::Select | QItemSelectionModel::Clear);

    saveBookmarks();
}

void BookmarkManager::moveDown()
{
    QModelIndex current = selectionModel()->currentIndex();
    int row = current.row();
    ++row;
    if (row == m_bookmarksList.size())
        row = 0;

    // swap current.row() and row
    Bookmark *b = m_bookmarksList.at(row);
    m_bookmarksList[row] = m_bookmarksList.at(current.row());
    m_bookmarksList[current.row()] = b;

    QModelIndex topLeft = current.sibling(current.row(), 0);
    QModelIndex bottomRight = current.sibling(row, 2);
    emit dataChanged(topLeft, bottomRight);
    selectionModel()->setCurrentIndex(current.sibling(row, 0), QItemSelectionModel::Select | QItemSelectionModel::Clear);

    saveBookmarks();
}

void BookmarkManager::editByFileAndLine(const FilePath &fileName, int lineNumber)
{
    Bookmark *b = findBookmark(fileName, lineNumber);
    QModelIndex current = selectionModel()->currentIndex();
    selectionModel()->setCurrentIndex(current.sibling(m_bookmarksList.indexOf(b), 0),
                                      QItemSelectionModel::Select | QItemSelectionModel::Clear);

    edit();
}

void BookmarkManager::edit()
{
    QModelIndex current = selectionModel()->currentIndex();
    Bookmark *b = m_bookmarksList.at(current.row());

    QDialog dlg;
    dlg.setWindowTitle(Tr::tr("Edit Bookmark"));
    auto layout = new QFormLayout(&dlg);
    auto noteEdit = new QLineEdit(b->note());
    noteEdit->setMinimumWidth(300);
    auto lineNumberSpinbox = new QSpinBox;
    lineNumberSpinbox->setRange(1, INT_MAX);
    lineNumberSpinbox->setValue(b->lineNumber());
    lineNumberSpinbox->setMaximumWidth(100);
    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    layout->addRow(Tr::tr("Note text:"), noteEdit);
    layout->addRow(Tr::tr("Line number:"), lineNumberSpinbox);
    layout->addWidget(buttonBox);
    if (dlg.exec() == QDialog::Accepted) {
        b->move(lineNumberSpinbox->value());
        b->updateNote(noteEdit->text().replace(QLatin1Char('\t'), QLatin1Char(' ')));
        emit dataChanged(current, current);
        saveBookmarks();
    }
}

/* Returns the bookmark at the given file and line number, or 0 if no such bookmark exists. */
Bookmark *BookmarkManager::findBookmark(const FilePath &filePath, int lineNumber)
{
    return Utils::findOrDefault(m_bookmarksMap.value(filePath),
                                Utils::equal(&Bookmark::lineNumber, lineNumber));
}

void BookmarkManager::insertBookmark(int idx, Bookmark *bookmark, bool userset)
{
    idx = qBound(0, idx, m_bookmarksList.size());
    beginInsertRows(QModelIndex(), idx, idx);

    m_bookmarksMap[bookmark->filePath()].append(bookmark);
    m_bookmarksList.insert(idx, bookmark);

    endInsertRows();
    if (userset) {
        updateActionStatus();
        saveBookmarks();
    }
    selectionModel()->setCurrentIndex(index(idx, 0, QModelIndex()),
                                      QItemSelectionModel::Select | QItemSelectionModel::Clear);
}

/* Adds a bookmark to the internal data structures. The 'userset' parameter
 * determines whether action status should be updated and whether the bookmarks
 * should be saved to the session settings.
 */
void BookmarkManager::addBookmark(Bookmark *bookmark, bool userset)
{
    insertBookmark(m_bookmarksList.size(), bookmark, userset);
}

/* Adds a new bookmark based on information parsed from the string. */
void BookmarkManager::addBookmark(const QString &s)
{
    // index3 is a frontier beetween note text and other bookmarks data
    int index3 = s.lastIndexOf(QLatin1Char('\t'));
    if (index3 < 0)
        index3 = s.size();
    int index2 = s.lastIndexOf(QLatin1Char(':'), index3 - 1);
    int index1 = s.indexOf(QLatin1Char(':'));

    if (index3 != -1 || index2 != -1 || index1 != -1) {
        const QString &filePath = s.mid(index1+1, index2-index1-1);
        const QString &note = s.mid(index3 + 1);
        const int lineNumber = s.mid(index2 + 1, index3 - index2 - 1).toInt();
        if (!filePath.isEmpty() && !findBookmark(FilePath::fromString(filePath), lineNumber)) {
            auto b = new Bookmark(lineNumber, this);
            b->updateFilePath(FilePath::fromString(filePath));
            b->setNote(note);
            addBookmark(b, false);
        }
    } else {
        qDebug() << "BookmarkManager::addBookmark() Invalid bookmark string:" << s;
    }
}

/* Puts the bookmark in a string for storing it in the settings. */
QString BookmarkManager::bookmarkToString(const Bookmark *b)
{
    const QLatin1Char colon(':');
    // Using \t as delimiter because any another symbol can be a part of note.
    const QLatin1Char noteDelimiter('\t');
    return colon + b->filePath().toString() +
            colon + QString::number(b->lineNumber()) +
            noteDelimiter + b->note();
}

/* Saves the bookmarks to the session settings. */
void BookmarkManager::saveBookmarks()
{
    QStringList list;
    for (const Bookmark *bookmark : std::as_const(m_bookmarksList))
        list << bookmarkToString(bookmark);

    SessionManager::setValue("Bookmarks", list);
}

/* Loads the bookmarks from the session settings. */
void BookmarkManager::loadBookmarks()
{
    removeAllBookmarks();
    const QStringList &list = SessionManager::value("Bookmarks").toStringList();
    for (const QString &bookmarkString : list)
        addBookmark(bookmarkString);

    updateActionStatus();
}

bool BookmarkManager::isAtCurrentBookmark() const
{
    Bookmark *bk = bookmarkForIndex(selectionModel()->currentIndex());
    if (!bk)
        return true;
    IEditor *currentEditor = EditorManager::currentEditor();
    return currentEditor
           && currentEditor->document()->filePath() == bk->filePath()
           && currentEditor->currentLine() == bk->lineNumber();
}

// BookmarkViewFactory

BookmarkViewFactory::BookmarkViewFactory(BookmarkManager *bm)
    : m_manager(bm)
{
    setDisplayName(Tr::tr("Bookmarks"));
    setPriority(300);
    setId("Bookmarks");
    setActivationSequence(QKeySequence(useMacShortcuts ? Tr::tr("Alt+Meta+M") : Tr::tr("Alt+M")));
}

NavigationView BookmarkViewFactory::createWidget()
{
    auto view = new BookmarkView(m_manager);
    view->setActivationMode(Utils::DoubleClickActivation); // QUESTION: is this useful ?
    return {view, view->createToolBarWidgets()};
}

} // TextEditor::Internal
