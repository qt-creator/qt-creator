// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmljsfunctionfilter.h"

#include "qmljsmodelmanager.h"
#include "qmljstoolstr.h"

#include <coreplugin/locator/ilocatorfilter.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>

#include <qmljs/parser/qmljsast_p.h>
#include <qmljs/qmljsdocument.h>
#include <qmljs/qmljsmodelmanagerinterface.h>
#include <qmljs/qmljsutils.h>

#include <utils/algorithm.h>
#include <utils/async.h>
#include <utils/filepath.h>
#include <utils/shutdownguard.h>

#include <QDebug>
#include <QHash>
#include <QMutex>
#include <QMutexLocker>
#include <QRegularExpression>

using namespace Core;
using namespace QmlJS;
using namespace QmlJS::AST;
using namespace Tasking;
using namespace Utils;

namespace QmlJSTools::Internal {

class LocatorData final : public QObject
{
public:
    LocatorData();

    enum EntryType
    {
        Function
    };

    class Entry
    {
    public:
        EntryType type;
        QString symbolName;
        QString displayName;
        QString extraInfo;
        FilePath fileName;
        int line;
        int column;
    };

    QHash<FilePath, QList<Entry>> entries() const;

private:
    void onDocumentUpdated(const QmlJS::Document::Ptr &doc);
    void onAboutToRemoveFiles(const FilePaths &files);

    mutable QMutex m_mutex;
    QHash<FilePath, QList<Entry>> m_entries;
};

LocatorData::LocatorData()
{
    ModelManagerInterface *manager = ModelManagerInterface::instance();
    Q_ASSERT(thread() == manager->thread()); // we do not protect accesses below

    // Force the updating of source file when updating a project (they could be cached, in such
    // case LocatorData::onDocumentUpdated will not be called.
    connect(manager,
            &ModelManagerInterface::projectInfoUpdated,
            [manager](const ModelManagerInterface::ProjectInfo &info) {
                FilePaths files;
                ProjectExplorer::Project *project = projectFromProjectInfo(info);
                QTC_ASSERT(project, return);
                for (const FilePath &f : project->files(ProjectExplorer::Project::SourceFiles))
                    files << f;
                manager->updateSourceFiles(files, true);
            });

    connect(manager, &ModelManagerInterface::documentUpdated,
            this, &LocatorData::onDocumentUpdated);
    connect(manager, &ModelManagerInterface::aboutToRemoveFiles,
            this, &LocatorData::onAboutToRemoveFiles);

    ProjectExplorer::ProjectManager *session = ProjectExplorer::ProjectManager::instance();
    if (session)
        connect(session,
                &ProjectExplorer::ProjectManager::projectRemoved,
                this,
                [this](ProjectExplorer::Project *) { m_entries.clear(); });
}

class FunctionFinder final : protected AST::Visitor
{
    QList<LocatorData::Entry> m_entries;
    Document::Ptr m_doc;
    QString m_context;
    QString m_documentContext;

public:
    QList<LocatorData::Entry> run(const Document::Ptr &doc)
    {
        m_doc = doc;
        if (!doc->componentName().isEmpty())
            m_documentContext = doc->componentName();
        else
            m_documentContext = doc->fileName().fileName();
        accept(doc->ast(), m_documentContext);
        return m_entries;
    }

protected:
    QString contextString(const QString &extra)
    {
        return QString::fromLatin1("%1, %2").arg(extra, m_documentContext);
    }

    LocatorData::Entry basicEntry(SourceLocation loc)
    {
        LocatorData::Entry entry;
        entry.type = LocatorData::Function;
        entry.extraInfo = m_context;
        entry.fileName = m_doc->fileName();
        entry.line = loc.startLine;
        entry.column = loc.startColumn - 1;
        return entry;
    }

    void accept(Node *ast, const QString &context)
    {
        const QString old = m_context;
        m_context = context;
        Node::accept(ast, this);
        m_context = old;
    }

    bool visit(FunctionDeclaration *ast) override
    {
        return visit(static_cast<FunctionExpression *>(ast));
    }

    bool visit(FunctionExpression *ast) override
    {
        if (ast->name.isEmpty())
            return true;

        LocatorData::Entry entry = basicEntry(ast->identifierToken);

        entry.type = LocatorData::Function;
        entry.displayName = ast->name.toString();
        entry.displayName += QLatin1Char('(');
        for (FormalParameterList *it = ast->formals; it; it = it->next) {
            if (it != ast->formals)
                entry.displayName += QLatin1String(", ");
            if (!it->element->bindingIdentifier.isEmpty())
                entry.displayName += it->element->bindingIdentifier.toString();
        }
        entry.displayName += QLatin1Char(')');
        entry.symbolName = entry.displayName;

        m_entries += entry;

        accept(ast->body, contextString(QString::fromLatin1("function %1").arg(entry.displayName)));
        return false;
    }

    bool visit(UiScriptBinding *ast) override
    {
        if (!ast->qualifiedId)
            return true;
        const QString qualifiedIdString = toString(ast->qualifiedId);

        if (cast<Block *>(ast->statement)) {
            LocatorData::Entry entry = basicEntry(ast->qualifiedId->identifierToken);
            entry.displayName = qualifiedIdString;
            entry.symbolName = qualifiedIdString;
            m_entries += entry;
        }

        accept(ast->statement, contextString(toString(ast->qualifiedId)));
        return false;
    }

    bool visit(UiObjectBinding *ast) override
    {
        if (!ast->qualifiedTypeNameId)
            return true;

        QString context = toString(ast->qualifiedTypeNameId);
        const QString id = idOfObject(ast);
        if (!id.isEmpty())
            context = QString::fromLatin1("%1 (%2)").arg(id, context);
        accept(ast->initializer, contextString(context));
        return false;
    }

    bool visit(UiObjectDefinition *ast) override
    {
        if (!ast->qualifiedTypeNameId)
            return true;

        QString context = toString(ast->qualifiedTypeNameId);
        const QString id = idOfObject(ast);
        if (!id.isEmpty())
            context = QString::fromLatin1("%1 (%2)").arg(id, context);
        accept(ast->initializer, contextString(context));
        return false;
    }

    bool visit(AST::BinaryExpression *ast) override
    {
        auto fieldExpr = AST::cast<AST::FieldMemberExpression *>(ast->left);
        auto funcExpr = AST::cast<AST::FunctionExpression *>(ast->right);

        if (fieldExpr && funcExpr && funcExpr->body && (ast->op == QSOperator::Assign)) {
            LocatorData::Entry entry = basicEntry(ast->operatorToken);

            entry.type = LocatorData::Function;
            entry.displayName = fieldExpr->name.toString();
            while (fieldExpr) {
                if (auto field = AST::cast<AST::FieldMemberExpression *>(fieldExpr->base)) {
                    entry.displayName.prepend(field->name.toString() + QLatin1Char('.'));
                    fieldExpr = field;
                } else {
                    if (auto ident = AST::cast<AST::IdentifierExpression *>(fieldExpr->base))
                        entry.displayName.prepend(ident->name.toString() + QLatin1Char('.'));
                    break;
                }
            }

            entry.displayName += QLatin1Char('(');
            for (FormalParameterList *it = funcExpr->formals; it; it = it->next) {
                if (it != funcExpr->formals)
                    entry.displayName += QLatin1String(", ");
                if (!it->element->bindingIdentifier.isEmpty())
                    entry.displayName += it->element->bindingIdentifier.toString();
            }
            entry.displayName += QLatin1Char(')');
            entry.symbolName = entry.displayName;

            m_entries += entry;

            accept(funcExpr->body, contextString(QString::fromLatin1("function %1").arg(entry.displayName)));
            return false;
        }

        return true;
    }

    void throwRecursionDepthError() override
    {
        qWarning("Warning: Hit maximum recursion limit visiting AST in FunctionFinder.");
    }
};

QHash<Utils::FilePath, QList<LocatorData::Entry>> LocatorData::entries() const
{
    QMutexLocker l(&m_mutex);
    return m_entries;
}

void LocatorData::onDocumentUpdated(const Document::Ptr &doc)
{
    QList<Entry> entries = FunctionFinder().run(doc);
    QMutexLocker l(&m_mutex);
    m_entries.insert(doc->fileName(), entries);
}

void LocatorData::onAboutToRemoveFiles(const Utils::FilePaths &files)
{
    QMutexLocker l(&m_mutex);
    for (const Utils::FilePath &file : files) {
        m_entries.remove(file);
    }
}

static void matches(QPromise<void> &promise, const LocatorStorage &storage,
                    const QHash<FilePath, QList<LocatorData::Entry>> &locatorEntries)
{
    const QString input = storage.input();
    const Qt::CaseSensitivity caseSensitivityForPrefix = ILocatorFilter::caseSensitivity(input);
    const QRegularExpression regexp = ILocatorFilter::createRegExp(input);
    if (!regexp.isValid())
        return;

    LocatorFilterEntries entries[int(ILocatorFilter::MatchLevel::Count)];
    for (const QList<LocatorData::Entry> &items : locatorEntries) {
        for (const LocatorData::Entry &info : items) {
            if (promise.isCanceled())
                return;

            if (info.type != LocatorData::Function)
                continue;

            const QRegularExpressionMatch match = regexp.match(info.symbolName);
            if (match.hasMatch()) {
                LocatorFilterEntry filterEntry;
                filterEntry.displayName = info.displayName;
                filterEntry.linkForEditor = {info.fileName, info.line, info.column};
                filterEntry.extraInfo = info.extraInfo;
                filterEntry.highlightInfo = ILocatorFilter::highlightInfo(match);

                if (filterEntry.displayName.startsWith(input, caseSensitivityForPrefix))
                    entries[int(ILocatorFilter::MatchLevel::Best)].append(filterEntry);
                else if (filterEntry.displayName.contains(input, caseSensitivityForPrefix))
                    entries[int(ILocatorFilter::MatchLevel::Better)].append(filterEntry);
                else
                    entries[int(ILocatorFilter::MatchLevel::Good)].append(filterEntry);
            }
        }
    }

    for (auto &entry : entries) {
        if (promise.isCanceled())
            return;

        if (entry.size() < 1000)
            Utils::sort(entry, LocatorFilterEntry::compareLexigraphically);
    }
    storage.reportOutput(std::accumulate(std::begin(entries), std::end(entries),
                                         LocatorFilterEntries()));
}

// QmlJSFunctionsFilter

class QmlJSFunctionsFilter final : public ILocatorFilter
{
public:
    QmlJSFunctionsFilter()
    {
        setId("Functions");
        setDisplayName(Tr::tr("QML Functions"));
        setDescription(Tr::tr("Locates QML functions in any open project."));
        setDefaultShortcutString("m");
    }

private:
    LocatorMatcherTasks matchers() final
    {
        const auto onSetup = [entries = m_data.entries()](Async<void> &async) {
            async.setConcurrentCallData(matches, *LocatorStorage::storage(), entries);
        };

        return {AsyncTask<void>(onSetup)};
    }

    LocatorData m_data;
};

void setupQmlJSFunctionsFilter()
{
    static GuardedObject<QmlJSFunctionsFilter> theQmlJSFunctionsFilter;
}

} // namespace QmlJSTools::Internal
