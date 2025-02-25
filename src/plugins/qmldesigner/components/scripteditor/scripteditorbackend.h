// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "propertytreemodel.h"
#include "scripteditorstatements.h"

namespace QmlDesigner {

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

    ConditionListModel(AbstractView *view);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QHash<int, QByteArray> roleNames() const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void setup();
    void setCondition(ScriptEditorStatements::MatchedCondition &condition);
    ScriptEditorStatements::MatchedCondition &condition();

    static ConditionToken tokenFromConditionToken(const ScriptEditorStatements::ConditionToken &token);

    static ConditionToken tokenFromComparativeStatement(
        const ScriptEditorStatements::ComparativeStatement &token);

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

    ScriptEditorStatements::ConditionToken toOperatorStatement(const ConditionToken &token);
    ScriptEditorStatements::ComparativeStatement toStatement(const ConditionToken &token);

    AbstractView *m_view = nullptr;
    ScriptEditorStatements::MatchedCondition &m_condition;
    QList<ConditionToken> m_tokens;
    bool m_valid = false;
    QString m_errorMessage;
    int m_errorIndex = -1;
};

class StatementDelegate : public QObject
{
    Q_OBJECT

public:
    explicit StatementDelegate(AbstractView *view);

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
    void setStatement(ScriptEditorStatements::MatchedStatement &statement);
    ScriptEditorStatements::MatchedStatement &statement();

signals:
    void actionTypeChanged();
    void statementChanged();

private:
    ActionType actionType() const;
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
    ScriptEditorStatements::MatchedStatement &m_statement;
    AbstractView *m_view = nullptr;
    StudioQmlTextBackend m_stringArgument;
    StudioQmlComboBoxBackend m_stateTargets;
    StudioQmlComboBoxBackend m_states;
};

class ScriptEditorBackend : public QObject
{
    Q_OBJECT
    Q_PROPERTY(ActionType actionType READ actionType NOTIFY actionTypeChanged)
    Q_PROPERTY(StatementDelegate *okStatement READ okStatement CONSTANT)
    Q_PROPERTY(StatementDelegate *koStatement READ koStatement CONSTANT)
    Q_PROPERTY(ConditionListModel *conditionListModel READ conditionListModel CONSTANT)
    Q_PROPERTY(bool hasCondition READ hasCondition NOTIFY hasConditionChanged)
    Q_PROPERTY(bool hasElse READ hasElse NOTIFY hasElseChanged)
    Q_PROPERTY(QString source READ source NOTIFY sourceChanged)
    Q_PROPERTY(QString indentedSource READ indentedSource NOTIFY sourceChanged)

    Q_PROPERTY(PropertyTreeModel *propertyTreeModel READ propertyTreeModel CONSTANT)
    Q_PROPERTY(PropertyListProxyModel *propertyListProxyModel READ propertyListProxyModel CONSTANT)

public:
    explicit ScriptEditorBackend(AbstractView *view);

    using ActionType = StatementDelegate::ActionType;

    Q_INVOKABLE void changeActionType(QmlDesigner::StatementDelegate::ActionType actionType);

    Q_INVOKABLE void addCondition();
    Q_INVOKABLE void removeCondition();

    Q_INVOKABLE void addElse();
    Q_INVOKABLE void removeElse();

    Q_INVOKABLE void setNewSource(const QString &newSource);

    Q_INVOKABLE virtual void update();

    Q_INVOKABLE void jumpToCode();

    static void registerDeclarativeType();

signals:
    void actionTypeChanged();
    void hasConditionChanged();
    void hasElseChanged();
    void sourceChanged();
    void popupShouldClose();
    void popupShouldOpen();

protected:
    bool blockReflection() const;
    ;

private:
    virtual AbstractProperty getSourceProperty() const;
    virtual void setPropertySource(const QString &source);

    BindingProperty getBindingProperty() const;
    void handleException();
    bool hasCondition() const;
    bool hasElse() const;
    void setHasCondition(bool b);
    void setHasElse(bool b);
    ActionType actionType() const;
    StatementDelegate *okStatement();
    StatementDelegate *koStatement();
    ConditionListModel *conditionListModel();
    QString indentedSource() const;
    QString source() const;
    void setSource(const QString &source);

    PropertyTreeModel *propertyTreeModel();
    PropertyListProxyModel *propertyListProxyModel();

    void setupCondition();
    void setupHandlerAndStatements();

    void handleOkStatementChanged();
    void handleKOStatementChanged();
    void handleConditionChanged();

    void commitNewSource(const QString &source);

    ActionType m_actionType;
    QString m_exceptionError;
    ScriptEditorStatements::Handler m_handler;
    StatementDelegate m_okStatementDelegate;
    StatementDelegate m_koStatementDelegate;
    ConditionListModel m_conditionListModel;
    bool m_hasCondition = false;
    bool m_hasElse = false;
    QString m_source;
    PropertyTreeModel m_propertyTreeModel;
    PropertyListProxyModel m_propertyListProxyModel;
    bool m_blockReflection = false;
    QPointer<AbstractView> m_view = nullptr;
};
} // namespace QmlDesigner
