/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** 
** Non-Open Source Usage  
** 
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.  
** 
** GNU General Public License Usage 
** 
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception version
** 1.2, included in the file GPL_EXCEPTION.txt in this package.  
** 
***************************************************************************/
#include "cppquickopenfilter.h"

#include <Literals.h>
#include <Symbols.h>
#include <SymbolVisitor.h>
#include <Scope.h>
#include <cplusplus/Overview.h>
#include <cplusplus/Icons.h>

#include <coreplugin/editormanager/ieditor.h>
#include <texteditor/itexteditor.h>
#include <texteditor/basetexteditor.h>

#include <QtCore/QMultiMap>

#include <functional>

using namespace CPlusPlus;

namespace CppTools {
namespace Internal {

class SearchSymbols: public std::unary_function<Document::Ptr, QList<ModelItemInfo> >,
                     protected SymbolVisitor
{
    Overview overview;
    Icons icons;
    QList<ModelItemInfo> items;

public:
    QList<ModelItemInfo> operator()(Document::Ptr doc)
    { return operator()(doc, QString()); }

    QList<ModelItemInfo> operator()(Document::Ptr doc, const QString &scope)
    {
        QString previousScope = switchScope(scope);
        items.clear();
        for (unsigned i = 0; i < doc->globalSymbolCount(); ++i) {
            accept(doc->globalSymbolAt(i));
        }
        (void) switchScope(previousScope);
        return items;
    }

protected:
    using SymbolVisitor::visit;

    void accept(Symbol *symbol)
    { Symbol::visitSymbol(symbol, this); }

    QString switchScope(const QString &scope)
    {
        QString previousScope = _scope;
        _scope = scope;
        return previousScope;
    }

    virtual bool visit(Enum *symbol)
    {
        QString name = symbolName(symbol);
        QString previousScope = switchScope(name);
        QIcon icon = icons.iconForSymbol(symbol);
        Scope *members = symbol->members();
        items.append(ModelItemInfo(name, QString(), ModelItemInfo::Enum,
                                   QString::fromUtf8(symbol->fileName(), symbol->fileNameLength()),
                                   symbol->line(),
                                   icon));
        for (unsigned i = 0; i < members->symbolCount(); ++i) {
            accept(members->symbolAt(i));
        }
        (void) switchScope(previousScope);
        return false;
    }

    virtual bool visit(Function *symbol)
    {
        QString name = symbolName(symbol);
        QString type = overview.prettyType(symbol->type());
        QIcon icon = icons.iconForSymbol(symbol);
        items.append(ModelItemInfo(name, type, ModelItemInfo::Method,
                                   QString::fromUtf8(symbol->fileName(), symbol->fileNameLength()),
                                   symbol->line(),
                                   icon));
        return false;
    }

    virtual bool visit(Namespace *symbol)
    {
        QString name = symbolName(symbol);
        QString previousScope = switchScope(name);
        Scope *members = symbol->members();
        for (unsigned i = 0; i < members->symbolCount(); ++i) {
            accept(members->symbolAt(i));
        }
        (void) switchScope(previousScope);
        return false;
    }
#if 0
    // This visit method would make function declaration be included in QuickOpen
    virtual bool visit(Declaration *symbol)
    {
        if (symbol->type()->isFunction()) {
            QString name = symbolName(symbol);
            QString type = overview.prettyType(symbol->type());
            //QIcon icon = ...;
            items.append(ModelItemInfo(name, type, ModelItemInfo::Method,
                                       QString::fromUtf8(symbol->fileName(), symbol->line()),
                                       symbol->line()));
        }
        return false;
    }
#endif
    virtual bool visit(Class *symbol)
    {
        QString name = symbolName(symbol);
        QString previousScope = switchScope(name);
        QIcon icon = icons.iconForSymbol(symbol);
        items.append(ModelItemInfo(name, QString(), ModelItemInfo::Class,
                                   QString::fromUtf8(symbol->fileName(), symbol->fileNameLength()),
                                   symbol->line(),
                                   icon));
        Scope *members = symbol->members();
        for (unsigned i = 0; i < members->symbolCount(); ++i) {
            accept(members->symbolAt(i));
        }
        (void) switchScope(previousScope);
        return false;
    }

    QString symbolName(Symbol *symbol) const
    {
        QString name = _scope;
        if (! name.isEmpty())
            name += QLatin1String("::");
        QString symbolName = overview.prettyName(symbol->name());
        if (symbolName.isEmpty()) {
            QString type;
            if (symbol->isNamespace()) {
                type = QLatin1String("namespace");
            } else if (symbol->isEnum()) {
                type = QLatin1String("enum");
            } else if (Class *c = symbol->asClass())  {
                if (c->isUnion()) {
                    type = QLatin1String("union");
                } else if (c->isStruct()) {
                    type = QLatin1String("struct");
                } else {
                    type = QLatin1String("class");
                }
            } else {
                type = QLatin1String("symbol");
            }
            symbolName = QLatin1String("<anonymous ");
            symbolName += type;
            symbolName += QLatin1String(">");
        }
        name += symbolName;
        return name;
    }

private:
    QString _scope;
};

} // namespace Internal
} // namespace CppTools

using namespace CppTools::Internal;

CppQuickOpenFilter::CppQuickOpenFilter(CppModelManager *manager, Core::EditorManager *editorManager)
    : m_manager(manager),
    m_editorManager(editorManager),
    m_forceNewSearchList(true)
{
    setShortcutString(":");
    setIncludedByDefault(false);

    connect(manager, SIGNAL(documentUpdated(CPlusPlus::Document::Ptr)),
            this, SLOT(onDocumentUpdated(CPlusPlus::Document::Ptr)));

    connect(manager, SIGNAL(aboutToRemoveFiles(QStringList)),
            this, SLOT(onAboutToRemoveFiles(QStringList)));
}

CppQuickOpenFilter::~CppQuickOpenFilter()
{ }

void CppQuickOpenFilter::onDocumentUpdated(CPlusPlus::Document::Ptr doc)
{
    m_searchList[doc->fileName()] = Info(doc);
}

void CppQuickOpenFilter::onAboutToRemoveFiles(const QStringList &files)
{
    foreach (QString file, files) {
        m_searchList.remove(file);
    }
}

void CppQuickOpenFilter::refresh(QFutureInterface<void> &future)
{
    Q_UNUSED(future);
}

QList<QuickOpen::FilterEntry> CppQuickOpenFilter::matchesFor(const QString &origEntry)
{
    QString entry = trimWildcards(origEntry);
    QList<QuickOpen::FilterEntry> entries;
    QStringMatcher matcher(entry, Qt::CaseInsensitive);
    const QRegExp regexp("*"+entry+"*", Qt::CaseInsensitive, QRegExp::Wildcard);
    if (!regexp.isValid())
        return entries;
    bool hasWildcard = (entry.contains('*') || entry.contains('?'));

    SearchSymbols search;
    QMutableMapIterator<QString, Info> it(m_searchList);
    while (it.hasNext()) {
        it.next();

        Info info = it.value();
        if (info.dirty) {
            info.dirty = false;
            info.items = search(info.doc);
            it.setValue(info);
        }

        QList<ModelItemInfo> items = info.items;

        foreach (ModelItemInfo info, items) {
            if ((hasWildcard && regexp.exactMatch(info.symbolName))
                    || (!hasWildcard && matcher.indexIn(info.symbolName) != -1)) {
                QVariant id = qVariantFromValue(info);
                QuickOpen::FilterEntry filterEntry(this, info.symbolName, id, info.icon);
                filterEntry.extraInfo = info.symbolType;
                entries.append(filterEntry);
            }
        }
    }

    return entries;
}

void CppQuickOpenFilter::accept(QuickOpen::FilterEntry selection) const
{
    ModelItemInfo info = qvariant_cast<CppTools::Internal::ModelItemInfo>(selection.internalData);

    TextEditor::BaseTextEditor::openEditorAt(info.fileName, info.line);
}
