// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "filedialog.h"

#include "algorithm.h"
#include "async.h"
#include "fancylineedit.h"
#include "filepathinfo.h"
#include "filesystemmodel.h"
#include "fileutils.h"
#include "fsengine/fileiconprovider.h"
#include "fsengine/fsengine.h"
#include "hostosinfo.h"
#include "infolabel.h"
#include "layoutbuilder.h"
#include "mimedatabase.h"
#include "overlaywidget.h"
#include "qtdesignwidgets.h"
#include "stylehelper.h"
#include "theme/theme.h"
#include "utility.h"
#include "utilsicons.h"
#include "utilstr.h"
#include "widgets.h"

#include <solutions/spinner/spinner.h>

#include <QClipboard>
#include <QComboBox>
#include <QCompleter>
#include <QDir>
#include <QGuiApplication>
#include <QDrag>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QElapsedTimer>
#include <QFileDialog>
#include <QFile>
#include <QFileIconProvider>
#include <QFrame>
#include <QHeaderView>
#include <QKeyEvent>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QMenu>
#include <QMimeData>
#include <QMimeDatabase>
#include <QPainter>
#include <QPlainTextEdit>
#include <QProgressDialog>
#include <QPromise>
#include <QPushButton>
#include <QSettings>
#include <QSortFilterProxyModel>
#include <QSplitter>
#include <QStackedWidget>
#include <QStandardItemModel>
#include <QStandardPaths>
#include <QStringListModel>
#include <QStyle>
#include <QStyledItemDelegate>
#include <QTextLayout>
#include <QTextOption>
#include <QTimer>
#include <QToolButton>
#include <QTreeView>
#include <QUrl>
#include <QtGui/qactiongroup.h>

using namespace Qt::StringLiterals;

namespace Utils {

// ===== Icon-view layout constants =====
//
// All tunable metrics for the icon (QListView::IconMode) view live here so they
// can be adjusted in one place. Each item is exactly kGridSize; within it the
// icon and the (one- or two-line) name each get their own rounded highlight,
// inset from the cell border and separated from one another.
constexpr QSize kGridSize{100, 104};    // size of each item cell
constexpr int kRows = 2;                // max name lines (label band height)
constexpr int kEditorMaxRows = 3;       // rename editor lines before it scrolls
constexpr int kOuterMargin = 1;         // cell border -> any background
constexpr int kGap = 2;                 // icon background -> name background
constexpr int kIconPad = 6;             // icon -> its background edge
constexpr int kNamePadX = 7;            // text -> name background side
constexpr int kNamePadY = 4;            // text -> name background top/bottom
constexpr int kHighlightRadius = 5;     // rounded-highlight corner radius
constexpr int kHoverAlpha = 50;         // alpha of the (unselected) hover highlight
constexpr int kEditorDocMargin = 2;     // rename editor document margin
constexpr int kMaxExtensionLen = 6;     // keep extension visible when eliding if shorter

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
        invalidate();
    }

    void setShowHidden(bool show)
    {
        m_showHidden = show;
        invalidateFilter();
    }

    void setHideFiltered(bool hide)
    {
        m_hideFiltered = hide;
        invalidateFilter();
    }

    void setDirectoriesOnly(bool on)
    {
        m_directoriesOnly = on;
        invalidateFilter();
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override
    {
        if (role == Qt::TextAlignmentRole && index.column() == FileSystemModel::SizeColumn)
            return QVariant::fromValue(Qt::AlignRight | Qt::AlignVCenter);
        // Inline rename edits the name column; seed the editor with the file name.
        if (role == Qt::EditRole && index.column() == FileSystemModel::NameColumn)
            return QSortFilterProxyModel::data(index, Qt::DisplayRole);
        // Gray out entries that fail the content filter. They stay interactive
        // (so they can be managed) but read as "not a valid choice here".
        if (role == Qt::ForegroundRole && !acceptsContent(mapToSource(index.siblingAtColumn(0))))
            return QGuiApplication::palette().brush(QPalette::Disabled, QPalette::Text);
        return QSortFilterProxyModel::data(index, role);
    }

    Qt::ItemFlags flags(const QModelIndex &index) const override
    {
        Qt::ItemFlags f = QSortFilterProxyModel::flags(index);
        if (!index.isValid())
            return f;
        const bool nameColumn = index.column() == FileSystemModel::NameColumn;
        // Entries rejected by the content filter are not selectable, so they
        // can't be picked as a result or chosen with a left click, but stay
        // enabled and renamable so the context menu can act on them (inline
        // rename needs ItemIsEnabled). They are grayed out via data().
        if (!acceptsContent(mapToSource(index.siblingAtColumn(0)))) {
            f &= ~Qt::ItemIsSelectable;
            if (nameColumn)
                f |= Qt::ItemIsEditable;
            return f;
        }
        f |= Qt::ItemIsDragEnabled;
        // Only the name column is renamable.
        if (nameColumn)
            f |= Qt::ItemIsEditable;
        return f;
    }

    // Whether the row behind \a proxyIndex passes the content filter. Convenience
    // for the dialog, which holds proxy indices.
    bool acceptsContentRow(const QModelIndex &proxyIndex) const
    {
        return acceptsContent(mapToSource(proxyIndex.siblingAtColumn(0)));
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

    static bool filterMatches(const QStringList &suffixes, const FilePath &fp)
    {
        const auto fn = fp.fileName();
        return Utils::anyOf(suffixes, [s = fp.suffix(), fn](const QString &suffix) {
            const bool hasDot = suffix.contains('.');
            return (hasDot && fn == suffix) || (!hasDot && s == suffix);
        });
    }

    // Whether the row passes the active content filter. Rows that fail are still
    // shown (grayed out via flags()), so this is kept separate from row filtering,
    // which only hides hidden files.
    bool acceptsContent(const QModelIndex &sourceIdx) const
    {
        // Fast path: with no name filter and not in directory mode nothing is
        // ever rejected, so don't fetch any roles. flags() calls this for every
        // row during layout, so this matters a lot for large result lists.
        if (!m_directoriesOnly && m_suffixes.isEmpty())
            return true;
        const auto flags = sourceIdx.data(FileSystemModel::FileFlagsRole)
                               .value<FilePathInfo::FileFlags>();
        const bool isDir = flags & FilePathInfo::DirectoryType;
        if (m_directoriesOnly)
            return isDir;
        if (isDir)
            return true;
        const auto fp = sourceIdx.data(FileSystemModel::FilePathRole).value<FilePath>();
        return filterMatches(m_suffixes, fp);
    }

protected:
    bool filterAcceptsRow(int row, const QModelIndex &parent) const override
    {
        const QModelIndex idx = sourceModel()->index(row, 0, parent);
        const auto flags = idx.data(FileSystemModel::FileFlagsRole)
                               .value<FilePathInfo::FileFlags>();
        if (m_hideFiltered && (flags & FilePathInfo::FileType) && !m_suffixes.isEmpty()) {
            const auto fp = idx.data(FileSystemModel::FilePathRole).value<FilePath>();
            if (!filterMatches(m_suffixes, fp))
                return false;
        }
        if (m_showHidden)
            return true;
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
    bool m_hideFiltered = !HostOsInfo::isMacHost();
    bool m_directoriesOnly = false;
};

// ===== NameEditDelegate =====

// Item delegate that customizes the inline rename editor (it preselects the
// base name and leaves the extension unselected, like Finder and Explorer) and
// outlines the row the context menu currently acts on (the "right-clicked"
// entry), which is tracked separately from the selection.
class NameEditDelegate : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    // The right-clicked row (a proxy index) to outline, or invalid for none.
    void setRightClickedRow(const QModelIndex &index) { m_rightClicked = index; }

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index)
        const override
    {
        QStyledItemDelegate::paint(painter, option, index);
        if (!m_rightClicked.isValid() || index.row() != m_rightClicked.row()
            || index.parent() != m_rightClicked.parent())
            return;
        // Outline the row: top/bottom on every cell, plus the outer left/right
        // edges, so a multi-column row reads as a single box.
        const QRect r = option.rect.adjusted(0, 0, -1, -1);
        const int lastColumn = index.model()->columnCount(index.parent()) - 1;
        painter->save();
        painter->setPen(QPen(option.palette.color(QPalette::Highlight), 1));
        painter->drawLine(r.topLeft(), r.topRight());
        painter->drawLine(r.bottomLeft(), r.bottomRight());
        if (index.column() == 0)
            painter->drawLine(r.topLeft(), r.bottomLeft());
        if (index.column() == lastColumn)
            painter->drawLine(r.topRight(), r.bottomRight());
        painter->restore();
    }

    void setEditorData(QWidget *editor, const QModelIndex &index) const override
    {
        QStyledItemDelegate::setEditorData(editor, index);
        auto *lineEdit = qobject_cast<QLineEdit *>(editor);
        if (!lineEdit)
            return;
        const QString name = lineEdit->text();
        const int dot = name.lastIndexOf(u'.');
        // A leading dot ("...gitignore") is part of the name, not an extension.
        const int end = dot > 0 ? dot : int(name.size());
        // Defer: the view selects the whole text right after this returns, so
        // applying our selection now would be overwritten.
        QTimer::singleShot(0, lineEdit, [lineEdit, end] { lineEdit->setSelection(0, end); });
    }

protected:
    // True when the given index is the row the context menu currently acts on.
    bool isRightClicked(const QModelIndex &index) const
    {
        return m_rightClicked.isValid() && index.row() == m_rightClicked.row()
               && index.parent() == m_rightClicked.parent();
    }

private:
    QPersistentModelIndex m_rightClicked;
};

// ===== IconViewDelegate =====

// Wrap-mode shared by measuring and drawing icon-view labels: break at word
// boundaries, but also mid-word when a single word is wider than the cell.
static constexpr QTextOption::WrapMode kLabelWrapMode
    = QTextOption::WrapAtWordBoundaryOrAnywhere;

// True if `text`, wrapped to `width`, occupies at most `maxLines` lines.
static bool fitsInLines(const QString &text, const QFont &font, int width, int maxLines)
{
    QTextLayout layout(text, font);
    QTextOption option;
    option.setWrapMode(kLabelWrapMode);
    layout.setTextOption(option);
    layout.beginLayout();
    int lines = 0;
    bool fits = true;
    while (true) {
        QTextLine line = layout.createLine();
        if (!line.isValid())
            break;
        line.setLineWidth(width);
        if (++lines > maxLines) {
            fits = false;
            break;
        }
    }
    layout.endLayout();
    return fits;
}

// Number of lines `text` occupies when wrapped to `width` (at least one).
static int wrappedLineCount(const QString &text, const QFont &font, int width)
{
    QTextLayout layout(text, font);
    QTextOption option;
    option.setWrapMode(kLabelWrapMode);
    layout.setTextOption(option);
    layout.beginLayout();
    int lines = 0;
    while (true) {
        QTextLine line = layout.createLine();
        if (!line.isValid())
            break;
        line.setLineWidth(width);
        ++lines;
    }
    layout.endLayout();
    return qMax(1, lines);
}

// Elide `text` so it wraps into at most `maxLines` lines of `width`. The
// ellipsis is inserted before the file extension, so a short extension (fewer
// than 6 characters, a leading dot excepted) stays fully visible.
static QString elideLabel(const QString &text, const QFont &font, int width, int maxLines)
{
    if (fitsInLines(text, font, width, maxLines))
        return text;

    const int dot = text.lastIndexOf(u'.');
    // A leading dot (".gitignore") is part of the name, not an extension.
    QString ext;
    if (dot > 0 && text.size() - dot - 1 < kMaxExtensionLen)
        ext = text.mid(dot);
    const QString base = ext.isEmpty() ? text : text.left(dot);
    const QString tail = QChar(0x2026) + ext; // "…" followed by ".ext"

    // Longest prefix of the base name that still fits together with the tail.
    int lo = 0;
    int hi = base.size();
    while (lo < hi) {
        const int mid = (lo + hi + 1) / 2;
        if (fitsInLines(base.left(mid) + tail, font, width, maxLines))
            lo = mid;
        else
            hi = mid - 1;
    }
    return base.left(lo) + tail;
}

// Delegate for the icon (QListView::IconMode) view. Each item occupies exactly
// the view's grid size, with the icon centered horizontally near the top and
// the (word-wrapped, centered) name below it. Inherits NameEditDelegate so the
// inline-rename editor and right-clicked outline keep working.
class IconViewDelegate : public NameEditDelegate
{
public:
    using NameEditDelegate::NameEditDelegate;

    // Rect the name text is drawn in: a kRows-line band anchored to the bottom
    // of the (margin-inset) content area. Shared by paint() and the editor.
    static QRect textRectFor(const QRect &cell, const QFontMetrics &fm)
    {
        const QRect content
            = cell.adjusted(kOuterMargin, kOuterMargin, -kOuterMargin, -kOuterMargin);
        const int nameHeight = kRows * fm.height() + 2 * kNamePadY;
        const QRect nameBackground(
            content.x(), content.bottom() - nameHeight + 1, content.width(), nameHeight);
        return nameBackground.adjusted(kNamePadX, kNamePadY, -kNamePadX, -kNamePadY);
    }

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        if (const auto *view = qobject_cast<const QListView *>(option.widget)) {
            const QSize grid = view->gridSize();
            if (grid.isValid())
                return grid;
        }
        return NameEditDelegate::sizeHint(option, index);
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index)
        const override
    {
        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);

        const QRect rect = opt.rect;
        const QRect content
            = rect.adjusted(kOuterMargin, kOuterMargin, -kOuterMargin, -kOuterMargin);

        const bool selected = opt.state & QStyle::State_Selected;
        const bool highlighted = opt.state & (QStyle::State_Selected | QStyle::State_MouseOver);

        // Fill a rounded highlight: solid for the selection, faint for hover.
        const auto fillHighlight = [&](const QRectF &r) {
            QColor color = opt.palette.color(QPalette::Highlight);
            if (!selected)
                color.setAlpha(kHoverAlpha);
            painter->save();
            painter->setRenderHint(QPainter::Antialiasing, true);
            painter->setPen(Qt::NoPen);
            painter->setBrush(color);
            painter->drawRoundedRect(r, kHighlightRadius, kHighlightRadius);
            painter->restore();
        };

        // Name band (where the text is drawn) and its surrounding background.
        const QRect textRect = textRectFor(rect, opt.fontMetrics);
        const QRect nameBackground = textRect.adjusted(-kNamePadX, -kNamePadY, kNamePadX, kNamePadY);

        // Icon: scaled (square) to fill the space between the top of the item
        // and the top of the name text, leaving room for its highlight padding,
        // and centered in that space.
        const int areaTop = content.top();
        const int areaBottom = nameBackground.top() - kGap;
        const int extent = qMax(0, qMin(content.width(), areaBottom - areaTop) - 2 * kIconPad);
        const QRect iconRect(
            content.x() + (content.width() - extent) / 2,
            areaTop + (areaBottom - areaTop - extent) / 2,
            extent,
            extent);

        // Rounded highlight around the icon, clamped so it stays within the item
        // and never reaches into the gap above the name background.
        if (highlighted) {
            QRectF iconBackground
                = QRectF(iconRect).adjusted(-kIconPad, -kIconPad, kIconPad, kIconPad);
            iconBackground.setTop(qMax<qreal>(iconBackground.top(), content.top()));
            iconBackground.setBottom(
                qMin<qreal>(iconBackground.bottom(), nameBackground.top() - kGap));
            fillHighlight(iconBackground);
        }

        const QIcon::Mode mode = selected ? QIcon::Selected : QIcon::Normal;
        painter->save();
        painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
        opt.icon.paint(painter, iconRect, Qt::AlignCenter, mode);
        painter->restore();

        // Name: centered, wrapped into at most kRows lines, and elided (before
        // the extension) when it would not otherwise fit.
        if (!opt.text.isEmpty() && textRect.height() > 0) {
            const QString text = elideLabel(opt.text, opt.font, textRect.width(), kRows);
            QTextOption textOption;
            textOption.setWrapMode(kLabelWrapMode);
            QTextLayout layout(text, opt.font);
            layout.setTextOption(textOption);
            layout.beginLayout();
            qreal y = 0;
            qreal usedWidth = 0;
            for (int n = 0; n < kRows; ++n) {
                QTextLine line = layout.createLine();
                if (!line.isValid())
                    break;
                line.setLineWidth(textRect.width());
                // Center each wrapped line horizontally within the band.
                const qreal x = qMax(0.0, (textRect.width() - line.naturalTextWidth()) / 2.0);
                line.setPosition(QPointF(x, y));
                usedWidth = qMax(usedWidth, line.naturalTextWidth());
                y += line.height();
            }
            layout.endLayout();

            // Rounded highlight hugging the actual text (kept within the
            // content). Its height follows the real line count, so a one-line
            // name gets a one-line background.
            if (highlighted) {
                const qreal width = qMin<qreal>(usedWidth + 2 * kNamePadX, content.width());
                const QRectF background(
                    content.x() + (content.width() - width) / 2.0,
                    nameBackground.top(),
                    width,
                    y + 2 * kNamePadY);
                fillHighlight(background);
            }

            painter->save();
            painter->setPen(
                opt.palette.color(selected ? QPalette::HighlightedText : QPalette::Text));
            layout.draw(painter, textRect.topLeft());
            painter->restore();
        }

        // Outline the row the context menu currently acts on.
        if (isRightClicked(index)) {
            painter->save();
            painter->setRenderHint(QPainter::Antialiasing, true);
            painter->setPen(QPen(opt.palette.color(QPalette::Highlight), 1));
            painter->setBrush(Qt::NoBrush);
            painter->drawRoundedRect(QRectF(content).adjusted(0.5, 0.5, -0.5, -0.5),
                                     kHighlightRadius,
                                     kHighlightRadius);
            painter->restore();
        }
    }

    // Use a wrapping, vertically-scrolling editor (instead of the single-line
    // QLineEdit) so a long name stays within the cell width while being renamed.
    QWidget *createEditor(QWidget *parent,
                          const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override
    {
        Q_UNUSED(option)
        Q_UNUSED(index)
        auto *editor = new QPlainTextEdit(parent);
        editor->setWordWrapMode(kLabelWrapMode);
        editor->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        editor->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        editor->setTabChangesFocus(true);
        editor->document()->setDocumentMargin(kEditorDocMargin);
        return editor;
    }

    void setEditorData(QWidget *editor, const QModelIndex &index) const override
    {
        auto *textEdit = qobject_cast<QPlainTextEdit *>(editor);
        if (!textEdit) {
            NameEditDelegate::setEditorData(editor, index);
            return;
        }
        const QString name = index.data(Qt::EditRole).toString();
        textEdit->setPlainText(name);
        // Preselect the base name, leaving the extension unselected (a leading
        // dot is part of the name), matching the tree view's rename editor.
        const int dot = name.lastIndexOf(u'.');
        const int end = dot > 0 ? dot : int(name.size());
        QTextCursor cursor = textEdit->textCursor();
        cursor.setPosition(0);
        cursor.setPosition(end, QTextCursor::KeepAnchor);
        textEdit->setTextCursor(cursor);
    }

    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index)
        const override
    {
        auto *textEdit = qobject_cast<QPlainTextEdit *>(editor);
        if (!textEdit) {
            NameEditDelegate::setModelData(editor, model, index);
            return;
        }
        // Wrapping is purely visual; a file name never contains newlines.
        QString name = textEdit->toPlainText();
        name.replace(u'\n', QString());
        model->setData(index, name, Qt::EditRole);
    }

    void updateEditorGeometry(QWidget *editor,
                              const QStyleOptionViewItem &option,
                              const QModelIndex &index) const override
    {
        auto *textEdit = qobject_cast<QPlainTextEdit *>(editor);
        if (!textEdit) {
            NameEditDelegate::updateEditorGeometry(editor, option, index);
            return;
        }

        const int frame = 2 * textEdit->frameWidth();
        const int docMargin = int(2 * textEdit->document()->documentMargin());
        const int lineHeight = option.fontMetrics.lineSpacing();

        // Match the painted name band: same width and top, so the editor sits
        // exactly where the label was. Wrapping keeps the text inside this width.
        const QRect textRect = textRectFor(option.rect, option.fontMetrics);
        const int width = textRect.width() + 2 * kNamePadX;
        const int x = textRect.x() - kNamePadX;

        // Size the height from the actual name (the editor's document is still
        // empty when this runs), capped at kEditorMaxRows, scrolling beyond that.
        const QString name = index.data(Qt::EditRole).toString();
        const int lines = qMin(wrappedLineCount(name, option.font, width - frame - docMargin),
                               kEditorMaxRows);
        const int height = lines * lineHeight + docMargin + frame;

        const int top = textRect.top() - kNamePadY;
        editor->setGeometry(x, top, width, height);
    }

    // The base class lets Enter insert a newline in text editors; here Enter
    // (without Shift) must commit the rename instead.
    bool eventFilter(QObject *object, QEvent *event) override
    {
        if (event->type() == QEvent::KeyPress) {
            if (auto *editor = qobject_cast<QPlainTextEdit *>(object)) {
                auto *keyEvent = static_cast<QKeyEvent *>(event);
                if ((keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter)
                    && !(keyEvent->modifiers() & Qt::ShiftModifier)) {
                    emit commitData(editor);
                    emit closeEditor(editor);
                    return true;
                }
            }
        }
        return NameEditDelegate::eventFilter(object, event);
    }
};

// ===== Views that ignore right-button and unselectable presses =====

// A right-click must only open the context menu, never move the current item or
// change the selection. Qt's views update the current index on any press, so
// swallow right-button presses here. The QEvent::ContextMenu that drives
// customContextMenuRequested() is delivered separately and is unaffected.
//
// Filtered-out entries stay enabled (so the context menu can act on them) but
// are not selectable. Qt still moves the current index onto any enabled item on
// a press, and some styles (notably on Linux) paint the current cell with a
// focus highlight that reads as a selection even though selectedIndexes()
// correctly excludes it. Swallow left presses on such entries too, so the
// current index — and the existing selection — are left untouched.
template<typename View>
class NoRightPressView : public View
{
public:
    using View::View;

protected:
    void mousePressEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::RightButton) {
            event->accept();
            return;
        }
        const QModelIndex index = View::indexAt(event->position().toPoint());
        if (index.isValid() && !(index.flags() & Qt::ItemIsSelectable)) {
            event->accept();
            return;
        }
        View::mousePressEvent(event);
    }
};

using FileTreeView = NoRightPressView<QTreeView>;
using FileListView = NoRightPressView<QListView>;

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
        else
            suffixes << part;
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
        constexpr StyleHelper::TextFormat tf {
            .themeColor = Theme::Token_Text_Muted,
            .uiElement = StyleHelper::UiElementCaptionStrong,
        };
        auto *it = new QStandardItem("  " + title);
        it->setData(true, SidebarIsSectionRole);
        it->setFlags(Qt::NoItemFlags);
        it->setFont(tf.font());
        it->setForeground(tf.color());
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
        const SP::StandardLocation locs[] = {
            SP::HomeLocation,
            SP::DesktopLocation,
            SP::DocumentsLocation,
            SP::DownloadLocation,
            SP::MusicLocation,
            SP::PicturesLocation,
        };
        for (SP::StandardLocation l : locs) {
            const QString p = SP::writableLocation(l);
            if (p.isEmpty())
                continue;
            const FilePath fp = FilePath::fromString(p);
            if (fp.isDir())
                appendRow(makeEntryItem(SP::displayName(l), fp));
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
            // Only list devices we can actually browse (those with file access).
            if (!root.hasFileAccess())
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

// ===== Recursive search =====

// Flat list of files/directories matching a search term, gathered by
// recursively walking a root directory on a background thread. Mirrors the
// roles and columns of FileSystemModel so it can be dropped in as the source
// model of FileFilterProxy while a search is active.
class SearchModel : public QAbstractListModel
{
    Q_OBJECT
public:
    struct Hit
    {
        FilePath path;
        FilePathInfo info;
        QString relPath; // path relative to the search root, shown in the Name column
        QString sortKey; // directories-first, lowercased relPath; matches FileSortRole
    };

    explicit SearchModel(QObject *parent = nullptr)
        : QAbstractListModel(parent)
    {}

    ~SearchModel() override { cancel(); }

    // Cancels any running search and starts a fresh recursive walk of \a root,
    // reporting entries whose file name contains \a text (case-insensitive).
    void search(const FilePath &root, const QString &text)
    {
        cancel();

        beginResetModel();
        m_hits.clear();
        endResetModel();

        m_future = Utils::asyncRun(QThread::LowPriority, [root, text](QPromise<Hit> &promise) {
            // Batch matches on the worker thread before reporting them. Reporting
            // every single hit would flood the GUI thread with resultsReadyAt
            // deliveries and block it; one addResults() per batch emits a single
            // signal for the whole range instead.
            QList<Hit> batch;
            QElapsedTimer sinceFlush;
            sinceFlush.start();
            const auto flush = [&] {
                if (!batch.isEmpty()) {
                    promise.addResults(batch);
                    batch.clear();
                }
                sinceFlush.restart();
            };

            root.iterateDirectory(
                [&](const FilePath &item, const FilePathInfo &info) {
                    if (promise.isCanceled())
                        return IterationPolicy::Stop;
                    if (item.fileName().contains(text, Qt::CaseInsensitive)) {
                        const bool isDir = info.fileFlags & FilePathInfo::DirectoryType;
                        const QString rel = item.relativePathFromDir(root);
                        batch.append(
                            Hit{item, info, rel, QChar(isDir ? '0' : '1') + rel.toLower()});
                        // Flush on size, or on time so sparse matches still appear.
                        if (batch.size() >= (256) || sinceFlush.elapsed() >= 100)
                            flush();
                    }
                    return IterationPolicy::Continue;
                },
                FileFilter(
                    {},
                    DirFilterFlag::NoDotAndDotDot | DirFilterFlag::AllEntries,
                    DirIteratorFlag::Subdirectories));
            flush();
        });

        if (!m_watcher) {
            m_watcher = new QFutureWatcher<Hit>(this);
            // Append batches at the end; never move the heavy Hit objects around.
            // The proxy keeps the view sorted by shuffling only its integer row
            // mapping, which is far cheaper than sorted insertion into m_hits.
            connect(m_watcher, &QFutureWatcherBase::resultsReadyAt, this, [this](int begin, int end) {
                const int first = m_hits.size();
                beginInsertRows({}, first, first + (end - begin) - 1);
                // No reserve(): append grows geometrically, so total copies stay
                // O(n). Reserving the exact size each batch would instead force a
                // reallocation that recopies every existing Hit -> O(n^2).
                for (int i = begin; i < end; ++i)
                    m_hits.append(m_watcher->future().resultAt(i));
                endInsertRows();
            });
            connect(m_watcher, &QFutureWatcherBase::finished, this, &SearchModel::searchFinished);
        }
        m_watcher->setFuture(m_future);
    }

    void cancel()
    {
        m_future.cancel();
        if (m_watcher)
            m_watcher->setFuture({});
        m_future = {};
    }

    int rowCount(const QModelIndex &parent = {}) const override
    {
        return parent.isValid() ? 0 : int(m_hits.size());
    }

    int columnCount(const QModelIndex &parent = {}) const override
    {
        return parent.isValid() ? 0 : FileSystemModel::NumColumns;
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override
    {
        if (!index.isValid() || index.row() >= m_hits.size())
            return {};
        const Hit &hit = m_hits.at(index.row());
        const bool isDir = hit.info.fileFlags & FilePathInfo::DirectoryType;

        switch (role) {
        case Qt::DisplayRole:
            switch (index.column()) {
            case FileSystemModel::NameColumn:
                return hit.relPath.isEmpty() ? hit.path.fileName() : hit.relPath;
            case FileSystemModel::SizeColumn:
                return isDir ? QVariant() : QLocale().formattedDataSize(hit.info.fileSize);
            case FileSystemModel::TypeColumn:
                if (isDir)
                    return Tr::tr("Directory");
                if (const QString ext = hit.path.suffix(); !ext.isEmpty())
                    return Tr::tr("%1 File").arg(ext.toUpper());
                return Tr::tr("File");
            case FileSystemModel::DateModifiedColumn:
                return hit.info.lastModified.isValid()
                           ? QLocale().toString(hit.info.lastModified, QLocale::ShortFormat)
                           : QVariant();
            }
            return {};
        case Qt::DecorationRole:
            if (index.column() == FileSystemModel::NameColumn)
                return isDir ? FileIconProvider::icon(QFileIconProvider::Folder)
                             : FileIconProvider::icon(QFileIconProvider::File);
            return {};
        case Qt::ToolTipRole:
            return hit.path.toUserOutput();
        case FileSystemModel::FilePathRole:
            return QVariant::fromValue(hit.path);
        case FileSystemModel::FileNameRole:
            return hit.path.fileName();
        case FileSystemModel::FileFlagsRole:
            return QVariant::fromValue(hit.info.fileFlags);
        case FileSystemModel::FileSortRole:
            return hit.sortKey;
        }
        return {};
    }

    QVariant headerData(int section, Qt::Orientation orientation, int role) const override
    {
        if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
            return {};
        switch (section) {
        case FileSystemModel::NameColumn:
            return Tr::tr("Name");
        case FileSystemModel::SizeColumn:
            return Tr::tr("Size");
        case FileSystemModel::TypeColumn:
            return Tr::tr("Type");
        case FileSystemModel::DateModifiedColumn:
            return Tr::tr("Date Modified");
        }
        return {};
    }

signals:
    void searchFinished();

private:
    QList<Hit> m_hits;
    QFuture<Hit> m_future;
    QFutureWatcher<Hit> *m_watcher = nullptr;
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

// ===== Go to folder overlay =====

// A small panel that drops in over the top of the dialog when the user invokes
// "Go to Folder" (Cmd+Shift+G on macOS), letting them type an absolute path.
// Mirrors the native macOS sheet: a labelled text field, prefilled with the
// folder currently shown, with directory completion that accepts on Return and
// dismisses on Escape or focus loss.
//
// Completion is device-aware: it lists the directory being typed through
// FileSystemModel (the same model the dialog browses with, including remote
// devices) and feeds the resulting child entries to a flat QCompleter. Listing
// is asynchronous, so the popup is refreshed once the directory loads.
class GoToFolderOverlay : public QFrame
{
    Q_OBJECT
public:
    explicit GoToFolderOverlay(QWidget *parent)
        : QFrame(parent)
    {
        setFrameShape(QFrame::StyledPanel);
        setAutoFillBackground(true);
        setFocusPolicy(Qt::NoFocus);
        hide();

        m_edit = new QLineEdit(this);
        m_edit->setPlaceholderText(Tr::tr("/path/to/folder"));
        m_edit->setClearButtonEnabled(true);

        m_listModel = new FileSystemModel(this);
        m_completionItems = new QStringListModel(this);
        m_completer = new QCompleter(m_completionItems, this);
        m_completer->setCompletionMode(QCompleter::PopupCompletion);
        m_completer->setFilterMode(Qt::MatchStartsWith);
        const bool caseInsensitive = HostOsInfo::isWindowsHost() || HostOsInfo::isMacHost();
        m_completer->setCaseSensitivity(caseInsensitive ? Qt::CaseInsensitive : Qt::CaseSensitive);
        m_edit->setCompleter(m_completer);

        using namespace Layouting;
        // clang-format off
        Column {
            new QLabel(Tr::tr("Go to the folder:"), this),
            m_edit,
        }.attachTo(this);
        // clang-format on

        m_edit->installEventFilter(this);
        parent->installEventFilter(this);
        // The completion popup is a Qt::Popup: while open it receives key events
        // directly (the field no longer does), so filter it too to drive Tab.
        m_completerPopup = m_completer->popup();
        m_completerPopup->installEventFilter(this);

        // Retarget the listing whenever the typed directory changes.
        connect(m_edit, &QLineEdit::textEdited, this, [this](const QString &text) {
            updateCompletions(text);
        });
        // The listing is async; refresh the candidate list (and re-show the
        // popup) once the directory we are completing against has loaded.
        connect(m_listModel, &FileSystemModel::directoryLoaded, this, [this](const FilePath &path) {
            if (path != m_completionDir)
                return;
            QStringList entries;
            const QModelIndex root = m_listModel->index(m_completionDir);
            const int rows = m_listModel->rowCount(root);
            entries.reserve(rows);
            for (int r = 0; r < rows; ++r) {
                const FilePath child = m_listModel->filePath(m_listModel->index(r, 0, root));
                entries << m_completionPrefix + child.fileName();
            }
            m_completionItems->setStringList(entries);
            if (m_edit->hasFocus() && !m_edit->text().isEmpty())
                m_completer->complete();
        });

        connect(m_edit, &QLineEdit::returnPressed, this, [this] {
            const QString text = m_edit->text().trimmed();
            if (!text.isEmpty())
                emit pathEntered(text);
            hide();
        });
    }

    // Shows the overlay prefilled with \a current, the folder the dialog shows.
    void popup(const FilePath &current)
    {
        reposition();
        m_edit->setText(current.toUserOutput());
        show();
        raise();
        m_edit->setFocus(Qt::ShortcutFocusReason);
        // Cursor at the end so the user can append a child directory directly.
        m_edit->end(false);
    }

signals:
    void pathEntered(const QString &path);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if (watched == parent() && event->type() == QEvent::Resize) {
            if (isVisible())
                reposition();
        } else if (watched == m_completerPopup) {
            // While the popup is open it gets keys directly. Keep Tab as "step
            // to next entry" here instead of letting it navigate focus away.
            if (event->type() == QEvent::KeyPress
                && static_cast<QKeyEvent *>(event)->key() == Qt::Key_Tab) {
                stepCompletion();
                return true;
            }
        } else if (watched == m_edit) {
            if (event->type() == QEvent::KeyPress) {
                auto *ke = static_cast<QKeyEvent *>(event);
                if (ke->key() == Qt::Key_Escape) {
                    hide();
                    return true;
                }
                // Tab opens the completion list and steps through it instead of
                // navigating focus away, which would dismiss the overlay.
                if (ke->key() == Qt::Key_Tab) {
                    stepCompletion();
                    return true;
                }
            }
            // Dismiss when focus leaves the field (e.g. a click elsewhere), but
            // not while the completion popup is open or taking focus itself.
            if (event->type() == QEvent::FocusOut) {
                const bool toPopup
                    = static_cast<QFocusEvent *>(event)->reason() == Qt::PopupFocusReason
                      || (m_completerPopup && m_completerPopup->isVisible());
                if (!toPopup)
                    hide();
            }
        }
        return QFrame::eventFilter(watched, event);
    }

private:
    // Opens the completion popup if needed and highlights the next entry (the
    // first if none is selected, wrapping after the last). Keyboard focus stays
    // in the field so the completer keeps the popup open.
    void stepCompletion()
    {
        if (!m_completerPopup->isVisible()) {
            m_completer->complete();
            if (!m_completerPopup->isVisible())
                return;
        }
        const int count = m_completerPopup->model()->rowCount();
        if (count == 0)
            return;
        const QModelIndex cur = m_completerPopup->currentIndex();
        const int next = cur.isValid() ? (cur.row() + 1) % count : 0;
        m_completerPopup->setCurrentIndex(m_completerPopup->model()->index(next, 0));
    }

    // Points the listing model at the directory portion of \a rawText (the part
    // up to the last separator), so completion offers that directory's children.
    void updateCompletions(const QString &rawText)
    {
        const int slash = qMax(rawText.lastIndexOf('/'), rawText.lastIndexOf('\\'));
        if (slash < 0)
            return;
        // The directory portion exactly as typed (e.g. "~/" or "/Users/me/").
        // Candidates are built from this so they share the typed text's form and
        // the completer's prefix matching works (notably for the ~ shorthand).
        const QString typedPrefix = rawText.left(slash + 1);
        QString expanded = typedPrefix;
        // Only expand "~" and "~/"; do not treat "~user" as a home directory.
        if (expanded == "~"_L1 || expanded.startsWith("~/"_L1))
            expanded = QDir::homePath() + expanded.mid(1);
        const FilePath dir = FilePath::fromUserInput(expanded);
        if (dir == m_completionDir)
            return; // Same directory: the completer filters the cached list live.
        m_completionDir = dir;
        m_completionPrefix = typedPrefix;
        m_completionItems->setStringList({}); // Drop stale entries until loaded.
        m_listModel->setRootPath(dir);
        // No view drives this model, so nothing would lazily fetch the listing;
        // kick it off explicitly. directoryLoaded then fills the candidates.
        const QModelIndex root = m_listModel->index(dir);
        if (m_listModel->canFetchMore(root))
            m_listModel->fetchMore(root);
    }

    void reposition()
    {
        QWidget *p = parentWidget();
        adjustSize();
        const int w = qBound(360, p->width() / 2, p->width() - 40);
        resize(w, sizeHint().height());
        move((p->width() - width()) / 2, 12);
    }

    QLineEdit *m_edit = nullptr;
    FileSystemModel *m_listModel = nullptr;
    QStringListModel *m_completionItems = nullptr;
    QCompleter *m_completer = nullptr;
    QAbstractItemView *m_completerPopup = nullptr;
    FilePath m_completionDir;
    QString m_completionPrefix; // Directory portion as typed; prepended to candidates.
};

// ===== Private =====

class FileDialogPrivate
{
public:
    FileSystemModel *m_model = nullptr;
    SearchModel *m_searchModel = nullptr;
    FileFilterProxy *m_proxy = nullptr;
    QTreeView *m_treeView = nullptr;
    QListView *m_listView = nullptr;
    QStackedWidget *m_viewStack = nullptr;
    SidebarModel *m_sidebarModel = nullptr;
    SidebarView *m_sidebarView = nullptr;
    QComboBox *m_pathCombo = nullptr;
    QComboBox *m_filterCombo = nullptr;
    GoToFolderOverlay *m_gotoOverlay = nullptr;
    QLabel *m_fileNameLabel = nullptr;
    FancyLineEdit *m_fileNameEdit = nullptr;
    FancyLineEdit *m_searchEdit = nullptr;
    SpinnerSolution::Spinner *m_spinner = nullptr;
    OverlayWidget *m_errorOverlay = nullptr;
    InfoLabel *m_errorLabel = nullptr;
    QAbstractButton *m_acceptButton = nullptr;
    // Coalesces keystrokes so the recursive walk only starts once typing
    // settles, avoiding a storm of started/canceled finds on remote devices.
    QTimer m_searchTimer;
    // Defers showing the spinner while the file system model fetches a
    // directory listing, so fast (local) listings do not flicker.
    QTimer m_spinnerDelay;
    FilePath m_currentDir;
    FilePath m_selectedFile;
    // The entry the context menu currently acts on (a proxy index). Tracked
    // separately from the selection so right-clicking never changes the
    // selected file; only valid while the context menu is open.
    QPersistentModelIndex m_rightClicked;
    QFileDialog::FileMode m_fileMode = QFileDialog::ExistingFile;
    QFileDialog::AcceptMode m_acceptMode = QFileDialog::AcceptOpen;
    // True while navigating via back/forward, so setDirectory() does not record
    // the move as a fresh history entry.
    bool m_navigatingHistory = false;
    bool m_showFullPaths = !HostOsInfo::isMacHost();

    bool acceptsAsSelection(bool isDir) const
    {
        return m_fileMode == QFileDialog::Directory ? isDir : !isDir;
    }

    bool isSearching() const { return m_proxy->sourceModel() == m_searchModel; }

    void setSpinnerRunning(bool running) { m_spinner->setVisible(running); }

    // Shows/hides the overlay reporting that the current directory could not be
    // listed (e.g. permission denied). The overlay sits on top of the file views.
    void setError(const QString &error)
    {
        m_errorLabel->setText(error);
        m_errorOverlay->setVisible(!error.isEmpty());
        m_errorOverlay->raise();
    }

    void clearError() { setError({}); }

    // Points both views at the contents of the current directory in the file
    // system model. index(0,0) is the directory node itself.
    void setNormalRoot()
    {
        const QModelIndex rootProxy = m_proxy->mapFromSource(m_model->index(0, 0));
        m_treeView->setRootIndex(rootProxy);
        m_listView->setRootIndex(rootProxy);
    }

    // Switches the proxy back to the file system model (if needed) and stops any
    // running search.
    void leaveSearch()
    {
        m_searchModel->cancel();
        m_spinnerDelay.stop();
        setSpinnerRunning(false);
        if (m_proxy->sourceModel() != m_model) {
            m_proxy->setSourceModel(m_model);
            setNormalRoot();
        }
    }

    // Refills the path combo with the current directory (index 0) followed by
    // each of its parents up to the root, so the user can jump straight to any
    // ancestor. Blocked while rebuilding so it does not look like a navigation.
    void rebuildPathCombo()
    {
        QSignalBlocker blocker(m_pathCombo);
        m_pathCombo->clear();
        // When showing full paths a single shared folder icon suffices and avoids
        // a per-path icon lookup (which may hit the file system) for each ancestor
        // on every navigation. With short labels the per-entry icon is shown, so
        // use the path's own icon to match the visual identity of each entry.
        static const QIcon folderIcon = FileIconProvider::icon(QFileIconProvider::Folder);
        FilePath p = m_currentDir;
        while (!p.isEmpty()) {
            const QString label = m_showFullPaths
                                      ? p.toUserOutput()
                                      : (p.fileName().isEmpty() ? p.toUserOutput() : p.fileName());
            const QIcon icon = m_showFullPaths ? folderIcon : p.icon();
            m_pathCombo->addItem(icon, label, QVariant::fromValue(p));
            const FilePath parent = p.parentDir();
            if (parent.isEmpty() || parent == p)
                break;
            p = parent;
        }
        m_pathCombo->setCurrentIndex(0);
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
    d->m_searchModel = new SearchModel(this);
    d->m_proxy = new FileFilterProxy(this);
    d->m_proxy->setSourceModel(d->m_model);

    // Detail view
    d->m_treeView = new FileTreeView(this);
    d->m_treeView->setModel(d->m_proxy);
    d->m_treeView->setSelectionMode(QAbstractItemView::SingleSelection);
    d->m_treeView->setRootIsDecorated(false);
    // Editing is started explicitly (rename action / F2), never by clicks, so a
    // double-click keeps navigating instead of opening the rename editor. The
    // delegate preselects the base name (not the extension) when editing. Each
    // view needs its own delegate instance: a delegate connects its editing
    // signals to the view it is installed on, and sharing one across both views
    // delivers commitData/closeEditor to the wrong view.
    d->m_treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    // Install on the whole view (not just the name column) so the right-clicked
    // row is outlined across every column. Only the name column is editable, so
    // the rename editor still only opens there.
    auto *treeNameDelegate = new NameEditDelegate(d->m_treeView);
    d->m_treeView->setItemDelegate(treeNameDelegate);
    // All rows are the same height, so the view can skip per-row size-hint
    // computation (which shapes the item text). Without this, relayout on every
    // insertion measures every row and dominates the profile.
    d->m_treeView->setUniformRowHeights(true);
    d->m_treeView->setSortingEnabled(true);
    d->m_treeView->sortByColumn(FileSystemModel::NameColumn, Qt::AscendingOrder);
    d->m_treeView->setAlternatingRowColors(true);
    d->m_treeView->setDragEnabled(true);
    d->m_treeView->setDragDropMode(QAbstractItemView::DragOnly);
    // Name stretches; the other columns use fixed initial widths the user can
    // drag. ResizeToContents must not be used here: it re-measures (and shapes
    // the text of) every row on each relayout, which dominates with long result
    // lists.
    d->m_treeView->header()->setSectionResizeMode(FileSystemModel::NameColumn, QHeaderView::Stretch);
    d->m_treeView->header()
        ->setSectionResizeMode(FileSystemModel::SizeColumn, QHeaderView::Interactive);
    d->m_treeView->header()
        ->setSectionResizeMode(FileSystemModel::TypeColumn, QHeaderView::Interactive);
    d->m_treeView->header()
        ->setSectionResizeMode(FileSystemModel::DateModifiedColumn, QHeaderView::Interactive);
    d->m_treeView->header()->setStretchLastSection(false);
    d->m_treeView->header()->resizeSection(FileSystemModel::SizeColumn, 90);
    d->m_treeView->header()->resizeSection(FileSystemModel::TypeColumn, 120);
    d->m_treeView->header()->resizeSection(FileSystemModel::DateModifiedColumn, 140);

    // Icon view
    d->m_listView = new FileListView(this);
    d->m_listView->setModel(d->m_proxy);
    d->m_listView->setSelectionModel(d->m_treeView->selectionModel());
    d->m_listView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    auto *listNameDelegate = new IconViewDelegate(d->m_listView);
    d->m_listView->setItemDelegate(listNameDelegate);
    d->m_listView->setViewMode(QListView::IconMode);
    d->m_listView->setResizeMode(QListView::Adjust);
    d->m_listView->setGridSize(kGridSize);
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

    d->m_pathCombo = new QComboBox(this);
    d->m_pathCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    // The items hold full paths, which can be long; keep the combo from dictating
    // the dialog width and let it elide the collapsed text instead.
    d->m_pathCombo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    d->m_pathCombo->setMinimumContentsLength(10);

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

    // The selected entries. Uses selectedIndexes() filtered to column 0 rather
    // than selectedRows(): the icon view only selects the single displayed
    // column, so selectedRows() (which requires every column of a row to be
    // selected) would come back empty there.
    auto selectedPaths = [this] {
        FilePaths paths;
        const QModelIndexList indexes = d->m_treeView->selectionModel()->selectedIndexes();
        for (const QModelIndex &idx : indexes) {
            if (idx.column() != FileSystemModel::NameColumn)
                continue;
            const FilePath fp = idx.data(FileSystemModel::FilePathRole).value<FilePath>();
            if (!fp.isEmpty())
                paths << fp;
        }
        return paths;
    };

    // The entries the file actions operate on: the right-clicked entry while a
    // context menu is up (so the actions act on what was clicked without
    // changing the selection), otherwise the current selection.
    auto actionPaths = [this, selectedPaths]() -> FilePaths {
        if (d->m_rightClicked.isValid()) {
            const FilePath fp = QModelIndex(d->m_rightClicked)
                                    .siblingAtColumn(FileSystemModel::NameColumn)
                                    .data(FileSystemModel::FilePathRole)
                                    .value<FilePath>();
            return fp.isEmpty() ? FilePaths{} : FilePaths{fp};
        }
        return selectedPaths();
    };

    // Moves the target entries to the bin (trash / recycle bin). Only local
    // files have a bin; entries on remote devices are reported as not moved.
    auto moveToTrash = [this, actionPaths] {
        const FilePaths paths = actionPaths();
        if (paths.isEmpty())
            return;

        QStringList failed;
        for (const FilePath &fp : paths) {
            if (!fp.isLocal() || !QFile::moveToTrash(fp.toFSPathString()))
                failed << fp.toUserOutput();
        }
        if (!failed.isEmpty()) {
            AsynchronousMessageBox::warning(
                Tr::tr("Move to Bin"),
                Tr::tr("Could not move the following to the bin:\n%1").arg(failed.join(u'\n')));
        }

        // Refresh so the trashed entries disappear from the listing.
        setDirectory(d->m_currentDir);
    };

    // Renames the target entry by opening the view's inline editor on its name.
    // The model performs the actual rename when editing commits. Prefers the
    // right-clicked entry (context menu) over the current selection.
    auto renameSelected = [this] {
        auto *view = qobject_cast<QAbstractItemView *>(d->m_viewStack->currentWidget());
        if (!view)
            return;
        const QModelIndex target = d->m_rightClicked.isValid() ? QModelIndex(d->m_rightClicked)
                                                               : view->currentIndex();
        const QModelIndex idx = target.siblingAtColumn(FileSystemModel::NameColumn);
        if (!idx.isValid())
            return;
        view->edit(idx);
    };

    // Puts the target entries on the clipboard as file URLs.
    auto copySelected = [actionPaths] {
        const FilePaths paths = actionPaths();
        if (paths.isEmpty())
            return;
        QList<QUrl> urls;
        for (const FilePath &fp : paths)
            urls << fp.toUrl();
        auto *mime = new QMimeData;
        mime->setUrls(urls);
        QGuiApplication::clipboard()->setMimeData(mime);
    };

    // Copies the clipboard's files into the current directory, recursing into
    // directories via FileUtils::copyRecursively. An entry gets a "<name> copy"
    // name when one of that name already exists, so pasting back into the same
    // directory duplicates rather than overwrites. A modal progress dialog
    // reports each copied file and lets the user cancel.
    auto pasteFiles = [this] {
        const QMimeData *mime = QGuiApplication::clipboard()->mimeData();
        if (!mime || !mime->hasUrls())
            return;

        // A target path in the current directory that does not collide with an
        // existing entry, falling back to "<name> copy", "<name> copy 2", ...
        const auto uniqueTarget = [this](const FilePath &src) {
            FilePath target = d->m_currentDir.pathAppended(src.fileName());
            if (!target.exists())
                return target;
            const QString base = src.completeBaseName();
            const QString suffix = src.suffix();
            const QString dotSuffix = suffix.isEmpty() ? QString() : u'.' + suffix;
            for (int i = 1;; ++i) {
                const QString copy = (i == 1) ? Tr::tr("%1 copy").arg(base)
                                              : Tr::tr("%1 copy %2").arg(base).arg(i);
                target = d->m_currentDir.pathAppended(copy + dotSuffix);
                if (!target.exists())
                    return target;
            }
        };

        QProgressDialog progress(Tr::tr("Copying Files"), Tr::tr("Cancel"), 0, 0, this);
        progress.setWindowModality(Qt::WindowModal);
        progress.setMinimumDuration(500); // Don't flash for quick copies.

        // Called per copied file. Bumping the value keeps the (busy) dialog
        // alive once its minimum duration has passed, and pumps the event loop
        // so Cancel stays responsive.
        auto updateProgress = [&progress](const FilePath &filePath) {
            progress.setValue(progress.value() + 1);
            progress.setLabelText(Tr::tr("Copying\n%1").arg(filePath.fileName()));
            return !progress.wasCanceled();
        };

        QStringList failed;
        for (const QUrl &url : mime->urls()) {
            const FilePath src = FilePath::fromUrl(url);
            if (src.isEmpty())
                continue;
            const FilePath target = uniqueTarget(src);
            FileUtils::CopyAskingForOverwrite copy(updateProgress);
            const Result<FileUtils::CopyResult> res
                = FileUtils::copyRecursively(src, target, copy());
            if (!res) {
                failed << src.toUserOutput();
            } else if (*res == FileUtils::CopyResult::Canceled) {
                target.removeRecursively(); // Drop the half-copied entry.
                break;
            }
        }

        if (!failed.isEmpty()) {
            AsynchronousMessageBox::warning(
                Tr::tr("Paste"),
                Tr::tr("Could not paste the following:\n%1").arg(failed.join(u'\n')));
        }
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
    const QKeySequence gotoShortcut = HostOsInfo::isMacHost()
                                          ? QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_G)
                                          : QKeySequence(Qt::CTRL | Qt::Key_L);

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

    QAction *hideFilteredAction = new QAction(Tr::tr("Hide filtered files"), this);
    hideFilteredAction->setCheckable(true);
    hideFilteredAction->setChecked(HostOsInfo::isMacHost() ? false : true);

    connect(hideFilteredAction, &QAction::toggled, this, [this](bool checked) {
        d->m_proxy->setHideFiltered(checked);
    });

    QAction *showFullPathsAction = new QAction(Tr::tr("Show full paths in ComboBox"), this);
    showFullPathsAction->setCheckable(true);
    showFullPathsAction->setChecked(d->m_showFullPaths);

    connect(showFullPathsAction, &QAction::toggled, this, [this](bool checked) {
        d->m_showFullPaths = checked;
        d->rebuildPathCombo();
    });

    using namespace Layouting;
    // clang-format off
    Column {
        Row {
            QtDesignWidgets::IconButton {
                bindTo(&backButton),
                icon(Icons::PREV_NEUTRAL),
                Layouting::toolTip(withShortcut(Tr::tr("Back"), backShortcut)),
                onClicked(this, goBack),
            },
            QtDesignWidgets::IconButton {
                bindTo(&forwardButton),
                icon(Icons::NEXT_NEUTRAL),
                Layouting::toolTip(withShortcut(Tr::tr("Forward"), forwardShortcut)),
                onClicked(this, goForward),
            },
            QtDesignWidgets::IconButton {
                bindTo(&upButton),
                icon(Icons::ARROW_UP),
                Layouting::toolTip(
                    withShortcut(Tr::tr("Go to parent directory"), parentShortcut)),
                onClicked(this, goToParent),
            },
            QtDesignWidgets::IconButton {
                icon(Icons::SLASH),
                Layouting::toolTip(withShortcut(Tr::tr("Go to folder"), gotoShortcut)),
                onClicked(this, [this] { d->m_gotoOverlay->popup(d->m_currentDir); }),
            },
            QtDesignWidgets::IconButton {
                icon(Icons::SETTINGS),
                Layouting::toolTip(Tr::tr("View options")),
                onClicked(this, [=] {
                    QMenu menu;
                    menu.addAction(iconViewAction);
                    menu.addAction(listViewAction);
                    menu.addSeparator();
                    menu.addAction(showHiddenAction);
                    menu.addAction(hideFilteredAction);
                    menu.addAction(showFullPathsAction);
                    menu.exec(QCursor::pos());
                }),
            },
            d->m_pathCombo,
            LineEdit {
                bindTo(&d->m_searchEdit),
                placeholderText(Tr::tr("Search")),
            },
        },

        Splitter {
            bindTo(&splitter),
            orientation(Qt::Horizontal),
            childrenCollapsible(false),
            d->m_sidebarView,

            Column {
                noMargin,

                Row {
                    st,
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
                    },
                    st,
                },

                d->m_viewStack,
                Column {
                    d->m_filterCombo,
                    Row {
                        QtDesignWidgets::Button {
                            text(Tr::tr("New Folder")),
                            onClicked(this, createFolder),
                            role(QtcButton::Role::MediumSecondary),
                        },
                        st,
                        DialogButtonBox {
                            dialogButton(QDialogButtonBox::ButtonRole::AcceptRole,
                                QtDesignWidgets::Button {
                                    bindTo(&d->m_acceptButton),
                                    stdButtonText(QDialogButtonBox::StandardButton::Open),
                                    enabled(false),
                                    onClicked(this, handleOpen),
                                    role(QtcButton::Role::MediumPrimary),
                                }
                            ),
                            dialogButton(QDialogButtonBox::ButtonRole::RejectRole,
                                QtDesignWidgets::Button {
                                    stdButtonText(QDialogButtonBox::StandardButton::Cancel),
                                    onClicked(this, [this] { reject(); }),
                                    role(QtcButton::Role::MediumSecondary),
                                }
                            ),
                        },
                    }
                },
            }
        }
    }.attachTo(this);
    // clang-format on

    const bool saveMode = d->m_acceptMode == QFileDialog::AcceptSave;
    d->m_fileNameEdit->setMinimumWidth(280);
    d->m_fileNameEdit->setVisible(saveMode);
    d->m_fileNameLabel->setVisible(saveMode);

    connect(d->m_fileNameEdit, &QLineEdit::textChanged, this, [this] {
        d->updateAcceptButtonState();
    });

    d->m_searchEdit->setButtonIcon(FancyLineEdit::Left, Icons::MAGNIFIER.icon());
    d->m_searchEdit->setButtonVisible(FancyLineEdit::Left, true);
    d->m_searchEdit->setButtonIcon(FancyLineEdit::Right, Icons::EDIT_CLEAR.icon());
    d->m_searchEdit->setButtonVisible(FancyLineEdit::Right, true);
    d->m_searchEdit->setButtonToolTip(FancyLineEdit::Right, Tr::tr("Clear"));
    d->m_searchEdit->setAutoHideButton(FancyLineEdit::Right, true);
    connect(d->m_searchEdit, &FancyLineEdit::rightButtonClicked, d->m_searchEdit, &QLineEdit::clear);
    d->m_searchEdit->setFixedWidth(180);
    // Overlay shown on top of the file views while a recursive search runs or
    // the file system model fetches a directory listing.
    d->m_spinner
        = new SpinnerSolution::Spinner(SpinnerSolution::SpinnerSize::Medium, d->m_viewStack);
    d->m_spinner->hide();

    // Overlay shown on top of the file views when the current directory cannot
    // be listed. It reports the error from the file system model.
    d->m_errorOverlay = new OverlayWidget(d->m_viewStack);
    d->m_errorLabel = new InfoLabel({}, InfoLabel::Error);
    d->m_errorLabel->setAlignment(Qt::AlignCenter);
    Layouting::Grid {
        Layouting::GridCell({Layouting::Align(Qt::AlignCenter, d->m_errorLabel)}),
    }.attachTo(d->m_errorOverlay);
    d->m_errorOverlay->attachToWidget(d->m_viewStack);
    d->m_errorOverlay->hide();
    d->m_searchTimer.setSingleShot(true);
    d->m_searchTimer.setInterval(300);
    connect(d->m_searchEdit, &QLineEdit::textChanged, this, [this](const QString &text) {
        if (text.trimmed().isEmpty()) {
            d->m_searchTimer.stop();
            d->leaveSearch();
            return;
        }
        d->m_searchTimer.start();
    });
    connect(&d->m_searchTimer, &QTimer::timeout, this, [this] {
        const QString needle = d->m_searchEdit->text().trimmed();
        if (needle.isEmpty())
            return;
        if (!d->isSearching()) {
            d->m_proxy->setSourceModel(d->m_searchModel);
            d->m_treeView->setRootIndex({});
            d->m_listView->setRootIndex({});
        }
        // Set the spinner running after search() so the cancel() it performs
        // internally (which may emit searchFinished) cannot leave it hidden.
        d->m_searchModel->search(d->m_currentDir, needle);
        d->m_spinnerDelay.stop(); // The search owns the spinner now.
        d->setSpinnerRunning(true);
    });
    connect(d->m_searchModel, &SearchModel::searchFinished, this, [this] {
        d->setSpinnerRunning(false);
    });

    // While the model fetches the listing of the current directory, show the
    // spinner — but only once the fetch takes noticeably long. The views
    // trigger the fetch lazily after setDirectory(), so this is signal-driven.
    d->m_spinnerDelay.setSingleShot(true);
    d->m_spinnerDelay.setInterval(200);
    connect(&d->m_spinnerDelay, &QTimer::timeout, this, [this] { d->setSpinnerRunning(true); });
    connect(
        d->m_model,
        &FileSystemModel::fetchingChanged,
        this,
        [this](const FilePath &path, bool fetching) {
            if (path != d->m_currentDir)
                return;
            if (fetching) {
                // A fresh listing supersedes any previously reported error.
                d->clearError();
                if (!d->isSearching())
                    d->m_spinnerDelay.start();
            } else {
                d->m_spinnerDelay.stop();
                // While searching, the spinner belongs to the search.
                if (!d->isSearching())
                    d->setSpinnerRunning(false);
            }
        });

    connect(
        d->m_model,
        &FileSystemModel::directoryLoadFailed,
        this,
        [this](const FilePath &path, const QString &error) {
            if (path != d->m_currentDir)
                return;
            d->setError(error);
        });

    // macOS-style "Go to Folder" (Cmd+Shift+G): drop in an overlay where the
    // user can type an absolute path. The same sequence works on other hosts.
    d->m_gotoOverlay = new GoToFolderOverlay(this);
    connect(d->m_gotoOverlay, &GoToFolderOverlay::pathEntered, this, [this](const QString &input) {
        QString text = input;
        if (text == "~"_L1 || text.startsWith("~/"_L1))
            text = QDir::homePath() + text.mid(1);
        FilePath path = FilePath::fromUserInput(text);
        if (!path.isAbsolutePath())
            path = d->m_currentDir.resolvePath(text);
        if (path.isDir()) {
            setDirectory(path);
        } else if (path.exists()) {
            d->m_selectedFile = path;
            accept();
        }
    });

    auto *goToFolderAction = new QAction(this);
    goToFolderAction->setShortcut(gotoShortcut);
    goToFolderAction->setShortcutContext(Qt::WindowShortcut);
    connect(goToFolderAction, &QAction::triggered, this, [this] {
        d->m_gotoOverlay->popup(d->m_currentDir);
    });
    addAction(goToFolderAction);

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

    // Context actions for the file views: rename, copy, paste and move-to-bin.
    // Their shortcuts are scoped to the file views so they do not shadow the same
    // key sequences while editing in the search or file-name fields.
    auto *renameAction = new QAction(Tr::tr("Rename..."), this);
    renameAction->setShortcut(QKeySequence(Qt::Key_F2));
    renameAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(renameAction, &QAction::triggered, this, renameSelected);

    auto *copyAction = new QAction(Icons::COPY.icon(), Tr::tr("Copy"), this);
    copyAction->setIconVisibleInMenu(true);
    copyAction->setShortcut(QKeySequence(QKeySequence::Copy));
    copyAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(copyAction, &QAction::triggered, this, copySelected);

    auto *pasteAction = new QAction(Icons::PASTE.icon(), Tr::tr("Paste"), this);
    pasteAction->setIconVisibleInMenu(true);
    pasteAction->setShortcut(QKeySequence(QKeySequence::Paste));
    pasteAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(pasteAction, &QAction::triggered, this, pasteFiles);

    // Move the selection to the bin: Cmd+Backspace on macOS, Delete elsewhere.
    auto *trashAction = new QAction(Icons::CLEAN.icon(), Tr::tr("Move to Bin"), this);
    trashAction->setIconVisibleInMenu(true);
    trashAction->setShortcut(HostOsInfo::isMacHost() ? QKeySequence(Qt::CTRL | Qt::Key_Backspace)
                                                     : QKeySequence(QKeySequence::Delete));
    trashAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(trashAction, &QAction::triggered, this, moveToTrash);

    // Register the shortcuts once, on the views' common parent, so the context
    // covers both the tree and icon view (and their editors) but not the search
    // or name fields (which are not its children). Adding each action to both
    // views instead would register every shortcut twice and make Qt treat them
    // as ambiguous - firing nothing, most visibly in the icon view.
    const QList<QAction *> fileActions{renameAction, copyAction, pasteAction, trashAction};
    for (QAction *action : fileActions)
        d->m_viewStack->addAction(action);

    // Keeps the actions' enabled state in sync with the target and clipboard.
    // The bin only exists for local files, so it stays disabled for remote ones.
    auto updateFileActions = [renameAction, copyAction, pasteAction, trashAction, actionPaths] {
        const FilePaths paths = actionPaths();
        const bool allLocal = !paths.isEmpty()
                              && std::all_of(paths.begin(), paths.end(), [](const FilePath &fp) {
                                     return fp.isLocal();
                                 });
        renameAction->setEnabled(paths.size() == 1);
        copyAction->setEnabled(!paths.isEmpty());
        const QMimeData *mime = QGuiApplication::clipboard()->mimeData();
        pasteAction->setEnabled(mime && mime->hasUrls());
        trashAction->setEnabled(allLocal);
    };
    updateFileActions();
    connect(
        d->m_treeView->selectionModel(),
        &QItemSelectionModel::selectionChanged,
        this,
        [updateFileActions] { updateFileActions(); });
    connect(QGuiApplication::clipboard(), &QClipboard::dataChanged, this, [updateFileActions] {
        updateFileActions();
    });

    // Committing an inline rename re-sorts the entry to its new position, which
    // drops the view's selection (most visibly in the icon view) and leaves the
    // actions disabled. Reselect the renamed entry once the edit settles so the
    // selection — and the actions — follow it.
    auto reselectRenamed = [this, updateFileActions](QWidget *editor) {
        auto *lineEdit = qobject_cast<QLineEdit *>(editor);
        if (!lineEdit)
            return;
        const FilePath renamed = d->m_currentDir.pathAppended(lineEdit->text());
        QTimer::singleShot(0, this, [this, renamed, updateFileActions] {
            const QModelIndex src = d->m_model->index(renamed);
            if (src.isValid()) {
                d->m_treeView->selectionModel()->setCurrentIndex(
                    d->m_proxy->mapFromSource(src),
                    QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
            }
            updateFileActions();
        });
    };
    connect(treeNameDelegate, &QAbstractItemDelegate::commitData, this, reselectRenamed);
    connect(listNameDelegate, &QAbstractItemDelegate::commitData, this, reselectRenamed);

    // Right-click on either view opens a context menu with the file actions.
    auto showContextMenu = [this,
                            renameAction,
                            copyAction,
                            pasteAction,
                            trashAction,
                            updateFileActions,
                            treeNameDelegate,
                            listNameDelegate](QAbstractItemView *view, const QPoint &pos) {
        // Track the right-clicked entry separately from the selection (which is
        // left untouched) and outline it for the duration of the menu. The
        // actions act on this entry, even when it is a filtered-out one.
        const QModelIndex idx = view->indexAt(pos).siblingAtColumn(FileSystemModel::NameColumn);
        d->m_rightClicked = idx;
        treeNameDelegate->setRightClickedRow(idx);
        listNameDelegate->setRightClickedRow(idx);
        view->viewport()->update();
        updateFileActions();

        QMenu menu;
        menu.addAction(renameAction);
        menu.addSeparator();
        menu.addAction(copyAction);
        menu.addAction(pasteAction);
        menu.addSeparator();
        menu.addAction(trashAction);
        menu.exec(view->viewport()->mapToGlobal(pos));

        d->m_rightClicked = QModelIndex();
        treeNameDelegate->setRightClickedRow({});
        listNameDelegate->setRightClickedRow({});
        view->viewport()->update();
        updateFileActions();
    };
    d->m_treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    d->m_listView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(d->m_treeView, &QWidget::customContextMenuRequested, this, [this, showContextMenu](const QPoint &pos) {
        showContextMenu(d->m_treeView, pos);
    });
    connect(d->m_listView, &QWidget::customContextMenuRequested, this, [this, showContextMenu](const QPoint &pos) {
        showContextMenu(d->m_listView, pos);
    });

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
        const QModelIndex idx = proxyIndex.siblingAtColumn(0);
        const FilePath fp = idx.data(FileSystemModel::FilePathRole).value<FilePath>();
        const auto flags = idx.data(FileSystemModel::FileFlagsRole).value<FilePathInfo::FileFlags>();
        const bool isDir = flags & FilePathInfo::DirectoryType;
        // Directories always navigate on activation (double-click). In Directory
        // mode, selecting the directory itself is reserved for the accept button.
        if (isDir) {
            setDirectory(fp);
        } else if (d->acceptsAsSelection(isDir) && d->m_proxy->acceptsContentRow(idx)) {
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
            [this] {
                // selectedIndexes() yields only selectable, enabled cells, so a
                // click on a filtered-out (enabled but non-selectable) entry
                // leaves it empty rather than producing an unusable index. The
                // changed-selection delta can be non-empty while its indexes()
                // are all filtered out, so query the model, not that delta.
                const QModelIndexList indexes
                    = d->m_treeView->selectionModel()->selectedIndexes();
                if (indexes.isEmpty()) {
                    d->m_selectedFile = {};
                    d->updateAcceptButtonState();
                    return;
                }
                const QModelIndex proxyIdx = indexes.constFirst().siblingAtColumn(0);
                const FilePath fp = proxyIdx.data(FileSystemModel::FilePathRole).value<FilePath>();
                const auto flags
                    = proxyIdx.data(FileSystemModel::FileFlagsRole).value<FilePathInfo::FileFlags>();
                const bool isDir = flags & FilePathInfo::DirectoryType;
                const bool acceptable
                    = d->acceptsAsSelection(isDir) && d->m_proxy->acceptsContentRow(proxyIdx);
                d->m_selectedFile = acceptable ? fp : FilePath();
                d->updateAcceptButtonState();
            });

    // Selecting an ancestor from the path combo navigates to it. Uses activated
    // (user interaction only) so the programmatic rebuilds do not navigate.
    connect(d->m_pathCombo, &QComboBox::activated, this, [this](int index) {
        const FilePath fp = d->m_pathCombo->itemData(index).value<FilePath>();
        if (!fp.isEmpty() && fp != d->m_currentDir)
            setDirectory(fp);
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

    // Navigating away from search results returns to the normal listing.
    if (!d->m_searchEdit->text().isEmpty()) {
        QSignalBlocker blocker(d->m_searchEdit);
        d->m_searchEdit->clear();
    }
    d->leaveSearch();

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
    d->rebuildPathCombo();
    d->updateAcceptButtonState();

    d->clearError();
    d->m_model->setRootPath(d->m_currentDir);

    // index(0,0) is the directory node itself; set as root so both views show its contents.
    d->setNormalRoot();
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
        d->m_filterCombo->addItem(f, f);
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
    d->m_acceptButton->setText(
        saveMode ? Layouting::standardButtonText(QDialogButtonBox::Save)
                 : Layouting::standardButtonText(QDialogButtonBox::Open));
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
    // selectedIndexes() filtered to column 0, not selectedRows(): the icon view
    // selects only its single displayed column, for which selectedRows() (which
    // wants every column selected) returns nothing.
    const QModelIndexList indexes = d->m_treeView->selectionModel()->selectedIndexes();
    for (const QModelIndex &proxyIdx : indexes) {
        if (proxyIdx.column() != FileSystemModel::NameColumn)
            continue;
        const auto flags
            = proxyIdx.data(FileSystemModel::FileFlagsRole).value<FilePathInfo::FileFlags>();
        if (d->acceptsAsSelection(flags & FilePathInfo::DirectoryType)
            && d->m_proxy->acceptsContentRow(proxyIdx))
            result << proxyIdx.data(FileSystemModel::FilePathRole).value<FilePath>();
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
