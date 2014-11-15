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
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "debuggertooltipmanager.h"
#include "debuggerinternalconstants.h"
#include "debuggerengine.h"
#include "debuggeractions.h"
#include "stackhandler.h"
#include "debuggercore.h"
#include "watchhandler.h"
#include "watchwindow.h"
#include "sourceutils.h"

#include <coreplugin/icore.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/documentmodel.h>
#include <texteditor/texteditor.h>

#include <utils/tooltip/tooltip.h>
#include <utils/tooltip/tipcontents.h>
#include <utils/qtcassert.h>

#include <QAbstractItemModel>
#include <QApplication>
#include <QClipboard>
#include <QDebug>
#include <QDesktopWidget>
#include <QFileInfo>
#include <QLabel>
#include <QScrollBar>
#include <QSortFilterProxyModel>
#include <QStack>
#include <QStandardItemModel>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

using namespace Core;
using namespace TextEditor;

namespace Debugger {
namespace Internal {

//#define DEBUG(x) qDebug() << x
#define DEBUG(x)

// Expire tooltips after n days on (no longer load them) in order
// to avoid them piling up.
enum { toolTipsExpiryDays = 6 };

const char sessionSettingsKeyC[] = "DebuggerToolTips";
const char sessionDocumentC[] = "DebuggerToolTips";
const char sessionVersionAttributeC[] = "version";
const char toolTipElementC[] = "DebuggerToolTip";
//const char toolTipClassAttributeC[] = "class";
const char fileNameAttributeC[] = "name";
const char functionAttributeC[] = "function";
const char textPositionAttributeC[] = "position";
const char textLineAttributeC[] = "line";
const char textColumnAttributeC[] = "column";
const char offsetXAttributeC[] = "offset_x";
const char offsetYAttributeC[] = "offset_y";
const char engineTypeAttributeC[] = "engine";
const char dateAttributeC[] = "date";
const char treeElementC[] = "tree";
const char treeExpressionAttributeC[] = "expression";
const char treeInameAttributeC[] = "iname";
const char modelElementC[] = "model";
const char modelColumnCountAttributeC[] = "columncount";
const char modelRowElementC[] = "row";
const char modelItemElementC[] = "item";

static void purgeClosedToolTips();

class DebuggerToolTipHolder;
QList<QPointer<DebuggerToolTipHolder>> m_tooltips;
bool m_debugModeActive;

// Forward a stream reader across end elements looking for the
// next start element of a desired type.
static bool readStartElement(QXmlStreamReader &r, const char *name)
{
    while (r.tokenType() != QXmlStreamReader::StartElement
            || r.name() != QLatin1String(name))
        switch (r.readNext()) {
        case QXmlStreamReader::EndDocument:
            return false;
        case QXmlStreamReader::NoToken:
        case QXmlStreamReader::Invalid:
            qWarning("'%s'/'%s' encountered while looking for start element '%s'.",
                    qPrintable(r.tokenString()),
                    qPrintable(r.name().toString()), name);
            return false;
        default:
            break;
        }
    return true;
}

// A label that can be dragged to drag something else.

class DraggableLabel : public QLabel
{
public:
    explicit DraggableLabel(QWidget *target)
        : m_target(target), m_moveStartPos(-1, -1), active(false)
    {}

    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);

public:
    QWidget *m_target;
    QPoint m_moveStartPos;
    QPoint m_offset;
    bool active;
};

void DraggableLabel::mousePressEvent(QMouseEvent * event)
{
    if (active && event->button() == Qt::LeftButton) {
        m_moveStartPos = event->globalPos();
        event->accept();
    }
    QLabel::mousePressEvent(event);
}

void DraggableLabel::mouseReleaseEvent(QMouseEvent * event)
{
    if (active && event->button() == Qt::LeftButton)
        m_moveStartPos = QPoint(-1, -1);
    QLabel::mouseReleaseEvent(event);
}

void DraggableLabel::mouseMoveEvent(QMouseEvent * event)
{
    if (active && (event->buttons() & Qt::LeftButton)) {
        if (m_moveStartPos != QPoint(-1, -1)) {
            const QPoint newPos = event->globalPos();
            const QPoint offset = newPos - m_moveStartPos;

            m_target->move(m_target->pos() + offset);
            m_offset += offset;

            m_moveStartPos = newPos;
        }
        event->accept();
    }
    QLabel::mouseMoveEvent(event);
}

// Helper for building a QStandardItemModel of a tree form (see TreeModelVisitor).
// The recursion/building is based on the scheme: \code
// <row><item1><item2>
//     <row><item11><item12></row>
// </row>
// \endcode

class StandardItemTreeModelBuilder
{
public:
    typedef QList<QStandardItem *> StandardItemRow;

    explicit StandardItemTreeModelBuilder(QStandardItemModel *m, Qt::ItemFlags f = Qt::ItemIsSelectable);

    void addItem(const QString &);
    void startRow();
    void endRow();

private:
    void pushRow();

    QStandardItemModel *m_model;
    const Qt::ItemFlags m_flags;
    StandardItemRow m_row;
    QStack<QStandardItem *> m_rowParents;
};

StandardItemTreeModelBuilder::StandardItemTreeModelBuilder(QStandardItemModel *m, Qt::ItemFlags f) :
    m_model(m), m_flags(f)
{
    m_model->removeRows(0, m_model->rowCount());
}

void StandardItemTreeModelBuilder::addItem(const QString &s)
{
    QStandardItem *item = new QStandardItem(s);
    item->setFlags(m_flags);
    m_row.push_back(item);
}

void StandardItemTreeModelBuilder::pushRow()
{
    if (m_rowParents.isEmpty())
        m_model->appendRow(m_row);
    else
        m_rowParents.top()->appendRow(m_row);
    m_rowParents.push(m_row.front());
    m_row.clear();
}

void StandardItemTreeModelBuilder::startRow()
{
    // Push parent in case rows are nested. This is a Noop for the very first row.
    if (!m_row.isEmpty())
        pushRow();
}

void StandardItemTreeModelBuilder::endRow()
{
    if (!m_row.isEmpty()) // Push row if no child rows have been encountered
        pushRow();
    m_rowParents.pop();
}

// Helper visitor base class for recursing over a tree model
// (see StandardItemTreeModelBuilder for the scheme).
class TreeModelVisitor
{
public:
    virtual void run() { run(QModelIndex()); }

protected:
    TreeModelVisitor(const QAbstractItemModel *model) : m_model(model) {}

    virtual void rowStarted() {}
    virtual void handleItem(const QModelIndex &m) = 0;
    virtual void rowEnded() {}

    const QAbstractItemModel *model() const { return m_model; }

private:
    void run(const QModelIndex &parent);

    const QAbstractItemModel *m_model;
};

void TreeModelVisitor::run(const QModelIndex &parent)
{
    const int columnCount = m_model->columnCount(parent);
    const int rowCount = m_model->rowCount(parent);
    for (int r = 0; r < rowCount; r++) {
        rowStarted();
        QModelIndex left;
        for (int c = 0; c < columnCount; c++) {
            const QModelIndex index = m_model->index(r, c, parent);
            handleItem(index);
            if (!c)
                left = index;
        }
        if (left.isValid())
            run(left);
        rowEnded();
    }
}

// Visitor writing out a tree model in XML format.
class XmlWriterTreeModelVisitor : public TreeModelVisitor
{
public:
    XmlWriterTreeModelVisitor(const QAbstractItemModel *model, QXmlStreamWriter &w);

    virtual void run();

protected:
    virtual void rowStarted() { m_writer.writeStartElement(QLatin1String(modelRowElementC)); }
    virtual void handleItem(const QModelIndex &m);
    virtual void rowEnded() { m_writer.writeEndElement(); }

private:
    const QString m_modelItemElement;
    QXmlStreamWriter &m_writer;
};

XmlWriterTreeModelVisitor::XmlWriterTreeModelVisitor(const QAbstractItemModel *model, QXmlStreamWriter &w) :
    TreeModelVisitor(model), m_modelItemElement(QLatin1String(modelItemElementC)), m_writer(w)
{
}

void XmlWriterTreeModelVisitor::run()
{
    m_writer.writeStartElement(QLatin1String(modelElementC));
    const int columnCount = model()->columnCount();
    m_writer.writeAttribute(QLatin1String(modelColumnCountAttributeC), QString::number(columnCount));
   TreeModelVisitor::run();
    m_writer.writeEndElement();
}

void XmlWriterTreeModelVisitor::handleItem(const QModelIndex &m)
{
    const QString value = m.data(Qt::DisplayRole).toString();
    if (value.isEmpty())
        m_writer.writeEmptyElement(m_modelItemElement);
    else
        m_writer.writeTextElement(m_modelItemElement, value);
}

// TreeModelVisitor for debugging/copying models
class DumpTreeModelVisitor : public TreeModelVisitor
{
public:
    enum Mode
    {
        DebugMode,      // For debugging, "|'data'|"
        ClipboardMode   // Tab-delimited "\tdata" for clipboard (see stack window)
    };
    explicit DumpTreeModelVisitor(const QAbstractItemModel *model, Mode m, QTextStream &s);

protected:
    virtual void rowStarted();
    virtual void handleItem(const QModelIndex &m);
    virtual void rowEnded();

private:
    const Mode m_mode;

    QTextStream &m_stream;
    int m_level;
    unsigned m_itemsInRow;
};

DumpTreeModelVisitor::DumpTreeModelVisitor(const QAbstractItemModel *model, Mode m, QTextStream &s) :
    TreeModelVisitor(model), m_mode(m), m_stream(s), m_level(0), m_itemsInRow(0)
{
    if (m_mode == DebugMode)
        m_stream << model->metaObject()->className() << '/' << model->objectName();
}

void DumpTreeModelVisitor::rowStarted()
{
    m_level++;
    if (m_itemsInRow) { // Nested row.
        m_stream << '\n';
        m_itemsInRow = 0;
    }
    switch (m_mode) {
    case DebugMode:
        m_stream << QString(2 * m_level, QLatin1Char(' '));
        break;
    case ClipboardMode:
        m_stream << QString(m_level, QLatin1Char('\t'));
        break;
    }
}

void DumpTreeModelVisitor::handleItem(const QModelIndex &m)
{
    const QString data = m.data().toString();
    switch (m_mode) {
    case DebugMode:
        if (m.column())
            m_stream << '|';
        m_stream << '\'' << data << '\'';
        break;
    case ClipboardMode:
        if (m.column())
            m_stream << '\t';
        m_stream << data;
        break;
    }
    m_itemsInRow++;
}

void DumpTreeModelVisitor::rowEnded()
{
    if (m_itemsInRow) {
        m_stream << '\n';
        m_itemsInRow = 0;
    }
    m_level--;
}

/*
static QDebug operator<<(QDebug d, const QAbstractItemModel &model)
{
    QString s;
    QTextStream str(&s);
    Debugger::Internal::DumpTreeModelVisitor v(&model, Debugger::Internal::DumpTreeModelVisitor::DebugMode, str);
    v.run();
    qCDebug(tooltip).nospace() << s;
    return d;
}
*/

/*!
    \class Debugger::Internal::TooltipFilterModel

    \brief The TooltipFilterModel class is a model for tooltips filtering an
    item on the watchhandler matching its tree on the iname.

    In addition, suppress the model's tooltip data to avoid a tooltip on a tooltip.
*/

class TooltipFilterModel : public QSortFilterProxyModel
{
public:
    TooltipFilterModel() {}

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const
    {
        return role == Qt::ToolTipRole
            ? QVariant() : QSortFilterProxyModel::data(index, role);
    }

    static bool isSubIname(const QByteArray &haystack, const QByteArray &needle)
    {
        return haystack.size() > needle.size()
           && haystack.startsWith(needle)
           && haystack.at(needle.size()) == '.';
    }

    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
    {
        const QModelIndex nameIndex = sourceModel()->index(sourceRow, 0, sourceParent);
        const QByteArray iname = nameIndex.data(LocalsINameRole).toByteArray();
//        DEBUG("ACCEPTING FILTER" << iname
//                 << (iname == m_iname || isSubIname(iname, m_iname) || isSubIname(m_iname, iname));
        return iname == m_iname || isSubIname(iname, m_iname) || isSubIname(m_iname, iname);
    }

    QByteArray m_iname;
};

/////////////////////////////////////////////////////////////////////////
//
// TreeModelCopyVisitor builds a QStandardItem from a tree model (copy).
//
/////////////////////////////////////////////////////////////////////////

class TreeModelCopyVisitor : public TreeModelVisitor
{
public:
    TreeModelCopyVisitor(const QAbstractItemModel *source, QStandardItemModel *target) :
        TreeModelVisitor(source), m_builder(target)
    {}

protected:
    virtual void rowStarted() { m_builder.startRow(); }
    virtual void handleItem(const QModelIndex &m) { m_builder.addItem(m.data().toString()); }
    virtual void rowEnded() { m_builder.endRow(); }

private:
    StandardItemTreeModelBuilder m_builder;
};

/*!
    \class Debugger::Internal::DebuggerToolTipTreeView

    \brief The DebuggerToolTipTreeView class is a treeview that adapts its size
    to the model contents (also while expanding)
    to be used within DebuggerTreeViewToolTipWidget.

*/

class DebuggerToolTipTreeView : public QTreeView
{
public:
    explicit DebuggerToolTipTreeView(QWidget *parent = 0);

    QSize sizeHint() const { return m_size; }

    void computeSize();

private:
    void expandNode(const QModelIndex &idx);
    void collapseNode(const QModelIndex &idx);
    int computeHeight(const QModelIndex &index) const;

    QSize m_size;
};

DebuggerToolTipTreeView::DebuggerToolTipTreeView(QWidget *parent) :
    QTreeView(parent)
{
    setHeaderHidden(true);
    setEditTriggers(NoEditTriggers);

    setUniformRowHeights(true);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    connect(this, &QTreeView::collapsed, this, &DebuggerToolTipTreeView::computeSize,
        Qt::QueuedConnection);
    connect(this, &QTreeView::expanded, this, &DebuggerToolTipTreeView::computeSize,
        Qt::QueuedConnection);

    connect(this, &QTreeView::expanded,
        this, &DebuggerToolTipTreeView::expandNode);
    connect(this, &QTreeView::collapsed,
        this, &DebuggerToolTipTreeView::collapseNode);
}

void DebuggerToolTipTreeView::expandNode(const QModelIndex &idx)
{
    model()->setData(idx, true, LocalsExpandedRole);
}

void DebuggerToolTipTreeView::collapseNode(const QModelIndex &idx)
{
    model()->setData(idx, false, LocalsExpandedRole);
}

int DebuggerToolTipTreeView::computeHeight(const QModelIndex &index) const
{
    int s = rowHeight(index);
    const int rowCount = model()->rowCount(index);
    for (int i = 0; i < rowCount; ++i)
        s += computeHeight(model()->index(i, 0, index));
    return s;
}

void DebuggerToolTipTreeView::computeSize()
{
    int columns = 30; // Decoration
    int rows = 0;
    bool rootDecorated = false;

    if (QAbstractItemModel *m = model()) {
        WatchTreeView::reexpand(this, m->index(0, 0));
        const int columnCount = m->columnCount();
        rootDecorated = m->rowCount() > 0;
        if (rootDecorated)
        for (int i = 0; i < columnCount; ++i) {
            resizeColumnToContents(i);
            columns += sizeHintForColumn(i);
        }
        if (columns < 100)
            columns = 100; // Prevent toolbar from shrinking when displaying 'Previous'
        rows += computeHeight(QModelIndex());

        // Fit tooltip to screen, showing/hiding scrollbars as needed.
        // Add a bit of space to account for tooltip border, and not
        // touch the border of the screen.
        QPoint pos(x(), y());
        QTC_ASSERT(QApplication::desktop(), return);
        QRect desktopRect = QApplication::desktop()->availableGeometry(pos);
        const int maxWidth = desktopRect.right() - pos.x() - 5 - 5;
        const int maxHeight = desktopRect.bottom() - pos.y() - 5 - 5;

        if (columns > maxWidth)
            rows += horizontalScrollBar()->height();

        if (rows > maxHeight) {
            setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
            rows = maxHeight;
            columns += verticalScrollBar()->width();
        } else {
            setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        }

        if (columns > maxWidth) {
            setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
            columns = maxWidth;
        } else {
            setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        }
    }

    m_size = QSize(columns + 5, rows + 5);
    setMinimumSize(m_size);
    setMaximumSize(m_size);
    setRootIsDecorated(rootDecorated);
}

QString DebuggerToolTipManager::treeModelClipboardContents(const QAbstractItemModel *model)
{
    QString rc;
    QTC_ASSERT(model, return rc);
    QTextStream str(&rc);
    DumpTreeModelVisitor v(model, DumpTreeModelVisitor::ClipboardMode, str);
    v.run();
    return rc;
}


/////////////////////////////////////////////////////////////////////////
//
// DebuggerToolTipWidget
//
/////////////////////////////////////////////////////////////////////////

class DebuggerToolTipWidget : public QWidget
{
public:
    DebuggerToolTipWidget()
    {
        setAttribute(Qt::WA_DeleteOnClose);

        isPinned = false;
        const QIcon pinIcon(QLatin1String(":/debugger/images/pin.xpm"));

        pinButton = new QToolButton;
        pinButton->setIcon(pinIcon);

        auto copyButton = new QToolButton;
        copyButton->setIcon(QIcon(QLatin1String(Core::Constants::ICON_COPY)));

        titleLabel = new DraggableLabel(this);
        titleLabel->setMinimumWidth(40); // Ensure a draggable area even if text is empty.
        titleLabel->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);

        auto toolBar = new QToolBar(this);
        toolBar->setProperty("_q_custom_style_disabled", QVariant(true));
        const QList<QSize> pinIconSizes = pinIcon.availableSizes();
        if (!pinIconSizes.isEmpty())
            toolBar->setIconSize(pinIconSizes.front());
        toolBar->addWidget(pinButton);
        toolBar->addWidget(copyButton);
        toolBar->addWidget(titleLabel);

        treeView = new DebuggerToolTipTreeView(this);
        treeView->setFocusPolicy(Qt::NoFocus);

        auto mainLayout = new QVBoxLayout(this);
        mainLayout->setSizeConstraint(QLayout::SetFixedSize);
        mainLayout->setContentsMargins(0, 0, 0, 0);
        mainLayout->addWidget(toolBar);
        mainLayout->addWidget(treeView);

        connect(copyButton, &QAbstractButton::clicked, [this] {
            QString clipboardText = DebuggerToolTipManager::treeModelClipboardContents(treeView->model());
            QClipboard *clipboard = QApplication::clipboard();
            clipboard->setText(clipboardText, QClipboard::Selection);
            clipboard->setText(clipboardText, QClipboard::Clipboard);
        });
        DEBUG("CREATE DEBUGGERTOOLTIP WIDGET");
    }

    ~DebuggerToolTipWidget()
    {
        DEBUG("DESTROY DEBUGGERTOOLTIP WIDGET");
    }

    void closeEvent(QCloseEvent *)
    {
        DEBUG("CLOSE DEBUGGERTOOLTIP WIDGET");
    }

    void enterEvent(QEvent *)
    {
        DEBUG("ENTER DEBUGGERTOOLTIP WIDGET");
    }

    void leaveEvent(QEvent *)
    {
        DEBUG("LEAVE DEBUGGERTOOLTIP WIDGET");
        if (BaseTextEditor *editor = BaseTextEditor::currentTextEditor())
            editor->editorWidget()->activateWindow();
    }

    void pin()
    {
        if (isPinned)
            return;
        isPinned = true;
        pinButton->setIcon(style()->standardIcon(QStyle::SP_DockWidgetCloseButton));

        if (parentWidget()) {
            // We are currently within a text editor tooltip:
            // Rip out of parent widget and re-show as a tooltip
            Utils::WidgetContent::pinToolTip(this);
        } else {
            // We have just be restored from session data.
            setWindowFlags(Qt::ToolTip);
        }
        titleLabel->active = true; // User can now drag
    }

public:
    bool isPinned;
    QToolButton *pinButton;
    DraggableLabel *titleLabel;
    DebuggerToolTipTreeView *treeView;
};

/////////////////////////////////////////////////////////////////////////
//
// DebuggerToolTipHolder
//
/////////////////////////////////////////////////////////////////////////

class DebuggerToolTipHolder : public QObject
{
public:
    DebuggerToolTipHolder(const DebuggerToolTipContext &context);
    ~DebuggerToolTipHolder();

    enum State { New, Pending, Acquired, Released };

    void acquireEngine();
    void releaseEngine();

    void saveSessionData(QXmlStreamWriter &w) const;
    void handleStackFrameCompleted(const QString &frameFile, const QString &frameFunction);

    void positionShow(const TextEditorWidget *editorWidget);

    void handleItemIsExpanded(const QModelIndex &sourceIdx);
    void updateTooltip(const StackFrame &frame);

    void setState(State newState);

public:
    QPointer<DebuggerToolTipWidget> widget;
    QPointer<DebuggerEngine> engine;
    QDate creationDate;
    DebuggerToolTipContext context;
    TooltipFilterModel filterModel; //!< Pointing to a valid watchModel
    QStandardItemModel defaultModel;

    State state;
    bool showNeeded;
};

static void hideAllToolTips()
{
    purgeClosedToolTips();
    foreach (const DebuggerToolTipHolder *tooltip, m_tooltips)
        tooltip->widget->hide();
}

void DebuggerToolTipHolder::handleItemIsExpanded(const QModelIndex &sourceIdx)
{
    QTC_ASSERT(filterModel.sourceModel() == sourceIdx.model(), return);
    QModelIndex mappedIdx = filterModel.mapFromSource(sourceIdx);
    QTC_ASSERT(widget.data(), return);
    if (!widget->treeView->isExpanded(mappedIdx))
        widget->treeView->expand(mappedIdx);
}

/*!
    \class Debugger::Internal::DebuggerToolTipContext

    \brief The DebuggerToolTipContext class specifies the file name and
    position where the tooltip is anchored.

    Uses redundant position or line column information to detect whether
    the underlying file has been changed
    on restoring.
*/

DebuggerToolTipContext::DebuggerToolTipContext()
    : position(0), line(0), column(0), scopeFromLine(0), scopeToLine(0)
{
}

static bool filesMatch(const QString &file1, const QString &file2)
{
    QFileInfo f1(file1);
    QFileInfo f2(file2);
    return f1.canonicalFilePath() == f2.canonicalFilePath();
}

bool DebuggerToolTipContext::matchesFrame(const StackFrame &frame) const
{
    return (fileName.isEmpty() || frame.file.isEmpty() || filesMatch(fileName, frame.file))
            //&& (function.isEmpty() || frame.function.isEmpty() || function == frame.function);
            && (frame.line <= 0 || (scopeFromLine <= frame.line && frame.line <= scopeToLine));
}

bool DebuggerToolTipContext::isSame(const DebuggerToolTipContext &other) const
{
    return filesMatch(fileName, other.fileName)
            && scopeFromLine == other.scopeFromLine
            && scopeToLine == other.scopeToLine
            && iname == other.iname;
}

QDebug operator<<(QDebug d, const DebuggerToolTipContext &c)
{
    QDebug nsp = d.nospace();
    nsp << c.fileName << '@' << c.line << ',' << c.column << " (" << c.position << ')'
        << "INAME: " << c.iname << " EXP: " << c.expression << " FUNCTION: " << c.function;
    return d;
}

/*!
    \class Debugger::Internal::DebuggerToolTipWidget

    \brief The DebuggerToolTipWidget class is a pinnable debugger tool tip
    widget.

    The debugger tooltip goes from the unpinned state (button
    showing 'Pin') to the pinned state (button showing 'Close').
    It consists of a title toolbar and a vertical main layout.
    The widget has the ability to save/restore tree model contents to XML.
    With the engine acquired, it sets a filter model (by expression) on
    one of the engine's models (debuggerModel).
    On release, it serializes and restores the data to a QStandardItemModel
    (defaultModel) and displays that.

    It is associated with file name and position with functionality to
    acquire and release the engine. When the debugger stops at a file, all
    matching tooltips acquire the engine, that is, display the engine data.
    When continuing or switching away from the frame, the tooltips release the
    engine, that is, store the data internally and keep displaying them
    marked as 'previous'.

    When restoring the data from a session, all tooltips start in 'released' mode.

    Stored tooltips expire after toolTipsExpiryDays while loading to prevent
    them accumulating.

    In addition, if the stored line number diverges too much from the current line
    number in positionShow(), the tooltip is also closed/discarded.

    The widget is that is first shown by the TextEditor's tooltip
    class and typically closed by it unless the user pins it.
    In that case, it is removed from the tip's layout, added to the DebuggerToolTipManager's
    list of pinned tooltips and re-shown as a global tooltip widget.
    As the debugger stop and continues, it shows the debugger values or a copy
    of them. On closing or session changes, the contents it saved.
*/

DebuggerToolTipHolder::DebuggerToolTipHolder(const DebuggerToolTipContext &context_)
{
    widget = new DebuggerToolTipWidget;
    widget->setObjectName(QLatin1String("DebuggerTreeViewToolTipWidget: ") + QLatin1String(context_.iname));

    context = context_;
    context.creationDate = QDate::currentDate();

    state = New;
    showNeeded = true;

    filterModel.m_iname = context.iname;

    QObject::connect(widget->pinButton, &QAbstractButton::clicked, [this] {
        if (widget->isPinned) {
            widget->close();
        } else {
            widget->pin();
        }
    });
    DEBUG("CREATE DEBUGGERTOOLTIPHOLDER" << context.iname);
}

DebuggerToolTipHolder::~DebuggerToolTipHolder()
{
    DEBUG("DESTROY DEBUGGERTOOLTIPHOLDER" << context.iname << " STATE: " << state);
    delete widget; widget.clear();
}

void DebuggerToolTipHolder::updateTooltip(const StackFrame &frame)
{
    const bool sameFrame = context.matchesFrame(frame);
    DEBUG("UPDATE TOOLTIP: STATE " << state << context.iname
          << "PINNED: " << widget->isPinned
          << "SHOW NEEDED: " << widget->isPinned
          << "SAME FRAME: " << sameFrame);

    if (state == Pending) {
        acquireEngine();
    } else if (state == Acquired && !sameFrame) {
        releaseEngine();
    } else if (state == Released && sameFrame) {
        acquireEngine();
    }

    if (showNeeded) {
        showNeeded = false;
        DEBUG("INITIAL SHOW");
        const Utils::WidgetContent widgetContent(widget, true);
        Utils::ToolTip::show(context.mousePosition, widgetContent, Internal::mainWindow());
    }

    if (state == Acquired) {
        // Save data to stream and restore to the backup m_defaultModel.
        // Doing it on releaseEngine() is too later.
        defaultModel.removeRows(0, defaultModel.rowCount());
        TreeModelCopyVisitor v(&filterModel, &defaultModel);
        v.run();

        widget->treeView->expand(filterModel.index(0, 0));
        WatchTreeView::reexpand(widget->treeView, filterModel.index(0, 0));
    }
}

void DebuggerToolTipHolder::setState(DebuggerToolTipHolder::State newState)
{
    bool ok = (state == New && newState == Pending)
        || (state == Pending && (newState == Acquired || newState == Released))
        || (state == Acquired && (newState == Released))
        || (state == Released && (newState == Acquired));

    // FIXME: These happen when a tooltip is re-used in findOrCreate.
    ok = ok
        || (state == Acquired && newState == Pending)
        || (state == Released && newState == Pending);

    DEBUG("TRANSITION STATE FROM " << state << " TO " << newState);
    QTC_ASSERT(ok, qDebug() << "Unexpected tooltip state transition from "
                            << state << " to " << newState);

    state = newState;
}

void DebuggerToolTipHolder::acquireEngine()
{
    DEBUG("ACQUIRE ENGINE: STATE " << state);
    setState(Acquired);

    QTC_ASSERT(widget, return);
    widget->titleLabel->setText(context.expression);
    widget->treeView->setModel(&filterModel);
    widget->treeView->setRootIndex(filterModel.index(0, 0));
    widget->treeView->expand(filterModel.index(0, 0));
    WatchTreeView::reexpand(widget->treeView, filterModel.index(0, 0));
}

void DebuggerToolTipHolder::releaseEngine()
{
    DEBUG("RELEASE ENGINE: STATE " << state);
    setState(Released);

    QTC_ASSERT(widget, return);
    widget->titleLabel->setText(DebuggerToolTipManager::tr("%1 (Previous)").arg(context.expression));
    widget->treeView->setModel(&defaultModel);
    widget->treeView->setRootIndex(defaultModel.index(0, 0));
    widget->treeView->expandAll();
}

void DebuggerToolTipHolder::positionShow(const TextEditorWidget *editorWidget)
{
    // Figure out new position of tooltip using the text edit.
    // If the line changed too much, close this tip.
    QTC_ASSERT(editorWidget, return);
    QTextCursor cursor = editorWidget->textCursor();
    cursor.setPosition(context.position);
    const int line = cursor.blockNumber();
    if (qAbs(context.line - line) > 2) {
        widget->close();
        return ;
    }

    const QPoint screenPos = editorWidget->toolTipPosition(cursor) + widget->titleLabel->m_offset;
    const QRect toolTipArea = QRect(screenPos, QSize(widget->sizeHint()));
    const QRect plainTextArea = QRect(editorWidget->mapToGlobal(QPoint(0, 0)), editorWidget->size());
    const bool visible = plainTextArea.intersects(toolTipArea);
    //    DEBUG("DebuggerToolTipWidget::positionShow() " << this << m_context
    //             << " line: " << line << " plainTextPos " << toolTipArea
    //             << " offset: " << m_titleLabel->m_offset
    //             << " Area: " << plainTextArea << " Screen pos: "
    //             << screenPos << te.widget << " visible=" << visible);

    if (visible) {
        widget->move(screenPos);
        widget->show();
    } else {
        widget->hide();
    }
}

static DebuggerToolTipHolder *findOrCreateTooltip(const DebuggerToolTipContext &context)
{
    purgeClosedToolTips();

    for (int i = 0, n = m_tooltips.size(); i != n; ++i) {
        DebuggerToolTipHolder *tooltip = m_tooltips.at(i);
        if (tooltip->context.isSame(context))
            return tooltip;
    }

    auto newTooltip = new DebuggerToolTipHolder(context);
    m_tooltips.push_back(newTooltip);
    return newTooltip;
}

static void restoreTreeModel(QXmlStreamReader &r, QStandardItemModel *m)
{
    StandardItemTreeModelBuilder builder(m);
    int columnCount = 1;
    bool withinModel = true;
    while (withinModel && !r.atEnd()) {
        const QXmlStreamReader::TokenType token = r.readNext();
        switch (token) {
        case QXmlStreamReader::StartElement: {
            const QStringRef element = r.name();
            // Root model element with column count.
            if (element == QLatin1String(modelElementC)) {
                if (const int cc = r.attributes().value(QLatin1String(modelColumnCountAttributeC)).toString().toInt())
                    columnCount = cc;
                m->setColumnCount(columnCount);
            } else if (element == QLatin1String(modelRowElementC)) {
                builder.startRow();
            } else if (element == QLatin1String(modelItemElementC)) {
                builder.addItem(r.readElementText());
            }
        }
            break; // StartElement
        case QXmlStreamReader::EndElement: {
            const QStringRef element = r.name();
            // Row closing: pop off parent.
            if (element == QLatin1String(modelRowElementC))
                builder.endRow();
            else if (element == QLatin1String(modelElementC))
                withinModel = false;
        }
            break; // EndElement
        default:
            break;
        } // switch
    } // while
}

// Parse a 'yyyyMMdd' date
static QDate dateFromString(const QString &date)
{
    return date.size() == 8 ?
        QDate(date.left(4).toInt(), date.mid(4, 2).toInt(), date.mid(6, 2).toInt()) :
        QDate();
}

static void loadSessionDataHelper(QXmlStreamReader &r)
{
    if (!readStartElement(r, toolTipElementC))
        return;
    const QXmlStreamAttributes attributes = r.attributes();
    DebuggerToolTipContext context;
    context.fileName = attributes.value(QLatin1String(fileNameAttributeC)).toString();
    context.position = attributes.value(QLatin1String(textPositionAttributeC)).toString().toInt();
    context.line = attributes.value(QLatin1String(textLineAttributeC)).toString().toInt();
    context.column = attributes.value(QLatin1String(textColumnAttributeC)).toString().toInt();
    context.function = attributes.value(QLatin1String(functionAttributeC)).toString();
    QPoint offset;
    const QString offsetXAttribute = QLatin1String(offsetXAttributeC);
    const QString offsetYAttribute = QLatin1String(offsetYAttributeC);
    if (attributes.hasAttribute(offsetXAttribute))
        offset.setX(attributes.value(offsetXAttribute).toString().toInt());
    if (attributes.hasAttribute(offsetYAttribute))
        offset.setY(attributes.value(offsetYAttribute).toString().toInt());
    context.mousePosition = offset;

    context.iname = attributes.value(QLatin1String(treeInameAttributeC)).toString().toLatin1();
    context.expression = attributes.value(QLatin1String(treeExpressionAttributeC)).toString();

//    const QStringRef className = attributes.value(QLatin1String(toolTipClassAttributeC));
    context.engineType = attributes.value(QLatin1String(engineTypeAttributeC)).toString();
    context.creationDate = dateFromString(attributes.value(QLatin1String(dateAttributeC)).toString());
    bool readTree = context.isValid();
    if (!context.creationDate.isValid() || context.creationDate.daysTo(QDate::currentDate()) > toolTipsExpiryDays) {
        // DEBUG("Expiring tooltip " << context.fileName << '@' << context.position << " from " << creationDate)
        //readTree = false;
    } else { //if (className != QLatin1String("Debugger::Internal::DebuggerToolTipWidget")) {
        //qWarning("Unable to create debugger tool tip widget of class %s", qPrintable(className.toString()));
        //readTree = false;
    }

    if (readTree) {
        DebuggerToolTipHolder *tw = findOrCreateTooltip(context);
        restoreTreeModel(r, &tw->defaultModel);
        tw->widget->pin();
        tw->widget->titleLabel->setText(DebuggerToolTipManager::tr("%1 (Restored").arg(context.expression));
        tw->widget->treeView->setModel(&tw->defaultModel);
        tw->widget->treeView->setRootIndex(tw->defaultModel.index(0, 0));
        tw->widget->treeView->expandAll();
    } else {
        r.readElementText(QXmlStreamReader::SkipChildElements); // Skip
    }

    r.readNext(); // Skip </tree>
}

void DebuggerToolTipHolder::saveSessionData(QXmlStreamWriter &w) const
{
    w.writeStartElement(QLatin1String(toolTipElementC));
    QXmlStreamAttributes attributes;
//    attributes.append(QLatin1String(toolTipClassAttributeC), QString::fromLatin1(metaObject()->className()));
    attributes.append(QLatin1String(fileNameAttributeC), context.fileName);
    if (!context.function.isEmpty())
        attributes.append(QLatin1String(functionAttributeC), context.function);
    attributes.append(QLatin1String(textPositionAttributeC), QString::number(context.position));
    attributes.append(QLatin1String(textLineAttributeC), QString::number(context.line));
    attributes.append(QLatin1String(textColumnAttributeC), QString::number(context.column));
    attributes.append(QLatin1String(dateAttributeC), creationDate.toString(QLatin1String("yyyyMMdd")));
    QPoint offset = widget->titleLabel->m_offset;
    if (offset.x())
        attributes.append(QLatin1String(offsetXAttributeC), QString::number(offset.x()));
    if (offset.y())
        attributes.append(QLatin1String(offsetYAttributeC), QString::number(offset.y()));
    attributes.append(QLatin1String(engineTypeAttributeC), context.engineType);
    attributes.append(QLatin1String(treeExpressionAttributeC), context.expression);
    attributes.append(QLatin1String(treeInameAttributeC), QLatin1String(context.iname));
    w.writeAttributes(attributes);

        w.writeStartElement(QLatin1String(treeElementC));
        XmlWriterTreeModelVisitor v(&filterModel, w);
        v.run();
        w.writeEndElement();

    w.writeEndElement();
}

/*!
    \class Debugger::Internal::DebuggerToolTipManager

    \brief The DebuggerToolTipManager class manages the pinned tooltip widgets,
    listens on editor scroll and main window move
    events and takes care of repositioning the tooltips.

    Listens to editor change and mode change. In debug mode, if there are tooltips
    for the current editor (by file name), positions and shows them.

    In addition, listens on state change and stack frame completed signals
    of the engine. If a stack frame is completed, has all matching tooltips
    (by file name and function) acquire the engine, others release.
*/

static DebuggerToolTipManager *m_instance = 0;

DebuggerToolTipManager::DebuggerToolTipManager()
{
    m_instance = this;
}

DebuggerToolTipManager::~DebuggerToolTipManager()
{
    m_instance = 0;
}

void DebuggerToolTipManager::slotUpdateVisibleToolTips()
{
    purgeClosedToolTips();
    if (m_tooltips.isEmpty())
        return;
    if (!m_debugModeActive) {
        hideAllToolTips();
        return;
    }

    BaseTextEditor *toolTipEditor = BaseTextEditor::currentTextEditor();
    if (!toolTipEditor) {
        hideAllToolTips();
        return;
    }

    const QString fileName = toolTipEditor->textDocument()->filePath();
    if (fileName.isEmpty()) {
        hideAllToolTips();
        return;
    }

    // Reposition and show all tooltips of that file.
    foreach (DebuggerToolTipHolder *tooltip, m_tooltips) {
        if (tooltip->context.fileName == fileName)
            tooltip->positionShow(toolTipEditor->editorWidget());
        else
            tooltip->widget->hide();
    }
}

void DebuggerToolTipManager::slotItemIsExpanded(const QModelIndex &idx)
{
    foreach (DebuggerToolTipHolder *tooltip, m_tooltips)
        tooltip->handleItemIsExpanded(idx);
}

void DebuggerToolTipManager::slotColumnAdjustmentRequested()
{
    foreach (DebuggerToolTipHolder *tooltip, m_tooltips) {
        QTC_ASSERT(tooltip, continue);
        QTC_ASSERT(tooltip->widget, continue);
        tooltip->widget->treeView->computeSize();
    }
}

void DebuggerToolTipManager::updateEngine(DebuggerEngine *engine)
{
    QTC_ASSERT(engine, return);
    purgeClosedToolTips();
    if (m_tooltips.isEmpty())
        return;

    // Stack frame changed: All tooltips of that file acquire the engine,
    // all others release (arguable, this could be more precise?)
    StackFrame frame = engine->stackHandler()->currentFrame();
    foreach (DebuggerToolTipHolder *tooltip, m_tooltips)
        tooltip->updateTooltip(frame);
    slotUpdateVisibleToolTips(); // Move tooltip when stepping in same file.
}


void DebuggerToolTipManager::registerEngine(DebuggerEngine *engine)
{
    DEBUG("REGISTER ENGINE");
    WatchModelBase *watchModel = engine->watchHandler()->model();
    connect(watchModel, &WatchModelBase::itemIsExpanded,
            m_instance, &DebuggerToolTipManager::slotItemIsExpanded);
    connect(watchModel, &WatchModelBase::columnAdjustmentRequested,
            m_instance, &DebuggerToolTipManager::slotColumnAdjustmentRequested);
}

void DebuggerToolTipManager::deregisterEngine(DebuggerEngine *engine)
{
    DEBUG("DEREGISTER ENGINE");
    QTC_ASSERT(engine, return);

    // FIXME: For now remove all.
    purgeClosedToolTips();
    foreach (const DebuggerToolTipHolder *tooltip, m_tooltips)
        tooltip->widget->close();
    purgeClosedToolTips();
    return;

    WatchModelBase *watchModel = engine->watchHandler()->model();
    disconnect(watchModel, &WatchModelBase::itemIsExpanded,
               m_instance, &DebuggerToolTipManager::slotItemIsExpanded);
    disconnect(watchModel, &WatchModelBase::columnAdjustmentRequested,
               m_instance, &DebuggerToolTipManager::slotColumnAdjustmentRequested);
    foreach (DebuggerToolTipHolder *tooltip, m_tooltips)
        if (tooltip->context.engineType == engine->objectName())
            tooltip->releaseEngine();
    saveSessionData();
}

bool DebuggerToolTipManager::hasToolTips()
{
    return !m_tooltips.isEmpty();
}

void DebuggerToolTipManager::sessionAboutToChange()
{
    closeAllToolTips();
}

void DebuggerToolTipManager::loadSessionData()
{
    return; // FIXME

    const QString data = sessionValue(sessionSettingsKeyC).toString();
    QXmlStreamReader r(data);
    r.readNextStartElement();
    if (r.tokenType() == QXmlStreamReader::StartElement && r.name() == QLatin1String(sessionDocumentC))
        while (!r.atEnd())
            loadSessionDataHelper(r);
}

void DebuggerToolTipManager::saveSessionData()
{
    return; // FIXME

    QString data;
    purgeClosedToolTips();

    QXmlStreamWriter w(&data);
    w.writeStartDocument();
    w.writeStartElement(QLatin1String(sessionDocumentC));
    w.writeAttribute(QLatin1String(sessionVersionAttributeC), QLatin1String("1.0"));
    foreach (DebuggerToolTipHolder *tooltip, m_tooltips)
        if (tooltip->widget->isPinned)
            tooltip->saveSessionData(w);
    w.writeEndDocument();

    setSessionValue(sessionSettingsKeyC, QVariant(data));
}

void DebuggerToolTipManager::closeAllToolTips()
{
    foreach (DebuggerToolTipHolder *tooltip, m_tooltips)
        tooltip->widget->close();
    m_tooltips.clear();
}

void DebuggerToolTipManager::resetLocation()
{
    purgeClosedToolTips();
    foreach (DebuggerToolTipHolder *tooltip, m_tooltips)
        tooltip->widget->pin();
}

static void slotTooltipOverrideRequested
    (TextEditorWidget *editorWidget, const QPoint &point, int pos, bool *handled)
{
    QTC_ASSERT(handled, return);
    QTC_ASSERT(editorWidget, return);
    *handled = false;

    if (!boolSetting(UseToolTipsInMainEditor))
        return;

    DebuggerEngine *engine = currentEngine();
    if (!engine || !engine->canDisplayTooltip())
        return;

    DebuggerToolTipContext context;
    context.engineType = engine->objectName();
    context.fileName = editorWidget->textDocument()->filePath();
    context.position = pos;
    context.mousePosition = point;
    editorWidget->convertPosition(pos, &context.line, &context.column);
    QString raw = cppExpressionAt(editorWidget, context.position, &context.line, &context.column,
                                  &context.function, &context.scopeFromLine, &context.scopeToLine);
    context.expression = fixCppExpression(raw);

    if (context.expression.isEmpty()) {
        const Utils::TextContent text(DebuggerToolTipManager::tr("No valid expression"));
        Utils::ToolTip::show(context.mousePosition, text, Internal::mainWindow());
        *handled = true;
        return;
    }

    // Prefer a filter on an existing local variable if it can be found.
    const WatchData *localVariable = engine->watchHandler()->findCppLocalVariable(context.expression);
    if (localVariable) {
        context.expression = QLatin1String(localVariable->exp);
        if (context.expression.isEmpty())
            context.expression = localVariable->name;
        context.iname = localVariable->iname;
    } else {
        context.iname = "tooltip." + context.expression.toLatin1().toHex();
    }

    DebuggerToolTipHolder *tooltip = findOrCreateTooltip(context);
    if (tooltip->state == DebuggerToolTipHolder::Pending) {
        DEBUG("FOUND PENDING TOOLTIP, WAITING...");
        *handled = true;
        return;
    }

    tooltip->filterModel.setSourceModel(engine->watchHandler()->model());
    tooltip->widget->titleLabel->setText(DebuggerToolTipManager::tr("Updating"));
    tooltip->setState(DebuggerToolTipHolder::Pending);

    if (localVariable) {
        tooltip->acquireEngine();
        DEBUG("SYNC IN STATE" << tooltip->state);
        tooltip->showNeeded = false;
        const Utils::WidgetContent widgetContent(tooltip->widget, true);
        Utils::ToolTip::show(context.mousePosition, widgetContent, Internal::mainWindow());
        *handled = true;
    } else {
        DEBUG("ASYNC TIP IN STATE" << tooltip->state);
        *handled = engine->setToolTipExpression(editorWidget, context);
        if (!*handled) {
            const Utils::TextContent text(DebuggerToolTipManager::tr("Expression too complex"));
            Utils::ToolTip::show(context.mousePosition, text, Internal::mainWindow());
            tooltip->widget->close();
        }
    }
}


static void slotEditorOpened(IEditor *e)
{
    // Move tooltip along when scrolled.
    if (BaseTextEditor *textEditor = qobject_cast<BaseTextEditor *>(e)) {
        TextEditorWidget *widget = textEditor->editorWidget();
        QObject::connect(widget->verticalScrollBar(), &QScrollBar::valueChanged,
                         &DebuggerToolTipManager::slotUpdateVisibleToolTips);
        QObject::connect(widget, &TextEditorWidget::tooltipOverrideRequested,
                         slotTooltipOverrideRequested);
    }
}

void DebuggerToolTipManager::debugModeEntered()
{
    // Hook up all signals in debug mode.
    if (!m_debugModeActive) {
        m_debugModeActive = true;
        QWidget *topLevel = ICore::mainWindow()->topLevelWidget();
        topLevel->installEventFilter(this);
        EditorManager *em = EditorManager::instance();
        connect(em, &EditorManager::currentEditorChanged,
                &DebuggerToolTipManager::slotUpdateVisibleToolTips);
        connect(em, &EditorManager::editorOpened, slotEditorOpened);

        foreach (IEditor *e, DocumentModel::editorsForOpenedDocuments())
            slotEditorOpened(e);
        // Position tooltips delayed once all the editor placeholder layouting is done.
        if (!m_tooltips.isEmpty())
            QTimer::singleShot(0, this, SLOT(slotUpdateVisibleToolTips()));
    }
}

void DebuggerToolTipManager::leavingDebugMode()
{
    // Remove all signals in debug mode.
    if (m_debugModeActive) {
        m_debugModeActive = false;
        hideAllToolTips();
        if (QWidget *topLevel = ICore::mainWindow()->topLevelWidget())
            topLevel->removeEventFilter(this);
        foreach (IEditor *e, DocumentModel::editorsForOpenedDocuments()) {
            if (BaseTextEditor *toolTipEditor = qobject_cast<BaseTextEditor *>(e)) {
                toolTipEditor->editorWidget()->verticalScrollBar()->disconnect(this);
                toolTipEditor->disconnect(this);
            }
        }
        EditorManager::instance()->disconnect(this);
    }
}

DebuggerToolTipContexts DebuggerToolTipManager::pendingTooltips(DebuggerEngine *engine)
{
    StackFrame frame = engine->stackHandler()->currentFrame();
    DebuggerToolTipContexts rc;
    foreach (DebuggerToolTipHolder *tooltip, m_tooltips) {
        const DebuggerToolTipContext &context = tooltip->context;
        if (context.iname.startsWith("tooltip") && context.matchesFrame(frame))
            rc.push_back(context);
    }
    return rc;
}

bool DebuggerToolTipManager::eventFilter(QObject *o, QEvent *e)
{
    if (m_tooltips.isEmpty())
        return false;
    switch (e->type()) {
    case QEvent::Move: { // Move along with parent (toplevel)
        const QMoveEvent *me = static_cast<const QMoveEvent *>(e);
        const QPoint dist = me->pos() - me->oldPos();
        purgeClosedToolTips();
        foreach (DebuggerToolTipHolder *tooltip, m_tooltips)
            if (tooltip->widget && tooltip->widget->isVisible())
                tooltip->widget->move(tooltip->widget->pos() + dist);
        }
        break;
    case QEvent::WindowStateChange: { // Hide/Show along with parent (toplevel)
        const QWindowStateChangeEvent *se = static_cast<const QWindowStateChangeEvent *>(e);
        const bool wasMinimized = se->oldState() & Qt::WindowMinimized;
        const bool isMinimized  = static_cast<const QWidget *>(o)->windowState() & Qt::WindowMinimized;
        if (wasMinimized ^ isMinimized) {
            purgeClosedToolTips();
            foreach (DebuggerToolTipHolder *tooltip, m_tooltips)
                tooltip->widget->setVisible(!isMinimized);
        }
    }
        break;
    default:
        break;
    }
    return false;
}

static void purgeClosedToolTips()
{
    for (int i = m_tooltips.size(); --i >= 0; ) {
        DebuggerToolTipHolder *tooltip = m_tooltips.at(i);
        if (!tooltip || !tooltip->widget) {
            DEBUG("PURGE TOOLTIP, LEFT: "  << m_tooltips.size());
            m_tooltips.removeAt(i);
        }
    }
}

} // namespace Internal
} // namespace Debugger
