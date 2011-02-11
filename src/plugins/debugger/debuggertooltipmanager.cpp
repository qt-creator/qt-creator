/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "debuggertooltipmanager.h"
#include "watchutils.h"
#include "debuggerengine.h"
#include "watchhandler.h"
#include "stackhandler.h"
#include "debuggercore.h"

#include <coreplugin/icore.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/imode.h>
#include <coreplugin/editormanager/editormanager.h>
#include <texteditor/itexteditor.h>

#include <utils/qtcassert.h>

#include <QtGui/QToolButton>
#include <QtGui/QToolBar>
#include <QtGui/QVBoxLayout>
#include <QtGui/QStyle>
#include <QtGui/QIcon>
#include <QtGui/QApplication>
#include <QtGui/QMainWindow>
#include <QtGui/QMoveEvent>
#include <QtGui/QDesktopWidget>
#include <QtGui/QScrollBar>
#include <QtGui/QSortFilterProxyModel>
#include <QtGui/QStandardItemModel>
#include <QtGui/QStandardItem>
#include <QtGui/QPlainTextEdit>
#include <QtGui/QTextCursor>
#include <QtGui/QTextDocument>
#include <QtGui/QLabel>
#include <QtGui/QMenu>
#include <QtGui/QAction>

#include <QtCore/QVariant>
#include <QtCore/QStack>
#include <QtCore/QDebug>
#include <QtCore/QTimer>

enum { debugToolTips = 0 };

// Expire tooltips after n days on (no longer load them) in order
// to avoid them piling up.
enum { toolTipsExpiryDays = 6 };

static const char sessionSettingsKeyC[] = "DebuggerToolTips";
static const char sessionDocumentC[] = "DebuggerToolTips";
static const char sessionVersionAttributeC[] = "version";
static const char toolTipElementC[] = "DebuggerToolTip";
static const char toolTipClassAttributeC[] = "class";
static const char fileNameAttributeC[] = "name";
static const char functionAttributeC[] = "function";
static const char textPositionAttributeC[] = "position";
static const char textLineAttributeC[] = "line";
static const char textColumnAttributeC[] = "column";
static const char engineTypeAttributeC[] = "engine";
static const char dateAttributeC[] = "date";
static const char treeElementC[] = "tree";
static const char treeModelAttributeC[] = "model"; // Locals/Watches
static const char treeExpressionAttributeC[] = "expression"; // Locals/Watches
static const char modelElementC[] = "model";
static const char modelColumnCountAttributeC[] = "columncount";
static const char modelRowElementC[] = "row";
static const char modelItemElementC[] = "item";

// Forward a stream reader across end elements looking for the
// next start element of a desired type.
static inline bool readStartElement(QXmlStreamReader &r, const char *name)
{
    if (debugToolTips > 1)
        qDebug("readStartElement: looking for '%s', currently at: %s/%s",
               name, qPrintable(r.tokenString()), qPrintable(r.name().toString()));
    while (r.tokenType() != QXmlStreamReader::StartElement || r.name() != QLatin1String(name))
        switch (r.readNext()) {
        case QXmlStreamReader::EndDocument:
            return false;
        case QXmlStreamReader::NoToken:
        case QXmlStreamReader::Invalid:
            qWarning("'%s'/'%s' encountered while looking for start element '%s'.",
                     qPrintable(r.tokenString()), qPrintable(r.name().toString()), name);
            return false;
        default:
            break;
        }
    return true;
}

static inline void debugMode(const QAbstractItemModel *model)
{
    QDebug nospace = qDebug().nospace();
    nospace << model << '\n';
    for (int r = 0; r < model->rowCount(); r++)
        nospace << '#' << r << ' ' << model->data(model->index(r, 0)).toString() << '\n';
}

static inline QPlainTextEdit *plainTextEditor(Core::IEditor *ie, QString *fileName = 0)
{
    if (const Core::IFile *file = ie->file())
        if (qobject_cast<TextEditor::ITextEditor *>(ie))
            if (QPlainTextEdit *pe = qobject_cast<QPlainTextEdit *>(ie->widget())) {
                if (fileName)
                    *fileName = file->fileName();
                return pe;
            }
    return 0;
}

static inline QPlainTextEdit *currentPlainTextEditor(QString *fileName = 0)
{
    if (Core::IEditor *ie = Core::EditorManager::instance()->currentEditor())
        return plainTextEditor(ie, fileName);
    return 0;
}

namespace Debugger {
namespace Internal {

/* Helper for building a QStandardItemModel of a tree form (see TreeModelVisitor).
 * The recursion/building is based on the scheme: \code
<row><item1><item2>
    <row><item11><item12></row>
</row>
\endcode */

class StandardItemTreeModelBuilder {
public:
    typedef QList<QStandardItem *> StandardItemRow;

    explicit StandardItemTreeModelBuilder(QStandardItemModel *m, Qt::ItemFlags f = Qt::ItemIsSelectable);

    inline void addItem(const QString &);
    inline void startRow();
    inline void endRow();

private:
    inline void pushRow();

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
    if (m_rowParents.isEmpty()) {
        m_model->appendRow(m_row);
    } else {
        m_rowParents.top()->appendRow(m_row);
    }
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

/* Helper visitor base class for recursing over a tree model
 * (see StandardItemTreeModelBuilder for the scheme). */
class TreeModelVisitor {
public:
    inline virtual void run() { run(QModelIndex()); }

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
class XmlWriterTreeModelVisitor : public TreeModelVisitor {
public:
    explicit XmlWriterTreeModelVisitor(const QAbstractItemModel *model, QXmlStreamWriter &w);

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
    if (value.isEmpty()) {
        m_writer.writeEmptyElement(m_modelItemElement);
    } else {
        m_writer.writeTextElement(m_modelItemElement, value);
    }
}

// TreeModelVisitor for debugging models
class DumpTreeModelVisitor : public TreeModelVisitor {
public:
    explicit DumpTreeModelVisitor(const QAbstractItemModel *model, QTextStream &s);

protected:
    virtual void rowStarted();
    virtual void handleItem(const QModelIndex &m);
    virtual void rowEnded();

private:
    QTextStream &m_stream;
    int m_level;
};

DumpTreeModelVisitor::DumpTreeModelVisitor(const QAbstractItemModel *model, QTextStream &s) :
    TreeModelVisitor(model), m_stream(s), m_level(0)
{
    m_stream << model->metaObject()->className() << '/' << model->objectName();
}

void DumpTreeModelVisitor::rowStarted()
{
    m_level++;
    m_stream << '\n' << QString(2 * m_level, QLatin1Char(' '));
}

void DumpTreeModelVisitor::handleItem(const QModelIndex &m)
{
    if (m.column())
        m_stream << '|';
    m_stream << '\'' << m.data().toString() << '\'';
}

void DumpTreeModelVisitor::rowEnded()
{
    m_level--;
}

} // namespace Internal
} // namespace Debugger

static inline QDebug operator<<(QDebug d, const QAbstractItemModel &model)
{
    QString s;
    QTextStream str(&s);
    Debugger::Internal::DumpTreeModelVisitor v(&model, str);
    v.run();
    qDebug().nospace() << s;
    return d;
}

namespace Debugger {
namespace Internal {

// Visitor building a QStandardItem from a tree model (copy).
class TreeModelCopyVisitor : public TreeModelVisitor {
public:
    explicit TreeModelCopyVisitor(const QAbstractItemModel *source, QStandardItemModel *target) :
        TreeModelVisitor(source), m_builder(target) {}

protected:
    virtual void rowStarted() { m_builder.startRow(); }
    virtual void handleItem(const QModelIndex &m) { m_builder.addItem(m.data().toString()); }
    virtual void rowEnded() { m_builder.endRow(); }

private:
    StandardItemTreeModelBuilder m_builder;
};

/*!
    \class PinnableToolTipWidget

    A pinnable tooltip that goes from State 'Unpinned' (button showing
    'Pin') to 'Pinned' (button showing 'Close').
    It consists of a title toolbar and a vertical main layout.
*/

PinnableToolTipWidget::PinnableToolTipWidget(QWidget *parent) :
    QWidget(parent),
    m_pinState(Unpinned),
    m_mainVBoxLayout(new QVBoxLayout),
    m_toolBar(new QToolBar),
    m_toolButton(new QToolButton),
    m_menu(new QMenu)
{
    setWindowFlags(Qt::ToolTip | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_DeleteOnClose);

    m_mainVBoxLayout->setSizeConstraint(QLayout::SetFixedSize);
    m_mainVBoxLayout->setContentsMargins(0, 0, 0, 0);

    const QIcon pinIcon(QLatin1String(":/debugger/images/pin.xpm"));
    const QList<QSize> pinIconSizes = pinIcon.availableSizes();

    m_toolButton->setIcon(pinIcon);
    m_menu->addAction(tr("Close All"), this, SIGNAL(closeAllRequested()));
    m_toolButton->setMenu(m_menu);
    m_toolButton->setPopupMode(QToolButton::MenuButtonPopup);
    connect(m_toolButton, SIGNAL(clicked()), this, SLOT(toolButtonClicked()));

    m_toolBar->setProperty("_q_custom_style_disabled", QVariant(true));
    if (!pinIconSizes.isEmpty())
        m_toolBar->setIconSize(pinIconSizes.front());
    m_toolBar->addWidget(m_toolButton);

    m_mainVBoxLayout->addWidget(m_toolBar);

    setLayout(m_mainVBoxLayout);
}

void PinnableToolTipWidget::addWidget(QWidget *w)
{
    w->setFocusPolicy(Qt::NoFocus);
    m_mainVBoxLayout->addWidget(w);
}

void PinnableToolTipWidget::addToolBarWidget(QWidget *w)
{
    m_toolBar->addWidget(w);
}

void PinnableToolTipWidget::pin()
{
    if (m_pinState == Unpinned) {
        m_pinState = Pinned;
        m_toolButton->setIcon(style()->standardIcon(QStyle::SP_DockWidgetCloseButton));
    }
}

void PinnableToolTipWidget::toolButtonClicked()
{
    switch (m_pinState) {
    case Unpinned:
        pin();
        break;
    case Pinned:
        close();
        break;
    }
}

void PinnableToolTipWidget::leaveEvent(QEvent *)
{
    if (m_pinState == Unpinned && QApplication::keyboardModifiers() == Qt::NoModifier) {
        if (debugToolTips)
            qDebug("ToolTipWidget::leaveEvent: closing %p", this);
        close();
    }
}

/*!
    \class DebuggerToolTipContext

    File name and position where the tooltip is anchored. Redundant position/line column
    information is used to detect if the underlying file has been changed
    on restoring.
*/

DebuggerToolTipContext::DebuggerToolTipContext() : position(0), line(0), column(0)
{
}

DebuggerToolTipContext DebuggerToolTipContext::fromEditor(Core::IEditor *ie, int pos)
{
    DebuggerToolTipContext rc;
    if (const Core::IFile *file = ie->file()) {
        if (const TextEditor::ITextEditor *te = qobject_cast<const TextEditor::ITextEditor *>(ie)) {
            rc.fileName = file->fileName();
            rc.position = pos;
            te->convertPosition(pos, &rc.line, &rc.column);
        }
    }
    return rc;
}

QDebug operator<<(QDebug d, const DebuggerToolTipContext &c)
{
    QDebug nsp = d.nospace();
    nsp << c.fileName << '@' << c.line << ',' << c.column << " (" << c.position << ')';
    if (!c.function.isEmpty())
        nsp << ' ' << c.function << "()";
    return d;
}

/*!
    \class AbstractDebuggerToolTipWidget

    Base class for a tool tip widget associated with file name
    and position with functionality to
    \list
    \o Save and restore values (session data) in XML form
    \o Acquire and release the engine: When the debugger stops at a file, all
       matching tooltips acquire the engine, that is, display the engine data.
       When continuing or switching away from the frame, the tooltips release the
       engine, that is, store the data internally and keep displaying them
       marked as 'previous'.
    \endlist
    When restoring the data from a session, all tooltips start in 'released' mode.

    Stored tooltips expire after toolTipsExpiryDays while loading to prevent
    them accumulating.

    In addition, if the stored line number diverges too much from the current line
    number in positionShow(), the tooltip is also closed/discarded.
*/

static inline QString msgReleasedText() { return AbstractDebuggerToolTipWidget::tr("Previous"); }

AbstractDebuggerToolTipWidget::AbstractDebuggerToolTipWidget(QWidget *parent) :
    PinnableToolTipWidget(parent),
    m_titleLabel(new QLabel), m_engineAcquired(false),
    m_creationDate(QDate::currentDate())
{
    m_titleLabel->setText(msgReleasedText());
    addToolBarWidget(m_titleLabel);
}

bool AbstractDebuggerToolTipWidget::matches(const QString &fileName,
                                            const QString &engineType,
                                            const QString &function) const
{
    if (fileName.isEmpty() || m_context.fileName != fileName)
        return false;
    // Optional.
    if (!engineType.isEmpty() && engineType != m_engineType)
        return false;
    if (function.isEmpty() || m_context.function.isEmpty())
        return true;
    return function == m_context.function;
}

void AbstractDebuggerToolTipWidget::acquireEngine(Debugger::DebuggerEngine *engine)
{
    QTC_ASSERT(engine, return; )


    if (debugToolTips)
        qDebug() << this << " acquiring" << engine << m_engineAcquired;
    if (m_engineAcquired)
        return;
    doAcquireEngine(engine);
    m_engineType = engine->objectName();
    m_engineAcquired = true;
    m_titleLabel->setText(QString());
}

void AbstractDebuggerToolTipWidget::releaseEngine()
{
    // Release engine of same type
    if (!m_engineAcquired)
        return;
    if (debugToolTips)
        qDebug() << "releaseEngine" << this;
    doReleaseEngine();
    m_titleLabel->setText(msgReleasedText());
    m_engineAcquired = false;
}

bool AbstractDebuggerToolTipWidget::positionShow(const QPlainTextEdit *pe)
{
    // Figure out new position of tooltip using the text edit.
    // If the line changed too much, close this tip.
    QTC_ASSERT(pe, return false; )
    QTextCursor cursor(pe->document());
    cursor.setPosition(m_context.position);
    const int line = cursor.blockNumber();
    if (qAbs(m_context.line - line) > 2) {
        if (debugToolTips)
            qDebug() << "Closing " << this << " in positionShow() lines "
                     << line << m_context.line;
        close();
        return false;
    }
    const QRect plainTextToolTipArea = QRect(pe->cursorRect(cursor).topLeft(), QSize(sizeHint()));
    const QRect plainTextArea = QRect(QPoint(0, 0), QPoint(pe->width(), pe->height()));
    const QPoint screenPos = pe->mapToGlobal(plainTextToolTipArea.topLeft());
    const bool visible = plainTextArea.contains(plainTextToolTipArea);
    if (debugToolTips)
        qDebug() << "DebuggerToolTipWidget::positionShow() " << m_context
                 << " line: " << line << " plainTextPos " << plainTextToolTipArea
                 << " Area: " << plainTextArea << " Screen pos: "
                 << screenPos << pe << " visible=" << visible
                 << " on " << pe->parentWidget()
                 << " at " << pe->mapToGlobal(QPoint(0, 0));

    if (!visible) {
        hide();
        return false;
    }

    move(screenPos);
    show();
    return true;
}

 // Parse a 'yyyyMMdd' date
static inline QDate dateFromString(const QString &date)
{
    return date.size() == 8 ?
        QDate(date.left(4).toInt(), date.mid(4, 2).toInt(), date.mid(6, 2).toInt()) :
        QDate();
}

AbstractDebuggerToolTipWidget *AbstractDebuggerToolTipWidget::loadSessionData(QXmlStreamReader &r)
{
    if (debugToolTips)
        qDebug() << ">DebuggerToolTipWidget::loadSessionData" << r.tokenString() << r.name();
    AbstractDebuggerToolTipWidget *rc = AbstractDebuggerToolTipWidget::loadSessionDataI(r);
    if (debugToolTips)
        qDebug() << "<DebuggerToolTipWidget::loadSessionData" << r.tokenString() << r.name() << " returns  " << rc;
    return rc;
}

AbstractDebuggerToolTipWidget *AbstractDebuggerToolTipWidget::loadSessionDataI(QXmlStreamReader &r)
{
    if (!readStartElement(r, toolTipElementC))
        return 0;
    const QXmlStreamAttributes attributes = r.attributes();
    DebuggerToolTipContext context;
    context.fileName = attributes.value(QLatin1String(fileNameAttributeC)).toString();
    context.position = attributes.value(QLatin1String(textPositionAttributeC)).toString().toInt();
    context.line = attributes.value(QLatin1String(textLineAttributeC)).toString().toInt();
    context.column = attributes.value(QLatin1String(textColumnAttributeC)).toString().toInt();
    context.function = attributes.value(QLatin1String(functionAttributeC)).toString();
    const QString className = attributes.value(QLatin1String(toolTipClassAttributeC)).toString();
    const QString engineType = attributes.value(QLatin1String(engineTypeAttributeC)).toString();
    const QDate creationDate = dateFromString(attributes.value(QLatin1String(dateAttributeC)).toString());
    if (!creationDate.isValid() || creationDate.daysTo(QDate::currentDate()) >  toolTipsExpiryDays) {
        if (debugToolTips)
            qDebug() << "Expiring tooltip " << context.fileName << '@' << context.position << " from " << creationDate;
        r.readElementText(QXmlStreamReader::SkipChildElements); // Skip
        return 0;
    }
    if (debugToolTips)
        qDebug() << "Creating tooltip " << context <<  " from " << creationDate;
    AbstractDebuggerToolTipWidget *rc = 0;
    if (className == "Debugger::Internal::DebuggerTreeViewToolTipWidget")
        rc = new DebuggerTreeViewToolTipWidget;
    if (rc) {
        rc->setContext(context);
        rc->setEngineType(engineType);
        rc->doLoadSessionData(r);
        rc->setCreationDate(creationDate);
        rc->pin();
    } else {
        qWarning("Unable to create debugger tool tip widget of class %s", qPrintable(className));
        r.readElementText(QXmlStreamReader::SkipChildElements); // Skip
    }
    return rc;
}

void AbstractDebuggerToolTipWidget::saveSessionData(QXmlStreamWriter &w) const
{
    w.writeStartElement(QLatin1String(toolTipElementC));
    QXmlStreamAttributes attributes;
    attributes.append(QLatin1String(toolTipClassAttributeC), QString::fromAscii(metaObject()->className()));
    attributes.append(QLatin1String(fileNameAttributeC), m_context.fileName);
    if (!m_context.function.isEmpty())
        attributes.append(QLatin1String(functionAttributeC), m_context.function);
    attributes.append(QLatin1String(textPositionAttributeC), QString::number(m_context.position));
    attributes.append(QLatin1String(textLineAttributeC), QString::number(m_context.line));
    attributes.append(QLatin1String(textColumnAttributeC), QString::number(m_context.column));
    attributes.append(QLatin1String(dateAttributeC), m_creationDate.toString(QLatin1String("yyyyMMdd")));
    if (!m_engineType.isEmpty())
        attributes.append(QLatin1String(engineTypeAttributeC), m_engineType);
    w.writeAttributes(attributes);
    doSaveSessionData(w);
    w.writeEndElement();
}

// Model for tooltips filtering a local variable using the locals model,
// taking the expression.
class DebuggerToolTipExpressionFilterModel : public QSortFilterProxyModel
{
public:
    explicit DebuggerToolTipExpressionFilterModel(QAbstractItemModel *model, const QString &exp, QObject *parent = 0);
    virtual bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;

private:
    const QString m_expression;
};

DebuggerToolTipExpressionFilterModel::DebuggerToolTipExpressionFilterModel(QAbstractItemModel *model,
                                                                           const QString &exp,
                                                                           QObject *parent) :
    QSortFilterProxyModel(parent), m_expression(exp)
{
    setSourceModel(model);
}

bool DebuggerToolTipExpressionFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    // Match on expression for top level, else pass through.
    if (sourceParent.isValid())
        return true;
    const QModelIndex expIndex = sourceModel()->index(sourceRow, 0, sourceParent);
    return expIndex.data().toString() == m_expression;
}

/*!
    \class DebuggerToolTipTreeView

    A treeview that adapts its size to the model contents (also while expanding)
    to be used within DebuggerTreeViewToolTipWidget.

*/

DebuggerToolTipTreeView::DebuggerToolTipTreeView(QWidget *parent) :
    QTreeView(parent)
{
    setHeaderHidden(true);

    setUniformRowHeights(true);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    connect(this, SIGNAL(collapsed(QModelIndex)), this, SLOT(computeSize()),
        Qt::QueuedConnection);
    connect(this, SIGNAL(expanded(QModelIndex)), this, SLOT(computeSize()),
        Qt::QueuedConnection);
}

QAbstractItemModel *DebuggerToolTipTreeView::swapModel(QAbstractItemModel *newModel)
{
    QAbstractItemModel *previousModel = model();
    if (previousModel != newModel) {
        if (previousModel)
            previousModel->disconnect(SIGNAL(rowsInserted(QModelIndex,int,int)), this);
        setModel(newModel);
        connect(newModel, SIGNAL(rowsInserted(QModelIndex,int,int)),
                this, SLOT(computeSize()), Qt::QueuedConnection);
        computeSize();
    }
    return previousModel;
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

    if (model()) {
        const int columnCount = model()->columnCount();
        rootDecorated = model()->rowCount() > 0;
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

/*!
    \class DebuggerTreeViewToolTipWidget

    Tool tip widget for tree views with functionality to save/restore tree
    model contents to XML.
    With the engine acquired, it sets a filter model (by expression) on
    one of the engine's models (debuggerModel).
    On release, it serializes and restores the data to a QStandardItemModel
    (defaultModel) and displays that.

*/

DebuggerTreeViewToolTipWidget::DebuggerTreeViewToolTipWidget(QWidget *parent) :
    AbstractDebuggerToolTipWidget(parent),
    m_debuggerModel(TooltipsWatch),
    m_treeView(new DebuggerToolTipTreeView),
    m_defaultModel(new QStandardItemModel(this))
{
    addWidget(m_treeView);
}

void DebuggerTreeViewToolTipWidget::doAcquireEngine(Debugger::DebuggerEngine *engine)
{
    // Create a filter model on the debugger's model and switch to it.
    QAbstractItemModel *model = 0;
    switch (m_debuggerModel) {
    case LocalsWatch:
        model = engine->localsModel();
        break;
    case WatchersWatch:
        model = engine->watchersModel();
        break;
    case TooltipsWatch:
        model = engine->toolTipsModel();
        break;
    }
    QTC_ASSERT(model, return ;)
    DebuggerToolTipExpressionFilterModel *filterModel =
            new DebuggerToolTipExpressionFilterModel(model, m_expression);
    m_treeView->swapModel(filterModel);
}

void DebuggerTreeViewToolTipWidget::doReleaseEngine()
{
    // Save data to stream and restore to the  m_defaultModel (QStandardItemModel)
    m_defaultModel->removeRows(0, m_defaultModel->rowCount());
    if (const QAbstractItemModel *model = m_treeView->model()) {
        TreeModelCopyVisitor v(model, m_defaultModel);
        v.run();
    }
    delete m_treeView->swapModel(m_defaultModel);
}

void DebuggerTreeViewToolTipWidget::restoreTreeModel(QXmlStreamReader &r, QStandardItemModel *m)
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
            if (element == QLatin1String(modelRowElementC)) {
                builder.endRow();
            } else if (element == QLatin1String(modelElementC)) {
                withinModel = false;
            }
        }
            break; // EndElement
        default:
            break;
        } // switch
    } // while
}

void DebuggerTreeViewToolTipWidget::doSaveSessionData(QXmlStreamWriter &w) const
{
    w.writeStartElement(QLatin1String(treeElementC));
    QXmlStreamAttributes attributes;
    attributes.append(QLatin1String(treeModelAttributeC), QString::number(m_debuggerModel));
    attributes.append(QLatin1String(treeExpressionAttributeC), m_expression);
    w.writeAttributes(attributes);
    if (QAbstractItemModel *model = m_treeView->model()) {
        XmlWriterTreeModelVisitor v(model, w);
        v.run();
    }
    w.writeEndElement();
}

void DebuggerTreeViewToolTipWidget::doLoadSessionData(QXmlStreamReader &r)
{
    if (!readStartElement(r, treeElementC))
        return;
    // Restore data to default model and show that.
    const QXmlStreamAttributes attributes = r.attributes();
    m_debuggerModel = attributes.value(QLatin1String(treeModelAttributeC)).toString().toInt();
    m_expression = attributes.value(QLatin1String(treeExpressionAttributeC)).toString();
    if (debugToolTips)
        qDebug() << "DebuggerTreeViewToolTipWidget::doLoadSessionData() " << m_debuggerModel << m_expression;
    setObjectName(QLatin1String("DebuggerTreeViewToolTipWidget: ") + m_expression);
    restoreTreeModel(r, m_defaultModel);
    r.readNext(); // Skip </tree>
    m_treeView->swapModel(m_defaultModel);
}

/*!
    \class DebuggerToolTipManager

    Manages the tooltip widgets, listens on editor scroll and main window move
    events and takes care of repositioning the tooltips.

    Listens to editor change and mode change. In debug mode, if there tooltips
    for the current editor (by file name), position and show them.

    In addition, listen on state and stack frame change of the engine.
    If a stack frame is activated, have all matching tooltips (by file name)
    acquire the engine, other release.

*/

DebuggerToolTipManager *DebuggerToolTipManager::m_instance = 0;

DebuggerToolTipManager::DebuggerToolTipManager(QObject *parent) :
    QObject(parent), m_debugModeActive(false)
{
    DebuggerToolTipManager::m_instance = this;
}

DebuggerToolTipManager::~DebuggerToolTipManager()
{
    DebuggerToolTipManager::m_instance = 0;
}

void DebuggerToolTipManager::registerEngine(DebuggerEngine *engine)
{
    connect(engine, SIGNAL(stateChanged(Debugger::DebuggerState)),
            this, SLOT(slotDebuggerStateChanged(Debugger::DebuggerState)));
    connect(engine, SIGNAL(stackFrameCompleted()), this, SLOT(slotStackFrameCompleted()));
}

void DebuggerToolTipManager::add(const QPoint &p, AbstractDebuggerToolTipWidget *toolTipWidget)
{
    closeUnpinnedToolTips();
    toolTipWidget->move(p);
    toolTipWidget->show();
    add(toolTipWidget);
}

void DebuggerToolTipManager::add(AbstractDebuggerToolTipWidget *toolTipWidget)
{
    QTC_ASSERT(toolTipWidget->context().isValid(), return; )
    connect(toolTipWidget, SIGNAL(closeAllRequested()), this, SLOT(closeAllToolTips()));
    m_tooltips.push_back(toolTipWidget);
}

DebuggerToolTipManager::DebuggerToolTipWidgetList &DebuggerToolTipManager::purgeClosedToolTips()
{
    if (!m_tooltips.isEmpty()) {
        for (DebuggerToolTipWidgetList::iterator it = m_tooltips.begin(); it != m_tooltips.end() ; ) {
            if (it->isNull()) {
                it = m_tooltips.erase(it);
            } else {
                ++it;
            }
        }
    }
    return m_tooltips;
}

void DebuggerToolTipManager::moveToolTipsBy(const QPoint &distance)
{
    foreach (const QPointer<AbstractDebuggerToolTipWidget> &tw, purgeClosedToolTips())
        if (tw->isVisible())
            tw->move (tw->pos() + distance);
}

bool DebuggerToolTipManager::eventFilter(QObject *, QEvent *e)
{
    // Move along with parent (toplevel)
    if (e->type() == QEvent::Move && isActive()) {
        const QMoveEvent *me = static_cast<const QMoveEvent *>(e);
        moveToolTipsBy(me->pos() - me->oldPos());
    }
    return false;
}

void DebuggerToolTipManager::sessionAboutToChange()
{
    closeAllToolTips();
}

void DebuggerToolTipManager::loadSessionData()
{
    const QString data = debuggerCore()->sessionValue(QLatin1String(sessionSettingsKeyC)).toString();
    if (data.isEmpty())
        return;
    QXmlStreamReader r(data);
    r.readNextStartElement();
    if (r.tokenType() != QXmlStreamReader::StartElement || r.name() != QLatin1String(sessionDocumentC))
        return;
    const double version = r.attributes().value(QLatin1String(sessionVersionAttributeC)).toString().toDouble();
    while (!r.atEnd())
        if (AbstractDebuggerToolTipWidget *tw = AbstractDebuggerToolTipWidget::loadSessionData(r))
            add(tw);

    if (debugToolTips)
        qDebug() << "DebuggerToolTipManager::loadSessionData version " << version << " restored " << m_tooltips.size();
    slotUpdateVisibleToolTips();
}

void DebuggerToolTipManager::saveSessionData()
{
    QString data;
    if (!purgeClosedToolTips().isEmpty()) {
        QXmlStreamWriter w(&data);
        w.writeStartDocument();
        w.writeStartElement(QLatin1String(sessionDocumentC));
        w.writeAttribute(QLatin1String(sessionVersionAttributeC), QLatin1String("1.0"));
        foreach (const QPointer<AbstractDebuggerToolTipWidget> &tw, m_tooltips)
            tw->saveSessionData(w);
        w.writeEndDocument();
    }
    if (debugToolTips)
        qDebug() << "DebuggerToolTipManager::saveSessionData" << m_tooltips.size() << data ;
    debuggerCore()->setSessionValue(QLatin1String(sessionSettingsKeyC), QVariant(data));
}

void DebuggerToolTipManager::closeAllToolTips()
{
    if (debugToolTips)
        qDebug() << "DebuggerToolTipManager::closeAllToolTips";

    foreach (const QPointer<AbstractDebuggerToolTipWidget> &tw, purgeClosedToolTips())
        tw->close();
    m_tooltips.clear();
}

void DebuggerToolTipManager::closeUnpinnedToolTips()
{
    if (debugToolTips)
        qDebug() << "DebuggerToolTipManager::closeUnpinnedToolTips";

    // Filter out unpinned ones, purge null on that occasion
    for (DebuggerToolTipWidgetList::iterator it = m_tooltips.begin(); it != m_tooltips.end() ; ) {
        if (it->isNull()) {
            it = m_tooltips.erase(it);
        } else if ((*it)->pinState() == AbstractDebuggerToolTipWidget::Unpinned) {
            (*it)->close();
            it = m_tooltips.erase(it);
        } else {
            ++it;
        }
    }
}

void DebuggerToolTipManager::hide()
{
    foreach (const QPointer<AbstractDebuggerToolTipWidget> &tw, purgeClosedToolTips())
        tw->hide();
}

void DebuggerToolTipManager::slotUpdateVisibleToolTips()
{
    if (purgeClosedToolTips().isEmpty())
        return;
    if (!m_debugModeActive) {
        hide();
        return;
    }

    QString fileName;
    QPlainTextEdit *plainTextEdit = currentPlainTextEditor(&fileName);

    if (debugToolTips)
        qDebug() << "DebuggerToolTipManager::slotUpdateVisibleToolTips() " << fileName << sender();

    if (fileName.isEmpty() || !plainTextEdit) {
        hide();
        return;
    }

    // Reposition and show all tooltips of that file.
    foreach (const QPointer<AbstractDebuggerToolTipWidget> &tw, m_tooltips) {
        if (tw->fileName() == fileName) {
            tw->positionShow(plainTextEdit);
        } else {
            tw->hide();
        }
    }
}

void DebuggerToolTipManager::slotDebuggerStateChanged(Debugger::DebuggerState state)
{
    const QObject *engine = sender();
    QTC_ASSERT(engine, return; )

    const QString name = engine->objectName();
    if (debugToolTips)
        qDebug() << "DebuggerToolTipWidget::debuggerStateChanged"
                 << engine << Debugger::DebuggerEngine::stateName(state);

    // Release at earliest possible convenience.
    switch (state) {
    case InferiorRunRequested:
    case DebuggerNotReady:
    case InferiorShutdownRequested:
    case EngineShutdownRequested:
    case DebuggerFinished:
    case EngineShutdownOk:
        foreach (const QPointer<AbstractDebuggerToolTipWidget> &tw, purgeClosedToolTips())
            if (tw->engineType() == name)
                tw->releaseEngine();
        break;
    default:
        break;
    }
}

void DebuggerToolTipManager::slotStackFrameCompleted()
{
    if (purgeClosedToolTips().isEmpty())
        return;
    DebuggerEngine *engine = qobject_cast<DebuggerEngine *>(sender());
    QTC_ASSERT(engine, return ; )

    // Stack frame changed: All tooltips of that file acquire the engine,
    // all others release (arguable, this could be more precise?)
    QString fileName;
    int lineNumber = 0;
    // Get the current frame
    const QString engineName = engine->objectName();
    QString function;
    const int index = engine->stackHandler()->currentIndex();
    if (index >= 0) {
        const StackFrame frame = engine->stackHandler()->currentFrame();
        if (frame.usable) {
            fileName = frame.file;
            lineNumber = frame.line;
            function = frame.function;
        }
    }
    if (debugToolTips)
        qDebug("DebuggerToolTipWidget::slotStackFrameCompleted(%s, %s@%d, %s())",
               qPrintable(engineName), qPrintable(fileName), lineNumber,
               qPrintable(function));
    unsigned acquiredCount = 0;
    foreach (const QPointer<AbstractDebuggerToolTipWidget> &tw, m_tooltips) {
        if (tw->matches(fileName, engineName, function)) {
            tw->acquireEngine(engine);
            acquiredCount++;
        } else {
            tw->releaseEngine();
        }
    }
    slotUpdateVisibleToolTips(); // Move out when stepping in same file.
    if (debugToolTips)
        qDebug() << "DebuggerToolTipWidget::slotStackFrameCompleted()"
                 << engineName << fileName << lineNumber << " acquired " << acquiredCount;
}

bool DebuggerToolTipManager::debug()
{
    return debugToolTips;
}

void DebuggerToolTipManager::slotEditorOpened(Core::IEditor *e)
{
    // Move tooltip along when scrolled.
    if (QPlainTextEdit *plainTextEdit = plainTextEditor(e)) {
        connect(plainTextEdit->verticalScrollBar(), SIGNAL(valueChanged(int)),
                this, SLOT(slotUpdateVisibleToolTips()));
    }
}

void DebuggerToolTipManager::debugModeEntered()
{
    if (debugToolTips)
        qDebug("DebuggerToolTipManager::debugModeEntered");

    // Hook up all signals in debug mode.
    if (!m_debugModeActive) {
        m_debugModeActive = true;
        QWidget *topLevel = Core::ICore::instance()->mainWindow()->topLevelWidget();
        topLevel->installEventFilter(this);
        Core::EditorManager *em = Core::EditorManager::instance();
        connect(em, SIGNAL(currentEditorChanged(Core::IEditor*)),
                this, SLOT(slotUpdateVisibleToolTips()));
        connect(em, SIGNAL(editorOpened(Core::IEditor*)),
                this, SLOT(slotEditorOpened(Core::IEditor*)));
        foreach (Core::IEditor *e, em->openedEditors())
            slotEditorOpened(e);
        // Position tooltips delayed once all the editor placeholder layouting is done.
        if (!m_tooltips.isEmpty())
            QTimer::singleShot(0, this, SLOT(slotUpdateVisibleToolTips()));
    }
}

void DebuggerToolTipManager::leavingDebugMode()
{
    if (debugToolTips)
            qDebug("DebuggerToolTipManager::leavingDebugMode");

    // Remove all signals in debug mode.
    if (m_debugModeActive) {
        m_debugModeActive = false;
        hide();
        QWidget *topLevel = Core::ICore::instance()->mainWindow()->topLevelWidget();
        topLevel->removeEventFilter(this);
        Core::EditorManager *em = Core::EditorManager::instance();
        foreach (Core::IEditor *e, em->openedEditors())
            if (QPlainTextEdit *plainTextEdit = plainTextEditor(e))
                plainTextEdit->verticalScrollBar()->disconnect(this);
        em->disconnect(this);
    }
}

QStringList DebuggerToolTipManager::treeWidgetExpressions(const QString &fileName,
                                                          const QString &engineType,
                                                          const QString &function) const
{
    QStringList rc;
    foreach (const QPointer<AbstractDebuggerToolTipWidget> &tw, m_tooltips)
        if (!tw.isNull() && tw->matches(fileName, engineType, function))
            if (const DebuggerTreeViewToolTipWidget *ttw = qobject_cast<const DebuggerTreeViewToolTipWidget *>(tw.data()))
                rc.push_back(ttw->expression());
    if (debugToolTips)
        qDebug() << "DebuggerToolTipManager::treeWidgetExpressions"
                 << fileName << engineType << function << rc;
    return rc;
}

} // namespace Internal
} // namespace Debugger
