/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "actioneditordialog.h"

#include "connectionvisitor.h"

#include <texteditor/texteditor.h>

#include <qmldesigner/qmldesignerplugin.h>
#include <qmljseditor/qmljseditor.h>
#include <qmljseditor/qmljseditordocument.h>
#include <texteditor/textdocument.h>

#include <QDialogButtonBox>
#include <QPushButton>
#include <QHBoxLayout>
#include <QComboBox>
#include <QPlainTextEdit>

static Q_LOGGING_CATEGORY(ceLog, "qtc.qmldesigner.connectioneditor", QtWarningMsg)

namespace QmlDesigner {

ActionEditorDialog::ActionEditorDialog(QWidget *parent)
    : AbstractEditorDialog(parent, tr("Connection Editor"))
{
    setupUIComponents();

    QObject::connect(m_comboBoxType, QOverload<int>::of(&QComboBox::activated),
                     [this] (int idx) { this->updateComboBoxes(idx, ComboBox::Type); });

    // Action connections
    QObject::connect(m_actionTargetItem, QOverload<int>::of(&QComboBox::activated),
                     [this] (int idx) { this->updateComboBoxes(idx, ComboBox::TargetItem); });
    QObject::connect(m_actionMethod, QOverload<int>::of(&QComboBox::activated),
                     [this] (int idx) { this->updateComboBoxes(idx, ComboBox::TargetProperty); });

    // Assignment connections
    QObject::connect(m_assignmentTargetItem, QOverload<int>::of(&QComboBox::activated),
                     [this] (int idx) { this->updateComboBoxes(idx, ComboBox::TargetItem); });
    QObject::connect(m_assignmentTargetProperty, QOverload<int>::of(&QComboBox::activated),
                     [this] (int idx) { this->updateComboBoxes(idx, ComboBox::TargetProperty); });
    QObject::connect(m_assignmentSourceItem, QOverload<int>::of(&QComboBox::activated),
                     [this] (int idx) { this->updateComboBoxes(idx, ComboBox::SourceItem); });
    QObject::connect(m_assignmentSourceProperty, QOverload<int>::of(&QComboBox::activated),
                     [this] (int idx) { this->updateComboBoxes(idx, ComboBox::SourceProperty); });
}

ActionEditorDialog::~ActionEditorDialog()
{
}

void ActionEditorDialog::adjustProperties()
{
    // Analyze the current connection editor statement/expression
    const auto qmlJSDocument = bindingEditorWidget()->qmlJsEditorDocument();
    auto doc = QmlJS::Document::create(QLatin1String("<expression>"), QmlJS::Dialect::JavaScript);
    doc->setSource(qmlJSDocument->plainText());
    bool parseResult = doc->parseExpression();

    if (!parseResult) {
        qCInfo(ceLog) << Q_FUNC_INFO << "Couldn't parse the expression!";
        return;
    }

    auto astNode = doc->ast();
    if (!astNode) {
        qCInfo(ceLog) << Q_FUNC_INFO << "There was no AST::Node in the document!";
        return;
    }

    ConnectionVisitor qmlVisitor;
    QmlJS::AST::Node::accept(astNode, &qmlVisitor);

    const auto expression = qmlVisitor.expression();

    if (expression.isEmpty()) {
        // Set all ComboBoxes to [Undefined], add connections to target item ComboBox
        fillAndSetTargetItem(undefinedString);
        fillAndSetTargetProperty(undefinedString);
        fillAndSetSourceItem(undefinedString);
        fillAndSetSourceProperty(undefinedString);
        return;
    }

    bool typeDone = false;
    bool targetDone = false;

    for (int i = 0; i < expression.count(); ++i) {
        switch (expression[i].first) {

        case QmlJS::AST::Node::Kind::Kind_CallExpression:
        {
            setType(ConnectionType::Action);
            typeDone = true;
        }
        break;

        case QmlJS::AST::Node::Kind::Kind_BinaryExpression:
        {
            setType(ConnectionType::Assignment);
            typeDone = true;
        }
        break;

        case QmlJS::AST::Node::Kind::Kind_FieldMemberExpression:
        {
            QString fieldMember = expression[i].second;
            ++i;// Increment index to get IdentifierExpression or next FieldMemberExpression
            while (expression[i].first == QmlJS::AST::Node::Kind::Kind_FieldMemberExpression) {
                fieldMember.prepend(expression[i].second + ".");
                ++i; // Increment index to get IdentifierExpression
            }

            if (targetDone && m_comboBoxType->currentIndex() != ConnectionType::Action) {
                fillAndSetSourceItem(expression[i].second);
                fillAndSetSourceProperty(fieldMember);
            } else {
                if (typeDone) {
                    fillAndSetTargetItem(expression[i].second);
                    fillAndSetTargetProperty(fieldMember);
                } else { // e.g. 'element.width'
                    // In this case Assignment is more likley
                    setType(ConnectionType::Assignment);
                    fillAndSetTargetItem(expression[i].second);
                    fillAndSetTargetProperty(fieldMember);
                    fillAndSetSourceItem(undefinedString);
                    fillAndSetSourceProperty(undefinedString);
                }
                targetDone = true;
            }

        }
        break;

        case QmlJS::AST::Node::Kind::Kind_TrueLiteral:
        case QmlJS::AST::Node::Kind::Kind_FalseLiteral:
        case QmlJS::AST::Node::Kind::Kind_NumericLiteral:
        case QmlJS::AST::Node::Kind::Kind_StringLiteral:
        {
            if (targetDone) {
                fillAndSetSourceItem(undefinedString);
                fillAndSetSourceProperty(expression[i].second, expression[i].first);
            } else {
                // In this case Assignment is more likley
                setType(ConnectionType::Assignment);
                fillAndSetTargetItem(undefinedString);
                fillAndSetTargetProperty(undefinedString);
                fillAndSetSourceItem(undefinedString);
                fillAndSetSourceProperty(expression[i].second, expression[i].first);
            }
        }
        break;

        case QmlJS::AST::Node::Kind::Kind_IdentifierExpression:
        {
            if (typeDone) {
                if (m_comboBoxType->currentIndex() == ConnectionType::Assignment) { // e.g. 'element = rectangle
                    if (targetDone) {
                        fillAndSetSourceItem(undefinedString);
                        fillAndSetSourceProperty(undefinedString);
                    } else {
                        fillAndSetTargetItem(expression[i].second);
                        fillAndSetTargetProperty(undefinedString);
                        targetDone = true;
                    }
                } else { // e.g. 'print("blabla")'
                    fillAndSetTargetItem(undefinedString);
                    fillAndSetTargetProperty(undefinedString);
                    targetDone = true;
                }
            } else { // e.g. 'element'
                // In this case Assignment is more likley
                setType(ConnectionType::Assignment);
                fillAndSetTargetItem(expression[i].second);
                fillAndSetTargetProperty(undefinedString);
                fillAndSetSourceItem(undefinedString);
                fillAndSetSourceProperty(undefinedString);
            }
        }
        break;

        default:
        {
            fillAndSetTargetItem(undefinedString);
            fillAndSetTargetProperty(undefinedString);
            fillAndSetSourceItem(undefinedString);
            fillAndSetSourceProperty(undefinedString);
        }
        break;
        }
    }
}

void ActionEditorDialog::setAllConnections(const QList<ConnectionOption> &connections,
                                           const QList<SingletonOption> &singletons,
                                           const QStringList &states)
{
    m_lock = true;

    m_connections = connections;
    m_singletons = singletons;
    m_states = states;
    adjustProperties();

    m_lock = false;
}

void ActionEditorDialog::updateComboBoxes(int index, ComboBox type)
{
    Q_UNUSED(index)

    const int currentType = m_comboBoxType->currentIndex();
    const int currentStack = m_stackedLayout->currentIndex();
    bool typeChanged = false;

    if (type == ComboBox::Type) {
        if (currentType != currentStack)
            typeChanged = true;
        else
            return; // Prevent rebuild of expression if type didn't change
    }

    if (typeChanged) {
        m_stackedLayout->setCurrentIndex(currentType);
        if (currentStack == ConnectionType::Action) {
            // Previous type was Action
            const auto targetItem = m_actionTargetItem->currentText();
            fillAndSetTargetItem(targetItem, true);
            fillAndSetTargetProperty(QString(), true);
            fillAndSetSourceItem(QString(), true);
            fillAndSetSourceProperty(QString(), QmlJS::AST::Node::Kind::Kind_Undefined, true);
        } else {
            // Previous type was Assignment
            const auto targetItem = m_assignmentTargetItem->currentText();
            fillAndSetTargetItem(targetItem, true);
            fillAndSetTargetProperty(QString(), true);
        }
    } else {
        if (currentType == ConnectionType::Action) {
            // Prevent rebuild of expression if undefinedString item was selected
            switch (type) {
            case ComboBox::TargetItem:
            if (m_actionTargetItem->currentText() == undefinedString)
                return;
            break;
            case ComboBox::TargetProperty:
            if (m_actionMethod->currentText() == undefinedString)
                return;
            break;
            default:
            break;
            }

            fillAndSetTargetItem(m_actionTargetItem->currentText());
            fillAndSetTargetProperty(m_actionMethod->currentText(), true);
        } else { // ConnectionType::Assignment
            const auto targetItem = m_assignmentTargetItem->currentText();
            const auto targetProperty = m_assignmentTargetProperty->currentText();
            const auto sourceItem = m_assignmentSourceItem->currentText();
            const auto sourceProperty = m_assignmentSourceProperty->currentText();

            // Prevent rebuild of expression if undefinedString item was selected
            switch (type) {
            case ComboBox::TargetItem:
            if (targetItem == undefinedString)
                return;
            break;
            case ComboBox::TargetProperty:
            if (targetProperty == undefinedString)
                return;
            break;
            case ComboBox::SourceItem:
            if (sourceItem == undefinedString)
                return;
            break;
            case ComboBox::SourceProperty:
            if (sourceProperty == undefinedString)
                return;
            break;
            default:
            break;
            }

            fillAndSetTargetItem(targetItem, true);
            fillAndSetTargetProperty(targetProperty, true);

            const auto sourcePropertyType = m_assignmentSourceProperty->currentData().value<TypeName>();

            if (type == ComboBox::SourceItem) {
                fillAndSetSourceItem(sourceItem, true);

                if (sourcePropertyType == specificItem)
                    fillAndSetSourceProperty(QString(),
                                             QmlJS::AST::Node::Kind::Kind_Undefined,
                                             true);
                else
                    fillAndSetSourceProperty(sourceProperty,
                                             QmlJS::AST::Node::Kind::Kind_Undefined,
                                             true);
            } else if (type == ComboBox::SourceProperty) {
                if (sourcePropertyType == specificItem) {
                    fillAndSetSourceItem(QString(), false);
                    fillAndSetSourceProperty(sourceProperty,
                                             QmlJS::AST::Node::Kind::Kind_StringLiteral,
                                             false);
                } else {
                    fillAndSetSourceProperty(sourceProperty);
                }
            } else {
                if (sourcePropertyType == specificItem) {
                    fillAndSetSourceItem(QString(), false);
                    fillAndSetSourceProperty(sourceProperty,
                                             QmlJS::AST::Node::Kind::Kind_StringLiteral,
                                             false);
                } else {
                    fillAndSetSourceItem(sourceItem, true);
                    fillAndSetSourceProperty(sourceProperty,
                                             QmlJS::AST::Node::Kind::Kind_Undefined,
                                             true);
                }
            }
        }
    }

    // Compose expression
    QString value;
    if (currentType == ConnectionType::Action) {
        const auto targetItem = m_actionTargetItem->currentText();
        const auto method = m_actionMethod->currentText();

        if (targetItem != undefinedString && method != undefinedString){
            value = targetItem + "." + method + "()";
        } else if (targetItem != undefinedString && method == undefinedString) {
            value = targetItem;
        }
    } else { // ConnectionType::Assignment
        const auto targetItem = m_assignmentTargetItem->currentText();
        const auto targetProperty = m_assignmentTargetProperty->currentText();
        const auto sourceItem = m_assignmentSourceItem->currentText();
        const auto sourceProperty = m_assignmentSourceProperty->currentText();

        QString lhs;

        if (targetItem != undefinedString && targetProperty != undefinedString) {
            lhs = targetItem + "." + targetProperty;
        } else if (targetItem != undefinedString && targetProperty == undefinedString) {
            lhs = targetItem;
        }

        QString rhs;

        if (sourceItem != undefinedString && sourceProperty != undefinedString) {
            rhs = sourceItem + "." + sourceProperty;
        } else if (sourceItem != undefinedString && sourceProperty == undefinedString) {
            rhs = sourceItem;
        } else if (sourceItem == undefinedString && sourceProperty != undefinedString) {
            const QString data = m_assignmentTargetProperty->currentData().toString();
            if (data == "string") {
                rhs = "\"" + sourceProperty + "\"";
            } else {
                rhs = sourceProperty;
            }
        }

        if (!lhs.isEmpty() && !rhs.isEmpty()) {
            value = lhs + " = " + rhs;
        } else {
            value = lhs + rhs;
        }
    }

    {
        const QSignalBlocker blocker(m_editorWidget);
        setEditorValue(value);
    }
}

void ActionEditorDialog::setupUIComponents()
{
    m_comboBoxType = new QComboBox(this);

    QMetaEnum metaEnum = QMetaEnum::fromType<ConnectionType>();
    for (int i = 0; i != metaEnum.keyCount(); ++i) {
        const char *key = QMetaEnum::fromType<ConnectionType>().valueToKey(i);
        m_comboBoxType->addItem(QString::fromLatin1(key));
    }

    m_comboBoxLayout->addWidget(m_comboBoxType);

    m_stackedLayout = new QStackedLayout();

    m_actionLayout = new QHBoxLayout();
    m_assignmentLayout = new QHBoxLayout();

    m_actionPlaceholder = new QWidget(this);
    m_actionPlaceholder->setLayout(m_actionLayout);

    m_assignmentPlaceholder = new QWidget(this);
    m_assignmentPlaceholder->setLayout(m_assignmentLayout);

    // Setup action ComboBoxes
    m_actionTargetItem = new QComboBox(this);
    m_actionMethod = new QComboBox(this);
    m_actionLayout->addWidget(m_actionTargetItem);
    m_actionLayout->addWidget(m_actionMethod);

    // Setup assignment ComboBoxes
    m_assignmentTargetItem = new QComboBox(this);
    m_assignmentTargetProperty = new QComboBox(this);
    m_assignmentSourceItem = new QComboBox(this);
    m_assignmentSourceProperty = new QComboBox(this);
    m_assignmentLayout->addWidget(m_assignmentTargetItem);
    m_assignmentLayout->addWidget(m_assignmentTargetProperty);
    m_assignmentLayout->addWidget(m_assignmentSourceItem);
    m_assignmentLayout->addWidget(m_assignmentSourceProperty);

    m_stackedLayout->addWidget(m_actionPlaceholder);
    m_stackedLayout->addWidget(m_assignmentPlaceholder);

    m_comboBoxLayout->addItem(m_stackedLayout);

    this->resize(720, 240);
}

void ActionEditorDialog::setType(ConnectionType type)
{
    m_comboBoxType->setCurrentIndex(type);
    m_stackedLayout->setCurrentIndex(type);
}

void ActionEditorDialog::fillAndSetTargetItem(const QString &value, bool useDefault)
{
    if (m_comboBoxType->currentIndex() == ConnectionType::Action) {
        m_actionTargetItem->clear();
        for (const auto &connection : qAsConst(m_connections)) {
            if (!connection.methods.isEmpty())
                m_actionTargetItem->addItem(connection.item);
        }

        if (m_actionTargetItem->findText(value) != -1) {
            m_actionTargetItem->setCurrentText(value);
        } else {
            if (useDefault && m_actionTargetItem->count())
                m_actionTargetItem->setCurrentIndex(0);
            else
                insertAndSetUndefined(m_actionTargetItem);
        }
    } else { // ConnectionType::Assignment
        m_assignmentTargetItem->clear();
        for (const auto &connection : qAsConst(m_connections)) {
            if (!connection.properties.isEmpty())
                m_assignmentTargetItem->addItem(connection.item);
        }

        if (m_assignmentTargetItem->findText(value) != -1) {
            m_assignmentTargetItem->setCurrentText(value);
        } else {
            if (useDefault && m_actionTargetItem->count())
                m_actionTargetItem->setCurrentIndex(0);
            else
                insertAndSetUndefined(m_assignmentTargetItem);
        }
    }
}

void ActionEditorDialog::fillAndSetTargetProperty(const QString &value, bool useDefault)
{
    if (m_comboBoxType->currentIndex() == ConnectionType::Action) {
        m_actionMethod->clear();
        const QString targetItem = m_actionTargetItem->currentText();
        const int idx = m_connections.indexOf(targetItem);

        if (idx == -1) {
            insertAndSetUndefined(m_actionMethod);
        } else {
            m_actionMethod->addItems(m_connections[idx].methods);

            if (m_actionMethod->findText(value) != -1) {
                m_actionMethod->setCurrentText(value);
            } else {
                if (useDefault && m_actionMethod->count())
                     m_actionMethod->setCurrentIndex(0);
                else
                    insertAndSetUndefined(m_actionMethod);
            }
        }
    } else { // ConnectionType::Assignment
        m_assignmentTargetProperty->clear();
        const QString targetItem = m_assignmentTargetItem->currentText();
        const int idx = m_connections.indexOf(targetItem);

        if (idx == -1) {
            insertAndSetUndefined(m_assignmentTargetProperty);
        } else {
            for (const auto &property : qAsConst(m_connections[idx].properties))
                m_assignmentTargetProperty->addItem(property.name, property.type);

            if (m_assignmentTargetProperty->findText(value) != -1) {
                m_assignmentTargetProperty->setCurrentText(value);
            } else {
                if (useDefault && m_assignmentTargetProperty->count())
                    m_assignmentTargetProperty->setCurrentIndex(0);
                else
                    insertAndSetUndefined(m_assignmentTargetProperty);
            }
        }
    }
}

void ActionEditorDialog::fillAndSetSourceItem(const QString &value, bool useDefault)
{
    m_assignmentSourceItem->clear();
    const TypeName targetPropertyType = m_assignmentTargetProperty->currentData().value<TypeName>();

    if (!targetPropertyType.isEmpty()) {
        for (const ConnectionOption &connection : qAsConst(m_connections)) {
            if (!connection.containsType(targetPropertyType))
                continue;

            m_assignmentSourceItem->addItem(connection.item);
        }

        // Add Constants
        for (const SingletonOption &singleton : qAsConst(m_singletons)) {
            if (!singleton.containsType(targetPropertyType))
                continue;

            m_assignmentSourceItem->addItem(singleton.item, singletonItem);
        }
    }

    if (m_assignmentSourceItem->findText(value) != -1) {
        m_assignmentSourceItem->setCurrentText(value);
    } else {
        if (useDefault && m_assignmentSourceItem->count())
            m_assignmentSourceItem->setCurrentIndex(0);
        else
            insertAndSetUndefined(m_assignmentSourceItem);
    }
}

void ActionEditorDialog::fillAndSetSourceProperty(const QString &value,
                                                  QmlJS::AST::Node::Kind kind,
                                                  bool useDefault)
{
    m_assignmentSourceProperty->clear();
    const TypeName targetPropertyType = m_assignmentTargetProperty->currentData().value<TypeName>();
    const QString targetProperty = m_assignmentTargetProperty->currentText();

    if (kind != QmlJS::AST::Node::Kind::Kind_Undefined) {
        if (targetPropertyType == "bool") {
            m_assignmentSourceProperty->addItem("true", specificItem);
            m_assignmentSourceProperty->addItem("false", specificItem);

            if (m_assignmentSourceProperty->findText(value) != -1)
                m_assignmentSourceProperty->setCurrentText(value);
            else
                insertAndSetUndefined(m_assignmentSourceProperty);
        } else if (targetProperty == "state") {
            for (const auto &state : qAsConst(m_states))
                m_assignmentSourceProperty->addItem(state, specificItem);

            if (m_assignmentSourceProperty->findText(value) != -1)
                m_assignmentSourceProperty->setCurrentText(value);
            else
                insertAndSetUndefined(m_assignmentSourceProperty);
        } else {
            m_assignmentSourceProperty->insertItem(0, value, specificItem);
            m_assignmentSourceProperty->setCurrentIndex(0);
        }

    } else {
        const TypeName sourceItemType = m_assignmentSourceItem->currentData().value<TypeName>();
        const QString sourceItem = m_assignmentSourceItem->currentText();
        // We need to distinguish between singleton (Constants) and standard item
        const int idx = (sourceItemType == singletonItem) ? m_singletons.indexOf(sourceItem)
                                                          : m_connections.indexOf(sourceItem);

        if (idx == -1) {
            insertAndSetUndefined(m_assignmentSourceProperty);
        } else {
            int specificsEnd = -1;
            // Add type specific items
            if (targetPropertyType == "bool") {
                m_assignmentSourceProperty->addItem("true", specificItem);
                m_assignmentSourceProperty->addItem("false", specificItem);
                specificsEnd = 2;
            } else if (targetProperty == "state") {
                for (const auto &state : qAsConst(m_states))
                    m_assignmentSourceProperty->addItem(state, specificItem);

                specificsEnd = m_states.count();
            }

            if (specificsEnd != -1)
                m_assignmentSourceProperty->insertSeparator(specificsEnd);

            if (sourceItemType == singletonItem) {
                for (const auto &property : qAsConst(m_singletons[idx].properties)) {
                    if (targetPropertyType.isEmpty() // TODO isEmpty correct?!
                        || property.type == targetPropertyType
                        || (isNumeric(property.type) && isNumeric(targetPropertyType)))
                        m_assignmentSourceProperty->addItem(property.name, property.type);
                }
            } else {
                for (const auto &property : qAsConst(m_connections[idx].properties)) {
                    if (targetPropertyType.isEmpty() // TODO isEmpty correct?!
                        || property.type == targetPropertyType
                        || (isNumeric(property.type) && isNumeric(targetPropertyType)))
                        m_assignmentSourceProperty->addItem(property.name, property.type);
                }
            }

            if (m_assignmentSourceProperty->findText(value) != -1 && !value.isEmpty()) {
                m_assignmentSourceProperty->setCurrentText(value);
            } else {
                if (useDefault && m_assignmentSourceProperty->count())
                    m_assignmentSourceProperty->setCurrentIndex(specificsEnd + 1);
                else
                    insertAndSetUndefined(m_assignmentSourceProperty);
            }
        }
    }
}

void ActionEditorDialog::insertAndSetUndefined(QComboBox *comboBox)
{
    comboBox->insertItem(0, undefinedString);
    comboBox->setCurrentIndex(0);
}

} // QmlDesigner namespace
