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

#include "debuggertooltipmanager.h"
#include "debuggerinternalconstants.h"
#include "watchutils.h"
#include "debuggerengine.h"
#include "debuggeractions.h"
#include "watchhandler.h"
#include "watchutils.h"
#include "stackhandler.h"
#include "debuggercore.h"

#include <coreplugin/icore.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/imode.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <texteditor/itexteditor.h>
#include <texteditor/basetexteditor.h>

#include <utils/tooltip/tooltip.h>
#include <utils/tooltip/tipcontents.h>
#include <utils/qtcassert.h>

#include <QToolButton>
#include <QToolBar>
#include <QVBoxLayout>
#include <QStyle>
#include <QIcon>
#include <QApplication>
#include <QMoveEvent>
#include <QDesktopWidget>
#include <QScrollBar>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QPlainTextEdit>
#include <QTextCursor>
#include <QTextDocument>
#include <QLabel>
#include <QClipboard>

#include <QVariant>
#include <QStack>
#include <QDebug>
#include <QTimer>

using namespace Core;
using namespace TextEditor;

enum { debugToolTips = 0 };
enum { debugToolTipPositioning = 0 };

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
static const char offsetXAttributeC[] = "offset_x";
static const char offsetYAttributeC[] = "offset_y";
static const char engineTypeAttributeC[] = "engine";
static const char dateAttributeC[] = "date";
static const char treeElementC[] = "tree";
static const char treeExpressionAttributeC[] = "expression";
static const char treeInameAttributeC[] = "iname";
static const char modelElementC[] = "model";
static const char modelColumnCountAttributeC[] = "columncount";
static const char modelRowElementC[] = "row";
static const char modelItemElementC[] = "item";

// Forward a stream reader across end elements looking for the
// next start element of a desired type.
static bool readStartElement(QXmlStreamReader &r, const char *name)
{
    if (debugToolTips > 1)
        qDebug("readStartElement: looking for '%s', currently at: %s/%s",
               name, qPrintable(r.tokenString()), qPrintable(r.name().toString()));
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

#if 0
static void debugMode(const QAbstractItemModel *model)
{
    QDebug nospace = qDebug().nospace();
    nospace << model << '\n';
    for (int r = 0; r < model->rowCount(); r++)
        nospace << '#' << r << ' ' << model->data(model->index(r, 0)).toString() << '\n';
}
#endif

namespace Debugger {
namespace Internal {

/* A Label that emits a signal when the user drags for moving the parent
 * widget around. */
class DraggableLabel : public QLabel
{
    Q_OBJECT
public:
    explicit DraggableLabel(QWidget *parent = 0);

    bool isActive() const { return m_active; }
    void setActive(bool v) { m_active = v; }

signals:
    void dragged(const QPoint &d);

protected:
    virtual void mousePressEvent(QMouseEvent * event);
    virtual void mouseReleaseEvent(QMouseEvent * event);
    virtual void mouseMoveEvent(QMouseEvent * event);

private:
    QPoint m_moveStartPos;
    bool m_active;
};

DraggableLabel::DraggableLabel(QWidget *parent) :
    QLabel(parent), m_moveStartPos(-1, -1), m_active(false)
{
}

void DraggableLabel::mousePressEvent(QMouseEvent * event)
{
    if (m_active && event->button() ==  Qt::LeftButton) {
        m_moveStartPos = event->globalPos();
        event->accept();
    }
    QLabel::mousePressEvent(event);
}

void DraggableLabel::mouseReleaseEvent(QMouseEvent * event)
{
    if (m_active && event->button() ==  Qt::LeftButton)
        m_moveStartPos = QPoint(-1, -1);
    QLabel::mouseReleaseEvent(event);
}

void DraggableLabel::mouseMoveEvent(QMouseEvent * event)
{
    if (m_active && (event->buttons() & Qt::LeftButton)) {
        if (m_moveStartPos != QPoint(-1, -1)) {
            const QPoint newPos = event->globalPos();
            emit dragged(event->globalPos() - m_moveStartPos);
            m_moveStartPos = newPos;
        }
        event->accept();
    }
    QLabel::mouseMoveEvent(event);
}

// A convenience struct to pass around all tooltip-relevant editor members
// (TextEditor, Widget, File, etc), constructing from a Core::IEditor.

class DebuggerToolTipEditor
{
public:
    explicit DebuggerToolTipEditor(IEditor *ie = 0);
    bool isValid() const { return textEditor && baseTextEditor && document; }
    operator bool() const { return isValid(); }
    QString fileName() const { return document ? document->fileName() : QString(); }

    static DebuggerToolTipEditor currentToolTipEditor();

    ITextEditor *textEditor;
    BaseTextEditorWidget *baseTextEditor;
    IDocument *document;
};

DebuggerToolTipEditor::DebuggerToolTipEditor(IEditor *ie) :
    textEditor(0), baseTextEditor(0), document(0)
{
    if (ie && ie->document()) {
        if (ITextEditor *te = qobject_cast<ITextEditor *>(ie)) {
            if (BaseTextEditorWidget *pe = qobject_cast<BaseTextEditorWidget *>(ie->widget())) {
                textEditor = te;
                baseTextEditor = pe;
                document = ie->document();
            }
        }
    }
}

DebuggerToolTipEditor DebuggerToolTipEditor::currentToolTipEditor()
{
    if (IEditor *ie = EditorManager::currentEditor())
        return DebuggerToolTipEditor(ie);
    return DebuggerToolTipEditor();
}

/* Helper for building a QStandardItemModel of a tree form (see TreeModelVisitor).
 * The recursion/building is based on the scheme: \code
<row><item1><item2>
    <row><item11><item12></row>
</row>
\endcode */

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

/* Helper visitor base class for recursing over a tree model
 * (see StandardItemTreeModelBuilder for the scheme). */
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

} // namespace Internal
} // namespace Debugger

/*
static QDebug operator<<(QDebug d, const QAbstractItemModel &model)
{
    QString s;
    QTextStream str(&s);
    Debugger::Internal::DumpTreeModelVisitor v(&model, Debugger::Internal::DumpTreeModelVisitor::DebugMode, str);
    v.run();
    qDebug().nospace() << s;
    return d;
}
*/

namespace Debugger {
namespace Internal {

// Visitor building a QStandardItem from a tree model (copy).
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

void DebuggerToolTipWidget::addWidget(QWidget *w)
{
    w->setFocusPolicy(Qt::NoFocus);
    m_mainVBoxLayout->addWidget(w);
}

void DebuggerToolTipWidget::addToolBarWidget(QWidget *w)
{
    m_toolBar->addWidget(w);
}

void DebuggerToolTipWidget::pin()
{
    if (m_isPinned)
        return;
    m_isPinned = true;
    m_toolButton->setIcon(style()->standardIcon(QStyle::SP_DockWidgetCloseButton));

    if (parentWidget()) {
        // We are currently within a text editor tooltip:
        // Rip out of parent widget and re-show as a tooltip
        Utils::WidgetContent::pinToolTip(this);
    } else {
        // We have just be restored from session data.
        setWindowFlags(Qt::ToolTip);
    }
    m_titleLabel->setActive(true); // User can now drag
}

void DebuggerToolTipWidget::toolButtonClicked()
{
    if (m_isPinned)
        close();
    else
        pin();
}

/*!
    \class Debugger::Internal::DebuggerToolTipContext

    File name and position where the tooltip is anchored. Redundant position/line column
    information is used to detect if the underlying file has been changed
    on restoring.
*/

DebuggerToolTipContext::DebuggerToolTipContext() : position(0), line(0), column(0)
{
}

DebuggerToolTipContext DebuggerToolTipContext::fromEditor(IEditor *ie, int pos)
{
    DebuggerToolTipContext rc;
    if (const IDocument *document = ie->document()) {
        if (const ITextEditor *te = qobject_cast<const ITextEditor *>(ie)) {
            rc.fileName = document->fileName();
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
    \class Debugger::Internal::DebuggerToolTipWidget

    A debugger tooltip is pinnable. It goes from the unpinned state (button
    showing 'Pin') to the pinned state (button showing 'Close').
    It consists of a title toolbar and a vertical main layout.
    The widget has the ability to save/restore tree model contents to XML.
    With the engine acquired, it sets a filter model (by expression) on
    one of the engine's models (debuggerModel).
    On release, it serializes and restores the data to a QStandardItemModel
    (defaultModel) and displays that.

    It is associated with file name and position with functionality to
    acquire and release the engine: When the debugger stops at a file, all
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


static QString msgReleasedText() { return DebuggerToolTipWidget::tr("Previous"); }

DebuggerToolTipWidget::DebuggerToolTipWidget(QWidget *parent) :
    QWidget(parent),
    m_isPinned(false),
    m_mainVBoxLayout(new QVBoxLayout),
    m_toolBar(new QToolBar),
    m_toolButton(new QToolButton),
    m_titleLabel(new DraggableLabel),
    m_engineAcquired(false),
    m_creationDate(QDate::currentDate()),
    m_treeView(new DebuggerToolTipTreeView),
    m_defaultModel(new QStandardItemModel(this))
{
    m_mainVBoxLayout->setSizeConstraint(QLayout::SetFixedSize);
    m_mainVBoxLayout->setContentsMargins(0, 0, 0, 0);

    const QIcon pinIcon(QLatin1String(":/debugger/images/pin.xpm"));
    const QList<QSize> pinIconSizes = pinIcon.availableSizes();

    m_toolButton->setIcon(pinIcon);
    connect(m_toolButton, SIGNAL(clicked()), this, SLOT(toolButtonClicked()));

    m_toolBar->setProperty("_q_custom_style_disabled", QVariant(true));
    if (!pinIconSizes.isEmpty())
        m_toolBar->setIconSize(pinIconSizes.front());
    m_toolBar->addWidget(m_toolButton);

    m_mainVBoxLayout->addWidget(m_toolBar);

    setLayout(m_mainVBoxLayout);
    QToolButton *copyButton = new QToolButton;
    copyButton->setIcon(QIcon(QLatin1String(Core::Constants::ICON_COPY)));
    connect(copyButton, SIGNAL(clicked()), this, SLOT(copy()));
    addToolBarWidget(copyButton);

    m_titleLabel->setText(msgReleasedText());
    m_titleLabel->setMinimumWidth(40); // Ensure a draggable area even if text is empty.
    connect(m_titleLabel, SIGNAL(dragged(QPoint)), this, SLOT(slotDragged(QPoint)));
    addToolBarWidget(m_titleLabel);
    addWidget(m_treeView);
}

bool DebuggerToolTipWidget::matches(const QString &fileName,
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

void DebuggerToolTipWidget::acquireEngine(DebuggerEngine *engine)
{
    QTC_ASSERT(engine, return);

    if (debugToolTips)
        qDebug() << this << " acquiring" << engine << m_engineAcquired;
    if (m_engineAcquired)
        return;
    doAcquireEngine(engine);
    m_engineType = engine->objectName();
    m_engineAcquired = true;
    m_titleLabel->setText(QString());
}

void DebuggerToolTipWidget::releaseEngine()
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

void DebuggerToolTipWidget::copy()
{
    const QString clipboardText = clipboardContents();
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(clipboardText, QClipboard::Selection);
    clipboard->setText(clipboardText, QClipboard::Clipboard);
}

void DebuggerToolTipWidget::slotDragged(const QPoint &p)
{
    move(pos() + p);
    m_offset += p;
}

bool DebuggerToolTipWidget::positionShow(const DebuggerToolTipEditor &te)
{
    // Figure out new position of tooltip using the text edit.
    // If the line changed too much, close this tip.
    QTC_ASSERT(te, return false);
    QTextCursor cursor(te.baseTextEditor->document());
    cursor.setPosition(m_context.position);
    const int line = cursor.blockNumber();
    if (qAbs(m_context.line - line) > 2) {
        if (debugToolTips)
            qDebug() << "Closing " << this << " in positionShow() lines "
                     << line << m_context.line;
        close();
        return false;
    }
    if (debugToolTipPositioning)
        qDebug() << "positionShow" << this << line << cursor.columnNumber();

    const QPoint screenPos = te.baseTextEditor->toolTipPosition(cursor) + m_offset;
    const QRect toolTipArea = QRect(screenPos, QSize(sizeHint()));
    const QRect plainTextArea = QRect(te.baseTextEditor->mapToGlobal(QPoint(0, 0)), te.baseTextEditor->size());
    const bool visible = plainTextArea.contains(toolTipArea);
    if (debugToolTips)
        qDebug() << "DebuggerToolTipWidget::positionShow() " << this << m_context
                 << " line: " << line << " plainTextPos " << toolTipArea
                 << " offset: " << m_offset
                 << " Area: " << plainTextArea << " Screen pos: "
                 << screenPos << te.baseTextEditor << " visible=" << visible;

    if (!visible) {
        hide();
        return false;
    }

    move(screenPos);
    show();
    return true;
}

 // Parse a 'yyyyMMdd' date
static QDate dateFromString(const QString &date)
{
    return date.size() == 8 ?
        QDate(date.left(4).toInt(), date.mid(4, 2).toInt(), date.mid(6, 2).toInt()) :
        QDate();
}

DebuggerToolTipWidget *DebuggerToolTipWidget::loadSessionData(QXmlStreamReader &r)
{
    if (debugToolTips)
        qDebug() << ">DebuggerToolTipWidget::loadSessionData" << r.tokenString() << r.name();
    DebuggerToolTipWidget *rc = DebuggerToolTipWidget::loadSessionDataI(r);
    if (debugToolTips)
        qDebug() << "<DebuggerToolTipWidget::loadSessionData" << r.tokenString() << r.name() << " returns  " << rc;
    return rc;
}

DebuggerToolTipWidget *DebuggerToolTipWidget::loadSessionDataI(QXmlStreamReader &r)
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
    QPoint offset;
    const QString offsetXAttribute = QLatin1String(offsetXAttributeC);
    const QString offsetYAttribute = QLatin1String(offsetYAttributeC);
    if (attributes.hasAttribute(offsetXAttribute))
        offset.setX(attributes.value(offsetXAttribute).toString().toInt());
    if (attributes.hasAttribute(offsetYAttribute))
        offset.setY(attributes.value(offsetYAttribute).toString().toInt());

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
        qDebug() << "Creating tooltip " << context <<  " from " << creationDate << offset;
    DebuggerToolTipWidget *rc = 0;
    if (className == QLatin1String("Debugger::Internal::DebuggerToolTipWidget"))
        rc = new DebuggerToolTipWidget;
    if (rc) {
        rc->setContext(context);
        rc->setAttribute(Qt::WA_DeleteOnClose);
        rc->setEngineType(engineType);
        rc->doLoadSessionData(r);
        rc->setCreationDate(creationDate);
        if (!offset.isNull())
            rc->setOffset(offset);
        rc->pin();
    } else {
        qWarning("Unable to create debugger tool tip widget of class %s", qPrintable(className));
        r.readElementText(QXmlStreamReader::SkipChildElements); // Skip
    }
    return rc;
}

void DebuggerToolTipWidget::saveSessionData(QXmlStreamWriter &w) const
{
    w.writeStartElement(QLatin1String(toolTipElementC));
    QXmlStreamAttributes attributes;
    attributes.append(QLatin1String(toolTipClassAttributeC), QString::fromLatin1(metaObject()->className()));
    attributes.append(QLatin1String(fileNameAttributeC), m_context.fileName);
    if (!m_context.function.isEmpty())
        attributes.append(QLatin1String(functionAttributeC), m_context.function);
    attributes.append(QLatin1String(textPositionAttributeC), QString::number(m_context.position));
    attributes.append(QLatin1String(textLineAttributeC), QString::number(m_context.line));
    attributes.append(QLatin1String(textColumnAttributeC), QString::number(m_context.column));
    attributes.append(QLatin1String(dateAttributeC), m_creationDate.toString(QLatin1String("yyyyMMdd")));
    if (m_offset.x())
        attributes.append(QLatin1String(offsetXAttributeC), QString::number(m_offset.x()));
    if (m_offset.y())
        attributes.append(QLatin1String(offsetYAttributeC), QString::number(m_offset.y()));
    if (!m_engineType.isEmpty())
        attributes.append(QLatin1String(engineTypeAttributeC), m_engineType);
    w.writeAttributes(attributes);
    doSaveSessionData(w);
    w.writeEndElement();
}

/*!
    \class Debugger::Internal::TooltipFilterModel

    \brief Model for tooltips filtering an item on the watchhandler matching its tree on the iname.

    In addition, suppress the model's tooltip data to avoid a tooltip on a tooltip.
*/

class TooltipFilterModel : public QSortFilterProxyModel
{
public:
    TooltipFilterModel(QAbstractItemModel *model, const QByteArray &iname)
        : m_iname(iname)
    {
        setSourceModel(model);
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const
    {
        return role == Qt::ToolTipRole
            ? QVariant() : QSortFilterProxyModel::data(index, role);
    }

    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;

private:
    const QByteArray m_iname;
};

static bool isSubIname(const QByteArray &haystack, const QByteArray &needle)
{
    return haystack.size() > needle.size()
           && haystack.startsWith(needle)
           && haystack.at(needle.size()) == '.';
}

bool TooltipFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    const QModelIndex nameIndex = sourceModel()->index(sourceRow, 0, sourceParent);
    const QByteArray iname = nameIndex.data(LocalsINameRole).toByteArray();
    return iname == m_iname || isSubIname(iname, m_iname) || isSubIname(m_iname, iname);
}

/*!
    \class Debugger::Internal::DebuggerToolTipTreeView

    A treeview that adapts its size to the model contents (also while expanding)
    to be used within DebuggerTreeViewToolTipWidget.

*/

DebuggerToolTipTreeView::DebuggerToolTipTreeView(QWidget *parent) :
    QTreeView(parent)
{
    setHeaderHidden(true);
    setEditTriggers(NoEditTriggers);

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
        //setRootIndex(newModel->index(0, 0));
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

void DebuggerToolTipWidget::doAcquireEngine(DebuggerEngine *engine)
{
    // Create a filter model on the debugger's model and switch to it.
    QAbstractItemModel *model = engine->watchModel();
    TooltipFilterModel *filterModel =
            new TooltipFilterModel(model, m_iname);
    swapModel(filterModel);
}

QAbstractItemModel *DebuggerToolTipWidget::swapModel(QAbstractItemModel *newModel)
{
    QAbstractItemModel *oldModel = m_treeView->swapModel(newModel);
    // When looking at some 'this.m_foo.x', expand all items
    if (newModel) {
        if (const int level = m_iname.count('.')) {
            QModelIndex index = newModel->index(0, 0);
            for (int i = 0; i < level && index.isValid(); i++, index = index.child(0, 0))
                m_treeView->setExpanded(index, true);
        }
    }
    return oldModel;
}

void DebuggerToolTipWidget::doReleaseEngine()
{
    // Save data to stream and restore to the  m_defaultModel (QStandardItemModel)
    m_defaultModel->removeRows(0, m_defaultModel->rowCount());
    if (const QAbstractItemModel *model = m_treeView->model()) {
        TreeModelCopyVisitor v(model, m_defaultModel);
        v.run();
    }
    delete swapModel(m_defaultModel);
}

void DebuggerToolTipWidget::restoreTreeModel(QXmlStreamReader &r, QStandardItemModel *m)
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

void DebuggerToolTipWidget::doSaveSessionData(QXmlStreamWriter &w) const
{
    w.writeStartElement(QLatin1String(treeElementC));
    QXmlStreamAttributes attributes;
    if (!m_expression.isEmpty())
        attributes.append(QLatin1String(treeExpressionAttributeC), m_expression);
    attributes.append(QLatin1String(treeInameAttributeC), QLatin1String(m_iname));
    w.writeAttributes(attributes);
    if (QAbstractItemModel *model = m_treeView->model()) {
        XmlWriterTreeModelVisitor v(model, w);
        v.run();
    }
    w.writeEndElement();
}

void DebuggerToolTipWidget::doLoadSessionData(QXmlStreamReader &r)
{
    if (!readStartElement(r, treeElementC))
        return;
    // Restore data to default model and show that.
    const QXmlStreamAttributes attributes = r.attributes();
    m_iname = attributes.value(QLatin1String(treeInameAttributeC)).toString().toLatin1();
    m_expression = attributes.value(QLatin1String(treeExpressionAttributeC)).toString();
    if (debugToolTips)
        qDebug() << "DebuggerTreeViewToolTipWidget::doLoadSessionData() " << m_debuggerModel << m_iname;
    setObjectName(QLatin1String("DebuggerTreeViewToolTipWidget: ") + QLatin1String(m_iname));
    restoreTreeModel(r, m_defaultModel);
    r.readNext(); // Skip </tree>
    m_treeView->swapModel(m_defaultModel);
}

QString DebuggerToolTipWidget::treeModelClipboardContents(const QAbstractItemModel *m)
{
    QString rc;
    QTextStream str(&rc);
    DumpTreeModelVisitor v(m, DumpTreeModelVisitor::ClipboardMode, str);
    v.run();
    return rc;
}

QString DebuggerToolTipWidget::clipboardContents() const
{
    if (const QAbstractItemModel *model = m_treeView->model())
        return DebuggerToolTipWidget::treeModelClipboardContents(model);
    return QString();
}

/*!
    \class Debugger::Internal::DebuggerToolTipManager

    Manages the pinned tooltip widgets, listens on editor scroll and main window move
    events and takes care of repositioning the tooltips.

    Listens to editor change and mode change. In debug mode, if there tooltips
    for the current editor (by file name), position and show them.

    In addition, listens on state change and stack frame completed signals
    of the engine. If a stack frame is completed, have all matching tooltips
    (by file name and function) acquire the engine, others release.
*/

DebuggerToolTipManager *DebuggerToolTipManager::m_instance = 0;

DebuggerToolTipManager::DebuggerToolTipManager(QObject *parent) :
    QObject(parent), m_debugModeActive(false),
    m_lastToolTipPoint(-1, -1), m_lastToolTipEditor(0)
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

void DebuggerToolTipManager::showToolTip(const QPoint &p, IEditor *editor,
                                         DebuggerToolTipWidget *toolTipWidget)
{
    QWidget *widget = editor->widget();
    if (debugToolTipPositioning)
        qDebug() << "DebuggerToolTipManager::showToolTip" << p << " Mouse at " << QCursor::pos();
    const Utils::WidgetContent widgetContent(toolTipWidget, true);
    Utils::ToolTip::instance()->show(p, widgetContent, widget);
    registerToolTip(toolTipWidget);
}

void DebuggerToolTipManager::registerToolTip(DebuggerToolTipWidget *toolTipWidget)
{
    QTC_ASSERT(toolTipWidget->context().isValid(), return);
    m_tooltips.push_back(toolTipWidget);
}

void DebuggerToolTipManager::purgeClosedToolTips()
{
    for (DebuggerToolTipWidgetList::iterator it = m_tooltips.begin(); it != m_tooltips.end() ; ) {
        if (it->isNull())
            it = m_tooltips.erase(it);
        else
            ++it;
    }
}

void DebuggerToolTipManager::moveToolTipsBy(const QPoint &distance)
{
    purgeClosedToolTips();
    foreach (const QPointer<DebuggerToolTipWidget> &tw, m_tooltips)
        if (tw->isVisible())
            tw->move(tw->pos() + distance);
}

bool DebuggerToolTipManager::eventFilter(QObject *o, QEvent *e)
{
    if (!hasToolTips())
        return false;
    switch (e->type()) {
    case QEvent::Move: { // Move along with parent (toplevel)
        const QMoveEvent *me = static_cast<const QMoveEvent *>(e);
        moveToolTipsBy(me->pos() - me->oldPos());
    }
        break;
    case QEvent::WindowStateChange: { // Hide/Show along with parent (toplevel)
        const QWindowStateChangeEvent *se = static_cast<const QWindowStateChangeEvent *>(e);
        const bool wasMinimized = se->oldState() & Qt::WindowMinimized;
        const bool isMinimized  = static_cast<const QWidget *>(o)->windowState() & Qt::WindowMinimized;
        if (wasMinimized ^ isMinimized) {
            purgeClosedToolTips();
            foreach (const QPointer<DebuggerToolTipWidget> &tw, m_tooltips)
                tw->setVisible(!isMinimized);
        }
    }
        break;
    default:
        break;
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
        if (DebuggerToolTipWidget *tw = DebuggerToolTipWidget::loadSessionData(r))
            registerToolTip(tw);

    if (debugToolTips)
        qDebug() << "DebuggerToolTipManager::loadSessionData version " << version << " restored " << m_tooltips.size();
    slotUpdateVisibleToolTips();
}

void DebuggerToolTipManager::saveSessionData()
{
    QString data;
    purgeClosedToolTips();
    if (!m_tooltips.isEmpty()) {
        QXmlStreamWriter w(&data);
        w.writeStartDocument();
        w.writeStartElement(QLatin1String(sessionDocumentC));
        w.writeAttribute(QLatin1String(sessionVersionAttributeC), QLatin1String("1.0"));
        foreach (const QPointer<DebuggerToolTipWidget> &tw, m_tooltips)
            if (tw->isPinned())
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

    purgeClosedToolTips();
    foreach (const QPointer<DebuggerToolTipWidget> &tw, m_tooltips)
        tw->close();
    m_tooltips.clear();
}

void DebuggerToolTipManager::hide()
{
    purgeClosedToolTips();
    foreach (const QPointer<DebuggerToolTipWidget> &tw, m_tooltips)
        tw->hide();
}

void DebuggerToolTipManager::slotUpdateVisibleToolTips()
{
    purgeClosedToolTips();
    if (m_tooltips.isEmpty())
        return;
    if (!m_debugModeActive) {
        hide();
        return;
    }

    DebuggerToolTipEditor toolTipEditor = DebuggerToolTipEditor::currentToolTipEditor();

    if (debugToolTips)
        qDebug() << "DebuggerToolTipManager::slotUpdateVisibleToolTips() " << sender();

    if (!toolTipEditor.isValid() || toolTipEditor.fileName().isEmpty()) {
        hide();
        return;
    }

    // Reposition and show all tooltips of that file.
    const QString fileName = toolTipEditor.fileName();
    foreach (const QPointer<DebuggerToolTipWidget> &tw, m_tooltips) {
        if (tw->fileName() == fileName)
            tw->positionShow(toolTipEditor);
        else
            tw->hide();
    }
}

void DebuggerToolTipManager::slotDebuggerStateChanged(DebuggerState state)
{
    const QObject *engine = sender();
    QTC_ASSERT(engine, return);

    const QString name = engine->objectName();
    if (debugToolTips)
        qDebug() << "DebuggerToolTipWidget::debuggerStateChanged"
                 << engine << DebuggerEngine::stateName(state);

    // Release at earliest possible convenience.
    switch (state) {
    case InferiorRunRequested:
    case DebuggerNotReady:
    case InferiorShutdownRequested:
    case EngineShutdownRequested:
    case DebuggerFinished:
    case EngineShutdownOk: {
        purgeClosedToolTips();
        foreach (const QPointer<DebuggerToolTipWidget> &tw, m_tooltips)
            if (tw->engineType() == name)
                tw->releaseEngine();
        break;
    }
    default:
        break;
    }
}

void DebuggerToolTipManager::slotStackFrameCompleted()
{
    purgeClosedToolTips();
    if (m_tooltips.isEmpty())
        return;
    DebuggerEngine *engine = qobject_cast<DebuggerEngine *>(sender());
    QTC_ASSERT(engine, return);

    // Stack frame changed: All tooltips of that file acquire the engine,
    // all others release (arguable, this could be more precise?)
    QString fileName;
    int lineNumber = 0;
    // Get the current frame.
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
    foreach (const QPointer<DebuggerToolTipWidget> &tw, m_tooltips) {
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

void DebuggerToolTipManager::slotEditorOpened(IEditor *e)
{
    // Move tooltip along when scrolled.
    if (DebuggerToolTipEditor toolTipEditor = DebuggerToolTipEditor(e)) {
        connect(toolTipEditor.baseTextEditor->verticalScrollBar(), SIGNAL(valueChanged(int)),
                this, SLOT(slotUpdateVisibleToolTips()));
        connect(toolTipEditor.textEditor,
            SIGNAL(tooltipOverrideRequested(TextEditor::ITextEditor*,QPoint,int,bool*)),
            SLOT(slotTooltipOverrideRequested(TextEditor::ITextEditor*,QPoint,int,bool*)));
    }
}

void DebuggerToolTipManager::debugModeEntered()
{
    if (debugToolTips)
        qDebug("DebuggerToolTipManager::debugModeEntered");

    // Hook up all signals in debug mode.
    if (!m_debugModeActive) {
        m_debugModeActive = true;
        QWidget *topLevel = ICore::mainWindow()->topLevelWidget();
        topLevel->installEventFilter(this);
        EditorManager *em = EditorManager::instance();
        connect(em, SIGNAL(currentEditorChanged(Core::IEditor*)),
                this, SLOT(slotUpdateVisibleToolTips()));
        connect(em, SIGNAL(editorOpened(Core::IEditor*)),
                this, SLOT(slotEditorOpened(Core::IEditor*)));
        foreach (IEditor *e, em->openedEditors())
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
        if (QWidget *topLevel = ICore::mainWindow()->topLevelWidget())
            topLevel->removeEventFilter(this);
        if (EditorManager *em = EditorManager::instance()) {
            foreach (IEditor *e, em->openedEditors()) {
                if (DebuggerToolTipEditor toolTipEditor = DebuggerToolTipEditor(e)) {
                    toolTipEditor.baseTextEditor->verticalScrollBar()->disconnect(this);
                    toolTipEditor.textEditor->disconnect(this);
                }
            }
            em->disconnect(this);
        }
        m_lastToolTipEditor = 0;
        m_lastToolTipPoint = QPoint(-1, -1);
    }
}

void DebuggerToolTipManager::slotTooltipOverrideRequested(ITextEditor *editor,
                                                          const QPoint &point,
                                                          int pos, bool *handled)
{
    QTC_ASSERT(handled, return);

    const int movedDistance = (point - m_lastToolTipPoint).manhattanLength();
    const bool samePosition = m_lastToolTipEditor == editor && movedDistance < 25;
    if (debugToolTipPositioning)
        qDebug() << ">slotTooltipOverrideRequested() " << editor << point
                 << "from " << m_lastToolTipPoint << ") pos: "
                 << pos << *handled
                 << " Same position=" << samePosition << " d=" << movedDistance;

    DebuggerEngine *currentEngine = 0;
    do {
        if (samePosition)
            *handled = true;

        if (*handled)
            break; // Avoid flicker.

        DebuggerCore  *core = debuggerCore();
        if (!core->boolSetting(UseToolTipsInMainEditor))
            break;

        currentEngine = core->currentEngine();
        if (!currentEngine || !currentEngine->canDisplayTooltip())
            break;

        const DebuggerToolTipContext context = DebuggerToolTipContext::fromEditor(editor, pos);
        if (context.isValid() && currentEngine->setToolTipExpression(point, editor, context)) {
            *handled = true;
            m_lastToolTipEditor = editor;
            m_lastToolTipPoint = point;
        }

    } while (false);

    // Other tooltip, close all in case mouse never entered the tooltip
    // and no leave was triggered.
    if (!*handled) {
        m_lastToolTipEditor = 0;
        m_lastToolTipPoint = QPoint(-1, -1);
    }
    if (debugToolTipPositioning)
        qDebug() << "<slotTooltipOverrideRequested() " << currentEngine << *handled;
}


DebuggerToolTipManager::ExpressionInamePairs
    DebuggerToolTipManager::treeWidgetExpressions(const QString &fileName,
                                                  const QString &engineType,
                                                  const QString &function) const
{
    ExpressionInamePairs rc;
    foreach (const QPointer<DebuggerToolTipWidget> &tw, m_tooltips) {
        if (!tw.isNull() && tw->matches(fileName, engineType, function))
            rc.push_back(ExpressionInamePair(tw->expression(), tw->iname()));
    }
    if (debugToolTips)
        qDebug() << "DebuggerToolTipManager::treeWidgetExpressions"
                 << fileName << engineType << function << rc;
    return rc;
}

} // namespace Internal
} // namespace Debugger

#include "debuggertooltipmanager.moc"
