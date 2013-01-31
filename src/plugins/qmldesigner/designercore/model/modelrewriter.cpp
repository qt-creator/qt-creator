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

#include <QCoreApplication>

#include "copyhelper.h"
#include "invalidmodelnodeexception.h"
#include "invalidpropertyexception.h"
#include "internalnode_p.h"
#include "modelrewriter.h"
#include "modificationgroupexception.h"
#include "nodeproperty.h"
#include "paster.h"
#include "textmodifier.h"
#include "textlocation.h"

/*!
\defgroup Rewriting
*/
/*!
\class QmlDesigner::Internal::ModelRewriter
\ingroup Rewriting
\brief The ModelRewriter class is the facade interface to all rewriting classes for QML files.

The rewriting consists of two phases. First, any changes are applied to the QML source, as held by the TextModifier. This is done by the
TextToModelMerger. After that, the QTextDocument of the TextModifier will fire a textChanged() signal, which triggers the second phase. In this
phase, the ModelRewriter will use the ModelToTextMerger to parse the QML (using QmlDocDocument) and change the Model according to the new input.

The strategy of the Model is to first apply the changes to the datastructures itself, and then tell the ModelRewriter to apply the changes to the QML
source file. By doing it in this order, the second phase of the rewriting will usually not make any changes to the Model.

However, as with any good rule, there are two exceptions. The most common one is that the QTextDocument of the TextModifier is also used by a text
editor. When the user changes anything in the QML source text using that editor, reparsing will be triggered too, and the Model will be changed to
reflect the user's changes. The second exception to the rule is that an error can occur while re-parsing the QML. (Note that this can also happen
due to a change by a text editor.) In this case, the TextToModelMerger will try to revert the model to the last known correct state.

The ModelRewriter has one requirement to do the application of changes to the text correctly. It assumes that every change is coming in separately.
This means, when, for example, addNode for a rectangle is called, only "Rectangle {}" is added to the QML source text. If the node has properties set,
the rewriting will not magically decide that these need to be added. So to add a rectangle on a certain position with a certain size, the
appropriate calls to addProperty will have to be made too. If these changes need to be made in one "bigger" change to the QML source text, then a
modification group can be created. All the changes in the modification group are applied to the QML source text in such a way that the underlying
QTextDocument will see it as one single change.

\see QmlDesigner::Model
*/

namespace QmlDesigner {
namespace Internal {

ModelRewriter::ModelRewriter(Model* model, TextModifier* textModifier):
        m_subcomponentManager(model->metaInfo()),
        m_textModifier(textModifier),
        m_modelToTextMerger(model),
        m_textToModelMerger(model, &m_subcomponentManager),
        m_lastRewriteFailed(false)
{
    Q_ASSERT(model);
    Q_ASSERT(textModifier);

    connect(m_textModifier, SIGNAL(textChanged()), this, SLOT(mergeChanges()));
}

/*!
  Starting point for the second phase of the rewriting: this will parse the QML source text, and apply any changes to the Model.

  \param errors A pointer to a list which can hold the QmlError objects which describe errors while parsing the QML source text. Passing
         <code>null</code> will leave all errors unreported.
 */
bool ModelRewriter::reloadAndMerge(QList<QmlError> *errors)
{
    if (QCoreApplication::arguments().contains("--no-resync"))
        return false;

    QByteArray newData = m_textModifier->text().toUtf8();

    if (m_textToModelMerger.reloadAndMerge(newData, errors)) {
        m_lastCorrectData = newData;
        return true;
    } else {
        m_textToModelMerger.reloadAndMerge(m_lastCorrectData, 0);
        return false;
    }
}

/*!
  Starting point for the second phase of the rewriting: this will parse the QML source text, and apply any changes to the Model.

  \param url The URL of the input data, used to load any sub-components, and used for reporting parser errors.
  \param errors A pointer to a list which can hold the QmlError objects which describe errors while parsing the QML source text. Passing
         <code>null</code> will leave all errors unreported.
 */
bool ModelRewriter::load(const QUrl &url, QList<QmlError> *errors)
{
    QByteArray newData = m_textModifier->text().toUtf8();

    if (m_textToModelMerger.load(newData, url, errors)) {
        m_lastCorrectData = newData;
        return true;
    } else {
        return false;
    }
}

/*!
  Sets the URL of the input file, which is used to report parser errors, and to load sub-components.

  \note Changing the file URL will force a re-load of all sub-components.
 */
void ModelRewriter::setFileUrl(const QUrl& url)
{
    m_textToModelMerger.setFileUrl(url);
}

/**
  Changes the value of a property in the given state in the QML source text.

  \param state The state for which the property is changed.
  \param name The name of the property to change.
  \param value The new value for the property.

  \note Changing a property value can only be done after the property has been added to that state.
  \see {@link addProperty}
 */
void ModelRewriter::changePropertyValue(const InternalNodeState::Pointer& state, const QString& name, const QVariant& value)
{
    if (state->modelNode()->isReadOnly())
        return; //the rewriter ignores read only nodes
    if (state.isNull() || !state->isValid())
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    if (!modificationGroupActive() && !state->location().isValid())
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    m_modelToTextMerger.changePropertyValue(state, name, value);

    if (!modificationGroupActive())
        m_modelToTextMerger.applyChanges(*m_textModifier);
}

/**
  Adds a property with the given value to the given state in the QML source text.

  \param state The state to which the property is added.
  \param name The name of the property.
  \param value The value for the new property.

  \see {@link changePropertyValue}
 */
void ModelRewriter::addProperty(const InternalNodeState::Pointer& state, const QString& name, const QVariant& value)
{
    if (state.isNull() || !state->isValid())
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

//    qDebug() << "add property" << name << "in node" << state->modelNode()->id() << "for state" << state->modelState()->name();

    m_modelToTextMerger.addProperty(state, name, value);

    if (!modificationGroupActive())
        m_modelToTextMerger.applyChanges(*m_textModifier);
}

/**
  Removes a property from the given state in the QML source text.

  \param state The state from which to remove the property.
  \param name The name of the property to remove.
 */
void ModelRewriter::removeProperty(const InternalNodeState::Pointer& state, const QString& name)
{
    if (state.isNull() || !state->isValid())
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

//    qDebug() << "remove property" << name << "in node" << state->modelNode()->id() << "for state" << state->modelState()->name();

    m_modelToTextMerger.removeProperty(state, name);

    if (!modificationGroupActive())
        m_modelToTextMerger.applyChanges(*m_textModifier);
}

void ModelRewriter::changeNodeId(const InternalNodePointer& node, const QString& id)
{
    if (node.isNull() || !node->isValid())
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    m_modelToTextMerger.changeNodeId(node, id);

    if (!modificationGroupActive())
        m_modelToTextMerger.applyChanges(*m_textModifier);
}

/**
  Move the node after the given afterNode.

  \param node the node to move
  \param afterNode the node before which the node is put. Pass in InternalNodePointer() to move the node to the end.
 */
void ModelRewriter::moveNodeBefore(const InternalNodePointer& node, const InternalNodePointer& beforeNode)
{
    if (node.isNull() || !node->isValid())
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    if (!modificationGroupActive() && !node->baseNodeState()->location().isValid())
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    if (!beforeNode.isNull() && beforeNode->isValid())
        if (!modificationGroupActive() && !beforeNode->baseNodeState()->location().isValid())
            throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    m_modelToTextMerger.moveNodeBefore(node, beforeNode);

    if (!modificationGroupActive())
        m_modelToTextMerger.applyChanges(*m_textModifier);
}

void ModelRewriter::addModelState(const InternalModelState::Pointer &state)
{
    if (state.isNull() || !state->isValid())
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    m_modelToTextMerger.addModelState(state);

    if (!modificationGroupActive())
        m_modelToTextMerger.applyChanges(*m_textModifier);
}

void ModelRewriter::removeModelState(const InternalModelState::Pointer &state)
{
    m_modelToTextMerger.removeModelState(state);

    if (!modificationGroupActive())
        m_modelToTextMerger.applyChanges(*m_textModifier);
}

void ModelRewriter::setStateName(const InternalModelStatePointer &state, const QString &name)
{
    if (state.isNull() || !state->isValid())
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    if (!modificationGroupActive() && !state->location().isValid())
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    m_modelToTextMerger.setStateName(state, name);

    if (!modificationGroupActive())
        m_modelToTextMerger.applyChanges(*m_textModifier);
}

void ModelRewriter::setStateWhenCondition(const InternalModelStatePointer &state, const QString &whenCondition)
{
    if (state.isNull() || !state->isValid())
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    if (!modificationGroupActive() && !state->location().isValid())
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    m_modelToTextMerger.setStateWhenCondition(state, whenCondition);

    if (!modificationGroupActive())
        m_modelToTextMerger.applyChanges(*m_textModifier);
}

void ModelRewriter::setAnchor(const InternalNodeState::Pointer &state, const QString &propertyName, const QVariant &value)
{
    if (state.isNull() || !state->isValid())
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    if (state->propertyLocation(propertyName).isValid())
        m_modelToTextMerger.changePropertyValue(state, propertyName, value);
    else
        m_modelToTextMerger.addProperty(state, propertyName, value);

    if (!modificationGroupActive())
        m_modelToTextMerger.applyChanges(*m_textModifier);
}

void ModelRewriter::removeAnchor(const InternalNodeState::Pointer &state, const QString &propertyName)
{
    if (state.isNull() || !state->isValid())
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    m_modelToTextMerger.removeProperty(state, propertyName);

    if (!modificationGroupActive())
        m_modelToTextMerger.applyChanges(*m_textModifier);
}

void ModelRewriter::setAnchorMargin(const InternalNodeState::Pointer &state, const QString &propertyName, const QVariant &value)
{
    if (state.isNull() || !state->isValid())
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    if (state->propertyLocation(propertyName).isValid()) {
        if (value.toDouble() == 0.0 && state->isBaseState())
            m_modelToTextMerger.removeProperty(state, propertyName);
        else
            m_modelToTextMerger.changePropertyValue(state, propertyName, value);
    } else {
        if (value.toDouble() == 0.0 && state->isBaseState()) {
            // do nothing
        } else {
            m_modelToTextMerger.addProperty(state, propertyName, value);
        }
    }

    if (!modificationGroupActive())
        m_modelToTextMerger.applyChanges(*m_textModifier);
}

void ModelRewriter::addNode(const InternalNodePointer& newNode, const InternalNodePointer& parentNode)
{
    if (newNode.isNull() || !newNode->isValid())
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    if (!modificationGroupActive() && !parentNode->baseNodeState()->location().isValid())
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    m_modelToTextMerger.addNode(newNode, parentNode);

    if (!modificationGroupActive())
        m_modelToTextMerger.applyChanges(*m_textModifier);
}

void ModelRewriter::removeNode(const InternalNodePointer &node)
{
    if (node.isNull() || !node->isValid())
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    if (!modificationGroupActive() && !node->baseNodeState()->location().isValid())
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    m_modelToTextMerger.removeNode(node);

    if (!modificationGroupActive())
        m_modelToTextMerger.applyChanges(*m_textModifier);
}

void ModelRewriter::reparentNode(const InternalNodePointer &child, const InternalNodePointer &newParent)
{
    if (child.isNull() || !child->isValid())
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);
    if (newParent.isNull() || !newParent->isValid())
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    if (!modificationGroupActive() && !newParent->baseNodeState()->location().isValid())
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    m_modelToTextMerger.reparentNode(child, newParent);

    if (!modificationGroupActive())
        m_modelToTextMerger.applyChanges(*m_textModifier);
}

ModificationGroupToken ModelRewriter::beginModificationGroup()
{
    ModificationGroupToken newToken(m_activeModificationGroups.size());
    m_activeModificationGroups.append(newToken);

    return newToken;
}

void ModelRewriter::endModificationGroup(const ModificationGroupToken& token)
{
    if (m_activeModificationGroups.isEmpty())
        throw ModificationGroupException(__LINE__, Q_FUNC_INFO, __FILE__);

    if (m_activeModificationGroups.last() != token)
        throw ModificationGroupException(__LINE__, Q_FUNC_INFO, __FILE__);

    if (!m_activeModificationGroups.removeAll(token))
        throw ModificationGroupException(__LINE__, Q_FUNC_INFO, __FILE__);

    if (!modificationGroupActive())
        m_modelToTextMerger.applyChanges(*m_textModifier);
}

bool ModelRewriter::lastRewriteFailed() const
{
    return m_lastRewriteFailed;
}

QMimeData *ModelRewriter::copy(const QList<InternalNodeState::Pointer> &nodeStates) const
{
    if (modificationGroupActive())
        throw ModificationGroupException(__LINE__, Q_FUNC_INFO, __FILE__);

    const QString sourceCode = m_textModifier->text();
    CopyHelper copyHelper(sourceCode);
    return copyHelper.createMimeData(nodeStates);
}

/*!
  Inserts the copied data into the given node.

  \note The mime-type for the data must be <tt>"application/x-qt-bauhaus"</tt>, otherwise the paste will fail.

  \param transferData The clip-board contents to paste.
  \param intoNode The node into which the clip-board contents are to be pasted.
  \return true if successful, false otherwise.
 */
bool ModelRewriter::paste(QMimeData *transferData, const InternalNode::Pointer &intoNode)
{
    if (!transferData)
        return false;

    if (intoNode.isNull() || !intoNode->isValid())
        throw InvalidModelNodeException(__LINE__, Q_FUNC_INFO, __FILE__);

    Paster paster(transferData, intoNode);
    if (paster.doPaste(m_modelToTextMerger)) {
        if (!modificationGroupActive())
            m_modelToTextMerger.applyChanges(*m_textModifier);
        return true;
    } else {
        m_modelToTextMerger.clear();
        return false;
    }
}

/*!
  Adds the given import to the QML file, if it doesn't already exist.

  \param import The import to add.
 */
void ModelRewriter::addImport(const Import &import)
{
    m_modelToTextMerger.addImport(import);

    if (!modificationGroupActive())
        m_modelToTextMerger.applyChanges(*m_textModifier);
}

/*!
  Removes the given import from the QML file.

  \param import The import to remove.
 */
void ModelRewriter::removeImport(const Import &import)
{
    m_modelToTextMerger.removeImport(import);

    if (!modificationGroupActive())
        m_modelToTextMerger.applyChanges(*m_textModifier);
}

/*!
  Slot connected to the textChanged() signal of the underlying QTextDocument. This slot will start the second part of the rewriting phase, being
  the part where the QML document is parsed and any changes being merged into the Model.

  Any errors that occur during this phase will be reported by emitting the errorDuringRewrite() signal.
 */
void ModelRewriter::mergeChanges()
{
//    qDebug() << "*** Running rewrite...";

    m_lastRewriteFailed = false;

    QList<QmlError> errors;
    if (reloadAndMerge(&errors)) {
//        qDebug() << "===> Roundtrip done";
    } else {
        qDebug() << "===> Error doing round-trip";
        qDebug() << "new data:" << m_textModifier->text().toUtf8();
        qDebug() << "old data:" << m_lastCorrectData;
        m_lastRewriteFailed = true;
        emit errorDuringRewrite(errors);
    }
}

/*!
  \fn void ModelRewriter::errorDuringRewrite(const QList<QmlError> &errors)
  Emitted when an error when reading or merging changes back after the underlying text (-document) changed.

  \note This signal can be emitted after a modification by from within the application, or by a change done by a text editor which uses the same
        QTextDocument.
 */

}
}
