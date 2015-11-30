/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
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
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "cppcodemodelinspectordialog.h"
#include "ui_cppcodemodelinspectordialog.h"
#include "cppeditor.h"
#include "cppeditordocument.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <cpptools/baseeditordocumentprocessor.h>
#include <cpptools/cppcodemodelinspectordumper.h>
#include <cpptools/cppmodelmanager.h>
#include <cpptools/cppworkingcopy.h>
#include <projectexplorer/project.h>

#include <cplusplus/CppDocument.h>
#include <cplusplus/Overview.h>
#include <cplusplus/Token.h>
#include <utils/qtcassert.h>

#include <QAbstractTableModel>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSortFilterProxyModel>

using namespace CPlusPlus;
using namespace CppTools;
namespace CMI = CppCodeModelInspector;

namespace {

template <class T> void resizeColumns(QTreeView *view)
{
    for (int column = 0; column < T::ColumnCount - 1; ++column)
        view->resizeColumnToContents(column);
}

TextEditor::BaseTextEditor *currentEditor()
{
    return qobject_cast<TextEditor::BaseTextEditor*>(Core::EditorManager::currentEditor());
}

QString fileInCurrentEditor()
{
    if (TextEditor::BaseTextEditor *editor = currentEditor())
        return editor->document()->filePath().toString();
    return QString();
}

class DepthFinder : public SymbolVisitor {
public:
    DepthFinder() : m_symbol(0), m_depth(-1), m_foundDepth(-1), m_stop(false) {}

    int operator()(const Document::Ptr &document, Symbol *symbol)
    {
        m_symbol = symbol;
        accept(document->globalNamespace());
        return m_foundDepth;
    }

    bool preVisit(Symbol *symbol)
    {
        if (m_stop)
            return false;

        if (symbol->asScope()) {
            ++m_depth;
            if (symbol == m_symbol) {
                m_foundDepth = m_depth;
                m_stop = true;
            }
            return true;
        }

        return false;
    }

    void postVisit(Symbol *symbol)
    {
        if (symbol->asScope())
            --m_depth;
    }

private:
    Symbol *m_symbol;
    int m_depth;
    int m_foundDepth;
    bool m_stop;
};

} // anonymous namespace

namespace CppEditor {
namespace Internal {

// --- FilterableView -----------------------------------------------------------------------------

class FilterableView : public QWidget
{
    Q_OBJECT
public:
    FilterableView(QWidget *parent);

    void setModel(QAbstractItemModel *model);
    QItemSelectionModel *selectionModel() const;
    void selectIndex(const QModelIndex &index);
    void resizeColumns(int columnCount);

signals:
    void filterChanged(const QString &filterText);

public slots:
    void clearFilter();

private:
    QTreeView *view;
    QLineEdit *lineEdit;
};

FilterableView::FilterableView(QWidget *parent)
    : QWidget(parent)
{
    view = new QTreeView(this);
    view->setAlternatingRowColors(true);
    view->setTextElideMode(Qt::ElideMiddle);
    view->setSortingEnabled(true);

    lineEdit = new QLineEdit(this);
    lineEdit->setPlaceholderText(QLatin1String("File Path"));
    QObject::connect(lineEdit, SIGNAL(textChanged(QString)), SIGNAL(filterChanged(QString)));

    QLabel *label = new QLabel(QLatin1String("&Filter:"), this);
    label->setBuddy(lineEdit);

    QPushButton *clearButton = new QPushButton(QLatin1String("&Clear"), this);
    QObject::connect(clearButton, SIGNAL(clicked()), SLOT(clearFilter()));

    QHBoxLayout *filterBarLayout = new QHBoxLayout();
    filterBarLayout->addWidget(label);
    filterBarLayout->addWidget(lineEdit);
    filterBarLayout->addWidget(clearButton);

    QVBoxLayout *mainLayout = new QVBoxLayout();
    mainLayout->addWidget(view);
    mainLayout->addLayout(filterBarLayout);

    setLayout(mainLayout);
}

void FilterableView::setModel(QAbstractItemModel *model)
{
    view->setModel(model);
}

QItemSelectionModel *FilterableView::selectionModel() const
{
    return view->selectionModel();
}

void FilterableView::selectIndex(const QModelIndex &index)
{
    if (index.isValid())  {
        view->selectionModel()->setCurrentIndex(index,
            QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    }
}

void FilterableView::resizeColumns(int columnCount)
{
    for (int column = 0; column < columnCount - 1; ++column)
        view->resizeColumnToContents(column);
}

void FilterableView::clearFilter()
{
    lineEdit->clear();
}

// --- KeyValueModel ------------------------------------------------------------------------------

class KeyValueModel : public QAbstractListModel
{
    Q_OBJECT
public:
    typedef QList<QPair<QString, QString> > Table;

    KeyValueModel(QObject *parent);
    void configure(const Table &table);
    void clear();

    enum Columns { KeyColumn, ValueColumn, ColumnCount };

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;

private:
    Table m_table;
};

KeyValueModel::KeyValueModel(QObject *parent) : QAbstractListModel(parent)
{
}

void KeyValueModel::configure(const Table &table)
{
    emit layoutAboutToBeChanged();
    m_table = table;
    emit layoutChanged();
}

void KeyValueModel::clear()
{
    emit layoutAboutToBeChanged();
    m_table.clear();
    emit layoutChanged();
}

int KeyValueModel::rowCount(const QModelIndex &/*parent*/) const
{
    return m_table.size();
}

int KeyValueModel::columnCount(const QModelIndex &/*parent*/) const
{
    return KeyValueModel::ColumnCount;
}

QVariant KeyValueModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DisplayRole) {
        const int row = index.row();
        const int column = index.column();
        if (column == KeyColumn) {
            return m_table.at(row).first;
        } else if (column == ValueColumn) {
            return m_table.at(row).second;
        }
    }
    return QVariant();
}

QVariant KeyValueModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
        case KeyColumn:
            return QLatin1String("Key");
        case ValueColumn:
            return QLatin1String("Value");
        default:
            return QVariant();
        }
    }
    return QVariant();
}

// --- SnapshotModel ------------------------------------------------------------------------------

class SnapshotModel : public QAbstractListModel
{
    Q_OBJECT
public:
    SnapshotModel(QObject *parent);
    void configure(const Snapshot &snapshot);
    void setGlobalSnapshot(const Snapshot &snapshot);

    QModelIndex indexForDocument(const QString &filePath);

    enum Columns { SymbolCountColumn, SharedColumn, FilePathColumn, ColumnCount };

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;

private:
    QList<Document::Ptr> m_documents;
    Snapshot m_globalSnapshot;
};

SnapshotModel::SnapshotModel(QObject *parent) : QAbstractListModel(parent)
{
}

void SnapshotModel::configure(const Snapshot &snapshot)
{
    emit layoutAboutToBeChanged();
    m_documents = CMI::Utils::snapshotToList(snapshot);
    emit layoutChanged();
}

void SnapshotModel::setGlobalSnapshot(const Snapshot &snapshot)
{
    m_globalSnapshot = snapshot;
}

QModelIndex SnapshotModel::indexForDocument(const QString &filePath)
{
    for (int i = 0, total = m_documents.size(); i < total; ++i) {
        const Document::Ptr document = m_documents.at(i);
        if (document->fileName() == filePath)
            return index(i, FilePathColumn);
    }
    return QModelIndex();
}

int SnapshotModel::rowCount(const QModelIndex &/*parent*/) const
{
    return m_documents.size();
}

int SnapshotModel::columnCount(const QModelIndex &/*parent*/) const
{
    return SnapshotModel::ColumnCount;
}

QVariant SnapshotModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DisplayRole) {
        const int column = index.column();
        Document::Ptr document = m_documents.at(index.row());
        if (column == SymbolCountColumn) {
            return document->control()->symbolCount();
        } else if (column == SharedColumn) {
            Document::Ptr globalDocument = m_globalSnapshot.document(document->fileName());
            const bool isShared
                = globalDocument && globalDocument->fingerprint() == document->fingerprint();
            return CMI::Utils::toString(isShared);
        } else if (column == FilePathColumn) {
            return QDir::toNativeSeparators(document->fileName());
        }
    }
    return QVariant();
}

QVariant SnapshotModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
        case SymbolCountColumn:
            return QLatin1String("Symbols");
        case SharedColumn:
            return QLatin1String("Shared");
        case FilePathColumn:
            return QLatin1String("File Path");
        default:
            return QVariant();
        }
    }
    return QVariant();
}

// --- IncludesModel ------------------------------------------------------------------------------

static bool includesSorter(const Document::Include &i1,
                           const Document::Include &i2)
{
    return i1.line() < i2.line();
}

class IncludesModel : public QAbstractListModel
{
    Q_OBJECT
public:
    IncludesModel(QObject *parent);
    void configure(const QList<Document::Include> &includes);
    void clear();

    enum Columns { ResolvedOrNotColumn, LineNumberColumn, FilePathsColumn, ColumnCount };

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;

private:
    QList<Document::Include> m_includes;
};

IncludesModel::IncludesModel(QObject *parent) : QAbstractListModel(parent)
{
}

void IncludesModel::configure(const QList<Document::Include> &includes)
{
    emit layoutAboutToBeChanged();
    m_includes = includes;
    qStableSort(m_includes.begin(), m_includes.end(), includesSorter);
    emit layoutChanged();
}

void IncludesModel::clear()
{
    emit layoutAboutToBeChanged();
    m_includes.clear();
    emit layoutChanged();
}

int IncludesModel::rowCount(const QModelIndex &/*parent*/) const
{
    return m_includes.size();
}

int IncludesModel::columnCount(const QModelIndex &/*parent*/) const
{
    return IncludesModel::ColumnCount;
}

QVariant IncludesModel::data(const QModelIndex &index, int role) const
{
    if (role != Qt::DisplayRole && role != Qt::ForegroundRole)
        return QVariant();

    static const QBrush greenBrush(QColor(0, 139, 69));
    static const QBrush redBrush(QColor(205, 38, 38));

    const Document::Include include = m_includes.at(index.row());
    const QString resolvedFileName = QDir::toNativeSeparators(include.resolvedFileName());
    const bool isResolved = !resolvedFileName.isEmpty();

    if (role == Qt::DisplayRole) {
        const int column = index.column();
        if (column == ResolvedOrNotColumn) {
            return CMI::Utils::toString(isResolved);
        } else if (column == LineNumberColumn) {
            return include.line();
        } else if (column == FilePathsColumn) {
            return QVariant(CMI::Utils::unresolvedFileNameWithDelimiters(include)
                            + QLatin1String(" --> ") + resolvedFileName);
        }
    } else if (role == Qt::ForegroundRole) {
        return isResolved ? greenBrush : redBrush;
    }

    return QVariant();
}

QVariant IncludesModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
        case ResolvedOrNotColumn:
            return QLatin1String("Resolved");
        case LineNumberColumn:
            return QLatin1String("Line");
        case FilePathsColumn:
            return QLatin1String("File Paths");
        default:
            return QVariant();
        }
    }
    return QVariant();
}

// --- DiagnosticMessagesModel --------------------------------------------------------------------

static bool diagnosticMessagesModelSorter(const Document::DiagnosticMessage &m1,
                                          const Document::DiagnosticMessage &m2)
{
    return m1.line() < m2.line();
}

class DiagnosticMessagesModel : public QAbstractListModel
{
    Q_OBJECT
public:
    DiagnosticMessagesModel(QObject *parent);
    void configure(const QList<Document::DiagnosticMessage> &messages);
    void clear();

    enum Columns { LevelColumn, LineColumnNumberColumn, MessageColumn, ColumnCount };

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;

private:
    QList<Document::DiagnosticMessage> m_messages;
};

DiagnosticMessagesModel::DiagnosticMessagesModel(QObject *parent) : QAbstractListModel(parent)
{
}

void DiagnosticMessagesModel::configure(
        const QList<Document::DiagnosticMessage> &messages)
{
    emit layoutAboutToBeChanged();
    m_messages = messages;
    qStableSort(m_messages.begin(), m_messages.end(), diagnosticMessagesModelSorter);
    emit layoutChanged();
}

void DiagnosticMessagesModel::clear()
{
    emit layoutAboutToBeChanged();
    m_messages.clear();
    emit layoutChanged();
}

int DiagnosticMessagesModel::rowCount(const QModelIndex &/*parent*/) const
{
    return m_messages.size();
}

int DiagnosticMessagesModel::columnCount(const QModelIndex &/*parent*/) const
{
    return DiagnosticMessagesModel::ColumnCount;
}

QVariant DiagnosticMessagesModel::data(const QModelIndex &index, int role) const
{
    if (role != Qt::DisplayRole && role != Qt::ForegroundRole)
        return QVariant();

    static const QBrush yellowOrangeBrush(QColor(237, 145, 33));
    static const QBrush redBrush(QColor(205, 38, 38));
    static const QBrush darkRedBrushQColor(QColor(139, 0, 0));

    const Document::DiagnosticMessage message = m_messages.at(index.row());
    const Document::DiagnosticMessage::Level level
        = static_cast<Document::DiagnosticMessage::Level>(message.level());

    if (role == Qt::DisplayRole) {
        const int column = index.column();
        if (column == LevelColumn) {
            return CMI::Utils::toString(level);
        } else if (column == LineColumnNumberColumn) {
            return QVariant(QString::number(message.line()) + QLatin1Char(':')
                            + QString::number(message.column()));
        } else if (column == MessageColumn) {
            return message.text();
        }
    } else if (role == Qt::ForegroundRole) {
        switch (level) {
        case Document::DiagnosticMessage::Warning:
            return yellowOrangeBrush;
        case Document::DiagnosticMessage::Error:
            return redBrush;
        case Document::DiagnosticMessage::Fatal:
            return darkRedBrushQColor;
        default:
            return QVariant();
        }
    }

    return QVariant();
}

QVariant DiagnosticMessagesModel::headerData(int section, Qt::Orientation orientation, int role)
    const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
        case LevelColumn:
            return QLatin1String("Level");
        case LineColumnNumberColumn:
            return QLatin1String("Line:Column");
        case MessageColumn:
            return QLatin1String("Message");
        default:
            return QVariant();
        }
    }
    return QVariant();
}

// --- MacrosModel --------------------------------------------------------------------------------

class MacrosModel : public QAbstractListModel
{
    Q_OBJECT
public:
    MacrosModel(QObject *parent);
    void configure(const QList<Macro> &macros);
    void clear();

    enum Columns { LineNumberColumn, MacroColumn, ColumnCount };

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;

private:
    QList<Macro> m_macros;
};

MacrosModel::MacrosModel(QObject *parent) : QAbstractListModel(parent)
{
}

void MacrosModel::configure(const QList<Macro> &macros)
{
    emit layoutAboutToBeChanged();
    m_macros = macros;
    emit layoutChanged();
}

void MacrosModel::clear()
{
    emit layoutAboutToBeChanged();
    m_macros.clear();
    emit layoutChanged();
}

int MacrosModel::rowCount(const QModelIndex &/*parent*/) const
{
    return m_macros.size();
}

int MacrosModel::columnCount(const QModelIndex &/*parent*/) const
{
    return MacrosModel::ColumnCount;
}

QVariant MacrosModel::data(const QModelIndex &index, int role) const
{
    const int column = index.column();
    if (role == Qt::DisplayRole || (role == Qt::ToolTipRole && column == MacroColumn)) {
        const Macro macro = m_macros.at(index.row());
        if (column == LineNumberColumn)
            return macro.line();
        else if (column == MacroColumn)
            return macro.toString();
    } else if (role == Qt::TextAlignmentRole) {
        return Qt::AlignTop + Qt::AlignLeft;
    }
    return QVariant();
}

QVariant MacrosModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
        case LineNumberColumn:
            return QLatin1String("Line");
        case MacroColumn:
            return QLatin1String("Macro");
        default:
            return QVariant();
        }
    }
    return QVariant();
}

// --- SymbolsModel -------------------------------------------------------------------------------

class SymbolsModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    SymbolsModel(QObject *parent);
    void configure(const Document::Ptr &document);
    void clear();

    enum Columns { SymbolColumn, LineNumberColumn, ColumnCount };

    QModelIndex index(int row, int column, const QModelIndex &parent) const;
    QModelIndex parent(const QModelIndex &child) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;

private:
    Document::Ptr m_document;
};

SymbolsModel::SymbolsModel(QObject *parent) : QAbstractItemModel(parent)
{
}

void SymbolsModel::configure(const Document::Ptr &document)
{
    QTC_CHECK(document);
    emit layoutAboutToBeChanged();
    m_document = document;
    emit layoutChanged();
}

void SymbolsModel::clear()
{
    emit layoutAboutToBeChanged();
    m_document.clear();
    emit layoutChanged();
}

static Symbol *indexToSymbol(const QModelIndex &index)
{
    if (Symbol *symbol = static_cast<Symbol*>(index.internalPointer()))
        return symbol;
    return 0;
}

static Scope *indexToScope(const QModelIndex &index)
{
    if (Symbol *symbol = indexToSymbol(index))
        return symbol->asScope();
    return 0;
}

QModelIndex SymbolsModel::index(int row, int column, const QModelIndex &parent) const
{
    Scope *scope = 0;
    if (parent.isValid())
        scope = indexToScope(parent);
    else if (m_document)
        scope = m_document->globalNamespace();

    if (scope) {
        if ((unsigned)row < scope->memberCount())
            return createIndex(row, column, scope->memberAt(row));
    }

    return QModelIndex();
}

QModelIndex SymbolsModel::parent(const QModelIndex &child) const
{
    if (!child.isValid())
        return QModelIndex();

    if (Symbol *symbol = indexToSymbol(child)) {
        if (Scope *scope = symbol->enclosingScope()) {
            const int row = DepthFinder()(m_document, scope);
            return createIndex(row, 0, scope);
        }
    }

    return QModelIndex();
}

int SymbolsModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        if (Scope *scope = indexToScope(parent))
            return scope->memberCount();
    } else {
        if (m_document)
            return m_document->globalNamespace()->memberCount();
    }
    return 0;
}

int SymbolsModel::columnCount(const QModelIndex &) const
{
    return ColumnCount;
}

QVariant SymbolsModel::data(const QModelIndex &index, int role) const
{
    const int column = index.column();
    if (role == Qt::DisplayRole) {
        Symbol *symbol = indexToSymbol(index);
        if (!symbol)
            return QVariant();
        if (column == LineNumberColumn) {
            return symbol->line();
        } else if (column == SymbolColumn) {
            QString name = Overview().prettyName(symbol->name());
            if (name.isEmpty())
                name = QLatin1String(symbol->isBlock() ? "<block>" : "<no name>");
            return name;
        }
    }
    return QVariant();
}

QVariant SymbolsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
        case SymbolColumn:
            return QLatin1String("Symbol");
        case LineNumberColumn:
            return QLatin1String("Line");
        default:
            return QVariant();
        }
    }
    return QVariant();
}

// --- TokensModel --------------------------------------------------------------------------------

class TokensModel : public QAbstractListModel
{
    Q_OBJECT
public:
    TokensModel(QObject *parent);
    void configure(TranslationUnit *translationUnit);
    void clear();

    enum Columns { SpelledColumn, KindColumn, IndexColumn, OffsetColumn, LineColumnNumberColumn,
                   BytesAndCodePointsColumn, GeneratedColumn, ExpandedColumn, WhiteSpaceColumn,
                   NewlineColumn, ColumnCount };

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;

private:
    struct TokenInfo {
        Token token;
        unsigned line;
        unsigned column;
    };
    QList<TokenInfo> m_tokenInfos;
};

TokensModel::TokensModel(QObject *parent) : QAbstractListModel(parent)
{
}

void TokensModel::configure(TranslationUnit *translationUnit)
{
    if (!translationUnit)
        return;

    emit layoutAboutToBeChanged();
    m_tokenInfos.clear();
    for (int i = 0, total = translationUnit->tokenCount(); i < total; ++i) {
        TokenInfo info;
        info.token = translationUnit->tokenAt(i);
        translationUnit->getPosition(info.token.utf16charsBegin(), &info.line, &info.column);
        m_tokenInfos.append(info);
    }
    emit layoutChanged();
}

void TokensModel::clear()
{
    emit layoutAboutToBeChanged();
    m_tokenInfos.clear();
    emit layoutChanged();
}

int TokensModel::rowCount(const QModelIndex &/*parent*/) const
{
    return m_tokenInfos.size();
}

int TokensModel::columnCount(const QModelIndex &/*parent*/) const
{
    return TokensModel::ColumnCount;
}

QVariant TokensModel::data(const QModelIndex &index, int role) const
{
    const int column = index.column();
    if (role == Qt::DisplayRole) {
        const TokenInfo info = m_tokenInfos.at(index.row());
        const Token token = info.token;
        if (column == SpelledColumn)
            return QString::fromUtf8(token.spell());
        else if (column == KindColumn)
            return CMI::Utils::toString(static_cast<Kind>(token.kind()));
        else if (column == IndexColumn)
            return index.row();
        else if (column == OffsetColumn)
            return token.bytesBegin();
        else if (column == LineColumnNumberColumn)
            return QString::fromLatin1("%1:%2").arg(CMI::Utils::toString(info.line),
                                                    CMI::Utils::toString(info.column));
        else if (column == BytesAndCodePointsColumn)
            return QString::fromLatin1("%1/%2").arg(CMI::Utils::toString(token.bytes()),
                                                    CMI::Utils::toString(token.utf16chars()));
        else if (column == GeneratedColumn)
            return CMI::Utils::toString(token.generated());
        else if (column == ExpandedColumn)
            return CMI::Utils::toString(token.expanded());
        else if (column == WhiteSpaceColumn)
            return CMI::Utils::toString(token.whitespace());
        else if (column == NewlineColumn)
            return CMI::Utils::toString(token.newline());
    } else if (role == Qt::TextAlignmentRole) {
        return Qt::AlignTop + Qt::AlignLeft;
    }
    return QVariant();
}

QVariant TokensModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
        case SpelledColumn:
            return QLatin1String("Spelled");
        case KindColumn:
            return QLatin1String("Kind");
        case IndexColumn:
            return QLatin1String("Index");
        case OffsetColumn:
            return QLatin1String("Offset");
        case LineColumnNumberColumn:
            return QLatin1String("Line:Column");
        case BytesAndCodePointsColumn:
            return QLatin1String("Bytes/Codepoints");
        case GeneratedColumn:
            return QLatin1String("Generated");
        case ExpandedColumn:
            return QLatin1String("Expanded");
        case WhiteSpaceColumn:
            return QLatin1String("Whitespace");
        case NewlineColumn:
            return QLatin1String("Newline");
        default:
            return QVariant();
        }
    }
    return QVariant();
}

// --- ProjectPartsModel --------------------------------------------------------------------------

class ProjectPartsModel : public QAbstractListModel
{
    Q_OBJECT
public:
    ProjectPartsModel(QObject *parent);

    void configure(const QList<ProjectInfo> &projectInfos,
                   const ProjectPart::Ptr &currentEditorsProjectPart);

    QModelIndex indexForCurrentEditorsProjectPart() const;
    ProjectPart::Ptr projectPartForProjectId(const QString &projectPartId) const;

    enum Columns { PartNameColumn, PartFilePathColumn, ColumnCount };

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;

private:
    QList<ProjectPart::Ptr> m_projectPartsList;
    int m_currentEditorsProjectPartIndex;
};

ProjectPartsModel::ProjectPartsModel(QObject *parent)
    : QAbstractListModel(parent), m_currentEditorsProjectPartIndex(-1)
{
}

void ProjectPartsModel::configure(const QList<ProjectInfo> &projectInfos,
                                  const ProjectPart::Ptr &currentEditorsProjectPart)
{
    emit layoutAboutToBeChanged();
    m_projectPartsList.clear();
    foreach (const ProjectInfo &info, projectInfos) {
        foreach (const ProjectPart::Ptr &projectPart, info.projectParts()) {
            if (!m_projectPartsList.contains(projectPart)) {
                m_projectPartsList << projectPart;
                if (projectPart == currentEditorsProjectPart)
                    m_currentEditorsProjectPartIndex = m_projectPartsList.size() - 1;
            }
        }
    }
    emit layoutChanged();
}

QModelIndex ProjectPartsModel::indexForCurrentEditorsProjectPart() const
{
    if (m_currentEditorsProjectPartIndex == -1)
        return QModelIndex();
    return createIndex(m_currentEditorsProjectPartIndex, PartFilePathColumn);
}

ProjectPart::Ptr ProjectPartsModel::projectPartForProjectId(const QString &projectPartId) const
{
    foreach (const ProjectPart::Ptr &part, m_projectPartsList) {
        if (part->id() == projectPartId)
            return part;
    }
    return ProjectPart::Ptr();
}

int ProjectPartsModel::rowCount(const QModelIndex &/*parent*/) const
{
    return m_projectPartsList.size();
}

int ProjectPartsModel::columnCount(const QModelIndex &/*parent*/) const
{
    return ProjectPartsModel::ColumnCount;
}

QVariant ProjectPartsModel::data(const QModelIndex &index, int role) const
{
    const int row = index.row();
    if (role == Qt::DisplayRole) {
        const int column = index.column();
        if (column == PartNameColumn)
            return m_projectPartsList.at(row)->displayName;
        else if (column == PartFilePathColumn)
            return QDir::toNativeSeparators(m_projectPartsList.at(row)->projectFile);
    } else if (role == Qt::UserRole) {
        return m_projectPartsList.at(row)->id();
    }
    return QVariant();
}

QVariant ProjectPartsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
        case PartNameColumn:
            return QLatin1String("Name");
        case PartFilePathColumn:
            return QLatin1String("Project File Path");
        default:
            return QVariant();
        }
    }
    return QVariant();
}

// --- WorkingCopyModel ---------------------------------------------------------------------------

class WorkingCopyModel : public QAbstractListModel
{
    Q_OBJECT
public:
    WorkingCopyModel(QObject *parent);

    void configure(const WorkingCopy &workingCopy);
    QModelIndex indexForFile(const QString &filePath);

    enum Columns { RevisionColumn, FilePathColumn, ColumnCount };

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;

private:
    struct WorkingCopyEntry {
        WorkingCopyEntry(const QString &filePath, const QByteArray &source, unsigned revision)
            : filePath(filePath), source(source), revision(revision)
        {}

        QString filePath;
        QByteArray source;
        unsigned revision;
    };

    QList<WorkingCopyEntry> m_workingCopyList;
};

WorkingCopyModel::WorkingCopyModel(QObject *parent) : QAbstractListModel(parent)
{
}

void WorkingCopyModel::configure(const WorkingCopy &workingCopy)
{
    emit layoutAboutToBeChanged();
    m_workingCopyList.clear();
    QHashIterator<Utils::FileName, QPair<QByteArray, unsigned> > it = workingCopy.iterator();
    while (it.hasNext()) {
        it.next();
        m_workingCopyList << WorkingCopyEntry(it.key().toString(), it.value().first,
                                              it.value().second);
    }
    emit layoutChanged();
}

QModelIndex WorkingCopyModel::indexForFile(const QString &filePath)
{
    for (int i = 0, total = m_workingCopyList.size(); i < total; ++i) {
        const WorkingCopyEntry entry = m_workingCopyList.at(i);
        if (entry.filePath == filePath)
            return index(i, FilePathColumn);
    }
    return QModelIndex();
}

int WorkingCopyModel::rowCount(const QModelIndex &/*parent*/) const
{
    return m_workingCopyList.size();
}

int WorkingCopyModel::columnCount(const QModelIndex &/*parent*/) const
{
    return WorkingCopyModel::ColumnCount;
}

QVariant WorkingCopyModel::data(const QModelIndex &index, int role) const
{
    const int row = index.row();
    if (role == Qt::DisplayRole) {
        const int column = index.column();
        if (column == RevisionColumn)
            return m_workingCopyList.at(row).revision;
        else if (column == FilePathColumn)
            return m_workingCopyList.at(row).filePath;
    } else if (role == Qt::UserRole) {
        return m_workingCopyList.at(row).source;
    }
    return QVariant();
}

QVariant WorkingCopyModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
        case RevisionColumn:
            return QLatin1String("Revision");
        case FilePathColumn:
            return QLatin1String("File Path");
        default:
            return QVariant();
        }
    }
    return QVariant();
}

// --- SnapshotInfo -------------------------------------------------------------------------------

class SnapshotInfo
{
public:
    enum Type { GlobalSnapshot, EditorSnapshot };
    SnapshotInfo(const Snapshot &snapshot, Type type)
        : snapshot(snapshot), type(type) {}

    Snapshot snapshot;
    Type type;
};

// --- CppCodeModelInspectorDialog ----------------------------------------------------------------

CppCodeModelInspectorDialog::CppCodeModelInspectorDialog(QWidget *parent)
    : QDialog(parent)
    , m_ui(new Ui::CppCodeModelInspectorDialog)
    , m_snapshotInfos(new QList<SnapshotInfo>())
    , m_snapshotView(new FilterableView(this))
    , m_snapshotModel(new SnapshotModel(this))
    , m_proxySnapshotModel(new QSortFilterProxyModel(this))
    , m_docGenericInfoModel(new KeyValueModel(this))
    , m_docIncludesModel(new IncludesModel(this))
    , m_docDiagnosticMessagesModel(new DiagnosticMessagesModel(this))
    , m_docMacrosModel(new MacrosModel(this))
    , m_docSymbolsModel(new SymbolsModel(this))
    , m_docTokensModel(new TokensModel(this))
    , m_projectPartsView(new FilterableView(this))
    , m_projectPartsModel(new ProjectPartsModel(this))
    , m_proxyProjectPartsModel(new QSortFilterProxyModel(this))
    , m_partGenericInfoModel(new KeyValueModel(this))
    , m_workingCopyView(new FilterableView(this))
    , m_workingCopyModel(new WorkingCopyModel(this))
    , m_proxyWorkingCopyModel(new QSortFilterProxyModel(this))
{
    m_ui->setupUi(this);
    m_ui->snapshotSelectorAndViewLayout->addWidget(m_snapshotView);
    m_ui->projectPartsSplitter->insertWidget(0, m_projectPartsView);
    m_ui->workingCopySplitter->insertWidget(0, m_workingCopyView);

    setAttribute(Qt::WA_DeleteOnClose);
    connect(Core::ICore::instance(), SIGNAL(coreAboutToClose()), SLOT(close()));

    m_proxySnapshotModel->setSourceModel(m_snapshotModel);
    m_proxySnapshotModel->setFilterKeyColumn(SnapshotModel::FilePathColumn);
    m_snapshotView->setModel(m_proxySnapshotModel);
    m_ui->docGeneralView->setModel(m_docGenericInfoModel);
    m_ui->docIncludesView->setModel(m_docIncludesModel);
    m_ui->docDiagnosticMessagesView->setModel(m_docDiagnosticMessagesModel);
    m_ui->docDefinedMacrosView->setModel(m_docMacrosModel);
    m_ui->docSymbolsView->setModel(m_docSymbolsModel);
    m_ui->docTokensView->setModel(m_docTokensModel);

    m_proxyProjectPartsModel->setSourceModel(m_projectPartsModel);
    m_proxyProjectPartsModel->setFilterKeyColumn(ProjectPartsModel::PartFilePathColumn);
    m_projectPartsView->setModel(m_proxyProjectPartsModel);
    m_ui->partGeneralView->setModel(m_partGenericInfoModel);

    m_proxyWorkingCopyModel->setSourceModel(m_workingCopyModel);
    m_proxyWorkingCopyModel->setFilterKeyColumn(WorkingCopyModel::FilePathColumn);
    m_workingCopyView->setModel(m_proxyWorkingCopyModel);

    connect(m_snapshotView->selectionModel(),
            SIGNAL(currentRowChanged(QModelIndex,QModelIndex)),
            SLOT(onDocumentSelected(QModelIndex,QModelIndex)));
    connect(m_snapshotView, SIGNAL(filterChanged(QString)),
            SLOT(onSnapshotFilterChanged(QString)));
    connect(m_ui->snapshotSelector, SIGNAL(currentIndexChanged(int)),
            SLOT(onSnapshotSelected(int)));
    connect(m_ui->docSymbolsView, SIGNAL(expanded(QModelIndex)),
            SLOT(onSymbolsViewExpandedOrCollapsed(QModelIndex)));
    connect(m_ui->docSymbolsView, SIGNAL(collapsed(QModelIndex)),
            SLOT(onSymbolsViewExpandedOrCollapsed(QModelIndex)));

    connect(m_projectPartsView->selectionModel(),
            SIGNAL(currentRowChanged(QModelIndex,QModelIndex)),
            SLOT(onProjectPartSelected(QModelIndex,QModelIndex)));
    connect(m_projectPartsView, SIGNAL(filterChanged(QString)),
            SLOT(onProjectPartFilterChanged(QString)));

    connect(m_workingCopyView->selectionModel(),
            SIGNAL(currentRowChanged(QModelIndex,QModelIndex)),
            SLOT(onWorkingCopyDocumentSelected(QModelIndex,QModelIndex)));
    connect(m_workingCopyView, SIGNAL(filterChanged(QString)),
            SLOT(onWorkingCopyFilterChanged(QString)));

    connect(m_ui->refreshButton, SIGNAL(clicked()), SLOT(onRefreshRequested()));
    connect(m_ui->closeButton, SIGNAL(clicked()), SLOT(close()));

    refresh();
}

CppCodeModelInspectorDialog::~CppCodeModelInspectorDialog()
{
    delete m_snapshotInfos;
    delete m_ui;
}

void CppCodeModelInspectorDialog::onRefreshRequested()
{
    refresh();
}

void CppCodeModelInspectorDialog::onSnapshotFilterChanged(const QString &pattern)
{
    m_proxySnapshotModel->setFilterWildcard(pattern);
}

void CppCodeModelInspectorDialog::onSnapshotSelected(int row)
{
    if (row < 0 || row >= m_snapshotInfos->size())
        return;

    m_snapshotView->clearFilter();
    const SnapshotInfo info = m_snapshotInfos->at(row);
    m_snapshotModel->configure(info.snapshot);
    m_snapshotView->resizeColumns(SnapshotModel::ColumnCount);

    if (info.type == SnapshotInfo::GlobalSnapshot) {
        // Select first document
        const QModelIndex index = m_proxySnapshotModel->index(0, SnapshotModel::FilePathColumn);
        m_snapshotView->selectIndex(index);
    } else if (info.type == SnapshotInfo::EditorSnapshot) {
        // Select first document, unless we can find the editor document
        QModelIndex index = m_snapshotModel->indexForDocument(fileInCurrentEditor());
        index = m_proxySnapshotModel->mapFromSource(index);
        if (!index.isValid())
            index = m_proxySnapshotModel->index(0, SnapshotModel::FilePathColumn);
        m_snapshotView->selectIndex(index);
    }
}

void CppCodeModelInspectorDialog::onDocumentSelected(const QModelIndex &current,
                                                     const QModelIndex &)
{
    if (current.isValid()) {
        const QModelIndex index = m_proxySnapshotModel->index(current.row(),
                                                              SnapshotModel::FilePathColumn);
        const QString filePath = QDir::fromNativeSeparators(
            m_proxySnapshotModel->data(index, Qt::DisplayRole).toString());
        const SnapshotInfo info = m_snapshotInfos->at(m_ui->snapshotSelector->currentIndex());
        updateDocumentData(info.snapshot.document(filePath));
    } else {
        clearDocumentData();
    }
}

void CppCodeModelInspectorDialog::onSymbolsViewExpandedOrCollapsed(const QModelIndex &)
{
    resizeColumns<SymbolsModel>(m_ui->docSymbolsView);
}

void CppCodeModelInspectorDialog::onProjectPartFilterChanged(const QString &pattern)
{
    m_proxyProjectPartsModel->setFilterWildcard(pattern);
}

void CppCodeModelInspectorDialog::onProjectPartSelected(const QModelIndex &current,
                                                        const QModelIndex &)
{
    if (current.isValid()) {
        QModelIndex index = m_proxyProjectPartsModel->mapToSource(current);
        if (index.isValid()) {
            index = m_projectPartsModel->index(index.row(), ProjectPartsModel::PartFilePathColumn);
            const QString projectPartId = m_projectPartsModel->data(index, Qt::UserRole).toString();
            updateProjectPartData(m_projectPartsModel->projectPartForProjectId(projectPartId));
        }
    } else {
        clearProjectPartData();
    }
}

void CppCodeModelInspectorDialog::onWorkingCopyFilterChanged(const QString &pattern)
{
    m_proxyWorkingCopyModel->setFilterWildcard(pattern);
}

void CppCodeModelInspectorDialog::onWorkingCopyDocumentSelected(const QModelIndex &current,
                                                                const QModelIndex &)
{
    if (current.isValid()) {
        const QModelIndex index = m_proxyWorkingCopyModel->mapToSource(current);
        if (index.isValid()) {
            const QString source
                = QString::fromUtf8(m_workingCopyModel->data(index, Qt::UserRole).toByteArray());
            m_ui->workingCopySourceEdit->setPlainText(source);
        }
    } else {
        m_ui->workingCopySourceEdit->clear();
    }
}

void CppCodeModelInspectorDialog::refresh()
{
    CppModelManager *cmmi = CppModelManager::instance();

    const int oldSnapshotIndex = m_ui->snapshotSelector->currentIndex();
    const bool selectEditorRelevant
        = m_ui->selectEditorRelevantEntriesAfterRefreshCheckBox->isChecked();

    // Snapshots and Documents
    m_snapshotInfos->clear();
    m_ui->snapshotSelector->clear();

    const Snapshot globalSnapshot = cmmi->snapshot();
    CppCodeModelInspector::Dumper dumper(globalSnapshot);
    m_snapshotModel->setGlobalSnapshot(globalSnapshot);

    m_snapshotInfos->append(SnapshotInfo(globalSnapshot, SnapshotInfo::GlobalSnapshot));
    const QString globalSnapshotTitle
        = QString::fromLatin1("Global/Indexing Snapshot (%1 Documents)").arg(globalSnapshot.size());
    m_ui->snapshotSelector->addItem(globalSnapshotTitle);
    dumper.dumpSnapshot(globalSnapshot, globalSnapshotTitle, /*isGlobalSnapshot=*/ true);

    TextEditor::BaseTextEditor *editor = currentEditor();
    CppTools::CppEditorDocumentHandle *cppEditorDocument = 0;
    if (editor) {
        const QString editorFilePath = editor->document()->filePath().toString();
        cppEditorDocument = cmmi->cppEditorDocument(editorFilePath);
        if (auto *documentProcessor = BaseEditorDocumentProcessor::get(editorFilePath)) {
            const Snapshot editorSnapshot = documentProcessor->snapshot();
            m_snapshotInfos->append(SnapshotInfo(editorSnapshot, SnapshotInfo::EditorSnapshot));
            const QString editorSnapshotTitle
                = QString::fromLatin1("Current Editor's Snapshot (%1 Documents)")
                    .arg(editorSnapshot.size());
            dumper.dumpSnapshot(editorSnapshot, editorSnapshotTitle);
            m_ui->snapshotSelector->addItem(editorSnapshotTitle);
        }
        CppEditorWidget *cppEditorWidget = qobject_cast<CppEditorWidget *>(editor->editorWidget());
        if (cppEditorWidget) {
            SemanticInfo semanticInfo = cppEditorWidget->semanticInfo();
            Snapshot snapshot;

            // Add semantic info snapshot
            snapshot = semanticInfo.snapshot;
            m_snapshotInfos->append(SnapshotInfo(snapshot, SnapshotInfo::EditorSnapshot));
            m_ui->snapshotSelector->addItem(
                QString::fromLatin1("Current Editor's Semantic Info Snapshot (%1 Documents)")
                    .arg(snapshot.size()));

            // Add a pseudo snapshot containing only the semantic info document since this document
            // is not part of the semantic snapshot.
            snapshot = Snapshot();
            snapshot.insert(cppEditorWidget->semanticInfo().doc);
            m_snapshotInfos->append(SnapshotInfo(snapshot, SnapshotInfo::EditorSnapshot));
            const QString snapshotTitle
                = QString::fromLatin1("Current Editor's Pseudo Snapshot with Semantic Info Document (%1 Documents)")
                    .arg(snapshot.size());
            dumper.dumpSnapshot(snapshot, snapshotTitle);
            m_ui->snapshotSelector->addItem(snapshotTitle);
        }
    }

    int snapshotIndex = 0;
    if (selectEditorRelevant) {
        for (int i = 0, total = m_snapshotInfos->size(); i < total; ++i) {
            const SnapshotInfo info = m_snapshotInfos->at(i);
            if (info.type == SnapshotInfo::EditorSnapshot) {
                snapshotIndex = i;
                break;
            }
        }
    } else if (oldSnapshotIndex < m_snapshotInfos->size()) {
        snapshotIndex = oldSnapshotIndex;
    }
    m_ui->snapshotSelector->setCurrentIndex(snapshotIndex);
    onSnapshotSelected(snapshotIndex);

    // Project Parts
    const ProjectPart::Ptr editorsProjectPart = cppEditorDocument
        ? cppEditorDocument->processor()->parser()->projectPart()
        : ProjectPart::Ptr();

    const QList<ProjectInfo> projectInfos = cmmi->projectInfos();
    dumper.dumpProjectInfos(projectInfos);
    m_projectPartsModel->configure(projectInfos, editorsProjectPart);
    m_projectPartsView->resizeColumns(ProjectPartsModel::ColumnCount);
    QModelIndex index = m_proxyProjectPartsModel->index(0, ProjectPartsModel::PartFilePathColumn);
    if (index.isValid()) {
        if (selectEditorRelevant && editorsProjectPart) {
            QModelIndex editorPartIndex = m_projectPartsModel->indexForCurrentEditorsProjectPart();
            editorPartIndex = m_proxyProjectPartsModel->mapFromSource(editorPartIndex);
            if (editorPartIndex.isValid())
                index = editorPartIndex;
        }
        m_projectPartsView->selectIndex(index);
    }

    // Working Copy
    const WorkingCopy workingCopy = cmmi->workingCopy();
    dumper.dumpWorkingCopy(workingCopy);
    m_workingCopyModel->configure(workingCopy);
    m_workingCopyView->resizeColumns(WorkingCopyModel::ColumnCount);
    if (workingCopy.size() > 0) {
        QModelIndex index = m_proxyWorkingCopyModel->index(0, WorkingCopyModel::FilePathColumn);
        if (selectEditorRelevant) {
            const QModelIndex eindex = m_workingCopyModel->indexForFile(fileInCurrentEditor());
            if (eindex.isValid())
                index = m_proxyWorkingCopyModel->mapFromSource(eindex);
        }
        m_workingCopyView->selectIndex(index);
    }

    // Merged entities
    dumper.dumpMergedEntities(cmmi->headerPaths(), cmmi->definedMacros());
}

enum DocumentTabs {
    DocumentGeneralTab,
    DocumentIncludesTab,
    DocumentDiagnosticsTab,
    DocumentDefinedMacrosTab,
    DocumentPreprocessedSourceTab,
    DocumentSymbolsTab,
    DocumentTokensTab
};

static QString docTabName(int tabIndex, int numberOfEntries = -1)
{
    const char *names[] = {
        "&General",
        "&Includes",
        "&Diagnostic Messages",
        "(Un)Defined &Macros",
        "P&reprocessed Source",
        "&Symbols",
        "&Tokens"
    };
    QString result = QLatin1String(names[tabIndex]);
    if (numberOfEntries != -1)
        result += QString::fromLatin1(" (%1)").arg(numberOfEntries);
    return result;
}

void CppCodeModelInspectorDialog::clearDocumentData()
{
    m_docGenericInfoModel->clear();

    m_ui->docTab->setTabText(DocumentIncludesTab, docTabName(DocumentIncludesTab));
    m_docIncludesModel->clear();

    m_ui->docTab->setTabText(DocumentDiagnosticsTab, docTabName(DocumentDiagnosticsTab));
    m_docDiagnosticMessagesModel->clear();

    m_ui->docTab->setTabText(DocumentDefinedMacrosTab, docTabName(DocumentDefinedMacrosTab));
    m_docMacrosModel->clear();

    m_ui->docPreprocessedSourceEdit->clear();

    m_docSymbolsModel->clear();

    m_ui->docTab->setTabText(DocumentTokensTab, docTabName(DocumentTokensTab));
    m_docTokensModel->clear();
}

void CppCodeModelInspectorDialog::updateDocumentData(const Document::Ptr &document)
{
    QTC_ASSERT(document, return);

    // General
    KeyValueModel::Table table = KeyValueModel::Table()
        << qMakePair(QString::fromLatin1("File Path"),
                     QDir::toNativeSeparators(document->fileName()))
        << qMakePair(QString::fromLatin1("Last Modified"),
                     CMI::Utils::toString(document->lastModified()))
        << qMakePair(QString::fromLatin1("Revision"),
                     CMI::Utils::toString(document->revision()))
        << qMakePair(QString::fromLatin1("Editor Revision"),
                     CMI::Utils::toString(document->editorRevision()))
        << qMakePair(QString::fromLatin1("Check Mode"),
                     CMI::Utils::toString(document->checkMode()))
        << qMakePair(QString::fromLatin1("Tokenized"),
                     CMI::Utils::toString(document->isTokenized()))
        << qMakePair(QString::fromLatin1("Parsed"),
                     CMI::Utils::toString(document->isParsed()))
        << qMakePair(QString::fromLatin1("Project Parts"),
                     CMI::Utils::partsForFile(document->fileName()))
        ;
    m_docGenericInfoModel->configure(table);
    resizeColumns<KeyValueModel>(m_ui->docGeneralView);

    // Includes
    m_docIncludesModel->configure(document->resolvedIncludes() + document->unresolvedIncludes());
    resizeColumns<IncludesModel>(m_ui->docIncludesView);
    m_ui->docTab->setTabText(DocumentIncludesTab,
        docTabName(DocumentIncludesTab, m_docIncludesModel->rowCount()));

    // Diagnostic Messages
    m_docDiagnosticMessagesModel->configure(document->diagnosticMessages());
    resizeColumns<DiagnosticMessagesModel>(m_ui->docDiagnosticMessagesView);
    m_ui->docTab->setTabText(DocumentDiagnosticsTab,
        docTabName(DocumentDiagnosticsTab, m_docDiagnosticMessagesModel->rowCount()));

    // Macros
    m_docMacrosModel->configure(document->definedMacros());
    resizeColumns<MacrosModel>(m_ui->docDefinedMacrosView);
    m_ui->docTab->setTabText(DocumentDefinedMacrosTab,
        docTabName(DocumentDefinedMacrosTab, m_docMacrosModel->rowCount()));

    // Source
    m_ui->docPreprocessedSourceEdit->setPlainText(QString::fromUtf8(document->utf8Source()));

    // Symbols
    m_docSymbolsModel->configure(document);
    resizeColumns<SymbolsModel>(m_ui->docSymbolsView);

    // Tokens
    m_docTokensModel->configure(document->translationUnit());
    resizeColumns<TokensModel>(m_ui->docTokensView);
    m_ui->docTab->setTabText(DocumentTokensTab,
        docTabName(DocumentTokensTab, m_docTokensModel->rowCount()));
}

enum ProjectPartTabs {
    ProjectPartGeneralTab,
    ProjectPartFilesTab,
    ProjectPartDefinesTab,
    ProjectPartHeaderPathsTab,
    ProjectPartPrecompiledHeadersTab
};

static QString partTabName(int tabIndex, int numberOfEntries = -1)
{
    const char *names[] = {
        "&General",
        "Project &Files",
        "&Defines",
        "&Header Paths",
        "Pre&compiled Headers"
    };
    QString result = QLatin1String(names[tabIndex]);
    if (numberOfEntries != -1)
        result += QString::fromLatin1(" (%1)").arg(numberOfEntries);
    return result;
}

void CppCodeModelInspectorDialog::clearProjectPartData()
{
    m_partGenericInfoModel->clear();

    m_ui->partProjectFilesEdit->clear();
    m_ui->projectPartTab->setTabText(ProjectPartFilesTab, partTabName(ProjectPartFilesTab));

    m_ui->partToolchainDefinesEdit->clear();
    m_ui->partProjectDefinesEdit->clear();
    m_ui->projectPartTab->setTabText(ProjectPartDefinesTab, partTabName(ProjectPartDefinesTab));

    m_ui->partHeaderPathsEdit->clear();
    m_ui->projectPartTab->setTabText(ProjectPartHeaderPathsTab,
                                     partTabName(ProjectPartHeaderPathsTab));

    m_ui->partPrecompiledHeadersEdit->clear();
    m_ui->projectPartTab->setTabText(ProjectPartPrecompiledHeadersTab,
                                     partTabName(ProjectPartPrecompiledHeadersTab));
}

void CppCodeModelInspectorDialog::updateProjectPartData(const ProjectPart::Ptr &part)
{
    QTC_ASSERT(part, return);

    // General
    QString projectName = QLatin1String("<None>");
    QString projectFilePath = QLatin1String("<None>");
    if (ProjectExplorer::Project *project = part->project) {
        projectName = project->displayName();
        projectFilePath = project->projectFilePath().toUserOutput();
    }
    KeyValueModel::Table table = KeyValueModel::Table()
        << qMakePair(QString::fromLatin1("Project Part Name"), part->displayName)
        << qMakePair(QString::fromLatin1("Project Part File"),
                     QDir::toNativeSeparators(part->projectFile))
        << qMakePair(QString::fromLatin1("Project Name"), projectName)
        << qMakePair(QString::fromLatin1("Project File"), projectFilePath)
        << qMakePair(QString::fromLatin1("Language Version"),
                     CMI::Utils::toString(part->languageVersion))
        << qMakePair(QString::fromLatin1("Language Extensions"),
                     CMI::Utils::toString(part->languageExtensions))
        << qMakePair(QString::fromLatin1("Qt Version"),
                     CMI::Utils::toString(part->qtVersion))
        ;
    if (!part->projectConfigFile.isEmpty())
        table.prepend(qMakePair(QString::fromLatin1("Project Config File"),
                                part->projectConfigFile));
    m_partGenericInfoModel->configure(table);
    resizeColumns<KeyValueModel>(m_ui->partGeneralView);

    // Project Files
    m_ui->partProjectFilesEdit->setPlainText(CMI::Utils::toString(part->files));
    m_ui->projectPartTab->setTabText(ProjectPartFilesTab,
        partTabName(ProjectPartFilesTab, part->files.size()));

    // Defines
    const QList<QByteArray> defineLines = part->toolchainDefines.split('\n')
        + part->projectDefines.split('\n');
    int numberOfDefines = 0;
    foreach (const QByteArray &line, defineLines) {
        if (line.startsWith("#define "))
            ++numberOfDefines;
    }
    m_ui->partToolchainDefinesEdit->setPlainText(QString::fromUtf8(part->toolchainDefines));
    m_ui->partProjectDefinesEdit->setPlainText(QString::fromUtf8(part->projectDefines));
    m_ui->projectPartTab->setTabText(ProjectPartDefinesTab,
        partTabName(ProjectPartDefinesTab, numberOfDefines));

    // Header Paths
    m_ui->partHeaderPathsEdit->setPlainText(CMI::Utils::pathListToString(part->headerPaths));
    m_ui->projectPartTab->setTabText(ProjectPartHeaderPathsTab,
        partTabName(ProjectPartHeaderPathsTab, part->headerPaths.size()));

    // Precompiled Headers
    m_ui->partPrecompiledHeadersEdit->setPlainText(
                CMI::Utils::pathListToString(part->precompiledHeaders));
    m_ui->projectPartTab->setTabText(ProjectPartPrecompiledHeadersTab,
        partTabName(ProjectPartPrecompiledHeadersTab, part->precompiledHeaders.size()));
}

bool CppCodeModelInspectorDialog::event(QEvent *e)
{
    if (e->type() == QEvent::ShortcutOverride) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(e);
        if (ke->key() == Qt::Key_Escape && !ke->modifiers()) {
            ke->accept();
            close();
            return false;
        }
    }
    return QDialog::event(e);
}

} // namespace Internal
} // namespace CppEditor

#include "cppcodemodelinspectordialog.moc"
