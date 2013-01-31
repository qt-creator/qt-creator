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

#include <QByteArray>
#include <QDataStream>
#include <QSet>

#include "changeimportsvisitor.h"
#include "copypasteutil.h"
#include "parsedqml.h"
#include "paster.h"
#include "qmlrewriter.h"

using namespace QmlJS;
using namespace QmlJS::AST;

namespace QmlDesigner {
namespace Internal {

class ASTExtractor: public AST::Visitor, protected CopyPasteUtil
{
private:
    Paster *m_paster;
    QSet<TextLocation> m_locations;
    bool m_inStates;
    QString m_stateName;

private:
    TextLocation toLocation(quint32 pos, quint32 len) const { return TextLocation((int) pos, (int) len); }
    TextLocation toLocation(const SourceLocation &firstSourceLocation, const SourceLocation &lastSourceLocation) const
    { return toLocation(firstSourceLocation.offset, lastSourceLocation.end() - firstSourceLocation.offset); }

public:
    ASTExtractor(Paster *paster, const QSet<TextLocation> &locations, const QString &sourceCode):
            CopyPasteUtil(sourceCode),
            m_paster(paster),
            m_locations(locations)
    {}

    bool operator()(QmlJS::AST::UiProgram *sourceAST) {
        m_inStates = false;
        m_stateName.clear();

        Node::accept(sourceAST->imports, this);

        if (sourceAST->members && sourceAST->members->member)
            visitRootMember(sourceAST->members->member);

        return m_locations.isEmpty();
    }

protected:
    bool switchInStates(bool newInStates) {
        bool oldInStates = m_inStates;
        m_inStates = newInStates;
        return oldInStates;
    }

    QString switchStateName(const QString &newStateName) {
        QString oldStateName = m_stateName;
        m_stateName = newStateName;
        return oldStateName;
    }

    bool visit(UiImportList *ast) {
        for (UiImportList *it = ast; it; it = it->next) {
            if (it->import) {
                m_paster->addImports(createImport(it->import));

                m_locations.remove(toLocation(it->import->firstSourceLocation(), it->import->lastSourceLocation()));
            }
        }

        return false;
    }

    void visitRootMember(UiObjectMember *rootMember) {
        if (UiObjectDefinition* root = AST::cast<UiObjectDefinition*>(rootMember)) {
            if (m_locations.contains(toLocation(root->firstSourceLocation(), root->lastSourceLocation()))) {
                m_paster->addNode(textAt(root->firstSourceLocation(), root->lastSourceLocation()));
            } else if (root->initializer) {
                for (UiObjectMemberList *it = root->initializer->members; it; it = it->next) {
                    if (!it->member)
                        continue;

                    if (UiArrayBinding *array = AST::cast<UiArrayBinding *>(it->member)) {
                        if (array->qualifiedId->name->asString() == "states") {
                            bool prevInStates = switchInStates(true);
                            Node::accept(array->members, this);
                            switchInStates(prevInStates);
                            continue;
                        }
                    }

                    Node::accept(it->member, this);
                }
            }
        }
    }

    bool visit(UiObjectDefinition *ast) {
        const SourceLocation start = ast->firstSourceLocation();
        const SourceLocation end = ast->lastSourceLocation();

        if (m_inStates) {
            if (ast->qualifiedTypeNameId && ast->qualifiedTypeNameId->name && ast->qualifiedTypeNameId->name->asString() == "State") {
                QString prevStateName = switchStateName(QString());
                Node::accept(ast->initializer, this);
                switchStateName(prevStateName);
                return false;
            } else if (ast->qualifiedTypeNameId && ast->qualifiedTypeNameId->name && ast->qualifiedTypeNameId->name->asString() == "PropertyChanges") {
                if (m_locations.remove(toLocation(start, end)))
                    m_paster->addNodeState(m_stateName, textAt(start, end));

                return false;
            } else {
                return true;
            }
        } else {
            if (m_locations.remove(toLocation(start, end)))
                m_paster->addNode(textAt(start, end));

            return false;
        }

        return true;
    }

    bool visit(UiObjectBinding *ast) {
        const SourceLocation start = ast->firstSourceLocation();
        const SourceLocation end = ast->lastSourceLocation();

        if (m_locations.remove(toLocation(start, end))) {
            m_paster->addNode(textAt(start, end));

            return false;
        }

        return true;
    }

    bool visit(UiScriptBinding *ast) {
        if (m_inStates && ast->qualifiedId && ast->qualifiedId->name && ast->qualifiedId->name->asString() == "name") {
            if (ExpressionStatement *stmt = AST::cast<ExpressionStatement *>(ast->statement)) {
                if (StringLiteral * str = AST::cast<StringLiteral *>(stmt->expression))
                    m_stateName = str->value->asString();
            }
        }

        return true;
    }
};

static inline void mergeNestedLocation(const TextLocation &newLocation, QList<TextLocation> &locations)
{
    for (int i = 0; i < locations.length(); ++i) {
        const TextLocation l = locations[i];

        if (l.contains(newLocation))
            return;

        if (newLocation.contains(l)) {
            locations.replace(i, newLocation);
            return;
        }
    }

    locations.append(newLocation);
}

static QList<TextLocation> mergeNestedLocation(const QList<TextLocation> &locations)
{
    QList<TextLocation> originals(locations);
    QList<TextLocation> result;

    if (originals.isEmpty())
        return result;

    result.append(originals.takeFirst());

    foreach (const TextLocation &l, originals) {
        mergeNestedLocation(l, result);
    }

    return result;
}

Paster::Paster(QMimeData *transferData, const InternalNode::Pointer &intoNode):
        m_targetNode(intoNode)
{
    Q_ASSERT(!intoNode.isNull() && intoNode->isValid());

    QList<TextLocation> locations = extractPasteData(transferData);

    m_pastedLocations = mergeNestedLocation(locations);
}

bool Paster::canPaste()
{
    if (m_pastedLocations.isEmpty() || m_pastedSource.isEmpty())
        return false;

    ParsedQML pastedQml(m_pastedSource, "<clipboard>");
    if (!pastedQml.isValid())
        return false;

    // TODO: expand this by checking if the paste locations can be found (esp. for states)
    return true;
}

bool Paster::doPaste(ModelToTextMerger &modelToTextMerger)
{
    if (m_pastedLocations.isEmpty() || m_pastedSource.isEmpty())
        return false;

    ParsedQML pastedQml(m_pastedSource, "<clipboard>");
    if (!pastedQml.isValid())
        return false;

    m_modelToTextMerger = &modelToTextMerger;

    ASTExtractor extractor(this, QSet<TextLocation>::fromList(m_pastedLocations), m_pastedSource);
    bool result = extractor(pastedQml.ast());

    m_modelToTextMerger = 0;
    return result;
}

void Paster::addImports(const Import &imports)
{
    m_modelToTextMerger->addImport(imports);
}

void Paster::addNode(const QString &nodeText)
{
    m_modelToTextMerger->pasteNode(m_targetNode->baseNodeState()->location(), nodeText);
}

void Paster::addNodeState(const QString &stateName, const QString &propertyChanges)
{
    foreach (const InternalNodeState::Pointer &nodeState, m_targetNode->nodeStates()) {
        const InternalModelState::Pointer modelState = nodeState->modelState();

        if (modelState->name() == stateName)
            m_modelToTextMerger->pastePropertyChanges(modelState, propertyChanges);
    }
}

QList<TextLocation> Paster::extractPasteData(QMimeData *transferData)
{
    QList<TextLocation> locations;

    if (!transferData)
        return locations;

    QByteArray data = transferData->data("application/x-qt-bauhaus");
    if (data.isEmpty())
        return locations;

    QDataStream is(&data, QIODevice::ReadOnly);

    int locationCount = 0;
    is >> locationCount;
    for (int i = 0; i < locationCount; ++i) {
        int position = 0, length = 0;
        is >> position;
        is >> length;
        locations.append(TextLocation(position, length));
    }

    is >> m_pastedSource;

    return locations;
}

} // namespace Internal
} // namespace QmlDesigner
