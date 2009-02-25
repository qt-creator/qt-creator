/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "bookmarkmanager.h"

#include "bookmark.h"
#include "bookmarksplugin.h"
#include "bookmarks_global.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/uniqueidmanager.h>
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/projectexplorer.h>
#include <texteditor/basetexteditor.h>
#include <utils/qtcassert.h>

#include <QtCore/QDebug>
#include <QtCore/QFileInfo>

#include <QtGui/QAction>
#include <QtGui/QContextMenuEvent>
#include <QtGui/QMenu>
#include <QtGui/QPainter>

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
    static int lwidth = fm.width("8888") + 18;

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
    // Check wheter we need to be fancy and paint some background
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

    QString lineText = index.data(BookmarkManager::LineText).toString().trimmed();
    painter->drawText(6, opt.rect.top() + fm.ascent() + fm.height() + 6, lineText);

    // Separator lines
    painter->setPen(QColor::fromRgb(150,150,150));
    painter->drawLine(0, opt.rect.bottom(), opt.rect.right(), opt.rect.bottom());
    painter->restore();
}

BookmarkView::BookmarkView(QWidget *parent)
    : QListView(parent)
{
    setWindowTitle(tr("Bookmarks"));
    setWindowIcon(QIcon(":/bookmarks/images/bookmark.png"));

    connect(this, SIGNAL(clicked(const QModelIndex &)),
            this, SLOT(gotoBookmark(const QModelIndex &)));

    m_bookmarkContext = new BookmarkContext(this);
    ICore::instance()->addContextObject(m_bookmarkContext);

    setItemDelegate(new BookmarkDelegate(this));
    setFrameStyle(QFrame::NoFrame);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setFocusPolicy(Qt::NoFocus);
}

BookmarkView::~BookmarkView()
{
    ICore::instance()->removeContextObject(m_bookmarkContext);
}

void BookmarkView::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu;
    QAction *remove = menu.addAction("&Remove Bookmark");
    QAction *removeAll = menu.addAction("Remove all Bookmarks");
    m_contextMenuIndex = indexAt(event->pos());
    if (!m_contextMenuIndex.isValid())
        remove->setEnabled(false);

    if (model()->rowCount() == 0)
        removeAll->setEnabled(false);

    connect(remove, SIGNAL(triggered()),
            this, SLOT(removeFromContextMenu()));
    connect(removeAll, SIGNAL(triggered()),
            this, SLOT(removeAll()));

    menu.exec(mapToGlobal(event->pos()));
}

void BookmarkView::removeFromContextMenu()
{
    removeBookmark(m_contextMenuIndex);
}

void BookmarkView::removeBookmark(const QModelIndex& index)
{
    BookmarkManager *manager = static_cast<BookmarkManager *>(model());
    Bookmark *bm = manager->bookmarkForIndex(index);
    manager->removeBookmark(bm);
}

// The perforcemance of this function could be greatly improved.
//
void BookmarkView::removeAll()
{
    BookmarkManager *manager = static_cast<BookmarkManager *>(model());
    while (manager->rowCount()) {
        QModelIndex index = manager->index(0, 0);
        removeBookmark(index);
    }
}

void BookmarkView::setModel(QAbstractItemModel *model)
{
    BookmarkManager *manager = qobject_cast<BookmarkManager *>(model);
    QTC_ASSERT(manager, return);
    QListView::setModel(model);
    setSelectionModel(manager->selectionModel());
    setSelectionMode(QAbstractItemView::SingleSelection);
    setSelectionBehavior(QAbstractItemView::SelectRows);
}

void BookmarkView::gotoBookmark(const QModelIndex &index)
{
    static_cast<BookmarkManager *>(model())->gotoBookmark(index);
}

////
// BookmarkContext
////

BookmarkContext::BookmarkContext(BookmarkView *widget)
    : Core::IContext(widget), m_bookmarkView(widget)
{
    m_context << UniqueIDManager::instance()->uniqueIdentifier(Constants::BOOKMARKS_CONTEXT);
}

QList<int> BookmarkContext::context() const
{
    return m_context;
}

QWidget *BookmarkContext::widget()
{
    return m_bookmarkView;
}

////
// BookmarkManager
////

BookmarkManager::BookmarkManager() :
    m_bookmarkIcon(QIcon(QLatin1String(":/bookmarks/images/bookmark.png")))
{
    m_selectionModel = new QItemSelectionModel(this, this);

    connect(Core::ICore::instance(), SIGNAL(contextChanged(Core::IContext*)),
            this, SLOT(updateActionStatus()));

    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    ProjectExplorerPlugin *projectExplorer = pm->getObject<ProjectExplorerPlugin>();

    connect(projectExplorer->session(), SIGNAL(sessionLoaded()),
            this, SLOT(loadBookmarks()));

    updateActionStatus();
}

BookmarkManager::~BookmarkManager()
{
    DirectoryFileBookmarksMap::iterator it, end;
    end = m_bookmarksMap.end();
    for (it = m_bookmarksMap.begin(); it != end; ++it) {
        FileNameBookmarksMap *bookmarks = it.value();
        qDeleteAll(bookmarks->values());
        delete bookmarks;
    }
}

QItemSelectionModel *BookmarkManager::selectionModel() const
{
    return m_selectionModel;
}

QModelIndex BookmarkManager::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid())
        return QModelIndex();
    else
        return createIndex(row, column, 0);
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
    else if (role == Qt::ToolTipRole)
        return m_bookmarksList.at(index.row())->filePath();

    return QVariant();
}

void BookmarkManager::toggleBookmark()
{
    TextEditor::ITextEditor *editor = currentTextEditor();
    if (!editor)
        return;

    toggleBookmark(editor->file()->fileName(), editor->currentLine());
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
    addBookmark(bookmark);
}

void BookmarkManager::updateBookmark(Bookmark *bookmark)
{
    int idx = m_bookmarksList.indexOf(bookmark);
    emit dataChanged(index(idx, 0, QModelIndex()), index(idx, 2, QModelIndex()));
    saveBookmarks();
}

void BookmarkManager::removeBookmark(Bookmark *bookmark)
{
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

    int idx = m_bookmarksList.indexOf(bookmark);
    beginRemoveRows(QModelIndex(), idx, idx);
    m_bookmarksList.removeAt(idx);
    endRemoveRows();

    if (selectionModel()->currentIndex().isValid())
        selectionModel()->setCurrentIndex(selectionModel()->currentIndex(), QItemSelectionModel::Select | QItemSelectionModel::Clear);

    updateActionStatus();
    saveBookmarks();
}

Bookmark *BookmarkManager::bookmarkForIndex(QModelIndex index)
{
    if (!index.isValid() || index.row() >= m_bookmarksList.size())
        return 0;
    return m_bookmarksList.at(index.row());
}

void BookmarkManager::gotoBookmark(const QModelIndex &idx)
{
    gotoBookmark(m_bookmarksList.at(idx.row()));
}

void BookmarkManager::gotoBookmark(Bookmark* bookmark)
{
    TextEditor::BaseTextEditor::openEditorAt(bookmark->filePath(),
                                             bookmark->lineNumber());
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
    QFileInfo fi(editor->file()->fileName());
    if (!m_bookmarksMap.contains(fi.path()))
        return;

    int firstLine = -1;
    int lastLine = -1;
    int prevLine = -1;
    int nextLine = -1;
    const QList<Bookmark*> marks = m_bookmarksMap.value(fi.path())->values(fi.fileName());
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
    em->addCurrentPositionToNavigationHistory(true);
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
    em->addCurrentPositionToNavigationHistory();
}

void BookmarkManager::next()
{
    QModelIndex current = selectionModel()->currentIndex();
    if (!current.isValid())
        return;
    int row = current.row() + 1;
    if (row == m_bookmarksList.size())
        row = 0;
    QModelIndex newIndex = current.sibling(row, current.column());
    selectionModel()->setCurrentIndex(newIndex, QItemSelectionModel::Select | QItemSelectionModel::Clear);
    gotoBookmark(newIndex);
}

void BookmarkManager::prev()
{
    QModelIndex current = selectionModel()->currentIndex();
    if (!current.isValid())
        return;
    int row = current.row();
    if (row == 0)
        row = m_bookmarksList.size();
     --row;
    QModelIndex newIndex = current.sibling(row, current.column());
    selectionModel()->setCurrentIndex(newIndex, QItemSelectionModel::Select | QItemSelectionModel::Clear);
    gotoBookmark(newIndex);
}

TextEditor::ITextEditor *BookmarkManager::currentTextEditor() const
{
    Core::EditorManager *em = Core::EditorManager::instance();
    Core::IEditor *currEditor = em->currentEditor();
    if (!currEditor)
        return 0;
    return qobject_cast<TextEditor::ITextEditor *>(currEditor);
}

/* Returns the current session. */
SessionManager *BookmarkManager::sessionManager() const
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    ProjectExplorerPlugin *pe = pm->getObject<ProjectExplorerPlugin>();
    return pe->session();
}

BookmarkManager::State BookmarkManager::state() const
{
    if (m_bookmarksMap.empty())
        return NoBookMarks;

    TextEditor::ITextEditor *editor = currentTextEditor();
    if (!editor)
        return HasBookMarks;

    const QFileInfo fi(editor->file()->fileName());

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
}

/* Returns the bookmark at the given file and line number, or 0 if no such bookmark exists. */
Bookmark* BookmarkManager::findBookmark(const QString &path, const QString &fileName, int lineNumber)
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
    int index2 = s.lastIndexOf(':');
    int index1 = s.indexOf(':');
    if (index2 != -1 || index1 != -1) {
        const QString &filePath = s.mid(index1+1, index2-index1-1);
        const int lineNumber = s.mid(index2 + 1).toInt();
        const QFileInfo fi(filePath);

        if (!filePath.isEmpty() && !findBookmark(fi.path(), fi.fileName(), lineNumber)) {
            Bookmark *b = new Bookmark(filePath, lineNumber, this);
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
    // Empty string was the name of the bookmark, which now is always ""
    return QLatin1String("") + colon + b->filePath() + colon + QString::number(b->lineNumber());
}

/* Saves the bookmarks to the session settings. */
void BookmarkManager::saveBookmarks()
{
    SessionManager *s = sessionManager();
    if (!s)
        return;

    QStringList list;
    foreach (const FileNameBookmarksMap *bookmarksMap, m_bookmarksMap)
        foreach (const Bookmark *bookmark, *bookmarksMap)
            list << bookmarkToString(bookmark);

    s->setValue("Bookmarks", list);
}

/* Loads the bookmarks from the session settings. */
void BookmarkManager::loadBookmarks()
{
    SessionManager *s = sessionManager();
    if (!s)
        return;

    const QStringList &list = s->value("Bookmarks").toStringList();
    foreach (const QString &bookmarkString, list)
        addBookmark(bookmarkString);

    updateActionStatus();
}

// BookmarkViewFactory

BookmarkViewFactory::BookmarkViewFactory(BookmarkManager *bm)
    : m_manager(bm)
{

}

QString BookmarkViewFactory::displayName()
{
    return "Bookmarks";
}

QKeySequence BookmarkViewFactory::activationSequence()
{
    return QKeySequence(Qt::ALT + Qt::Key_M);
}

Core::NavigationView BookmarkViewFactory::createWidget()
{
    BookmarkView *bookmarkView = new BookmarkView();
    bookmarkView->setModel(m_manager);
    Core::NavigationView view;
    view.widget = bookmarkView;
    return view;
}
