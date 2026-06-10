// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "filedialog.h"

#include "fancylineedit.h"
#include "filepathinfo.h"
#include "filesystemmodel.h"
#include "fsengine/fsengine.h"
#include "hostosinfo.h"
#include "layoutbuilder.h"
#include "messagebox.h"
#include "mimedatabase.h"
#include "qtcwidgets.h"
#include "theme/theme.h"
#include "utilsicons.h"
#include "utilstr.h"

#include <QComboBox>
#include <QDir>
#include <QDrag>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileIconProvider>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QMenu>
#include <QMimeData>
#include <QMimeDatabase>
#include <QPainter>
#include <QPushButton>
#include <QSettings>
#include <QSortFilterProxyModel>
#include <QSplitter>
#include <QStackedWidget>
#include <QStandardItemModel>
#include <QStandardPaths>
#include <QStyle>
#include <QStyledItemDelegate>
#include <QToolButton>
#include <QTreeView>
#include <QUrl>
#include <QtGui/qactiongroup.h>

using namespace Qt::StringLiterals;

namespace Utils {

// ===== FileFilterProxy =====

class FileFilterProxy : public QSortFilterProxyModel
{
public:
    explicit FileFilterProxy(QObject *parent = nullptr)
        : QSortFilterProxyModel(parent)
    {
        setSortRole(FileSystemModel::FileSortRole);
        setDynamicSortFilter(true);
    }

    void setSuffixes(const QStringList &suffixes)
    {
        m_suffixes = suffixes;
        invalidateFilter();
    }

    void setShowHidden(bool show)
    {
        m_showHidden = show;
        invalidateFilter();
    }

    void setDirectoriesOnly(bool on)
    {
        m_directoriesOnly = on;
        invalidateFilter();
    }

    Qt::ItemFlags flags(const QModelIndex &index) const override
    {
        Qt::ItemFlags f = QSortFilterProxyModel::flags(index);
        if (!index.isValid())
            return f;
        // Items rejected by the active content filter (wrong suffix, or a plain
        // file in Directory mode) stay visible but grayed out and unselectable.
        if (!acceptsContent(mapToSource(index.siblingAtColumn(0))))
            return f & ~(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        return f | Qt::ItemIsDragEnabled;
    }

    QMimeData *mimeData(const QModelIndexList &indexes) const override
    {
        QList<QUrl> urls;
        for (const QModelIndex &idx : indexes) {
            if (!idx.isValid() || idx.column() != 0)
                continue;
            const auto fp = idx.data(FileSystemModel::FilePathRole).value<FilePath>();
            if (!fp.isEmpty())
                urls << QUrl::fromLocalFile(fp.toUserOutput());
        }
        if (urls.isEmpty())
            return nullptr;
        auto *mime = new QMimeData;
        mime->setUrls(urls);
        return mime;
    }

    // Whether the row passes the active content filter. Rows that fail are still
    // shown (grayed out via flags()), so this is kept separate from row filtering,
    // which only hides hidden files.
    bool acceptsContent(const QModelIndex &sourceIdx) const
    {
        const auto flags = sourceIdx.data(FileSystemModel::FileFlagsRole)
                               .value<FilePathInfo::FileFlags>();
        const bool isDir = flags & FilePathInfo::DirectoryType;
        if (m_directoriesOnly)
            return isDir;
        if (isDir)
            return true;
        if (m_suffixes.isEmpty())
            return true;
        const auto fp = sourceIdx.data(FileSystemModel::FilePathRole).value<FilePath>();
        return m_suffixes.contains(fp.suffix().toLower());
    }

protected:
    bool filterAcceptsRow(int row, const QModelIndex &parent) const override
    {
        if (m_showHidden)
            return true;
        const QModelIndex idx = sourceModel()->index(row, 0, parent);
        const auto flags = idx.data(FileSystemModel::FileFlagsRole)
                               .value<FilePathInfo::FileFlags>();
        const bool hiddenByFlag = flags & FilePathInfo::HiddenFlag;
        const bool hiddenByDot
            = idx.data(FileSystemModel::FileNameRole).toString().startsWith('.');
        return !(hiddenByFlag || hiddenByDot);
    }

    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override
    {
        return left.data(FileSystemModel::FileSortRole).toString()
               < right.data(FileSystemModel::FileSortRole).toString();
    }

private:
    QStringList m_suffixes;
    bool m_showHidden = false;
    bool m_directoriesOnly = false;
};

// ===== helpers =====

static QString nameFilterForMime(const QString &mimeType)
{
    MimeDatabase db;
    const MimeType mime = db.mimeTypeForName(mimeType);
    if (!mime.isValid())
        return {};
    if (mime.isDefault())
        return QCoreApplication::translate("QFileDialog", "All files (*)");
    const QString patterns = mime.globPatterns().join(u' ');
    return mime.comment() + " ("_L1 + patterns + u')';
}

static QString truncatedFilter(const QString &filter, int maxLen = 60)
{
    if (filter.size() <= maxLen)
        return filter;
    const int open = filter.indexOf('(');
    // If we can fit the description and an opening paren, truncate inside the pattern list.
    if (open > 0 && open < maxLen - 3)
        return filter.left(maxLen - 2) + u"…)";
    return filter.left(maxLen - 1) + u'…';
}

static bool looksLikeMimeType(const QString &filter)
{
    // MIME types look like "type/subtype"; Qt name filters always contain "("
    return filter.contains('/') && !filter.contains('(');
}

static QStringList parseSuffixes(const QString &filter)
{
    const int open = filter.indexOf('(');
    const int close = filter.lastIndexOf(')');
    if (open < 0 || close <= open)
        return {};
    const QString glob = filter.mid(open + 1, close - open - 1);
    QStringList suffixes;
    for (const QString &part : glob.split(' ', Qt::SkipEmptyParts)) {
        if (part == u"*" || part == u"*.*")
            return {};
        if (part.startsWith(u"*."))
            suffixes << part.mid(2).toLower();
    }
    return suffixes;
}

// ===== Sidebar =====

static constexpr int SidebarFilePathRole = Qt::UserRole + 100;
static constexpr int SidebarIsSectionRole = Qt::UserRole + 101;
static constexpr int SidebarIsFavoriteRole = Qt::UserRole + 102;
static constexpr char s_favoriteMime[] = "application/x-qtc-sidebar-favorite";

class SidebarModel : public QStandardItemModel
{
    Q_OBJECT
public:
    explicit SidebarModel(QObject *parent = nullptr)
        : QStandardItemModel(parent)
    {
        m_favHeader = makeSectionItem(Tr::tr("Favorites"));
        m_locHeader = makeSectionItem(Tr::tr("Locations"));
        m_devHeader = makeSectionItem(Tr::tr("Devices"));

        appendRow(m_favHeader);
        appendRow(m_locHeader);
        populateLocations();
        appendRow(m_devHeader);
        populateDevices();

        loadFavorites();
    }

    void addFavorite(const FilePath &path, int row = -1)
    {
        const int locRow = m_locHeader->row();
        for (int r = m_favHeader->row() + 1; r < locRow; ++r) {
            if (item(r)->data(SidebarFilePathRole).value<FilePath>() == path)
                return;
        }
        const int insertAt = (row < 0) ? locRow
                                        : qBound(m_favHeader->row() + 1, row, locRow);
        insertRow(insertAt, makeEntryItem(path.fileName(), path, true));
        saveFavorites();
    }

    void removeFavorite(const FilePath &path)
    {
        const int locRow = m_locHeader->row();
        for (int r = m_favHeader->row() + 1; r < locRow; ++r) {
            if (item(r)->data(SidebarFilePathRole).value<FilePath>() == path) {
                removeRow(r);
                saveFavorites();
                return;
            }
        }
    }

    bool isInFavoritesSection(int row) const
    {
        return row > m_favHeader->row() && row < m_locHeader->row();
    }

    int favHeaderRow() const { return m_favHeader->row(); }
    int locHeaderRow() const { return m_locHeader->row(); }

    void reorderFavorite(int fromRow, int toRow)
    {
        const int locRow = m_locHeader->row();
        if (!isInFavoritesSection(fromRow) || toRow < m_favHeader->row() + 1 || toRow > locRow
            || fromRow == toRow || fromRow == toRow - 1)
            return;
        QList<QStandardItem *> rowItems = takeRow(fromRow);
        if (rowItems.isEmpty())
            return;
        // takeRow shifts all subsequent rows up by 1; adjust toRow accordingly
        int insertAt = (fromRow < toRow) ? toRow - 1 : toRow;
        insertAt = qBound(m_favHeader->row() + 1, insertAt, m_locHeader->row());
        insertRow(insertAt, rowItems);
        saveFavorites();
    }

private:
    static QStandardItem *makeSectionItem(const QString &title)
    {
        auto *it = new QStandardItem(title);
        it->setData(true, SidebarIsSectionRole);
        it->setFlags(Qt::NoItemFlags);
        return it;
    }

    static QStandardItem *makeEntryItem(
        const QString &name, const FilePath &path, bool isFavorite = false)
    {
        QFileIconProvider prov;
        auto *it = new QStandardItem(name);
        it->setData(QVariant::fromValue(path), SidebarFilePathRole);
        it->setData(false, SidebarIsSectionRole);
        it->setData(isFavorite, SidebarIsFavoriteRole);
        Qt::ItemFlags flags = Qt::ItemIsSelectable | Qt::ItemIsEnabled;
        if (isFavorite)
            flags |= Qt::ItemIsDragEnabled;
        it->setFlags(flags);
        it->setIcon(path.icon());
        return it;
    }

    void populateLocations()
    {
        using SP = QStandardPaths;
        struct Loc {
            SP::StandardLocation loc;
            QString name;
        };
        const Loc locs[] = {
            {SP::HomeLocation,      Tr::tr("Home")     },
            {SP::DesktopLocation,   Tr::tr("Desktop")  },
            {SP::DocumentsLocation, Tr::tr("Documents")},
            {SP::DownloadLocation,  Tr::tr("Downloads")},
            {SP::MusicLocation,     Tr::tr("Music")    },
            {SP::PicturesLocation,  Tr::tr("Pictures") },
        };
        for (const auto &l : locs) {
            const QString p = SP::writableLocation(l.loc);
            if (p.isEmpty())
                continue;
            const FilePath fp = FilePath::fromString(p);
            if (fp.isDir())
                appendRow(makeEntryItem(l.name, fp));
        }
    }

    void populateDevices()
    {
        for (const QFileInfo &drive : QDir::drives()) {
            const FilePath fp = FilePath::fromUserInput(drive.filePath());
            auto *it = new QStandardItem(drive.filePath());
            it->setData(QVariant::fromValue(fp), SidebarFilePathRole);
            it->setData(false, SidebarIsSectionRole);
            it->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            it->setIcon(fp.icon());
            appendRow(it);
        }
        for (const FilePath &root : FSEngine::registeredDeviceRoots()) {
            if (root.isLocal())
                continue;
            appendRow(makeEntryItem(root.host().toString(), root));
        }
    }

    void loadFavorites()
    {
        QSettings settings;
        settings.beginGroup(u"FileDialog"_s);
        const QStringList paths = settings.value(u"Favorites"_s).toStringList();
        settings.endGroup();
        for (const QString &p : paths) {
            const FilePath fp = FilePath::fromString(p);
            if (fp.isDir())
                insertRow(m_locHeader->row(), makeEntryItem(fp.fileName(), fp, true));
        }
    }

    void saveFavorites()
    {
        QStringList paths;
        const int favStart = m_favHeader->row() + 1;
        const int locRow = m_locHeader->row();
        for (int r = favStart; r < locRow; ++r)
            paths << item(r)->data(SidebarFilePathRole).value<FilePath>().toFSPathString();
        QSettings settings;
        settings.beginGroup(u"FileDialog"_s);
        settings.setValue(u"Favorites"_s, paths);
        settings.endGroup();
    }

    QStandardItem *m_favHeader = nullptr;
    QStandardItem *m_locHeader = nullptr;
    QStandardItem *m_devHeader = nullptr;
};

class SidebarView : public QListView
{
    Q_OBJECT
public:
    explicit SidebarView(QWidget *parent = nullptr)
        : QListView(parent)
    {
        setAcceptDrops(true);
        setDragEnabled(true);
        setDropIndicatorShown(false);
        setDefaultDropAction(Qt::MoveAction);
        setFrameShape(QFrame::NoFrame);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setEditTriggers(QAbstractItemView::NoEditTriggers);
        setSelectionMode(QAbstractItemView::SingleSelection);
    }

    int draggedRow() const { return m_draggedRow; }

signals:
    void favoriteDropped(const Utils::FilePath &path, int row);
    void favoriteRemoved(const Utils::FilePath &path);
    void favoriteReordered(int fromRow, int toRow);

protected:
    void startDrag(Qt::DropActions) override
    {
        const QModelIndex idx = currentIndex();
        if (!idx.isValid() || !idx.data(SidebarIsFavoriteRole).toBool())
            return;

        const FilePath fp = idx.data(SidebarFilePathRole).value<FilePath>();

        auto *mime = new QMimeData;
        mime->setData(s_favoriteMime, QByteArray::number(idx.row()));

        auto *drag = new QDrag(this);
        drag->setMimeData(mime);
        drag->setPixmap(dragPixmap(idx));
        const QRect vr = visualRect(idx);
        drag->setHotSpot(QPoint(16, vr.height() / 2));

        m_draggedRow = idx.row();
        viewport()->update();

        const Qt::DropAction result = drag->exec(Qt::MoveAction);

        m_draggedRow = -1;
        m_dropIndicatorRow = -1;
        viewport()->update();

        if (result != Qt::MoveAction)
            emit favoriteRemoved(fp);
    }

    void dragEnterEvent(QDragEnterEvent *event) override
    {
        if (event->mimeData()->hasFormat(s_favoriteMime) || event->mimeData()->hasUrls())
            event->acceptProposedAction();
        else
            event->ignore();
    }

    // Updates m_dropIndicatorRow/m_dropInsertBelow from a viewport position.
    // Returns true if the position maps to a valid insertion spot in the
    // favorites section. When allowEnd is set (external drops), a position
    // past the last favorite snaps to the end of the section.
    bool updateDropIndicator(const QPoint &pos, bool allowEnd)
    {
        const auto *m = qobject_cast<const SidebarModel *>(model());
        if (!m)
            return false;
        const QModelIndex target = indexAt(pos);
        if (target.isValid() && m->isInFavoritesSection(target.row())) {
            m_dropIndicatorRow = target.row();
            m_dropInsertBelow = (pos.y() > visualRect(target).center().y());
            return true;
        }
        if (allowEnd) {
            // Snap to the end of the favorites section.
            const int lastFav = m->locHeaderRow() - 1;
            if (lastFav > m->favHeaderRow()) {
                m_dropIndicatorRow = lastFav;
                m_dropInsertBelow = true;
            } else {
                m_dropIndicatorRow = m->favHeaderRow();
                m_dropInsertBelow = true;
            }
            return true;
        }
        return false;
    }

    void dragMoveEvent(QDragMoveEvent *event) override
    {
        const QPoint pos = event->position().toPoint();
        if (event->mimeData()->hasFormat(s_favoriteMime)) {
            if (updateDropIndicator(pos, false)) {
                viewport()->update();
                event->acceptProposedAction();
            } else {
                m_dropIndicatorRow = -1;
                viewport()->update();
                event->ignore();
            }
            return;
        }
        if (event->mimeData()->hasUrls()) {
            updateDropIndicator(pos, true);
            viewport()->update();
            event->acceptProposedAction();
        } else {
            event->ignore();
        }
    }

    void dragLeaveEvent(QDragLeaveEvent *) override
    {
        m_dropIndicatorRow = -1;
        viewport()->update();
    }

    void dropEvent(QDropEvent *event) override
    {
        if (event->mimeData()->hasFormat(s_favoriteMime)) {
            const int srcRow = event->mimeData()->data(s_favoriteMime).toInt();
            const int dropRow = m_dropIndicatorRow;
            const bool insertBelow = m_dropInsertBelow;
            m_dropIndicatorRow = -1;
            viewport()->update();
            if (dropRow >= 0)
                emit favoriteReordered(srcRow, dropRow + (insertBelow ? 1 : 0));
            event->setDropAction(Qt::MoveAction);
            event->accept();
            return;
        }
        int insertRow = (m_dropIndicatorRow >= 0)
                            ? m_dropIndicatorRow + (m_dropInsertBelow ? 1 : 0)
                            : -1;
        m_dropIndicatorRow = -1;
        viewport()->update();
        for (const QUrl &url : event->mimeData()->urls()) {
            // Finder appends a trailing slash to directory URLs, which makes
            // fileName() empty; clean it so the favorite gets a proper label.
            const FilePath fp = FilePath::fromUrl(url);
            if (fp.isDir()) {
                emit favoriteDropped(fp, insertRow);
                if (insertRow >= 0)
                    ++insertRow;
            }
        }
        event->acceptProposedAction();
    }

    void paintEvent(QPaintEvent *event) override
    {
        QListView::paintEvent(event);
        if (m_dropIndicatorRow < 0)
            return;
        const QModelIndex idx = model()->index(m_dropIndicatorRow, 0);
        const QRect r = visualRect(idx);
        const int y = m_dropInsertBelow ? r.bottom() + 1 : r.top();
        QPainter painter(viewport());
        const QColor highlight = creatorColor(Theme::TextColorLink);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(QPen(highlight, 2));
        painter.drawLine(r.left() + 6, y, r.right() - 6, y);
        painter.setBrush(highlight);
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(QPointF(r.left() + 6, y), 2.5, 2.5);
        painter.drawEllipse(QPointF(r.right() - 6, y), 2.5, 2.5);
    }

private:
    QPixmap dragPixmap(const QModelIndex &idx) const
    {
        const QRect vr = visualRect(idx);
        const qreal dpr = devicePixelRatioF();
        QPixmap pm(vr.size() * dpr);
        pm.setDevicePixelRatio(dpr);
        pm.fill(Qt::transparent);

        QPainter painter(&pm);
        const QIcon icon = idx.data(Qt::DecorationRole).value<QIcon>();
        const int iconSize = qMin(vr.height() - 4, 16);
        const int x = 4;
        if (!icon.isNull()) {
            const QRect iconRect(x, (vr.height() - iconSize) / 2, iconSize, iconSize);
            icon.paint(&painter, iconRect);
        }
        const int textX = x + iconSize + 4;
        painter.setPen(palette().color(QPalette::Text));
        painter.drawText(
            QRect(textX, 0, vr.width() - textX, vr.height()),
            Qt::AlignLeft | Qt::AlignVCenter,
            idx.data(Qt::DisplayRole).toString());
        painter.end();
        return pm;
    }

    int m_draggedRow = -1;
    int m_dropIndicatorRow = -1;
    bool m_dropInsertBelow = false;
};

class SidebarDelegate : public QStyledItemDelegate
{
public:
    explicit SidebarDelegate(SidebarView *view)
        : QStyledItemDelegate(view)
        , m_view(view)
    {}

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index)
        const override
    {
        if (index.data(SidebarIsSectionRole).toBool()) {
            painter->save();
            QFont f = option.font;
            f.setPointSize(qMax(f.pointSize() - 2, 7));
            f.setBold(true);
            painter->setFont(f);
            QColor c = option.palette.color(QPalette::WindowText);
            c.setAlphaF(0.55f);
            painter->setPen(c);
            const QRect r = option.rect.adjusted(8, 0, -4, 0);
            painter->drawText(
                r, Qt::AlignLeft | Qt::AlignVCenter, index.data(Qt::DisplayRole).toString());
            painter->restore();
            return;
        }
        if (index.row() == m_view->draggedRow()) {
            painter->save();
            painter->setPen(QPen(option.palette.color(QPalette::Mid), 1, Qt::DashLine));
            painter->drawRoundedRect(option.rect.adjusted(4, 2, -4, -2), 3, 3);
            painter->restore();
            return;
        }
        QStyledItemDelegate::paint(painter, option, index);
    }

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        if (index.data(SidebarIsSectionRole).toBool())
            return {option.rect.width(), 22};
        const QSize base = QStyledItemDelegate::sizeHint(option, index);
        return {base.width(), qMax(base.height(), 24)};
    }

private:
    SidebarView *m_view;
};

// ===== Navigation history =====

// Back/forward history shared by all FileDialog instances and kept for the
// lifetime of the application process (it is not persisted to disk).
class NavigationHistory
{
public:
    static NavigationHistory &instance()
    {
        static NavigationHistory s_instance;
        return s_instance;
    }

    // Records a new location, dropping any entries the user could go "forward"
    // to. Consecutive visits to the same location are ignored.
    void visit(const FilePath &path)
    {
        if (path.isEmpty())
            return;
        if (m_index >= 0 && m_entries.at(m_index) == path)
            return;
        m_entries.resize(m_index + 1);
        m_entries.append(path);
        m_index = m_entries.size() - 1;
    }

    bool canGoBack() const { return m_index > 0; }
    bool canGoForward() const { return m_index >= 0 && m_index < m_entries.size() - 1; }

    FilePath goBack() { return canGoBack() ? m_entries.at(--m_index) : FilePath(); }
    FilePath goForward() { return canGoForward() ? m_entries.at(++m_index) : FilePath(); }

private:
    FilePaths m_entries;
    int m_index = -1;
};

// ===== Private =====

class FileDialogPrivate
{
public:
    FileSystemModel *m_model = nullptr;
    FileFilterProxy *m_proxy = nullptr;
    QTreeView *m_treeView = nullptr;
    QListView *m_listView = nullptr;
    QStackedWidget *m_viewStack = nullptr;
    SidebarModel *m_sidebarModel = nullptr;
    SidebarView *m_sidebarView = nullptr;
    QComboBox *m_filterCombo = nullptr;
    QLabel *m_fileNameLabel = nullptr;
    FancyLineEdit *m_fileNameEdit = nullptr;
    QAbstractButton *m_acceptButton = nullptr;
    FilePath m_currentDir;
    FilePath m_selectedFile;
    QFileDialog::FileMode m_fileMode = QFileDialog::ExistingFile;
    QFileDialog::AcceptMode m_acceptMode = QFileDialog::AcceptOpen;
    // True while navigating via back/forward, so setDirectory() does not record
    // the move as a fresh history entry.
    bool m_navigatingHistory = false;

    bool acceptsAsSelection(bool isDir) const
    {
        return m_fileMode == QFileDialog::Directory ? isDir : !isDir;
    }

    // The directory currently shown, when it is itself a valid target. In
    // Directory mode the current directory can be opened without selecting a child.
    FilePath acceptableCurrentDir() const
    {
        if (m_fileMode == QFileDialog::Directory && m_currentDir.isDir())
            return m_currentDir;
        return {};
    }

    // Save mode accepts whatever the user typed; open mode needs a valid selection
    // (or, in Directory mode, a valid current directory).
    void updateAcceptButtonState() const
    {
        const bool enable = (m_acceptMode == QFileDialog::AcceptSave)
                                ? !m_fileNameEdit->text().trimmed().isEmpty()
                                : (!m_selectedFile.isEmpty() || !acceptableCurrentDir().isEmpty());
        m_acceptButton->setEnabled(enable);
    }
};

// ===== FileDialog =====

FileDialog::FileDialog(QWidget *parent)
    : QDialog(parent)
    , d(std::make_unique<FileDialogPrivate>())
{
    setWindowTitle(Tr::tr("Open File"));
    resize(860, 520);

    d->m_model = new FileSystemModel(this);
    d->m_proxy = new FileFilterProxy(this);
    d->m_proxy->setSourceModel(d->m_model);

    // Detail view
    d->m_treeView = new QTreeView(this);
    d->m_treeView->setModel(d->m_proxy);
    d->m_treeView->setSelectionMode(QAbstractItemView::SingleSelection);
    d->m_treeView->setRootIsDecorated(false);
    d->m_treeView->setSortingEnabled(true);
    d->m_treeView->sortByColumn(FileSystemModel::NameColumn, Qt::AscendingOrder);
    d->m_treeView->setAlternatingRowColors(true);
    d->m_treeView->setDragEnabled(true);
    d->m_treeView->setDragDropMode(QAbstractItemView::DragOnly);
    d->m_treeView->header()->setSectionResizeMode(FileSystemModel::NameColumn,
                                                  QHeaderView::Stretch);
    d->m_treeView->header()->setSectionResizeMode(FileSystemModel::SizeColumn,
                                                  QHeaderView::ResizeToContents);
    d->m_treeView->header()->setSectionResizeMode(FileSystemModel::TypeColumn,
                                                  QHeaderView::ResizeToContents);
    d->m_treeView->header()->setSectionResizeMode(FileSystemModel::DateModifiedColumn,
                                                  QHeaderView::ResizeToContents);

    // Icon view
    d->m_listView = new QListView(this);
    d->m_listView->setModel(d->m_proxy);
    d->m_listView->setSelectionModel(d->m_treeView->selectionModel());
    d->m_listView->setViewMode(QListView::IconMode);
    d->m_listView->setResizeMode(QListView::Adjust);
    d->m_listView->setIconSize(QSize(48, 48));
    d->m_listView->setGridSize(QSize(90, 80));
    d->m_listView->setWordWrap(true);
    d->m_listView->setUniformItemSizes(true);
    d->m_listView->setDragEnabled(true);
    d->m_listView->setDragDropMode(QAbstractItemView::DragOnly);

    d->m_viewStack = new QStackedWidget(this);
    d->m_viewStack->addWidget(d->m_treeView);
    d->m_viewStack->addWidget(d->m_listView);

    // Sidebar
    d->m_sidebarModel = new SidebarModel(this);
    d->m_sidebarView = new SidebarView(this);
    d->m_sidebarView->setModel(d->m_sidebarModel);
    d->m_sidebarView->setItemDelegate(new SidebarDelegate(d->m_sidebarView));
    d->m_sidebarView->setMinimumWidth(120);

    d->m_filterCombo = new QComboBox(this);
    d->m_filterCombo->setMinimumWidth(180);

    auto handleOpen = [this] {
        if (d->m_acceptMode == QFileDialog::AcceptSave) {
            const QString name = d->m_fileNameEdit->text().trimmed();
            if (name.isEmpty())
                return;
            const FilePath path = FilePath::fromUserInput(name);
            d->m_selectedFile = path.isAbsolutePath() ? path : d->m_currentDir.pathAppended(name);
            accept();
        } else if (!d->m_selectedFile.isEmpty()) {
            accept();
        } else if (const FilePath dir = d->acceptableCurrentDir(); !dir.isEmpty()) {
            d->m_selectedFile = dir;
            accept();
        }
    };

    auto goToParent = [this] {
        const FilePath parent = d->m_currentDir.parentDir();
        if (!parent.isEmpty() && parent != d->m_currentDir)
            setDirectory(parent);
    };

    auto navigateHistory = [this](const FilePath &target) {
        if (target.isEmpty())
            return;
        d->m_navigatingHistory = true;
        setDirectory(target);
        d->m_navigatingHistory = false;
    };

    auto goBack = [navigateHistory] { navigateHistory(NavigationHistory::instance().goBack()); };
    auto goForward = [navigateHistory] {
        navigateHistory(NavigationHistory::instance().goForward());
    };

    auto createFolder = [this] {
        bool ok = false;
        const QString name
            = QInputDialog::getText(
                  this,
                  Tr::tr("New Folder"),
                  Tr::tr("Name of new folder inside \"%1\":").arg(d->m_currentDir.fileName()),
                  QLineEdit::Normal,
                  Tr::tr("untitled folder"),
                  &ok)
                  .trimmed();
        if (!ok || name.isEmpty())
            return;

        const FilePath newDir = d->m_currentDir.pathAppended(name);
        if (newDir.exists()) {
            AsynchronousMessageBox::warning(
                Tr::tr("New Folder"),
                Tr::tr("A file or folder named \"%1\" already exists.").arg(name));
            return;
        }
        if (!newDir.createDir()) {
            AsynchronousMessageBox::warning(
                Tr::tr("New Folder"), Tr::tr("Could not create folder \"%1\".").arg(name));
            return;
        }

        // Refresh the listing so the new folder shows up, then select it.
        setDirectory(d->m_currentDir);
    };

    QSplitter *splitter = nullptr;
    QtcIconButton *upButton = nullptr;
    QtcIconButton *backButton = nullptr;
    QtcIconButton *forwardButton = nullptr;

    // Shortcuts are also wired up as QActions below; keep the sequences here so
    // the button tooltips can advertise them.
    const QKeySequence backShortcut(QKeySequence::Back);
    const QKeySequence forwardShortcut(QKeySequence::Forward);
    const QKeySequence parentShortcut = HostOsInfo::isWindowsHost()
                                            ? QKeySequence(Qt::ALT | Qt::Key_Up)
                                            : QKeySequence(Qt::CTRL | Qt::Key_Up);

    // "Back (⌘[)" — appends the native shortcut text when there is one.
    const auto withShortcut = [](const QString &text, const QKeySequence &seq) {
        const QString native = seq.toString(QKeySequence::NativeText);
        return native.isEmpty() ? text : text + " ("_L1 + native + u')';
    };

    QActionGroup *viewModeGroup = new QActionGroup(this);
    QAction *iconViewAction = viewModeGroup->addAction(Icons::DOWNLOAD.icon(), Tr::tr("Icons view"));
    QAction *listViewAction = viewModeGroup->addAction(Icons::MAGNIFIER.icon(), Tr::tr("List view"));

    iconViewAction->setCheckable(true);
    iconViewAction->setIconVisibleInMenu(true);
    listViewAction->setCheckable(true);
    listViewAction->setIconVisibleInMenu(true);
    listViewAction->setChecked(true);

    connect(iconViewAction, &QAction::triggered, this, [this] { setViewMode(ViewMode::Icons); });
    connect(listViewAction, &QAction::triggered, this, [this] { setViewMode(ViewMode::List); });

    QAction *showHiddenAction = new QAction(Tr::tr("Show hidden files"), this);
    showHiddenAction->setIcon(Icons::EYE_OPEN_TOOLBAR.icon());
    showHiddenAction->setCheckable(true);
    showHiddenAction->setChecked(false);
    showHiddenAction->setIconVisibleInMenu(true);

    connect(showHiddenAction, &QAction::toggled, this, [this](bool checked) {
        d->m_proxy->setShowHidden(checked);
    });

    using namespace Layouting;
    // clang-format off
    Column {
        Splitter {
            bindTo(&splitter),
            orientation(Qt::Horizontal),
            childrenCollapsible(false),
            d->m_sidebarView,

            Column {
                noMargin,

                Grid {
                    columnStretch(0, 0),
                    columnStretch(1, 0),
                    columnStretch(2, 0),
                    columnStretch(3, 1),
                    Row {
                        QtcWidgets::IconButton {
                            bindTo(&backButton),
                            icon(Icons::PREV_NEUTRAL),
                            Layouting::toolTip(withShortcut(Tr::tr("Back"), backShortcut)),
                            onClicked(this, goBack),
                        },
                        QtcWidgets::IconButton {
                            bindTo(&forwardButton),
                            icon(Icons::NEXT_NEUTRAL),
                            Layouting::toolTip(withShortcut(Tr::tr("Forward"), forwardShortcut)),
                            onClicked(this, goForward),
                        },
                        QtcWidgets::IconButton {
                            bindTo(&upButton),
                            icon(Icons::ARROW_UP),
                            Layouting::toolTip(
                                withShortcut(Tr::tr("Go to parent directory"), parentShortcut)),
                            onClicked(this, goToParent),
                        },
                        QtcWidgets::IconButton {
                            icon(Icons::SETTINGS),
                            Layouting::toolTip(Tr::tr("View options")),
                            onClicked(this, [=] {
                                QMenu menu;
                                menu.addAction(iconViewAction);
                                menu.addAction(listViewAction);
                                menu.addSeparator();
                                menu.addAction(showHiddenAction);
                                menu.exec(QCursor::pos());
                            }),
                        },
                    },
                    Row {
                        Label {
                            bindTo(&d->m_fileNameLabel),
                            text(Tr::tr("Save As:")),
                        },
                        LineEdit {
                            bindTo(&d->m_fileNameEdit),
                            placeholderText(Tr::tr("File Name")),
                            onReturnPressed(this, [this](auto &lineEdit) {
                                if (lineEdit.text().isEmpty())
                                    return;
                                const FilePath path = FilePath::fromUserInput(lineEdit.text());
                                if (path.isDir()) {
                                    setDirectory(path);
                                } else if (path.exists() || d->m_acceptMode == QFileDialog::AcceptSave) {
                                    // When saving, a not-yet-existing file name is a valid choice.
                                    d->m_selectedFile = path;
                                    accept();
                                }
                            }),
                        }
                    }
                },
                d->m_viewStack,
                Column {
                    d->m_filterCombo,
                    Row {
                        QtcWidgets::Button {
                            text(Tr::tr("New Folder")),
                            onClicked(this, createFolder),
                            role(QtcButton::Role::MediumSecondary),
                        },
                        st,
                        If (HostOsInfo::isMacHost()) >> Then {
                            QtcWidgets::Button {
                                text(Tr::tr("Cancel")),
                                onClicked(this, [this] { reject(); }),
                                role(QtcButton::Role::MediumSecondary),
                            },
                        },
                        QtcWidgets::Button {
                            bindTo(&d->m_acceptButton),
                            text(Tr::tr("Open")),
                            enabled(false),
                            onClicked(this, handleOpen),
                            role(QtcButton::Role::MediumPrimary),
                        },
                        If (!HostOsInfo::isMacHost()) >> Then {
                            QtcWidgets::Button {
                                text(Tr::tr("Cancel")),
                                onClicked(this, [this] { reject(); }),
                                role(QtcButton::Role::MediumSecondary),
                            },
                        }
                    }
                },
            }
        }
    }.attachTo(this);
    // clang-format on

    const bool saveMode = d->m_acceptMode == QFileDialog::AcceptSave;
    d->m_fileNameEdit->setVisible(saveMode);
    d->m_fileNameLabel->setVisible(saveMode);

    connect(d->m_fileNameEdit, &QLineEdit::textChanged, this, [this] {
        d->updateAcceptButtonState();
    });

    // Native "go to parent" shortcut: Cmd+Up on macOS, Alt+Up elsewhere.
    auto *parentAction = new QAction(this);
    parentAction->setShortcut(parentShortcut);
    parentAction->setShortcutContext(Qt::WindowShortcut);
    connect(parentAction, &QAction::triggered, this, goToParent);
    addAction(parentAction);

    // Native back/forward shortcuts (Alt+Left/Right, Cmd+[ / Cmd+] on macOS).
    auto *backAction = new QAction(this);
    backAction->setShortcut(backShortcut);
    backAction->setShortcutContext(Qt::WindowShortcut);
    connect(backAction, &QAction::triggered, this, goBack);
    addAction(backAction);

    auto *forwardAction = new QAction(this);
    forwardAction->setShortcut(forwardShortcut);
    forwardAction->setShortcutContext(Qt::WindowShortcut);
    connect(forwardAction, &QAction::triggered, this, goForward);
    addAction(forwardAction);

    connect(
        this,
        &FileDialog::directoryChanged,
        this,
        [upButton, backButton, forwardButton](const FilePath &dir) {
            const FilePath parent = dir.parentDir();
            upButton->setEnabled(!parent.isEmpty() && parent != dir);
            backButton->setEnabled(NavigationHistory::instance().canGoBack());
            forwardButton->setEnabled(NavigationHistory::instance().canGoForward());
        });

    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({160, 660});

    QSizePolicy sp = splitter->sizePolicy();
    sp.setVerticalStretch(1);
    splitter->setSizePolicy(sp);

    d->m_filterCombo->hide();

    setViewMode(FileDialog::ViewMode::List);

    auto onActivated = [this](const QModelIndex &proxyIndex) {
        const QModelIndex srcIndex = d->m_proxy->mapToSource(proxyIndex.siblingAtColumn(0));
        const FilePath fp = d->m_model->filePath(srcIndex);
        const bool isDir = d->m_model->isDir(srcIndex);
        // Directories always navigate on activation (double-click). In Directory
        // mode, selecting the directory itself is reserved for the accept button.
        if (isDir) {
            setDirectory(fp);
        } else if (d->acceptsAsSelection(isDir)) {
            d->m_selectedFile = fp;
            accept();
        }
    };
    connect(d->m_treeView, &QAbstractItemView::activated, this, onActivated);
    connect(d->m_listView, &QAbstractItemView::activated, this, onActivated);

    // Both views share the same selection model — one connection suffices.
    connect(d->m_treeView->selectionModel(),
            &QItemSelectionModel::selectionChanged,
            this,
            [this](const QItemSelection &selected) {
                if (selected.isEmpty()) {
                    d->m_selectedFile = {};
                    d->updateAcceptButtonState();
                    return;
                }
                const QModelIndex proxyIdx =
                    selected.indexes().constFirst().siblingAtColumn(0);
                const QModelIndex srcIdx = d->m_proxy->mapToSource(proxyIdx);
                const FilePath fp = d->m_model->filePath(srcIdx);
                d->m_selectedFile = d->acceptsAsSelection(d->m_model->isDir(srcIdx)) ? fp : FilePath();
                d->updateAcceptButtonState();
            });

    connect(d->m_filterCombo,
            &QComboBox::currentIndexChanged,
            this,
            [this](int) {
                const QString filter = d->m_filterCombo->currentData().toString();
                d->m_proxy->setSuffixes(parseSuffixes(filter));
            });

    connect(d->m_sidebarView, &QAbstractItemView::clicked, this, [this](const QModelIndex &idx) {
        if (idx.data(SidebarIsSectionRole).toBool())
            return;
        const FilePath fp = idx.data(SidebarFilePathRole).value<FilePath>();
        if (!fp.isEmpty())
            setDirectory(fp);
    });

    connect(d->m_sidebarView, &SidebarView::favoriteDropped, this, [this](const FilePath &fp, int row) {
        d->m_sidebarModel->addFavorite(fp, row);
    });

    connect(d->m_sidebarView, &SidebarView::favoriteRemoved, this, [this](const FilePath &fp) {
        d->m_sidebarModel->removeFavorite(fp);
    });

    connect(d->m_sidebarView, &SidebarView::favoriteReordered, this, [this](int from, int to) {
        d->m_sidebarModel->reorderFavorite(from, to);
    });
}

FileDialog::~FileDialog() = default;

void FileDialog::setDirectory(const FilePath &path)
{
    if (path.isEmpty())
        return;

    if (path.isDir()) {
        d->m_currentDir = path;
        d->m_selectedFile = {};
    } else {
        d->m_currentDir = path.parentDir();
        d->m_selectedFile = path;
        d->m_fileNameEdit->setText(path.fileName());
    }

    if (!d->m_navigatingHistory)
        NavigationHistory::instance().visit(d->m_currentDir);

    emit directoryChanged(d->m_currentDir);
    d->updateAcceptButtonState();

    d->m_model->setRootPath(d->m_currentDir);

    // index(0,0) is the directory node itself; set as root so both views show its contents.
    const QModelIndex rootSrc = d->m_model->index(0, 0);
    const QModelIndex rootProxy = d->m_proxy->mapFromSource(rootSrc);
    d->m_treeView->setRootIndex(rootProxy);
    d->m_listView->setRootIndex(rootProxy);
}

FilePath FileDialog::directory() const
{
    return d->m_currentDir;
}

void FileDialog::setNameFilters(const QStringList &filters)
{
    // Each entry may itself be a ";;"-separated list of filters (Qt convention).
    QStringList individual;
    for (const QString &f : filters)
        individual << f.split(u";;"_s, Qt::SkipEmptyParts);

    QStringList resolved;
    resolved.reserve(individual.size());
    for (const QString &f : std::as_const(individual)) {
        const QString trimmed = f.trimmed();
        if (looksLikeMimeType(trimmed)) {
            const QString nameFilter = nameFilterForMime(trimmed);
            if (!nameFilter.isEmpty())
                resolved << nameFilter;
        } else {
            resolved << trimmed;
        }
    }

    d->m_filterCombo->clear();
    for (const QString &f : std::as_const(resolved))
        d->m_filterCombo->addItem(truncatedFilter(f), f);
    if (!resolved.isEmpty())
        d->m_proxy->setSuffixes(parseSuffixes(resolved.first()));

    const bool hasFilters = !resolved.isEmpty();
    d->m_filterCombo->setVisible(hasFilters);
}

void FileDialog::setViewMode(ViewMode mode)
{
    const bool icons = (mode == ViewMode::Icons);
    d->m_viewStack->setCurrentIndex(icons ? 1 : 0);
}

void FileDialog::setMode(QFileDialog::FileMode mode)
{
    d->m_fileMode = mode;

    const auto selectionMode = (mode == QFileDialog::ExistingFiles)
                                   ? QAbstractItemView::ExtendedSelection
                                   : QAbstractItemView::SingleSelection;
    d->m_treeView->setSelectionMode(selectionMode);
    d->m_listView->setSelectionMode(selectionMode);

    d->m_proxy->setDirectoriesOnly(mode == QFileDialog::Directory);

    setWindowTitle(mode == QFileDialog::Directory ? Tr::tr("Open Directory") : Tr::tr("Open File"));
}

void FileDialog::setAcceptMode(QFileDialog::AcceptMode mode)
{
    d->m_acceptMode = mode;
    const bool saveMode = mode == QFileDialog::AcceptSave;
    d->m_acceptButton->setText(saveMode ? Tr::tr("Save") : Tr::tr("Open"));
    d->m_fileNameEdit->setVisible(saveMode);
    d->m_fileNameLabel->setVisible(saveMode);
    d->updateAcceptButtonState();
}

FilePath FileDialog::selectedFile() const
{
    return d->m_selectedFile;
}

QString FileDialog::selectedNameFilter() const
{
    const QString filter = d->m_filterCombo->currentData().toString();
    if (!filter.isEmpty())
        return filter;
    return QCoreApplication::translate("QFileDialog", "All files (*)");
}

FilePaths FileDialog::selectedFiles() const
{
    if (d->m_acceptMode == QFileDialog::AcceptSave) {
        const QString name = d->m_fileNameEdit->text().trimmed();
        if (name.isEmpty())
            return {};
        const FilePath path = FilePath::fromUserInput(name);
        return {path.isAbsolutePath() ? path : d->m_currentDir.pathAppended(name)};
    }

    FilePaths result;
    const QModelIndexList indexes = d->m_treeView->selectionModel()->selectedRows(0);
    for (const QModelIndex &proxyIdx : indexes) {
        const QModelIndex srcIdx = d->m_proxy->mapToSource(proxyIdx);
        if (d->acceptsAsSelection(d->m_model->isDir(srcIdx)))
            result << d->m_model->filePath(srcIdx);
    }
    // No child selected: fall back to the recorded selection (e.g. the current
    // directory accepted in Directory mode).
    if (result.isEmpty() && !d->m_selectedFile.isEmpty())
        result << d->m_selectedFile;
    return result;
}

// Builds, configures and runs a modal dialog in the given mode. Returns the
// selected paths on accept, an empty list otherwise.
static FilePaths runDialog(
    QWidget *parent,
    const QString &caption,
    const FilePath &dir,
    const QStringList &nameFilters,
    QFileDialog::FileMode mode,
    QFileDialog::AcceptMode acceptMode = QFileDialog::AcceptOpen)
{
    FileDialog dlg(parent);
    dlg.setMode(mode);
    dlg.setAcceptMode(acceptMode);

    if (!caption.isEmpty())
        dlg.setWindowTitle(caption);
    dlg.setDirectory(dir.isEmpty() ? FilePath::fromString(QDir::homePath()) : dir);
    if (!nameFilters.isEmpty())
        dlg.setNameFilters(nameFilters);

    if (dlg.exec() != QDialog::Accepted)
        return {};
    return dlg.selectedFiles();
}

FilePath FileDialog::getOpenFilePath(
    const QString &caption, const FilePath &dir, const QStringList &nameFilters, QWidget *parent)
{
    const FilePaths files = runDialog(parent, caption, dir, nameFilters, QFileDialog::ExistingFile);
    return files.isEmpty() ? FilePath() : files.first();
}

FilePaths FileDialog::getOpenFilePaths(
    const QString &caption, const FilePath &dir, const QStringList &nameFilters, QWidget *parent)
{
    return runDialog(parent, caption, dir, nameFilters, QFileDialog::ExistingFiles);
}

FilePath FileDialog::getOpenDirectory(const QString &caption, const FilePath &dir, QWidget *parent)
{
    const FilePaths dirs = runDialog(parent, caption, dir, {}, QFileDialog::Directory);
    return dirs.isEmpty() ? FilePath() : dirs.first();
}

FilePath FileDialog::getSaveFilePath(
    const QString &caption, const FilePath &dir, const QStringList &nameFilters, QWidget *parent)
{
    const FilePaths files
        = runDialog(parent, caption, dir, nameFilters, QFileDialog::AnyFile, QFileDialog::AcceptSave);
    return files.isEmpty() ? FilePath() : files.first();
}

} // namespace Utils

#include "filedialog.moc"
