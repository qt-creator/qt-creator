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

#include "bookmarkmanager.h"

#include "bookmark.h"
#include "bookmarksplugin.h"
#include "bookmarks_global.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>
#include <texteditor/basetexteditor.h>
#include <utils/tooltip/tooltip.h>
#include <utils/tooltip/tipcontents.h>
#include <utils/qtcassert.h>

#include <QDebug>
#include <QDir>
#include <QFileInfo>

#include <QAction>
#include <QContextMenuEvent>
#include <QMenu>
#include <QPainter>
#include <QInputDialog>

Q_DECLARE_METATYPE(Bookmarks::Internal::Bookmark*)

using namespace Bookmarks;
using namespace Bookmarks::Internal;
using namespace ProjectExplorer;
using namespace Core;

BookmarkDelegate::BookmarkDelegate(QObject *parent)
    : QStyledItemDelegate(parent), m_normalPixmap(0), m_selectedPixmap(0)
{
}

BookmarkDelegate::~BookmarkDelegate()
{
    delete m_normalPixmap;
    delete m_selectedPixmap;
}

QSize BookmarkDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItemV4 opt = option;
    initStyleOption(&opt, index);

    QFontMetrics fm(option.font);
    QSize s;
    s.setWidth(option.rect.width());
    s.setHeight(fm.height() * 2 + 10);
    return s;
}

void BookmarkDelegate::generateGradientPixmap(int width, int height, QColor color, bool selected) const
{

    QColor c = color;
    c.setAlpha(0);

    QPixmap *pixmap = new QPixmap(width+1, height);
    pixmap->fill(c);

    QPainter painter(pixmap);
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
    QStyleOptionViewItemV4 opt = option;
    initStyleOption(&opt, index);
    painter->save();

    QFontMetrics fm(opt.font);
    static int lwidth = fm.width(QLatin1String("8888")) + 18;

    QColor backgroundColor;
    QColor textColor;

    bool selected = opt.state & QStyle::State_Selected;

    if (selected) {
        painter->setBrush(opt.palette.highlight().color());
        backgroundColor = opt.palette.highlight().color();
        if (!m_selectedPixmap)
            generateGradientPixmap(lwidth, fm.height()+1, backgroundColor, selected);
    } else {
        painter->setBrush(opt.palette.background().color());
        backgroundColor = opt.palette.background().color();
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
    QString topLeft = index.data(BookmarkManager::Filename ).toString();
    painter->drawText(6, 2 + opt.rect.top() + fm.ascent(), topLeft);

    QString topRight = index.data(BookmarkManager::LineNumber).toString();
    // Check whether we need to be fancy and paint some background
    int fwidth = fm.width(topLeft);
    if (fwidth + lwidth > opt.rect.width()) {
        int left = opt.rect.right() - lwidth;
        painter->drawPixmap(left, opt.rect.top(), selected? *m_selectedPixmap : *m_normalPixmap);
    }
    // topRight
    painter->drawText(opt.rect.right() - fm.width(topRight) - 6 , 2 + opt.rect.top() + fm.ascent(), topRight);

    // Directory
    QColor mix;
    mix.setRgbF(0.7 * textColor.redF()   + 0.3 * backgroundColor.redF(),
                0.7 * textColor.greenF() + 0.3 * backgroundColor.greenF(),
                0.7 * textColor.blueF()  + 0.3 * backgroundColor.blueF());
    painter->setPen(mix);
//
//    QString directory = index.data(BookmarkManager::Directory).toString();
//    int availableSpace = opt.rect.width() - 12;
//    if (fm.width(directory) > availableSpace) {
//        // We need a shorter directory
//        availableSpace -= fm.width("...");
//
//        int pos = directory.size();
//        int idx;
//        forever {
//            idx = directory.lastIndexOf("/", pos-1);
//            if (idx == -1) {
//                // Can't happen, this means the string did fit after all?
//                break;
//            }
//            int width = fm.width(directory.mid(idx, pos-idx));
//            if (width > availableSpace) {
//                directory = "..." + directory.mid(pos);
//                break;
//            } else {
//                pos = idx;
//                availableSpace -= width;
//            }
//        }
//    }
//
//    painter->drawText(3, opt.rect.top() + fm.ascent() + fm.height() + 6, directory);

    QString lineText = index.data(BookmarkManager::Note).toString().trimmed();
    if (lineText.isEmpty())
        lineText = index.data(BookmarkManager::LineText).toString().trimmed();

    painter->drawText(6, opt.rect.top() + fm.ascent() + fm.height() + 6, lineText);

    // Separator lines
    painter->setPen(QColor::fromRgb(150,150,150));
    painter->drawLine(0, opt.rect.bottom(), opt.rect.right(), opt.rect.bottom());
    painter->restore();
}

BookmarkView::BookmarkView(QWidget *parent)  :
    QListView(parent),
    m_bookmarkContext(new BookmarkContext(this)),
    m_manager(0)
{
    setWindowTitle(tr("Bookmarks"));

    connect(this, SIGNAL(clicked(QModelIndex)),
            this, SLOT(gotoBookmark(QModelIndex)));

    ICore::addContextObject(m_bookmarkContext);

    setItemDelegate(new BookmarkDelegate(this));
    setFrameStyle(QFrame::NoFrame);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setFocusPolicy(Qt::NoFocus);
}

BookmarkView::~BookmarkView()
{
    ICore::removeContextObject(m_bookmarkContext);
}

void BookmarkView::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu;
    QAction *moveUp = menu.addAction(tr("Move Up"));
    QAction *moveDown = menu.addAction(tr("Move Down"));
    QAction *remove = menu.addAction(tr("&Remove"));
    QAction *removeAll = menu.addAction(tr("Remove All"));
    QAction *editNote = menu.addAction(tr("Edit note"));

    m_contextMenuIndex = indexAt(event->pos());
    if (!m_contextMenuIndex.isValid()) {
        moveUp->setEnabled(false);
        moveDown->setEnabled(false);
        remove->setEnabled(false);
    }

    if (model()->rowCount() == 0)
        removeAll->setEnabled(false);

    connect(moveUp, SIGNAL(triggered()),
            m_manager, SLOT(moveUp()));
    connect(moveDown, SIGNAL(triggered()),
            m_manager, SLOT(moveDown()));
    connect(remove, SIGNAL(triggered()),
            this, SLOT(removeFromContextMenu()));
    connect(removeAll, SIGNAL(triggered()),
            this, SLOT(removeAll()));
    connect(editNote, SIGNAL(triggered()),
            m_manager, SLOT(editNote()));

    menu.exec(mapToGlobal(event->pos()));
}

void BookmarkView::removeFromContextMenu()
{
    removeBookmark(m_contextMenuIndex);
}

void BookmarkView::removeBookmark(const QModelIndex& index)
{
    Bookmark *bm = m_manager->bookmarkForIndex(index);
    m_manager->removeBookmark(bm);
}

// The perforcemance of this function could be greatly improved.
//
void BookmarkView::removeAll()
{
    while (m_manager->rowCount()) {
        QModelIndex index = m_manager->index(0, 0);
        removeBookmark(index);
    }
}

void BookmarkView::setModel(QAbstractItemModel *model)
{
    BookmarkManager *manager = qobject_cast<BookmarkManager *>(model);
    QTC_ASSERT(manager, return);
    m_manager = manager;
    QListView::setModel(model);
    setSelectionModel(manager->selectionModel());
    setSelectionMode(QAbstractItemView::SingleSelection);
    setSelectionBehavior(QAbstractItemView::SelectRows);
}

void BookmarkView::gotoBookmark(const QModelIndex &index)
{
    Bookmark *bk = m_manager->bookmarkForIndex(index);
    if (!m_manager->gotoBookmark(bk))
        m_manager->removeBookmark(bk);
}

////
// BookmarkContext
////

BookmarkContext::BookmarkContext(QWidget *widget)
    : Core::IContext(widget)
{
      setWidget(widget);
      setContext(Core::Context(Constants::BOOKMARKS_CONTEXT));
}

////
// BookmarkManager
////

BookmarkManager::BookmarkManager() :
    m_bookmarkIcon(QLatin1String(":/bookmarks/images/bookmark.png")),
    m_selectionModel(new QItemSelectionModel(this, this))
{
    connect(Core::ICore::instance(), SIGNAL(contextChanged(Core::IContext*,Core::Context)),
            this, SLOT(updateActionStatus()));

    connect(ProjectExplorerPlugin::instance()->session(), SIGNAL(sessionLoaded(QString)),
            this, SLOT(loadBookmarks()));

    updateActionStatus();
}

BookmarkManager::~BookmarkManager()
{
    DirectoryFileBookmarksMap::iterator it, end;
    end = m_bookmarksMap.end();
    for (it = m_bookmarksMap.begin(); it != end; ++it) {
        FileNameBookmarksMap *bookmarks = it.value();
        qDeleteAll(*bookmarks);
        delete bookmarks;
    }
}

QItemSelectionModel *BookmarkManager::selectionModel() const
{
    return m_selectionModel;
}

bool BookmarkManager::hasBookmarkInPosition(const QString &fileName, int lineNumber)
{
    QFileInfo fi(fileName);
    return findBookmark(fi.path(), fi.fileName(), lineNumber);
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

    if (role == BookmarkManager::Filename)
        return m_bookmarksList.at(index.row())->fileName();
    else if (role == BookmarkManager::LineNumber)
        return m_bookmarksList.at(index.row())->lineNumber();
    else if (role == BookmarkManager::Directory)
        return m_bookmarksList.at(index.row())->path();
    else if (role == BookmarkManager::LineText)
        return m_bookmarksList.at(index.row())->lineText();
    else if (role == BookmarkManager::Note)
        return m_bookmarksList.at(index.row())->note();
    else if (role == Qt::ToolTipRole)
        return QDir::toNativeSeparators(m_bookmarksList.at(index.row())->filePath());

    return QVariant();
}

void BookmarkManager::toggleBookmark()
{
    TextEditor::ITextEditor *editor = currentTextEditor();
    if (!editor)
        return;

    toggleBookmark(editor->document()->fileName(), editor->currentLine());
}

void BookmarkManager::toggleBookmark(const QString &fileName, int lineNumber)
{
    const QFileInfo fi(fileName);
    const int editorLine = lineNumber;

    // Remove any existing bookmark on this line
    if (Bookmark *mark = findBookmark(fi.path(), fi.fileName(), lineNumber)) {
        // TODO check if the bookmark is really on the same markable Interface
        removeBookmark(mark);
        return;
    }

    // Add a new bookmark if no bookmark existed on this line
    Bookmark *bookmark = new Bookmark(fi.filePath(), editorLine, this);
    bookmark->init();
    addBookmark(bookmark);
}

void BookmarkManager::updateBookmark(Bookmark *bookmark)
{
    int idx = m_bookmarksList.indexOf(bookmark);
    emit dataChanged(index(idx, 0, QModelIndex()), index(idx, 2, QModelIndex()));
    saveBookmarks();
}

void BookmarkManager::removeAllBookmarks()
{
    if (m_bookmarksList.isEmpty())
        return;
    beginRemoveRows(QModelIndex(), 0, m_bookmarksList.size() - 1);

    DirectoryFileBookmarksMap::const_iterator it, end;
    end = m_bookmarksMap.constEnd();
    for (it = m_bookmarksMap.constBegin(); it != end; ++it) {
        FileNameBookmarksMap *files = it.value();
        FileNameBookmarksMap::const_iterator jt, jend;
        jend = files->constEnd();
        for (jt = files->constBegin(); jt != jend; ++jt) {
            delete jt.value();
        }
        files->clear();
        delete files;
    }
    m_bookmarksMap.clear();
    m_bookmarksList.clear();
    endRemoveRows();
}

void BookmarkManager::removeBookmark(Bookmark *bookmark)
{
    int idx = m_bookmarksList.indexOf(bookmark);
    beginRemoveRows(QModelIndex(), idx, idx);

    const QFileInfo fi(bookmark->filePath() );
    FileNameBookmarksMap *files = m_bookmarksMap.value(fi.path());

    FileNameBookmarksMap::iterator i = files->begin();
    while (i != files->end()) {
        if (i.value() == bookmark) {
            files->erase(i);
            delete bookmark;
            break;
        }
        ++i;
    }
    if (files->count() <= 0) {
        m_bookmarksMap.remove(fi.path());
        delete files;
    }


    m_bookmarksList.removeAt(idx);
    endRemoveRows();

    if (selectionModel()->currentIndex().isValid())
        selectionModel()->setCurrentIndex(selectionModel()->currentIndex(), QItemSelectionModel::Select | QItemSelectionModel::Clear);

    updateActionStatus();
    saveBookmarks();
}

Bookmark *BookmarkManager::bookmarkForIndex(const QModelIndex &index)
{
    if (!index.isValid() || index.row() >= m_bookmarksList.size())
        return 0;
    return m_bookmarksList.at(index.row());
}


bool BookmarkManager::gotoBookmark(Bookmark *bookmark)
{
    using namespace TextEditor;
    if (ITextEditor *editor = qobject_cast<ITextEditor *>(BaseTextEditorWidget::openEditorAt(bookmark->filePath(),
                                                                                             bookmark->lineNumber()))) {
        return (editor->currentLine() == bookmark->lineNumber());
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
    TextEditor::ITextEditor *editor = currentTextEditor();
    int editorLine = editor->currentLine();
    QFileInfo fi(editor->document()->fileName());
    if (!m_bookmarksMap.contains(fi.path()))
        return;

    int firstLine = -1;
    int lastLine = -1;
    int prevLine = -1;
    int nextLine = -1;
    const QList<Bookmark *> marks = m_bookmarksMap.value(fi.path())->values(fi.fileName());
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

    Core::EditorManager *em = Core::EditorManager::instance();
    em->addCurrentPositionToNavigationHistory();
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
        removeBookmark(bk);
        if (m_bookmarksList.isEmpty()) // No bookmarks anymore ...
            return;
    }
}

void BookmarkManager::prev()
{
    QModelIndex current = selectionModel()->currentIndex();
    if (!current.isValid())
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
        removeBookmark(bk);
        if (m_bookmarksList.isEmpty())
            return;
    }
}

TextEditor::ITextEditor *BookmarkManager::currentTextEditor() const
{
    Core::IEditor *currEditor = EditorManager::currentEditor();
    return qobject_cast<TextEditor::ITextEditor *>(currEditor);
}

/* Returns the current session. */
SessionManager *BookmarkManager::sessionManager() const
{
    return ProjectExplorerPlugin::instance()->session();
}

BookmarkManager::State BookmarkManager::state() const
{
    if (m_bookmarksMap.empty())
        return NoBookMarks;

    TextEditor::ITextEditor *editor = currentTextEditor();
    if (!editor)
        return HasBookMarks;

    const QFileInfo fi(editor->document()->fileName());

    const DirectoryFileBookmarksMap::const_iterator dit = m_bookmarksMap.constFind(fi.path());
    if (dit == m_bookmarksMap.constEnd())
        return HasBookMarks;

    return HasBookmarksInDocument;
}

void BookmarkManager::updateActionStatus()
{
    emit updateActions(state());
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

void BookmarkManager::editNote(const QString &fileName, int lineNumber)
{
    QFileInfo fi(fileName);
    Bookmark *b = findBookmark(fi.path(), fi.fileName(), lineNumber);
    QModelIndex current = selectionModel()->currentIndex();
    selectionModel()->setCurrentIndex(current.sibling(m_bookmarksList.indexOf(b), 0),
                                      QItemSelectionModel::Select | QItemSelectionModel::Clear);

    editNote();
}

void BookmarkManager::editNote()
{
    QModelIndex current = selectionModel()->currentIndex();
    Bookmark *b = m_bookmarksList.at(current.row());

    bool inputOk = false;
    QString noteText = QInputDialog::getText(0, tr("Edit note"),
                                             tr("Note text:"), QLineEdit::Normal,
                                             b->note(), &inputOk);
    if (inputOk) {
        b->updateNote(noteText.replace(QLatin1Char('\t'), QLatin1Char(' ')));
        emit dataChanged(current, current);
        saveBookmarks();
    }
}

/* Returns the bookmark at the given file and line number, or 0 if no such bookmark exists. */
Bookmark *BookmarkManager::findBookmark(const QString &path, const QString &fileName, int lineNumber)
{
    if (m_bookmarksMap.contains(path)) {
        foreach (Bookmark *bookmark, m_bookmarksMap.value(path)->values(fileName)) {
            if (bookmark->lineNumber() == lineNumber)
                return bookmark;
        }
    }
    return 0;
}

/* Adds a bookmark to the internal data structures. The 'userset' parameter
 * determines whether action status should be updated and whether the bookmarks
 * should be saved to the session settings.
 */
void BookmarkManager::addBookmark(Bookmark *bookmark, bool userset)
{
    beginInsertRows(QModelIndex(), m_bookmarksList.size(), m_bookmarksList.size());
    const QFileInfo fi(bookmark->filePath());
    const QString &path = fi.path();

    if (!m_bookmarksMap.contains(path))
        m_bookmarksMap.insert(path, new FileNameBookmarksMap());
    m_bookmarksMap.value(path)->insert(fi.fileName(), bookmark);

    m_bookmarksList.append(bookmark);

    endInsertRows();
    if (userset) {
        updateActionStatus();
        saveBookmarks();
    }
    selectionModel()->setCurrentIndex(index(m_bookmarksList.size()-1 , 0, QModelIndex()), QItemSelectionModel::Select | QItemSelectionModel::Clear);
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
        const QFileInfo fi(filePath);

        if (!filePath.isEmpty() && !findBookmark(fi.path(), fi.fileName(), lineNumber)) {
            Bookmark *b = new Bookmark(filePath, lineNumber, this);
            b->setNote(note);
            b->init();
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
    // Empty string was the name of the bookmark, which now is always ""
    return QLatin1String("") + colon + b->filePath() +
            colon + QString::number(b->lineNumber()) +
            noteDelimiter + b->note();
}

/* Saves the bookmarks to the session settings. */
void BookmarkManager::saveBookmarks()
{
    QStringList list;
    foreach (const Bookmark *bookmark, m_bookmarksList)
            list << bookmarkToString(bookmark);

    sessionManager()->setValue(QLatin1String("Bookmarks"), list);
}

void BookmarkManager::operateTooltip(TextEditor::ITextEditor *textEditor, const QPoint &pos, Bookmark *mark)
{
    if (!mark)
        return;

    if (mark->note().isEmpty())
        Utils::ToolTip::instance()->hide();
    else
        Utils::ToolTip::instance()->show(pos, Utils::TextContent(mark->note()), textEditor->widget());
}

/* Loads the bookmarks from the session settings. */
void BookmarkManager::loadBookmarks()
{
    removeAllBookmarks();
    const QStringList &list = sessionManager()->value(QLatin1String("Bookmarks")).toStringList();
    foreach (const QString &bookmarkString, list)
        addBookmark(bookmarkString);

    updateActionStatus();
}

void BookmarkManager::handleBookmarkRequest(TextEditor::ITextEditor *textEditor,
                                            int line,
                                            TextEditor::ITextEditor::MarkRequestKind kind)
{
    if (kind == TextEditor::ITextEditor::BookmarkRequest && textEditor->document())
        toggleBookmark(textEditor->document()->fileName(), line);
}

void BookmarkManager::handleBookmarkTooltipRequest(TextEditor::ITextEditor *textEditor, const QPoint &pos,
                                            int line)
{
    if (textEditor->document()) {
        const QFileInfo fi(textEditor->document()->fileName());
        Bookmark *mark = findBookmark(fi.path(), fi.fileName(), line);
        operateTooltip(textEditor, pos, mark);
    }
}

// BookmarkViewFactory

BookmarkViewFactory::BookmarkViewFactory(BookmarkManager *bm)
    : m_manager(bm)
{
}

QString BookmarkViewFactory::displayName() const
{
    return BookmarkView::tr("Bookmarks");
}

int BookmarkViewFactory::priority() const
{
    return 300;
}

Id BookmarkViewFactory::id() const
{
    return Id("Bookmarks");
}

QKeySequence BookmarkViewFactory::activationSequence() const
{
    return QKeySequence(Core::UseMacShortcuts ? tr("Alt+Meta+M") : tr("Alt+M"));
}

Core::NavigationView BookmarkViewFactory::createWidget()
{
    BookmarkView *bookmarkView = new BookmarkView();
    bookmarkView->setModel(m_manager);
    Core::NavigationView view;
    view.widget = bookmarkView;
    return view;
}
