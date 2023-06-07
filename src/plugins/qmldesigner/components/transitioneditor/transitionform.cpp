// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "transitionform.h"
#include "timelineform.h"
#include "ui_transitionform.h"

#include <abstractview.h>
#include <bindingproperty.h>
#include <exception>
#include <nodelistproperty.h>
#include <nodemetainfo.h>
#include <rewritertransaction.h>
#include <variantproperty.h>
#include <qmlitemnode.h>

#include <coreplugin/messagebox.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

namespace QmlDesigner {

TransitionForm::TransitionForm(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::TransitionForm)
{
    ui->setupUi(this);

    connect(ui->idLineEdit, &QLineEdit::editingFinished, [this]() {
        QTC_ASSERT(m_transition.isValid(), return );

        static QString lastString;

        const QString newId = ui->idLineEdit->text();

        if (newId == lastString)
            return;

        lastString = newId;

        if (newId == m_transition.id())
            return;

        bool error = false;

        if (!ModelNode::isValidId(newId)) {
            Core::AsynchronousMessageBox::warning(tr("Invalid ID"),
                                                  tr("%1 is an invalid ID.").arg(newId));
            error = true;
        } else if (m_transition.view()->hasId(newId)) {
            Core::AsynchronousMessageBox::warning(tr("Invalid ID"),
                                                  tr("%1 already exists.").arg(newId));
            error = true;
        } else {
            m_transition.setIdWithRefactoring(newId);
        }

        if (error) {
            lastString.clear();
            ui->idLineEdit->setText(m_transition.id());
        }
    });

    connect(ui->listWidgetTo, &QListWidget::itemChanged, this, [this]() {
        QTC_ASSERT(m_transition.isValid(), return );
        const QmlItemNode root(m_transition.view()->rootModelNode());
        QTC_ASSERT(root.isValid(), return );
        const int stateCount = root.states().names().size();

        QStringList stateNames;

        for (const QListWidgetItem *item : ui->listWidgetTo->findItems("*", Qt::MatchWildcard)) {
            if (item->checkState() == Qt::Checked)
                stateNames.append(item->text());
        }

        QString toValue;
        if (stateCount == stateNames.size())
            toValue = "*";
        else
            toValue = stateNames.join(",");

        m_transition.view()->executeInTransaction("TransitionForm::Set To", [this, toValue]() {
            m_transition.variantProperty("to").setValue(toValue);
        });
    });

    connect(ui->listWidgetFrom, &QListWidget::itemChanged, this, [this]() {
        QTC_ASSERT(m_transition.isValid(), return );
        const QmlItemNode root(m_transition.view()->rootModelNode());
        QTC_ASSERT(root.isValid(), return );
        const int stateCount = root.states().names().size();

        QStringList stateNames;

        for (const QListWidgetItem *item : ui->listWidgetFrom->findItems("*", Qt::MatchWildcard)) {
            if (item->checkState() == Qt::Checked)
                stateNames.append(item->text());
        }

        QString fromValue;
        if (stateCount == stateNames.size())
            fromValue = "*";
        else
            fromValue = stateNames.join(",");

        m_transition.view()->executeInTransaction("TransitionForm::Set To", [this, fromValue]() {
            m_transition.variantProperty("from").setValue(fromValue);
        });
    });
}

TransitionForm::~TransitionForm()
{
    delete ui;
}

void TransitionForm::setTransition(const ModelNode &transition)
{
    m_transition = transition;

    if (m_transition.isValid()) {
        ui->idLineEdit->setText(m_transition.displayName());
    }
    setupStatesLists();
}

ModelNode TransitionForm::transition() const
{
    return m_transition;
}

void TransitionForm::setupStatesLists()
{
    bool bTo = ui->listWidgetTo->blockSignals(true);
    bool bFrom = ui->listWidgetFrom->blockSignals(true);
    QAbstractItemModel *modelTo = ui->listWidgetTo->model();
    modelTo->removeRows(0, modelTo->rowCount());

    QAbstractItemModel *modelFrom = ui->listWidgetFrom->model();
    modelFrom->removeRows(0, modelFrom->rowCount());

    bool starFrom = true;
    bool starTo = true;

    QStringList fromList;
    QStringList toList;

    if (m_transition.hasVariantProperty("from")
        && m_transition.variantProperty("from").value().toString().trimmed() != "*") {
        starFrom = false;
        fromList = m_transition.variantProperty("from").value().toString().split(",");
    }

    if (m_transition.hasVariantProperty("to")
        && m_transition.variantProperty("to").value().toString().trimmed() != "*") {
        starTo = false;
        toList = m_transition.variantProperty("to").value().toString().split(",");
    }

    if (const QmlItemNode root = m_transition.view()->rootModelNode()) {
        const QmlModelStateGroup states = root.states();
        for (const QString &stateName : states.names()) {
            auto itemTo = new QListWidgetItem(stateName, ui->listWidgetTo);
            ui->listWidgetTo->addItem(itemTo);
            itemTo->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
            if (starTo || toList.contains(stateName))
                itemTo->setCheckState(Qt::Checked);
            else
                itemTo->setCheckState(Qt::Unchecked);

            auto itemFrom = new QListWidgetItem(stateName, ui->listWidgetFrom);
            ui->listWidgetFrom->addItem(itemFrom);
            itemFrom->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
            if (starFrom || fromList.contains(stateName))
                itemFrom->setCheckState(Qt::Checked);
            else
                itemFrom->setCheckState(Qt::Unchecked);
        }
    }

    ui->listWidgetTo->blockSignals(bTo);
    ui->listWidgetFrom->blockSignals(bFrom);
}

} // namespace QmlDesigner
