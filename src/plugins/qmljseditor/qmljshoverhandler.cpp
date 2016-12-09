/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "qmljshoverhandler.h"
#include "qmljseditor.h"
#include "qmljseditorconstants.h"
#include "qmljseditordocument.h"
#include "qmlexpressionundercursor.h"

#include <coreplugin/icore.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/helpmanager.h>
#include <utils/qtcassert.h>
#include <extensionsystem/pluginmanager.h>
#include <qmljs/qmljscontext.h>
#include <qmljs/qmljsscopechain.h>
#include <qmljs/qmljsinterpreter.h>
#include <qmljs/qmljsvalueowner.h>
#include <qmljs/parser/qmljsast_p.h>
#include <qmljs/parser/qmljsastfwd_p.h>
#include <qmljs/qmljsutils.h>
#include <qmljs/qmljsqrcparser.h>
#include <texteditor/texteditor.h>
#include <texteditor/helpitem.h>
#include <utils/tooltip/tooltip.h>

#include <QDir>
#include <QList>
#include <QStringRef>
#include <QRegularExpression>
#include <QRegularExpressionMatch>

using namespace Core;
using namespace QmlJS;
using namespace TextEditor;

namespace QmlJSEditor {
namespace Internal {

namespace {

    QString textAt(const Document::Ptr doc,
                   const AST::SourceLocation &from,
                   const AST::SourceLocation &to)
    {
        return doc->source().mid(from.offset, to.end() - from.begin());
    }

    AST::UiObjectInitializer *nodeInitializer(AST::Node *node)
    {
        AST::UiObjectInitializer *initializer = 0;
        if (const AST::UiObjectBinding *binding = AST::cast<const AST::UiObjectBinding *>(node))
            initializer = binding->initializer;
         else if (const AST::UiObjectDefinition *definition =
                  AST::cast<const AST::UiObjectDefinition *>(node))
            initializer = definition->initializer;
        return initializer;
    }

    template <class T>
    bool posIsInSource(const unsigned pos, T *node)
    {
        if (node &&
            pos >= node->firstSourceLocation().begin() && pos < node->lastSourceLocation().end()) {
            return true;
        }
        return false;
    }
}

QmlJSHoverHandler::QmlJSHoverHandler() : m_modelManager(0)
{
    m_modelManager = ModelManagerInterface::instance();
}

static inline QString getModuleName(const ScopeChain &scopeChain, const Document::Ptr &qmlDocument,
                                    const ObjectValue *value)
{
    if (!value)
        return QString();

    const CppComponentValue *qmlValue = value_cast<CppComponentValue>(value);
    if (qmlValue) {
        const QString moduleName = qmlValue->moduleName();
        const Imports *imports = scopeChain.context()->imports(qmlDocument.data());
        const ImportInfo importInfo = imports->info(qmlValue->className(), scopeChain.context().data());
        if (importInfo.isValid() && importInfo.type() == ImportType::Library) {
            const int majorVersion = importInfo.version().majorVersion();
            const int minorVersion = importInfo.version().minorVersion();
            return moduleName + QString::number(majorVersion) + QLatin1Char('.')
                    + QString::number(minorVersion) ;
        }
        return QString();
    } else {
        QString typeName = value->className();

        const Imports *imports = scopeChain.context()->imports(qmlDocument.data());
        const ImportInfo importInfo = imports->info(typeName, scopeChain.context().data());
        if (importInfo.isValid() && importInfo.type() == ImportType::Library) {
            const QString moduleName = importInfo.name();
            const int majorVersion = importInfo.version().majorVersion();
            const int minorVersion = importInfo.version().minorVersion();
            return moduleName + QString::number(majorVersion) + QLatin1Char('.')
                    + QString::number(minorVersion) ;
        } else if (importInfo.isValid() && importInfo.type() == ImportType::Directory) {
            const QString path = importInfo.path();
            const QDir dir(qmlDocument->path());
            // should probably try to make it relatve to some import path, not to the document path
            QString relativeDir = dir.relativeFilePath(path);
            const QString name = relativeDir.replace(QLatin1Char('/'), QLatin1Char('.'));
            return name;
        } else if (importInfo.isValid() && importInfo.type() == ImportType::QrcDirectory) {
            QString path = QrcParser::normalizedQrcDirectoryPath(importInfo.path());
            path = path.mid(1, path.size() - ((path.size() > 1) ? 2 : 1));
            const QString name = path.replace(QLatin1Char('/'), QLatin1Char('.'));
            return name;
        }
    }
    return QString();
}

bool QmlJSHoverHandler::setQmlTypeHelp(const ScopeChain &scopeChain, const Document::Ptr &qmlDocument,
                                       const ObjectValue *value, const QStringList &qName)
{
    QString moduleName = getModuleName(scopeChain, qmlDocument, value);

    QMap<QString, QUrl> urlMap;

    QString helpId;
    do {
        QStringList helpIdPieces(qName);
        helpIdPieces.prepend(moduleName);
        helpIdPieces.prepend(QLatin1String("QML"));
        helpId = helpIdPieces.join(QLatin1Char('.'));
        urlMap = HelpManager::linksForIdentifier(helpId);
        if (!urlMap.isEmpty())
            break;
        if (helpIdPieces.size() > 3) {
            QString lm = helpIdPieces.value(2);
            helpIdPieces.removeAt(2);
            helpId = helpIdPieces.join(QLatin1Char('.'));
            urlMap = HelpManager::linksForIdentifier(helpId);
            if (!urlMap.isEmpty())
                break;
            helpIdPieces.replace(1, lm);
            urlMap = HelpManager::linksForIdentifier(helpId);
            if (!urlMap.isEmpty())
                break;
        }
        helpIdPieces.removeAt(1);
        helpId = helpIdPieces.join(QLatin1Char('.'));
        urlMap = HelpManager::linksForIdentifier(helpId);
        if (!urlMap.isEmpty())
            break;
        return false;
    } while (0);

    // Check if the module name contains a major version.
    QRegularExpression version("^([^\\d]*)(\\d+)\\.*\\d*$");
    QRegularExpressionMatch m = version.match(moduleName);
    if (m.hasMatch()) {
        QMap<QString, QUrl> filteredUrlMap;
        QStringRef maj = m.capturedRef(2);
        for (auto x = urlMap.begin(); x != urlMap.end(); ++x) {
            QString urlModuleName = x.value().path().split('/')[1];
            if (urlModuleName.contains(maj))
                filteredUrlMap.insert(x.key(), x.value());
        }
        if (!filteredUrlMap.isEmpty()) {
            // Use the url as helpId, to disambiguate different versions
            helpId = filteredUrlMap.first().toString();
            const HelpItem helpItem(helpId, qName.join(QLatin1Char('.')), HelpItem::QmlComponent, filteredUrlMap);
            setLastHelpItemIdentified(helpItem);
            return true;
        }
    }
    setLastHelpItemIdentified(HelpItem(helpId, qName.join(QLatin1Char('.')), HelpItem::QmlComponent));
    return true;
}

void QmlJSHoverHandler::identifyMatch(TextEditorWidget *editorWidget, int pos)
{
    reset();

    if (!m_modelManager)
        return;

    QmlJSEditorWidget *qmlEditor = qobject_cast<QmlJSEditorWidget *>(editorWidget);
    QTC_ASSERT(qmlEditor, return);

    const QmlJSTools::SemanticInfo &semanticInfo = qmlEditor->qmlJsEditorDocument()->semanticInfo();
    if (!semanticInfo.isValid() || qmlEditor->qmlJsEditorDocument()->isSemanticInfoOutdated())
        return;

    QList<AST::Node *> rangePath = semanticInfo.rangePath(pos);

    const Document::Ptr qmlDocument = semanticInfo.document;
    ScopeChain scopeChain = semanticInfo.scopeChain(rangePath);

    QList<AST::Node *> astPath = semanticInfo.astPath(pos);
    QTC_ASSERT(!astPath.isEmpty(), return);
    AST::Node *node = astPath.last();

    if (rangePath.isEmpty()) {
        // Is the cursor on an import? The ast path will have an UiImport
        // member in the last or second to last position!
        AST::UiImport *import = 0;
        if (astPath.size() >= 1)
            import = AST::cast<AST::UiImport *>(astPath.last());
        if (!import && astPath.size() >= 2)
            import = AST::cast<AST::UiImport *>(astPath.at(astPath.size() - 2));
        if (import)
            handleImport(scopeChain, import);
        // maybe parsing failed badly, still try to identify types
        quint32 i,j;
        i = j = pos;
        QString nameAtt;
        for (;;) {
            QChar c = qmlEditor->document()->characterAt(j);
            if (!c.isLetterOrNumber()) break;
            nameAtt.append(c);
            ++j;
        }
        QStringList qName;
        while (i>0) {
            --i;
            QChar c = qmlEditor->document()->characterAt(i);
            if (c.isLetterOrNumber()) {
                nameAtt.prepend(c);
            } else if (c == QLatin1Char('.')) {
                qName.append(nameAtt);
                nameAtt.clear();
            } else {
                qName.append(nameAtt);
                break;
            }
        }
        const ObjectValue *value = scopeChain.context()->lookupType(qmlDocument.data(), qName);
        setQmlTypeHelp(scopeChain, qmlDocument, value, qName);
        matchDiagnosticMessage(qmlEditor, pos);
        return;
    }
    if (matchDiagnosticMessage(qmlEditor, pos))
        return;
    if (matchColorItem(scopeChain, qmlDocument, rangePath, pos))
        return;

    handleOrdinaryMatch(scopeChain, node);


    setQmlHelpItem(scopeChain, qmlDocument, node);
}

bool QmlJSHoverHandler::matchDiagnosticMessage(QmlJSEditorWidget *qmlEditor, int pos)
{
    foreach (const QTextEdit::ExtraSelection &sel,
             qmlEditor->extraSelections(TextEditorWidget::CodeWarningsSelection)) {
        if (pos >= sel.cursor.selectionStart() && pos <= sel.cursor.selectionEnd()) {
            setToolTip(sel.format.toolTip());
            return true;
        }
    }
    foreach (const QTextLayout::FormatRange &range,
             qmlEditor->qmlJsEditorDocument()->diagnosticRanges()) {
        if (pos >= range.start && pos < range.start+range.length) {
            setToolTip(range.format.toolTip());
            return true;
        }
    }
    return false;
}

bool QmlJSHoverHandler::matchColorItem(const ScopeChain &scopeChain,
                                  const Document::Ptr &qmlDocument,
                                  const QList<AST::Node *> &astPath,
                                  unsigned pos)
{
    AST::UiObjectInitializer *initializer = nodeInitializer(astPath.last());
    if (!initializer)
        return false;

    AST::UiObjectMember *member = 0;
    for (AST::UiObjectMemberList *list = initializer->members; list; list = list->next) {
        if (posIsInSource(pos, list->member)) {
            member = list->member;
            break;
        }
    }
    if (!member)
        return false;

    QString color;
    const Value *value = 0;
    if (const AST::UiScriptBinding *binding = AST::cast<const AST::UiScriptBinding *>(member)) {
        if (binding->qualifiedId && posIsInSource(pos, binding->statement)) {
            value = scopeChain.evaluate(binding->qualifiedId);
            if (value && value->asColorValue()) {
                color = textAt(qmlDocument,
                               binding->statement->firstSourceLocation(),
                               binding->statement->lastSourceLocation());
            }
        }
    } else if (const AST::UiPublicMember *publicMember =
               AST::cast<const AST::UiPublicMember *>(member)) {
        if (!publicMember->name.isEmpty() && posIsInSource(pos, publicMember->statement)) {
            value = scopeChain.lookup(publicMember->name.toString());
            if (const Reference *ref = value->asReference())
                value = scopeChain.context()->lookupReference(ref);
            if (value && value->asColorValue()) {
                color = textAt(qmlDocument,
                               publicMember->statement->firstSourceLocation(),
                               publicMember->statement->lastSourceLocation());
            }
        }
    }

    if (!color.isEmpty()) {
        color.remove(QLatin1Char('\''));
        color.remove(QLatin1Char('\"'));
        color.remove(QLatin1Char(';'));

        m_colorTip = QmlJS::toQColor(color);
        if (m_colorTip.isValid()) {
            setToolTip(color);
            return true;
        }
    }
    return false;
}

void QmlJSHoverHandler::handleOrdinaryMatch(const ScopeChain &scopeChain, AST::Node *node)
{
    if (node && !(AST::cast<AST::StringLiteral *>(node) != 0 ||
                  AST::cast<AST::NumericLiteral *>(node) != 0)) {
        const Value *value = scopeChain.evaluate(node);
        prettyPrintTooltip(value, scopeChain.context());
    }
}

void QmlJSHoverHandler::handleImport(const ScopeChain &scopeChain, AST::UiImport *node)
{
    const Imports *imports = scopeChain.context()->imports(scopeChain.document().data());
    if (!imports)
        return;

    foreach (const Import &import, imports->all()) {
        if (import.info.ast() == node) {
            if (import.info.type() == ImportType::Library
                    && !import.libraryPath.isEmpty()) {
                QString msg = tr("Library at %1").arg(import.libraryPath);
                const LibraryInfo &libraryInfo = scopeChain.context()->snapshot().libraryInfo(import.libraryPath);
                if (libraryInfo.pluginTypeInfoStatus() == LibraryInfo::DumpDone) {
                    msg += QLatin1Char('\n');
                    msg += tr("Dumped plugins successfully.");
                } else if (libraryInfo.pluginTypeInfoStatus() == LibraryInfo::TypeInfoFileDone) {
                    msg += QLatin1Char('\n');
                    msg += tr("Read typeinfo files successfully.");
                }
                setToolTip(msg);
            } else {
                setToolTip(import.info.path());
            }
            break;
        }
    }
}

void QmlJSHoverHandler::reset()
{
    m_colorTip = QColor();
}

void QmlJSHoverHandler::operateTooltip(TextEditorWidget *editorWidget, const QPoint &point)
{
    if (toolTip().isEmpty())
        Utils::ToolTip::hide();
    else if (m_colorTip.isValid())
        Utils::ToolTip::show(point, m_colorTip, editorWidget);
    else
        BaseHoverHandler::operateTooltip(editorWidget, point);
}

void QmlJSHoverHandler::prettyPrintTooltip(const Value *value,
                                      const ContextPtr &context)
{
    if (! value)
        return;

    if (const ObjectValue *objectValue = value->asObjectValue()) {
        PrototypeIterator iter(objectValue, context);
        while (iter.hasNext()) {
            const ObjectValue *prototype = iter.next();
            const QString className = prototype->className();

            if (! className.isEmpty()) {
                setToolTip(className);
                break;
            }
        }
    } else if (const QmlEnumValue *enumValue =
               value_cast<QmlEnumValue>(value)) {
        setToolTip(enumValue->name());
    }

    if (toolTip().isEmpty()) {
        if (!value->asUndefinedValue() && !value->asUnknownValue()) {
            const QString typeId = context->valueOwner()->typeId(value);
            setToolTip(typeId);
        }
    }
}

// if node refers to a property, its name and defining object are returned - otherwise zero
static const ObjectValue *isMember(const ScopeChain &scopeChain,
                                                AST::Node *node, QString *name)
{
    const ObjectValue *owningObject = 0;
    if (AST::IdentifierExpression *identExp = AST::cast<AST::IdentifierExpression *>(node)) {
        if (identExp->name.isEmpty())
            return 0;
        *name = identExp->name.toString();
        scopeChain.lookup(*name, &owningObject);
    } else if (AST::FieldMemberExpression *fme = AST::cast<AST::FieldMemberExpression *>(node)) {
        if (!fme->base || fme->name.isEmpty())
            return 0;
        *name = fme->name.toString();
        const Value *base = scopeChain.evaluate(fme->base);
        if (!base)
            return 0;
        owningObject = base->asObjectValue();
        if (owningObject)
            owningObject->lookupMember(*name, scopeChain.context(), &owningObject);
    } else if (AST::UiQualifiedId *qid = AST::cast<AST::UiQualifiedId *>(node)) {
        if (qid->name.isEmpty())
            return 0;
        *name = qid->name.toString();
        const Value *value = scopeChain.lookup(*name, &owningObject);
        for (AST::UiQualifiedId *it = qid->next; it; it = it->next) {
            if (!value)
                return 0;
            const ObjectValue *next = value->asObjectValue();
            if (!next || it->name.isEmpty())
                return 0;
            *name = it->name.toString();
            value = next->lookupMember(*name, scopeChain.context(), &owningObject);
        }
    }
    return owningObject;
}

bool QmlJSHoverHandler::setQmlHelpItem(const ScopeChain &scopeChain,
                                  const Document::Ptr &qmlDocument,
                                  AST::Node *node)
{
    QString name;
    QString moduleName;
    if (const ObjectValue *scope = isMember(scopeChain, node, &name)) {
        // maybe it's a type?
        if (!name.isEmpty() && name.at(0).isUpper()) {
            if (AST::UiQualifiedId *qualifiedId = AST::cast<AST::UiQualifiedId *>(node)) {
                const ObjectValue *value = scopeChain.context()->lookupType(qmlDocument.data(), qualifiedId);
                if (setQmlTypeHelp(scopeChain, qmlDocument, value, QStringList(qualifiedId->name.toString())))
                    return true;
            }
        }

        // otherwise, it's probably a property
        const ObjectValue *lastScope;
        scope->lookupMember(name, scopeChain.context(), &lastScope);
        PrototypeIterator iter(scope, scopeChain.context());
        while (iter.hasNext()) {
            const ObjectValue *cur = iter.next();

            const QString className = cur->className();
            if (!className.isEmpty()) {
                moduleName = getModuleName(scopeChain, qmlDocument, cur);
                QString helpId;
                do {
                    helpId = QLatin1String("QML.") + moduleName + QLatin1Char('.') + className
                            + QLatin1String("::") + name;
                    if (!HelpManager::linksForIdentifier(helpId).isEmpty())
                        break;
                    helpId = QLatin1String("QML.") + className + QLatin1String("::") + name;
                    if (!HelpManager::linksForIdentifier(helpId).isEmpty())
                        break;
                    helpId = className + QLatin1String("::") + name;
                    if (!HelpManager::linksForIdentifier(helpId).isEmpty())
                        break;
                    helpId.clear();
                } while (0);
                if (!helpId.isEmpty()) {
                    setLastHelpItemIdentified(HelpItem(helpId, name, HelpItem::QmlProperty));
                    return true;
                }
            }

            if (cur == lastScope)
                break;
        }
    } else {
        // it might be a type, but the scope chain is broken (mismatched braces)
        if (AST::UiQualifiedId *qualifiedId = AST::cast<AST::UiQualifiedId *>(node)) {
            const ObjectValue *value = scopeChain.context()->lookupType(qmlDocument.data(),
                                                                        qualifiedId);
            if (setQmlTypeHelp(scopeChain, qmlDocument, value,
                               QStringList(qualifiedId->name.toString())))
                return true;
        }
    }
    return false;
}

} // namespace Internal
} // namespace QmlJSEditor

