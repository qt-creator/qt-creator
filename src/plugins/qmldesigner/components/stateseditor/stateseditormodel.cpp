// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "stateseditormodel.h"
#include "stateseditorview.h"

#include <bindingproperty.h>
#include <modelnode.h>
#include <nodelistproperty.h>
#include <nodemetainfo.h>
#include <rewriterview.h>
#include <variantproperty.h>

#include <coreplugin/icore.h>
#include <coreplugin/messagebox.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QDebug>
#include <QWidget>

enum {
    debug = false
};

namespace QmlDesigner {

StatesEditorModel::StatesEditorModel(StatesEditorView *view)
    : QAbstractListModel(view)
    , m_statesEditorView(view)
    , m_hasExtend(false)
    , m_extendedStates()
{
    QObject::connect(this, &StatesEditorModel::dataChanged, [this]() { emit baseStateChanged(); });
}

int StatesEditorModel::count() const
{
    return rowCount();
}

QModelIndex StatesEditorModel::index(int row, int column, const QModelIndex &parent) const
{
    if (m_statesEditorView.isNull())
        return {};

    int internalNodeId = 0;
    if (row > 0)
        internalNodeId = m_statesEditorView->activeStatesGroupNode()
                             .nodeListProperty("states")
                             .at(row - 1)
                             .internalId();

    return hasIndex(row, column, parent) ? createIndex(row, column, internalNodeId) : QModelIndex();
}

int StatesEditorModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid() || m_statesEditorView.isNull() || !m_statesEditorView->model())
        return 0;

    if (!m_statesEditorView->activeStatesGroupNode().hasNodeListProperty("states"))
        return 1; // base state

    return m_statesEditorView->activeStatesGroupNode().nodeListProperty("states").count() + 1;
}

void StatesEditorModel::reset()
{
    QAbstractListModel::beginResetModel();
    QAbstractListModel::endResetModel();

    evaluateExtend();
    emit baseStateChanged();
}

QVariant StatesEditorModel::data(const QModelIndex &index, int role) const
{
    if (index.parent().isValid() || index.column() != 0 || m_statesEditorView.isNull()
        || !m_statesEditorView->hasModelNodeForInternalId(index.internalId()))
        return QVariant();

    ModelNode stateNode;

    if (index.internalId() > 0)
        stateNode = m_statesEditorView->modelNodeForInternalId(index.internalId());

    switch (role) {
    case StateNameRole: {
        if (index.row() == 0) {
            return tr("base state", "Implicit default state");
        } else {
            if (stateNode.hasVariantProperty("name"))
                return stateNode.variantProperty("name").value();
            else
                return QVariant();
        }
    }

    case StateImageSourceRole: {
        static int randomNumber = 0;
        randomNumber++;
        if (index.row() == 0)
            return QString("image://qmldesigner_stateseditor/baseState-%1").arg(randomNumber);
        else
            return QString("image://qmldesigner_stateseditor/%1-%2").arg(index.internalId()).arg(randomNumber);
    }

    case InternalNodeId:
        return index.internalId();

    case HasWhenCondition:
        return stateNode.isValid() && stateNode.hasProperty("when");

    case WhenConditionString: {
        if (stateNode.isValid() && stateNode.hasBindingProperty("when"))
            return stateNode.bindingProperty("when").expression();
        else
            return QString();
    }

    case IsDefault: {
        QmlModelState modelState(stateNode);
        if (modelState.isValid())
            return modelState.isDefault();
        return false;
    }

    case ModelHasDefaultState:
        return hasDefaultState();

    case HasExtend:
        return stateNode.isValid() && stateNode.hasProperty("extend");

    case ExtendString: {
        if (stateNode.isValid() && stateNode.hasVariantProperty("extend"))
            return stateNode.variantProperty("extend").value();
        else
            return QString();
    }
    }

    return QVariant();
}

QHash<int, QByteArray> StatesEditorModel::roleNames() const
{
    static QHash<int, QByteArray> roleNames{{StateNameRole, "stateName"},
                                            {StateImageSourceRole, "stateImageSource"},
                                            {InternalNodeId, "internalNodeId"},
                                            {HasWhenCondition, "hasWhenCondition"},
                                            {WhenConditionString, "whenConditionString"},
                                            {IsDefault, "isDefault"},
                                            {ModelHasDefaultState, "modelHasDefaultState"},
                                            {HasExtend, "hasExtend"},
                                            {ExtendString, "extendString"}};
    return roleNames;
}

void StatesEditorModel::insertState(int stateIndex)
{
    if (stateIndex >= 0) {
        const int updateIndex = stateIndex + 1;
        beginInsertRows(QModelIndex(), updateIndex, updateIndex);
        endInsertRows();

        emit dataChanged(index(updateIndex, 0), index(updateIndex, 0));
    }
}

void StatesEditorModel::updateState(int beginIndex, int endIndex)
{
    if (beginIndex >= 0 && endIndex >= 0)
        emit dataChanged(index(beginIndex, 0), index(endIndex, 0));
}

void StatesEditorModel::removeState(int stateIndex)
{
    if (stateIndex >= 0) {
        beginRemoveRows(QModelIndex(), 0, stateIndex);
        endRemoveRows();
    }
}

void StatesEditorModel::renameState(int internalNodeId, const QString &newName)
{
    if (newName == m_statesEditorView->currentStateName())
        return;

    if (newName.isEmpty() ||! m_statesEditorView->validStateName(newName)) {
        QTimer::singleShot(0, this, [newName] {
            Core::AsynchronousMessageBox::warning(
                        tr("Invalid state name"),
                        newName.isEmpty() ?
                            tr("The empty string as a name is reserved for the base state.") :
                            tr("Name already used in another state"));
        });
        reset();
    } else {
        m_statesEditorView->renameState(internalNodeId, newName);
    }
}

void StatesEditorModel::setWhenCondition(int internalNodeId, const QString &condition)
{
    m_statesEditorView->setWhenCondition(internalNodeId, condition);
}

void StatesEditorModel::resetWhenCondition(int internalNodeId)
{
    m_statesEditorView->resetWhenCondition(internalNodeId);
}

QStringList StatesEditorModel::autoComplete(const QString &text, int pos, bool explicitComplete)
{
    Model *model = m_statesEditorView->model();
    if (model && model->rewriterView())
        return model->rewriterView()->autoComplete(text, pos, explicitComplete);

    return {};
}

QVariant StatesEditorModel::stateModelNode(int internalNodeId)
{
    if (!m_statesEditorView->model())
        return QVariant();

    ModelNode node = m_statesEditorView->modelNodeForInternalId(internalNodeId);

    return QVariant::fromValue(m_statesEditorView->modelNodeForInternalId(internalNodeId));
}

void StatesEditorModel::setStateAsDefault(int internalNodeId)
{
    m_statesEditorView->setStateAsDefault(internalNodeId);
}

void StatesEditorModel::resetDefaultState()
{
    m_statesEditorView->resetDefaultState();
}

bool StatesEditorModel::hasDefaultState() const
{
    return m_statesEditorView->hasDefaultState();
}

void StatesEditorModel::setAnnotation(int internalNodeId)
{
    m_statesEditorView->setAnnotation(internalNodeId);
}

void StatesEditorModel::removeAnnotation(int internalNodeId)
{
    m_statesEditorView->removeAnnotation(internalNodeId);
}

bool StatesEditorModel::hasAnnotation(int internalNodeId) const
{
    return m_statesEditorView->hasAnnotation(internalNodeId);
}

QStringList StatesEditorModel::stateGroups() const
{
    if (!m_statesEditorView->isAttached())
        return {};

    const auto groupMetaInfo = m_statesEditorView->model()->qtQuickStateGroupMetaInfo();

    auto stateGroups = Utils::transform(m_statesEditorView->allModelNodesOfType(groupMetaInfo),
                                        [](const ModelNode &node) { return node.displayName(); });
    stateGroups.prepend(tr("Default"));
    return stateGroups;
}

QString StatesEditorModel::activeStateGroup() const
{
    if (auto stateGroup = m_statesEditorView->activeStatesGroupNode())
        return stateGroup.displayName();

    return {};
}

void StatesEditorModel::setActiveStateGroup(const QString &name)
{
    if (!m_statesEditorView->isAttached())
        return;

    const auto groupMetaInfo = m_statesEditorView->model()->qtQuickStateGroupMetaInfo();

    auto modelNode = Utils::findOrDefault(m_statesEditorView->allModelNodesOfType(groupMetaInfo),
                                          [&name](const ModelNode &node) {
                                              return node.displayName() == name;
                                          });

    QTC_ASSERT(!modelNode.isValid(), return );

    if (modelNode.isValid())
        m_statesEditorView->setActiveStatesGroupNode(modelNode);
}

int StatesEditorModel::activeStateGroupIndex() const
{
    return m_statesEditorView->activeStatesGroupIndex();
}

void StatesEditorModel::setActiveStateGroupIndex(int index)
{
    m_statesEditorView->setActiveStatesGroupIndex(index);
}

bool StatesEditorModel::renameActiveStateGroup(const QString &name)
{
    auto stateGroup = m_statesEditorView->activeStatesGroupNode();

    if (!stateGroup.isValid() || stateGroup.isRootNode())
        return false;

    if (!QmlDesigner::ModelNode::isValidId(name) || m_statesEditorView->hasId(name)) {
        QString errMsg = QmlDesigner::ModelNode::getIdValidityErrorMessage(name);
        if (!errMsg.isEmpty())
            Core::AsynchronousMessageBox::warning(tr("Invalid ID"), errMsg);
        else
            Core::AsynchronousMessageBox::warning(tr("Invalid ID"),
                                                  tr("%1 already exists.").arg(name));
        return false;
    }

    stateGroup.setIdWithRefactoring(name);
    emit stateGroupsChanged();
    return true;
}

void StatesEditorModel::addStateGroup(const QString &name)
{
    m_statesEditorView->executeInTransaction("createStateGroup", [this, name]() {
        const TypeName typeName = "QtQuick.StateGroup";
        auto metaInfo = m_statesEditorView->model()->metaInfo(typeName);
        int minorVersion = metaInfo.minorVersion();
        int majorVersion = metaInfo.majorVersion();
        auto stateGroupNode = m_statesEditorView->createModelNode(typeName,
                                                                  majorVersion,
                                                                  minorVersion);
        stateGroupNode.setIdWithoutRefactoring(m_statesEditorView->model()->generateNewId(name));

        m_statesEditorView->rootModelNode().defaultNodeAbstractProperty().reparentHere(
            stateGroupNode);
        m_statesEditorView->setActiveStatesGroupNode(stateGroupNode);
    });
}

void StatesEditorModel::removeStateGroup()
{
    if (m_statesEditorView->activeStatesGroupNode().isRootNode())
        return;

    m_statesEditorView->executeInTransaction("removeStateGroup", [this]() {
        m_statesEditorView->activeStatesGroupNode().destroy();
    });
}

QVariantMap StatesEditorModel::get(int idx) const
{
    const QHash<int, QByteArray> &names = roleNames();
    QHash<int, QByteArray>::const_iterator i = names.constBegin();

    QVariantMap res;
    QModelIndex modelIndex = index(idx);

    while (i != names.constEnd()) {
        QVariant data = modelIndex.data(i.key());

        res[QString::fromUtf8(i.value())] = data;
        ++i;
    }
    return res;
}

QVariantMap StatesEditorModel::baseState() const
{
    return get(0);
}

bool StatesEditorModel::hasExtend() const
{
    return m_hasExtend;
}

QStringList StatesEditorModel::extendedStates() const
{
    return m_extendedStates;
}

void StatesEditorModel::move(int from, int to)
{
    // This does not alter the code (rewriter) which means the reordering is not presistent

    if (from == to)
        return;

    int specialIndex = (from < to ? to + 1 : to);
    beginMoveRows(QModelIndex(), from, from, QModelIndex(), specialIndex);
    endMoveRows();
}

void StatesEditorModel::drop(int from, int to)
{
    m_statesEditorView->moveStates(from, to);
}

void StatesEditorModel::evaluateExtend()
{
    bool hasExtend = m_statesEditorView->hasExtend();

    if (m_hasExtend != hasExtend) {
        m_hasExtend = hasExtend;
        emit hasExtendChanged();
    }

    auto extendedStates = m_statesEditorView->extendedStates();

    if (extendedStates.size() != m_extendedStates.size()) {
        m_extendedStates = extendedStates;
        emit extendedStatesChanged();
        return;
    }

    for (int i = 0; i != m_extendedStates.size(); ++i) {
        if (extendedStates[i] != m_extendedStates[i]) {
            m_extendedStates = extendedStates;
            emit extendedStatesChanged();
            return;
        }
    }
}

bool StatesEditorModel::canAddNewStates() const
{
    return m_canAddNewStates;
}

void StatesEditorModel::setCanAddNewStates(bool b)
{
    if (b == m_canAddNewStates)
        return;

    m_canAddNewStates = b;

    emit canAddNewStatesChanged();
}

} // namespace QmlDesigner
