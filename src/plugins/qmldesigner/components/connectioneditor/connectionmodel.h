// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <connectioneditorstatements.h>
#include <propertytreemodel.h>
#include <studioquickwidget.h>

#include <QAbstractListModel>
#include <QStandardItemModel>

namespace QmlDesigner {

class AbstractProperty;
class ModelNode;
class BindingProperty;
class SignalHandlerProperty;
class VariantProperty;

class ConnectionView;
class ConnectionModelBackendDelegate;

class ConnectionModel : public QStandardItemModel
{
    Q_OBJECT

    Q_PROPERTY(ConnectionModelBackendDelegate *delegate READ delegate CONSTANT)

public:
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)

public:
    enum ColumnRoles {
        TargetModelNodeRow = 0,
        TargetPropertyNameRow = 1,
        SourceRow = 2
    };
    enum UserRoles {
        InternalIdRole = Qt::UserRole + 1,
        TargetPropertyNameRole,
        TargetNameRole,
        ActionTypeRole
    };
    ConnectionModel(ConnectionView *parent = nullptr);

    Qt::ItemFlags flags(const QModelIndex &modelIndex) const override;

    void resetModel();
    SignalHandlerProperty signalHandlerPropertyForRow(int rowNumber) const;
    ConnectionView *connectionView() const;

    QStringList getSignalsForRow(int row) const;
    QStringList getflowActionTriggerForRow(int row) const;
    ModelNode getTargetNodeForConnection(const ModelNode &connection) const;

    void addConnection();

    void bindingPropertyChanged(const BindingProperty &bindingProperty);
    void variantPropertyChanged(const VariantProperty &variantProperty);
    void abstractPropertyChanged(const AbstractProperty &abstractProperty);

    void deleteConnectionByRow(int currentRow);
    void removeRowFromTable(const SignalHandlerProperty &property);

    Q_INVOKABLE void add();
    Q_INVOKABLE void remove(int row);

    void setCurrentIndex(int i);
    int currentIndex() const;

    void selectProperty(const SignalHandlerProperty &property);

    void nodeAboutToBeRemoved(const ModelNode &removedNode);
    void modelAboutToBeDetached();

signals:
    void currentIndexChanged();

protected:
    void addModelNode(const ModelNode &modelNode);
    void addConnection(const ModelNode &modelNode);
    void addSignalHandler(const SignalHandlerProperty &bindingProperty);
    void removeModelNode(const ModelNode &modelNode);
    void removeConnection(const ModelNode &modelNode);
    void updateSource(int row);
    void updateSignalName(int rowNumber);
    void updateTargetNode(int rowNumber);
    void updateCustomData(QStandardItem *item, const SignalHandlerProperty &signalHandlerProperty);
    QStringList getPossibleSignalsForConnection(const ModelNode &connection) const;

    QHash<int, QByteArray> roleNames() const override;

private:
    void handleDataChanged(const QModelIndex &topLeft, const QModelIndex& bottomRight);
    void handleException();
    ConnectionModelBackendDelegate *delegate() const;

private:
    ConnectionView *m_connectionView;
    bool m_lock = false;
    QString m_exceptionError;
    ConnectionModelBackendDelegate *m_delegate = nullptr;
    int m_currentIndex = -1;
};

class ConditionListModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(bool valid READ valid NOTIFY validChanged)
    Q_PROPERTY(bool empty READ empty NOTIFY emptyChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
    Q_PROPERTY(int errorIndex READ errorIndex NOTIFY errorIndexChanged)

public:
    enum ConditionType { Intermediate, Invalid, Operator, Literal, Variable, Shadow };
    Q_ENUM(ConditionType)

    struct ConditionToken
    {
        ConditionType type;
        QString value;
    };

    ConditionListModel(ConnectionModel *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QHash<int, QByteArray> roleNames() const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void setup();
    void setCondition(ConnectionEditorStatements::MatchedCondition &condition);
    ConnectionEditorStatements::MatchedCondition &condition();

    static ConditionToken tokenFromConditionToken(
        const ConnectionEditorStatements::ConditionToken &token);

    static ConditionToken tokenFromComparativeStatement(
        const ConnectionEditorStatements::ComparativeStatement &token);

    Q_INVOKABLE void insertToken(int index, const QString &value);
    Q_INVOKABLE void updateToken(int index, const QString &value);
    Q_INVOKABLE void appendToken(const QString &value);
    Q_INVOKABLE void removeToken(int index);

    Q_INVOKABLE void insertIntermediateToken(int index, const QString &value);
    Q_INVOKABLE void insertShadowToken(int index, const QString &value);
    Q_INVOKABLE void setShadowToken(int index, const QString &value);

    bool valid() const;
    bool empty() const;

    //for debugging
    Q_INVOKABLE void command(const QString &string);

    void setInvalid(const QString &errorMessage, int index = -1);
    void setValid();

    QString error() const;
    int errorIndex() const;

    Q_INVOKABLE bool operatorAllowed(int cursorPosition);

signals:
    void validChanged();
    void emptyChanged();
    void conditionChanged();
    void errorChanged();
    void errorIndexChanged();

private:
    void internalSetup();
    ConditionToken valueToToken(const QString &value);
    void resetModel();
    int checkOrder() const;
    void validateAndRebuildTokens();
    void rebuildTokens();

    ConnectionEditorStatements::ConditionToken toOperatorStatement(const ConditionToken &token);
    ConnectionEditorStatements::ComparativeStatement toStatement(const ConditionToken &token);

    ConnectionModel *m_connectionModel = nullptr;
    ConnectionEditorStatements::MatchedCondition &m_condition;
    QList<ConditionToken> m_tokens;
    bool m_valid = false;
    QString m_errorMessage;
    int m_errorIndex = -1;
};

class ConnectionModelStatementDelegate : public QObject
{
    Q_OBJECT

public:
    explicit ConnectionModelStatementDelegate(ConnectionModel *parent = nullptr);

    enum ActionType { CallFunction, Assign, ChangeState, SetProperty, PrintMessage, Custom };

    Q_ENUM(ActionType)

    Q_PROPERTY(ActionType actionType READ actionType NOTIFY actionTypeChanged)

    Q_PROPERTY(PropertyTreeModelDelegate *function READ function CONSTANT)
    Q_PROPERTY(PropertyTreeModelDelegate *lhs READ lhs CONSTANT)
    Q_PROPERTY(PropertyTreeModelDelegate *rhsAssignment READ rhsAssignment CONSTANT)
    Q_PROPERTY(StudioQmlTextBackend *stringArgument READ stringArgument CONSTANT)
    Q_PROPERTY(StudioQmlComboBoxBackend *states READ states CONSTANT)
    Q_PROPERTY(StudioQmlComboBoxBackend *stateTargets READ stateTargets CONSTANT)

    void setActionType(ActionType type);
    void setup();
    void setStatement(ConnectionEditorStatements::MatchedStatement &statement);
    ConnectionEditorStatements::MatchedStatement &statement();

signals:
    void actionTypeChanged();
    void statementChanged();

private:
    ActionType actionType() const;
    PropertyTreeModelDelegate *signal();
    PropertyTreeModelDelegate *function();
    PropertyTreeModelDelegate *lhs();
    PropertyTreeModelDelegate *rhsAssignment();
    StudioQmlTextBackend *stringArgument();
    StudioQmlComboBoxBackend *stateTargets();
    StudioQmlComboBoxBackend *states();

    void handleFunctionChanged();
    void handleLhsChanged();
    void handleRhsAssignmentChanged();
    void handleStringArgumentChanged();
    void handleStateChanged();
    void handleStateTargetsChanged();

    void setupAssignment();
    void setupSetProperty();
    void setupCallFunction();
    void setupChangeState();
    void setupStates();
    void setupPrintMessage();
    void setupPropertyType();
    QString baseStateName() const;

    ActionType m_actionType;
    PropertyTreeModelDelegate m_functionDelegate;
    PropertyTreeModelDelegate m_lhsDelegate;
    PropertyTreeModelDelegate m_rhsAssignmentDelegate;
    ConnectionEditorStatements::MatchedStatement &m_statement;
    ConnectionModel *m_model = nullptr;
    StudioQmlTextBackend m_stringArgument;
    StudioQmlComboBoxBackend m_stateTargets;
    StudioQmlComboBoxBackend m_states;
};

class ConnectionModelBackendDelegate : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int currentRow READ currentRow WRITE setCurrentRow NOTIFY currentRowChanged)

    Q_PROPERTY(ActionType actionType READ actionType NOTIFY actionTypeChanged)
    Q_PROPERTY(PropertyTreeModelDelegate *signal READ signal CONSTANT)
    Q_PROPERTY(ConnectionModelStatementDelegate *okStatement READ okStatement CONSTANT)
    Q_PROPERTY(ConnectionModelStatementDelegate *koStatement READ koStatement CONSTANT)
    Q_PROPERTY(ConditionListModel *conditionListModel READ conditionListModel CONSTANT)
    Q_PROPERTY(bool hasCondition READ hasCondition NOTIFY hasConditionChanged)
    Q_PROPERTY(bool hasElse READ hasElse NOTIFY hasElseChanged)
    Q_PROPERTY(QString source READ source NOTIFY sourceChanged)
    Q_PROPERTY(QString indentedSource READ indentedSource NOTIFY sourceChanged)

    Q_PROPERTY(PropertyTreeModel *propertyTreeModel READ propertyTreeModel CONSTANT)
    Q_PROPERTY(PropertyListProxyModel *propertyListProxyModel READ propertyListProxyModel CONSTANT)

public:
    explicit ConnectionModelBackendDelegate(ConnectionModel *parent = nullptr);

    using ActionType = ConnectionModelStatementDelegate::ActionType;

    Q_INVOKABLE void changeActionType(
        QmlDesigner::ConnectionModelStatementDelegate::ActionType actionType);

    Q_INVOKABLE void addCondition();
    Q_INVOKABLE void removeCondition();

    Q_INVOKABLE void addElse();
    Q_INVOKABLE void removeElse();

    Q_INVOKABLE void setNewSource(const QString &newSource);

    void setCurrentRow(int i);
    void update();

signals:
    void currentRowChanged();
    void actionTypeChanged();
    void hasConditionChanged();
    void hasElseChanged();
    void sourceChanged();
    void popupShouldClose();

private:
    int currentRow() const;
    void handleException();
    bool hasCondition() const;
    bool hasElse() const;
    void setHasCondition(bool b);
    void setHasElse(bool b);
    ActionType actionType() const;
    PropertyTreeModelDelegate *signal();
    ConnectionModelStatementDelegate *okStatement();
    ConnectionModelStatementDelegate *koStatement();
    ConditionListModel *conditionListModel();
    QString indentedSource() const;
    QString source() const;
    void setSource(const QString &source);

    PropertyTreeModel *propertyTreeModel();
    PropertyListProxyModel *propertyListProxyModel();

    void setupCondition();
    void setupHandlerAndStatements();

    void handleTargetChanged();
    void handleOkStatementChanged();
    void handleKOStatementChanged();
    void handleConditionChanged();

    void commitNewSource(const QString &source);

    ActionType m_actionType;
    QString m_exceptionError;
    int m_currentRow = -1;
    ConnectionEditorStatements::Handler m_handler;
    PropertyTreeModelDelegate m_signalDelegate;
    ConnectionModelStatementDelegate m_okStatementDelegate;
    ConnectionModelStatementDelegate m_koStatementDelegate;
    ConditionListModel m_conditionListModel;
    bool m_hasCondition = false;
    bool m_hasElse = false;
    QString m_source;
    PropertyTreeModel m_propertyTreeModel;
    PropertyListProxyModel m_propertyListProxyModel;
    bool m_blockReflection = false;
};

} // namespace QmlDesigner
