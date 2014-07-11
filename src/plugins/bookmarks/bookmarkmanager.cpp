/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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
#include <texteditor/itexteditor.h>
#include <utils/tooltip/tooltip.h>
#include <utils/tooltip/tipcontents.h>
#include <utils/qtcassert.h>
#include <utils/checkablemessagebox.h>

#include <QAction>
#include <QContextMenuEvent>
#include <QDebug>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileInfo>
#include <QFormLayout>
#include <QLineEdit>
#include <QMenu>
#include <QPainter>
#include <QSpinBox>

Q_DECLARE_METATYPE(Bookmarks::Internal::Bookmark*)

using namespace ProjectExplorer;
using namespace TextEditor;
using namespace Core;
using namespace Utils;

namespace Bookmarks {
namespace Internal {

BookmarkDelegate::BookmarkDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
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
    QString topLeft = index.data(BookmarkManager::Filename).toString();
    painter->drawText(6, 2 + opt.rect.top() + fm.ascent(), topLeft);

    QString topRight = index.data(BookmarkManager::LineNumber).toString();
    // Check whether we need to be fancy and paint some background
    int fwidth = fm.width(topLeft);
    if (fwidth + lwidth > opt.rect.width()) {
        int left = opt.rect.right() - lwidth;
        painter->drawPixmap(left, opt.rect.top(), selected ? m_selectedPixmap : m_normalPixmap);
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

BookmarkView::BookmarkView(BookmarkManager *manager)  :
    m_bookmarkContext(new Core::IContext(this)),
    m_manager(manager)
{
    setWindowTitle(tr("Bookmarks"));

    m_bookmarkContext->setWidget(this);
    m_bookmarkContext->setContext(Core::Context(Constants::BOOKMARKS_CONTEXT));

    ICore::addContextObject(m_bookmarkContext);

    Utils::ListView::setModel(manager);

    setItemDelegate(new BookmarkDelegate(this));
    setFrameStyle(QFrame::NoFrame);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setFocusPolicy(Qt::NoFocus);
    setSelectionModel(manager->selectionModel());
    setSelectionMode(QAbstractItemView::SingleSelection);
    setSelectionBehavior(QAbstractItemView::SelectRows);

    connect(this, SIGNAL(clicked(QModelIndex)),
            this, SLOT(gotoBookmark(QModelIndex)));
    connect(this, SIGNAL(activated(QModelIndex)),
            this, SLOT(gotoBookmark(QModelIndex)));
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
    QAction *edit = menu.addAction(tr("&Edit"));
    menu.addSeparator();
    QAction *remove = menu.addAction(tr("&Remove"));
    menu.addSeparator();
    QAction *removeAll = menu.addAction(tr("Remove All"));

    m_contextMenuIndex = indexAt(event->pos());
    if (!m_contextMenuIndex.isValid()) {
        moveUp->setEnabled(false);
        moveDown->setEnabled(false);
        remove->setEnabled(false);
        edit->setEnabled(false);
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
    connect(edit, SIGNAL(triggered()),
            m_manager, SLOT(edit()));

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

void BookmarkView::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Delete) {
        removeBookmark(currentIndex());
        event->accept();
        return;
    }
    Utils::ListView::keyPressEvent(event);
}

void BookmarkView::removeAll()
{
    if (Utils::CheckableMessageBox::doNotAskAgainQuestion(this,
            tr("Remove All Bookmarks"),
            tr("Are you sure you want to remove all bookmarks from all files in the current session?"),
            ICore::settings(),
            QLatin1String("RemoveAllBookmarks")) != QDialogButtonBox::Yes)
        return;

    // The performance of this function could be greatly improved.
    while (m_manager->rowCount()) {
        QModelIndex index = m_manager->index(0, 0);
        removeBookmark(index);
    }
}

void BookmarkView::gotoBookmark(const QModelIndex &index)
{
    Bookmark *bk = m_manager->bookmarkForIndex(index);
    if (!m_manager->gotoBookmark(bk))
        m_manager->removeBookmark(bk);
}

////
// BookmarkManager
////

BookmarkManager::BookmarkManager() :
    m_bookmarkIcon(QLatin1String(":/bookmarks/images/bookmark.png")),
    m_selectionModel(new QItemSelectionModel(this, this))
{
    connect(Core::ICore::instance(), SIGNAL(contextChanged(QList<Core::IContext*>,Core::Context)),
            this, SLOT(updateActionStatus()));

    connect(SessionManager::instance(), SIGNAL(sessionLoaded(QString)),
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
        return QFileInfo(bookMark->fileName()).fileName();
    if (role == BookmarkManager::LineNumber)
        return bookMark->lineNumber();
    if (role == BookmarkManager::Directory)
        return QFileInfo(bookMark->fileName()).path();
    if (role == BookmarkManager::LineText)
        return bookMark->lineText();
    if (role == BookmarkManager::Note)
        return bookMark->note();
    if (role == Qt::ToolTipRole)
        return QDir::toNativeSeparators(bookMark->fileName());
    return QVariant();
}

void BookmarkManager::toggleBookmark()
{
    ITextEditor *editor = ITextEditor::currentTextEditor();
    if (!editor)
        return;

    toggleBookmark(editor->document()->filePath(), editor->currentLine());
}

void BookmarkManager::toggleBookmark(const QString &fileName, int lineNumber)
{
    const int editorLine = lineNumber;

    // Remove any existing bookmark on this line
    if (Bookmark *mark = findBookmark(fileName, lineNumber)) {
        // TODO check if the bookmark is really on the same markable Interface
        removeBookmark(mark);
        return;
    }

    // Add a new bookmark if no bookmark existed on this line
    Bookmark *bookmark = new Bookmark(fileName, editorLine, this);
    bookmark->init();
    addBookmark(bookmark);
}

void BookmarkManager::updateBookmark(Bookmark *bookmark)
{
    const int idx = m_bookmarksList.indexOf(bookmark);
    if (idx == -1)
        return;

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

    const QFileInfo fi(bookmark->fileName());
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

Bookmark *BookmarkManager::bookmarkForIndex(const QModelIndex &index) const
{
    if (!index.isValid() || index.row() >= m_bookmarksList.size())
        return 0;
    return m_bookmarksList.at(index.row());
}

bool BookmarkManager::gotoBookmark(Bookmark *bookmark)
{
    if (IEditor *editor = EditorManager::openEditorAt(bookmark->fileName(), bookmark->lineNumber()))
        return editor->currentLine() == bookmark->lineNumber();
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
    ITextEditor *editor = ITextEditor::currentTextEditor();
    int editorLine = editor->currentLine();
    QFileInfo fi(editor->document()->filePath());
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

BookmarkManager::State BookmarkManager::state() const
{
    if (m_bookmarksMap.empty())
        return NoBookMarks;

    ITextEditor *editor = ITextEditor::currentTextEditor();
    if (!editor)
        return HasBookMarks;

    const QFileInfo fi(editor->document()->filePath());

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

void BookmarkManager::edit(const QString &fileName, int lineNumber)
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
    dlg.setWindowTitle(tr("Edit Bookmark"));
    auto layout = new QFormLayout(&dlg);
    auto noteEdit = new QLineEdit(b->note());
    noteEdit->setMinimumWidth(300);
    auto lineNumberSpinbox = new QSpinBox;
    lineNumberSpinbox->setRange(1, INT_MAX);
    lineNumberSpinbox->setValue(b->lineNumber());
    lineNumberSpinbox->setMaximumWidth(100);
    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, SIGNAL(accepted()), &dlg, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), &dlg, SLOT(reject()));
    layout->addRow(tr("Note text:"), noteEdit);
    layout->addRow(tr("Line number:"), lineNumberSpinbox);
    layout->addWidget(buttonBox);
    if (dlg.exec() == QDialog::Accepted) {
        b->move(lineNumberSpinbox->value());
        b->updateNote(noteEdit->text().replace(QLatin1Char('\t'), QLatin1Char(' ')));
        emit dataChanged(current, current);
        saveBookmarks();
    }
}

/* Returns the bookmark at the given file and line number, or 0 if no such bookmark exists. */
Bookmark *BookmarkManager::findBookmark(const QString &filePath, int lineNumber)
{
    QFileInfo fi(filePath);
    QString path = fi.path();
    if (m_bookmarksMap.contains(path)) {
        foreach (Bookmark *bookmark, m_bookmarksMap.value(path)->values(fi.fileName())) {
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
    const QFileInfo fi(bookmark->fileName());
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
        if (!filePath.isEmpty() && !findBookmark(filePath, lineNumber)) {
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
    return colon + b->fileName() +
            colon + QString::number(b->lineNumber()) +
            noteDelimiter + b->note();
}

/* Saves the bookmarks to the session settings. */
void BookmarkManager::saveBookmarks()
{
    QStringList list;
    foreach (const Bookmark *bookmark, m_bookmarksList)
        list << bookmarkToString(bookmark);

    SessionManager::setValue(QLatin1String("Bookmarks"), list);
}

void BookmarkManager::operateTooltip(ITextEditor *textEditor, const QPoint &pos, Bookmark *mark)
{
    if (!mark)
        return;

    if (mark->note().isEmpty())
        ToolTip::hide();
    else
        ToolTip::show(pos, TextContent(mark->note()), textEditor->widget());
}

/* Loads the bookmarks from the session settings. */
void BookmarkManager::loadBookmarks()
{
    removeAllBookmarks();
    const QStringList &list = SessionManager::value(QLatin1String("Bookmarks")).toStringList();
    foreach (const QString &bookmarkString, list)
        addBookmark(bookmarkString);

    updateActionStatus();
}

void BookmarkManager::handleBookmarkRequest(ITextEditor *textEditor,
                                            int line,
                                            ITextEditor::MarkRequestKind kind)
{
    if (kind == ITextEditor::BookmarkRequest && textEditor->document())
        toggleBookmark(textEditor->document()->filePath(), line);
}

void BookmarkManager::handleBookmarkTooltipRequest(ITextEditor *textEditor, const QPoint &pos,
                                            int line)
{
    if (textEditor->document()) {
        Bookmark *mark = findBookmark(textEditor->document()->filePath(), line);
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
    return "Bookmarks";
}

QKeySequence BookmarkViewFactory::activationSequence() const
{
    return QKeySequence(Core::UseMacShortcuts ? tr("Alt+Meta+M") : tr("Alt+M"));
}

Core::NavigationView BookmarkViewFactory::createWidget()
{
    BookmarkView *bookmarkView = new BookmarkView(m_manager);
    Core::NavigationView view;
    view.widget = bookmarkView;
    return view;
}

} // namespace Internal
} // namespace Bookmarks
