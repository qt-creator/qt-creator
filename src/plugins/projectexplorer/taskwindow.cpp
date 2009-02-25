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

#include "taskwindow.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/uniqueidmanager.h>
#include <extensionsystem/pluginmanager.h>
#include <texteditor/itexteditor.h>
#include <texteditor/basetexteditor.h>
#include <projectexplorerconstants.h>

#include <QtCore/QDir>
#include <QtGui/QKeyEvent>
#include <QtGui/QHeaderView>
#include <QtGui/QListView>
#include <QtGui/QPainter>
#include <QtCore/QAbstractItemModel>
#include <QtGui/QApplication>
#include <QtGui/QClipboard>
#include <QtGui/QFont>
#include <QtGui/QFontMetrics>
#include <QtGui/QTextLayout>

using namespace ProjectExplorer::Internal;

// Internal Struct for TaskModel
struct TaskItem
{
    QString description;
    QString file;
    int line;
    bool fileNotFound;
    ProjectExplorer::BuildParserInterface::PatternType type;
};

class ProjectExplorer::Internal::TaskModel : public QAbstractItemModel
{
public:
    // Model stuff
    TaskModel();
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &child) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    void clear();
    void addTask(ProjectExplorer::BuildParserInterface::PatternType type,
                         const QString &description, const QString &file, int line);
    int sizeOfFile();
    int sizeOfLineNumber();
    QModelIndex firstError() const;
    void setFileNotFound(const QModelIndex &index, bool b);

    enum Roles { File = Qt::UserRole, Line, Description, FileNotFound, Type };
private:
    QList<TaskItem> m_items;
    int m_maxSizeOfFileName;
};

////
//  TaskView
////

TaskView::TaskView(QWidget *parent)
    : QListView(parent)
{
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

TaskView::~TaskView()
{

}

void TaskView::resizeEvent(QResizeEvent *e)
{
    Q_UNUSED(e);
    static_cast<TaskDelegate *>(itemDelegate())->emitSizeHintChanged(selectionModel()->currentIndex());
}

void TaskView::keyPressEvent(QKeyEvent *e)
{
    if (!e->modifiers() && e->key() == Qt::Key_Return) {
        emit activated(currentIndex());
        e->accept();
        return;
    }
    QListView::keyPressEvent(e);
}

/////
// TaskModel
/////

TaskModel::TaskModel()
{
    m_maxSizeOfFileName = 0;
}

void TaskModel::addTask(ProjectExplorer::BuildParserInterface::PatternType type, const QString &description, const QString &file, int line)
{
    TaskItem task;
    task.description = description;
    task.file = file;
    task.line = line;
    task.type = type;
    task.fileNotFound = false;

    beginInsertRows(QModelIndex(), m_items.size(), m_items.size());
    m_items.append(task);
    endInsertRows();

    QFont font;
    QFontMetrics fm(font);
    QString filename = task.file;
    int pos = filename.lastIndexOf("/");
    if (pos != -1)
        filename = file.mid(pos +1);
    m_maxSizeOfFileName = qMax(m_maxSizeOfFileName, fm.width(filename));
}

void TaskModel::clear()
{
    if (m_items.isEmpty())
        return;
    beginRemoveRows(QModelIndex(), 0, m_items.size() -1);
    m_items.clear();
    endRemoveRows();
    m_maxSizeOfFileName = 0;
}


QModelIndex TaskModel::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid())
        return QModelIndex();
    return createIndex(row, column, 0);
}

QModelIndex TaskModel::parent(const QModelIndex &child) const
{
    Q_UNUSED(child);
    return QModelIndex();
}

int TaskModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_items.count();
}

int TaskModel::columnCount(const QModelIndex &parent) const
{
        return parent.isValid() ? 0 : 1;
}

QVariant TaskModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_items.size() || index.column() != 0)
        return QVariant();

    if (role == TaskModel::File)
        return m_items.at(index.row()).file;
    else if (role == TaskModel::Line)
        return m_items.at(index.row()).line;
    else if (role == TaskModel::Description)
        return m_items.at(index.row()).description;
    else if (role == TaskModel::FileNotFound)
        return m_items.at(index.row()).fileNotFound;
    else if (role == TaskModel::Type)
        return (int)m_items.at(index.row()).type;
    else if (role == Qt::DecorationRole) {
         if (m_items.at(index.row()).type == ProjectExplorer::BuildParserInterface::Error) {
           return QIcon(":/projectexplorer/images/compile_error.png");
       } else if (m_items.at(index.row()).type == ProjectExplorer::BuildParserInterface::Warning) {
           return QIcon(":/projectexplorer/images/compile_warning.png");
       } else {
           return QIcon(":/projectexplorer/images/compile_unspecified.png");
       }
    }
    return QVariant();
}

int TaskModel::sizeOfFile()
{
    return m_maxSizeOfFileName;
}

int TaskModel::sizeOfLineNumber()
{
    QFont font;
    QFontMetrics fm(font);
    return fm.width("8888");
}

QModelIndex TaskModel::firstError() const
{
    int size = m_items.size();
    for (int i=0; i<size; ++i) {
        if (m_items.at(i).type == ProjectExplorer::BuildParserInterface::Error) {
            return index(i, 0);
        }
    }
    return QModelIndex();
}

void TaskModel::setFileNotFound(const QModelIndex &idx, bool b)
{
    if (idx.isValid() && idx.row() < m_items.size()) {
        m_items[idx.row()].fileNotFound = b;
        emit dataChanged(idx, idx);
    }
}

/////
// TaskWindow
/////

TaskWindow::TaskWindow()
{
    Core::ICore *core = Core::ICore::instance();

    m_model = new TaskModel;
    m_listview = new TaskView;

    m_listview->setModel(m_model);
    m_listview->setFrameStyle(QFrame::NoFrame);
    m_listview->setWindowTitle(tr("Build Issues"));
    m_listview->setSelectionMode(QAbstractItemView::SingleSelection);
    TaskDelegate *tld = new TaskDelegate(this);
    m_listview->setItemDelegate(tld);
    m_listview->setWindowIcon(QIcon(":/qt4projectmanager/images/window.png"));
    m_listview->setContextMenuPolicy(Qt::ActionsContextMenu);
    m_listview->setAttribute(Qt::WA_MacShowFocusRect, false);

    m_taskWindowContext = new TaskWindowContext(m_listview);
    core->addContextObject(m_taskWindowContext);

    m_copyAction = new QAction(QIcon(Core::Constants::ICON_COPY), tr("&Copy"), this);
    core->actionManager()->
            registerAction(m_copyAction, Core::Constants::COPY, m_taskWindowContext->context());
    m_listview->addAction(m_copyAction);

    connect(m_listview->selectionModel(), SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)),
            tld, SLOT(currentChanged(const QModelIndex &, const QModelIndex &)));

    connect(m_listview, SIGNAL(activated(const QModelIndex &)),
            this, SLOT(showTaskInFile(const QModelIndex &)));
    connect(m_listview, SIGNAL(clicked(const QModelIndex &)),
            this, SLOT(showTaskInFile(const QModelIndex &)));

    connect(m_copyAction, SIGNAL(triggered()), SLOT(copy()));

    m_errorCount = 0;
    m_currentTask = -1;
}

TaskWindow::~TaskWindow()
{
    Core::ICore::instance()->removeContextObject(m_taskWindowContext);
    delete m_listview;
    delete m_model;
}

QList<QWidget*> TaskWindow::toolBarWidgets() const
{
    return QList<QWidget*>();
}

QWidget *TaskWindow::outputWidget(QWidget *)
{
    return m_listview;
}

void TaskWindow::clearContents()
{
    m_errorCount = 0;
    m_currentTask = -1;
    m_model->clear();
    m_copyAction->setEnabled(false);
    emit tasksChanged();
}

void TaskWindow::visibilityChanged(bool /* b */)
{
}

void TaskWindow::addItem(ProjectExplorer::BuildParserInterface::PatternType type,
                         const QString &description, const QString &file, int line)
{
    m_model->addTask(type, description, file, line);
    if (type == ProjectExplorer::BuildParserInterface::Error)
        ++m_errorCount;
    m_copyAction->setEnabled(true);
    emit tasksChanged();
}

void TaskWindow::showTaskInFile(const QModelIndex &index)
{
    if (!index.isValid())
        return;
    QString file = index.data(TaskModel::File).toString();
    int line = index.data(TaskModel::Line).toInt();
    if (file.isEmpty() || line == -1)
        return;

    if (QFileInfo(file).exists()) {
        TextEditor::BaseTextEditor::openEditorAt(file, line);
        Core::EditorManager::instance()->ensureEditorManagerVisible();
    }
    else
        m_model->setFileNotFound(index, true);
    m_listview->selectionModel()->setCurrentIndex(index, QItemSelectionModel::Select);
    m_listview->selectionModel()->select(index, QItemSelectionModel::ClearAndSelect);
}

void TaskWindow::copy()
{
    QModelIndex index = m_listview->selectionModel()->currentIndex();
    QString file = index.data(TaskModel::File).toString();
    QString line = index.data(TaskModel::Line).toString();
    QString description = index.data(TaskModel::Description).toString();
    QString type;
    switch (index.data(TaskModel::Type).toInt()) {
    case ProjectExplorer::BuildParserInterface::Error:
        type = "error: ";
        break;
    case ProjectExplorer::BuildParserInterface::Warning:
        type = "warning: ";
        break;
    }

    QApplication::clipboard()->setText(file + ':' + line + ": " + type + description);
}

int TaskWindow::numberOfTasks() const
{
    return m_model->rowCount(QModelIndex());
}

int TaskWindow::numberOfErrors() const
{
    return m_errorCount;
}

int TaskWindow::priorityInStatusBar() const
{
    return 90;
}

void TaskWindow::gotoFirstError()
{
    QModelIndex idx = m_model->firstError();
    if (idx.isValid())
        showTaskInFile(idx);
}

bool TaskWindow::hasFocus()
{
    return m_listview->hasFocus();
}

bool TaskWindow::canFocus()
{
    return m_model->rowCount();
}

#include <QDebug>

void TaskWindow::setFocus()
{
    if (m_model->rowCount()) {
        m_listview->setFocus();
        if (m_listview->currentIndex() == QModelIndex()) {
            m_listview->setCurrentIndex(m_model->index(0,0, QModelIndex()));
        }
    }
}

/////
// Delegate
/////

TaskDelegate::TaskDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

TaskDelegate::~TaskDelegate()
{
}

QSize TaskDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItemV4 opt = option;
    initStyleOption(&opt, index);


    QFontMetrics fm(option.font);
    QSize s;
    s.setWidth(option.rect.width());
    const QAbstractItemView * view = qobject_cast<const QAbstractItemView *>(opt.widget);
    TaskModel *model = static_cast<TaskModel *>(view->model());
    int width = opt.rect.width() - model->sizeOfFile() - model->sizeOfLineNumber() - 12 - 22;
    if (view->selectionModel()->currentIndex() == index) {
        QString description = index.data(TaskModel::Description).toString();
        // Layout the description
        int leading = fm.leading();
        int height = 0;
        QTextLayout tl(description);
        tl.beginLayout();
        while (true) {
            QTextLine line = tl.createLine();
            if (!line.isValid())
                break;
            line.setLineWidth(width);
            height += leading;
            line.setPosition(QPoint(0, height));
            height += static_cast<int>(line.height());
        }
        tl.endLayout();

        s.setHeight(height + leading + fm.height() + 3);
    } else {
        s.setHeight(fm.height() + 3);
    }
    return s;
}

void TaskDelegate::emitSizeHintChanged(const QModelIndex &index)
{
    emit sizeHintChanged(index);
}

void TaskDelegate::currentChanged(const QModelIndex &current, const QModelIndex &previous)
{
    emit sizeHintChanged(current);
    emit sizeHintChanged(previous);
}

void TaskDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItemV4 opt = option;
    initStyleOption(&opt, index);
    painter->save();

    QFontMetrics fm(opt.font);
    QColor backgroundColor;
    QColor textColor;

    const QAbstractItemView * view = qobject_cast<const QAbstractItemView *>(opt.widget);
    bool selected = view->selectionModel()->currentIndex() == index;

    if (selected) {
        painter->setBrush(opt.palette.highlight().color());
        backgroundColor = opt.palette.highlight().color();
    } else {
        painter->setBrush(opt.palette.background().color());
        backgroundColor = opt.palette.background().color();
    }
    painter->setPen(Qt::NoPen);
    painter->drawRect(opt.rect);

    // Set Text Color
    if (selected)
        textColor = opt.palette.highlightedText().color();
    else
        textColor = opt.palette.text().color();

    painter->setPen(textColor);

    QIcon icon = index.data(Qt::DecorationRole).value<QIcon>();
    painter->drawPixmap(2, opt.rect.top() + 2, icon.pixmap(16, 16));

    TaskModel *model = static_cast<TaskModel *>(view->model());
    int width = opt.rect.width() - model->sizeOfFile() - model->sizeOfLineNumber() - 12 - 22;
    if (!selected) {
        // in small mode we lay out differently
        QString bottom = index.data(TaskModel::Description).toString();
        painter->drawText(22, 2 + opt.rect.top() + fm.ascent(), bottom);
        if (fm.width(bottom) > width) {
            // draw a gradient to mask the text
            int gwidth = opt.rect.right() - width;
            QLinearGradient lg(QPoint(width, 0), QPoint(width+gwidth, 0));
            QColor c = backgroundColor;
            c.setAlpha(0);
            lg.setColorAt(0, c);
            lg.setColorAt(20.0/gwidth, backgroundColor);
            painter->fillRect(width, 2 + opt.rect.top(), gwidth, fm.height() + 1, lg);
        }
    } else {
        // Description
        QString description = index.data(TaskModel::Description).toString();
        // Layout the description
        int leading = fm.leading();
        int height = 0;
        QTextLayout tl(description);
        tl.beginLayout();
        while (true) {
            QTextLine line = tl.createLine();
            if (!line.isValid())
                break;
            line.setLineWidth(width);
            height += leading;
            line.setPosition(QPoint(0, height));
            height += static_cast<int>(line.height());
        }
        tl.endLayout();
        tl.draw(painter, QPoint(22, 2 + opt.rect.top()));
        //painter->drawText(22, 2 + opt.rect.top() + fm.ascent(), description);

        QColor mix;
        mix.setRgb( static_cast<int>(0.7 * textColor.red()   + 0.3 * backgroundColor.red()),
                static_cast<int>(0.7 * textColor.green() + 0.3 * backgroundColor.green()),
                static_cast<int>(0.7 * textColor.blue()  + 0.3 * backgroundColor.blue()));
        painter->setPen(mix);

        QString directory = index.data(TaskModel::File).toString();
        int secondBaseLine = 2 + fm.ascent() + opt.rect.top() + height + leading; //opt.rect.top() + fm.ascent() + fm.height() + 6;
        if (index.data(TaskModel::FileNotFound).toBool()) {
            QString fileNotFound = tr("File not found: %1").arg(directory);
            painter->setPen(Qt::red);
            painter->drawText(22, secondBaseLine, fileNotFound);
        } else {
            painter->drawText(22, secondBaseLine, directory);
        }
    }

    painter->setPen(textColor);
    // Assemble string for the right side
    // just filename + linenumer
    QString file = index.data(TaskModel::File).toString();
    int pos = file.lastIndexOf("/");
    if (pos != -1)
        file = file.mid(pos +1);
    painter->drawText(width + 22 + 4, 2 + opt.rect.top() + fm.ascent(), file);

    QString topRight = index.data(TaskModel::Line).toString();
    painter->drawText(opt.rect.right() - fm.width(topRight) - 6 , 2 + opt.rect.top() + fm.ascent(), topRight);
    // Separator lines
    painter->setPen(QColor::fromRgb(150,150,150));
    painter->drawLine(0, opt.rect.bottom(), opt.rect.right(), opt.rect.bottom());
    painter->restore();
}

TaskWindowContext::TaskWindowContext(QWidget *widget)
    : Core::IContext(widget), m_taskList(widget)
{
    Core::UniqueIDManager *uidm = Core::UniqueIDManager::instance();
    m_context << uidm->uniqueIdentifier(Core::Constants::C_PROBLEM_PANE);
}

QList<int> TaskWindowContext::context() const
{
    return m_context;
}

QWidget *TaskWindowContext::widget()
{
    return m_taskList;
}

