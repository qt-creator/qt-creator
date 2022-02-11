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

#include "connectionviewwidget.h"
#include "connectionview.h"
#include "ui_connectionviewwidget.h"

#include "delegates.h"
#include "backendmodel.h"
#include "bindingmodel.h"
#include "connectionmodel.h"
#include "dynamicpropertiesmodel.h"
#include "theme.h"
#include "signalhandlerproperty.h"

#include <designersettings.h>
#include <qmldesignerplugin.h>

#include <nodemetainfo.h>

#include <bindingeditor/actioneditor.h>
#include <bindingeditor/bindingeditor.h>

#include <coreplugin/coreconstants.h>
#include <qmlprojectmanager/qmlproject.h>
#include <utils/fileutils.h>
#include <utils/utilsicons.h>

#include <QToolButton>
#include <QStyleFactory>
#include <QMenu>
#include <QShortcut>

namespace QmlDesigner {

namespace Internal {

ConnectionViewWidget::ConnectionViewWidget(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::ConnectionViewWidget)
{
    m_connectionEditor = new QmlDesigner::ActionEditor(this);
    m_bindingEditor = new QmlDesigner::BindingEditor(this);
    m_dynamicEditor = new QmlDesigner::BindingEditor(this);

    editorForConnection();
    editorForBinding();
    editorForDynamic();


    setWindowTitle(tr("Connections", "Title of connection view"));
    ui->setupUi(this);

    QStyle *style = QStyleFactory::create("fusion");
    ui->stackedWidget->setStyle(style);

    //ui->tabWidget->tabBar()->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    ui->tabBar->setUsesScrollButtons(true);
    ui->tabBar->setElideMode(Qt::ElideRight);

    ui->tabBar->addTab(tr("Connections", "Title of connection view"));
    ui->tabBar->addTab(tr("Bindings", "Title of connection view"));
    ui->tabBar->addTab(tr("Properties", "Title of dynamic properties view"));

    const Qt::Alignment headerAlignment = Qt::AlignLeft | Qt::AlignVCenter;
    ui->connectionView->horizontalHeader()->setDefaultAlignment(headerAlignment);
    ui->bindingView->horizontalHeader()->setDefaultAlignment(headerAlignment);
    ui->dynamicPropertiesView->horizontalHeader()->setDefaultAlignment(headerAlignment);
    ui->backendView->horizontalHeader()->setDefaultAlignment(headerAlignment);

    const QList<QToolButton*> buttons = createToolBarWidgets();

    for (auto toolButton : buttons)
        ui->toolBar->addWidget(toolButton);

    auto settings = QmlDesignerPlugin::instance()->settings();

    if (!QmlProjectManager::QmlProject::isQtDesignStudio())
        ui->tabBar->addTab(tr("Backends", "Title of dynamic properties view"));

    ui->tabBar->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);

    QByteArray sheet = Utils::FileReader::fetchQrc(":/connectionview/stylesheet.css");
    sheet += Utils::FileReader::fetchQrc(":/qmldesigner/scrollbar.css");
    setStyleSheet(Theme::replaceCssColors(QString::fromUtf8(sheet)));

    connect(ui->tabBar, &QTabBar::currentChanged,
            ui->stackedWidget, &QStackedWidget::setCurrentIndex);

    connect(ui->tabBar, &QTabBar::currentChanged,
            this, &ConnectionViewWidget::handleTabChanged);

    ui->stackedWidget->setCurrentIndex(0);
}

ConnectionViewWidget::~ConnectionViewWidget()
{
    delete m_connectionEditor;
    delete m_bindingEditor;
    delete m_dynamicEditor;
    delete ui;
}

void ConnectionViewWidget::setBindingModel(BindingModel *model)
{
    ui->bindingView->setModel(model);
    ui->bindingView->verticalHeader()->hide();
    ui->bindingView->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->bindingView->setItemDelegate(new BindingDelegate);
    connect(ui->bindingView->selectionModel(), &QItemSelectionModel::currentRowChanged,
            this, &ConnectionViewWidget::bindingTableViewSelectionChanged);
}

void ConnectionViewWidget::setConnectionModel(ConnectionModel *model)
{
    ui->connectionView->setModel(model);
    ui->connectionView->verticalHeader()->hide();
    ui->connectionView->horizontalHeader()->setDefaultSectionSize(160);
    ui->connectionView->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->connectionView->setItemDelegate(new ConnectionDelegate);

    connect(ui->connectionView->selectionModel(), &QItemSelectionModel::currentRowChanged,
            this, &ConnectionViewWidget::connectionTableViewSelectionChanged);
}

void ConnectionViewWidget::contextMenuEvent(QContextMenuEvent *event)
{
    auto tablePos = [&](QTableView *targetView) {
        // adjusting qpoint to the qtableview entrances:
        QPoint posInTable(targetView->mapFromGlobal(mapToGlobal(event->pos())));
        posInTable.ry() -= targetView->horizontalHeader()->height();
        return posInTable;
    };

    switch (currentTab()) {
    case ConnectionTab:
        if (ui->connectionView != nullptr) {
            QTableView *targetView = ui->connectionView;
            // making sure that we have source column in our hands:
            const QModelIndex index = targetView->indexAt(tablePos(targetView)).siblingAtColumn(ConnectionModel::SourceRow);
            if (!index.isValid())
                return;

            QMenu menu(this);

            menu.addAction(tr("Open Connection Editor"), this, [&]() {
                auto *connectionModel = qobject_cast<ConnectionModel *>(targetView->model());
                const SignalHandlerProperty property = connectionModel->signalHandlerPropertyForRow(index.row());
                const ModelNode node = property.parentModelNode();

                const QString targetName =  index.siblingAtColumn(ConnectionModel::TargetModelNodeRow).data().toString()
                        + "." + property.name();

                m_connectionEditor->showWidget();
                m_connectionEditor->setConnectionValue(index.data().toString());
                m_connectionEditor->setModelIndex(index);
                m_connectionEditor->setModelNode(node);
                m_connectionEditor->prepareConnections();
                m_connectionEditor->updateWindowName(targetName);
            });

            QMap<QString, QVariant> data;
            data["ModelNode"] = index.siblingAtColumn(ConnectionModel::TargetModelNodeRow).data();
            data["Signal"] = index.siblingAtColumn(ConnectionModel::TargetPropertyNameRow).data();
            DesignerActionManager &designerActionManager = QmlDesignerPlugin::instance()->designerActionManager();
            const auto actions = designerActionManager.actionsForTargetView(
                ActionInterface::TargetView::ConnectionEditor);

            for (const auto &actionInterface : actions) {
                auto *action = actionInterface->action();
                action->setData(data);
                menu.addAction(action);
            }

            menu.exec(event->globalPos());
        }
        break;

    case BindingTab:
        if (ui->bindingView != nullptr) {
            QTableView *targetView = bindingTableView();
            const QModelIndex index = targetView->indexAt(tablePos(targetView)).siblingAtColumn(BindingModel::SourcePropertyNameRow);
            if (!index.isValid())
                return;

            QMenu menu(this);

            menu.addAction(tr("Open Binding Editor"), this, [&]() {
                BindingModel *bindingModel = qobject_cast<BindingModel*>(targetView->model());
                const BindingProperty property = bindingModel->bindingPropertyForRow(index.row());

                if (!property.isValid() || !property.isBindingProperty())
                    return;

                const ModelNode node = property.parentModelNode();
                const TypeName typeName = property.isDynamic() ? property.dynamicTypeName()
                                                               : node.metaInfo().propertyTypeName(property.name());

                const QString targetName = node.displayName() + "." + property.name();

                m_bindingEditor->showWidget();
                m_bindingEditor->setBindingValue(property.expression());
                m_bindingEditor->setModelNode(node);
                m_bindingEditor->setBackendValueTypeName(typeName);
                m_bindingEditor->setTargetName(targetName);
                m_bindingEditor->prepareBindings();
                m_bindingEditor->updateWindowName();

                m_bindingIndex = index;
            });
            menu.exec(event->globalPos());
        }
        break;

    case DynamicPropertiesTab:
        if (ui->dynamicPropertiesView != nullptr) {
            QTableView *targetView = dynamicPropertiesTableView();
            const QModelIndex index = targetView->indexAt(tablePos(targetView)).siblingAtColumn(DynamicPropertiesModel::PropertyValueRow);
            if (!index.isValid())
                return;

            DynamicPropertiesModel *propertiesModel = qobject_cast<DynamicPropertiesModel *>(targetView->model());
            QMenu menu(this);

            menu.addAction(tr("Open Binding Editor"), this, [&]() {
                AbstractProperty abstractProperty = propertiesModel->abstractPropertyForRow(index.row());
                if (!abstractProperty.isValid())
                    return;

                const ModelNode node = abstractProperty.parentModelNode();
                QString newExpression;

                if (abstractProperty.isBindingProperty())
                    newExpression = abstractProperty.toBindingProperty().expression();
                else if (abstractProperty.isVariantProperty())
                    newExpression = abstractProperty.toVariantProperty().value().toString();
                else
                    return;

                const QString targetName = node.displayName() + "." + abstractProperty.name();

                m_dynamicEditor->showWidget();
                m_dynamicEditor->setBindingValue(newExpression);
                m_dynamicEditor->setModelNode(node);
                m_dynamicEditor->setBackendValueTypeName(abstractProperty.dynamicTypeName());
                m_dynamicEditor->setTargetName(targetName);
                m_dynamicEditor->prepareBindings();
                m_dynamicEditor->updateWindowName();

                m_dynamicIndex = index;
            });

            menu.addAction(tr("Reset Property"), this, [&]() {
                propertiesModel->resetProperty(propertiesModel->abstractPropertyForRow(index.row()).name());
            });

            menu.exec(event->globalPos());
        }
        break;
    default:
        break;

    }

}

void ConnectionViewWidget::setDynamicPropertiesModel(DynamicPropertiesModel *model)
{
    ui->dynamicPropertiesView->setModel(model);
    ui->dynamicPropertiesView->verticalHeader()->hide();
    ui->dynamicPropertiesView->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->dynamicPropertiesView->setItemDelegate(new DynamicPropertiesDelegate);
    connect(ui->dynamicPropertiesView->selectionModel(), &QItemSelectionModel::currentRowChanged,
            this, &ConnectionViewWidget::dynamicPropertiesTableViewSelectionChanged);
}

void ConnectionViewWidget::setBackendModel(BackendModel *model)
{
     ui->backendView->setModel(model);
     ui->backendView->verticalHeader()->hide();
     ui->backendView->setSelectionMode(QAbstractItemView::SingleSelection);
     ui->backendView->setItemDelegate(new BackendDelegate);
     model->resetModel();
     connect(ui->backendView->selectionModel(), &QItemSelectionModel::currentRowChanged,
             this, &ConnectionViewWidget::backendTableViewSelectionChanged);
}

QList<QToolButton *> ConnectionViewWidget::createToolBarWidgets()
{
    QList<QToolButton *> buttons;

    buttons << new QToolButton();
    buttons.constLast()->setIcon(Utils::Icons::PLUS_TOOLBAR.icon());
    buttons.constLast()->setToolTip(tr("Add binding or connection."));
    connect(buttons.constLast(), &QAbstractButton::clicked, this, &ConnectionViewWidget::addButtonClicked);
    connect(this, &ConnectionViewWidget::setEnabledAddButton, buttons.constLast(), &QWidget::setEnabled);

    buttons << new QToolButton();
    buttons.constLast()->setIcon(Utils::Icons::MINUS.icon());
    buttons.constLast()->setToolTip(tr("Remove selected binding or connection."));
    connect(buttons.constLast(), &QAbstractButton::clicked, this, &ConnectionViewWidget::removeButtonClicked);
    connect(this, &ConnectionViewWidget::setEnabledRemoveButton, buttons.constLast(), &QWidget::setEnabled);

    QAction *deleteShortcut = new QAction(this);
    this->addAction(deleteShortcut);
    deleteShortcut->setShortcuts({QKeySequence::Delete, QKeySequence::Backspace});
    deleteShortcut->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(deleteShortcut, &QAction::triggered, this, &ConnectionViewWidget::removeButtonClicked);

    return buttons;
}

ConnectionViewWidget::TabStatus ConnectionViewWidget::currentTab() const
{
    switch (ui->stackedWidget->currentIndex()) {
    case 0: return ConnectionTab;
    case 1: return BindingTab;
    case 2: return DynamicPropertiesTab;
    case 3: return BackendTab;
    default: return InvalidTab;
    }
}

void ConnectionViewWidget::resetItemViews()
{
    if (currentTab() == ConnectionTab) {
        ui->connectionView->selectionModel()->clear();

    } else if (currentTab() == BindingTab) {
        ui->bindingView->selectionModel()->clear();

    } else if (currentTab() == DynamicPropertiesTab) {
        ui->dynamicPropertiesView->selectionModel()->clear();
    } else if (currentTab() == BackendTab) {
        ui->backendView->selectionModel()->clear();
    }
    invalidateButtonStatus();
}

void ConnectionViewWidget::invalidateButtonStatus()
{
    if (currentTab() == ConnectionTab) {
        emit setEnabledRemoveButton(ui->connectionView->selectionModel()->hasSelection());
        emit setEnabledAddButton(true);
    } else if (currentTab() == BindingTab) {
        emit setEnabledRemoveButton(ui->bindingView->selectionModel()->hasSelection());
        auto bindingModel = qobject_cast<BindingModel*>(ui->bindingView->model());
        emit setEnabledAddButton(bindingModel->connectionView()->model() &&
                                 bindingModel->connectionView()->selectedModelNodes().count() == 1);

    } else if (currentTab() == DynamicPropertiesTab) {
        emit setEnabledRemoveButton(ui->dynamicPropertiesView->selectionModel()->hasSelection());
        auto dynamicPropertiesModel = qobject_cast<DynamicPropertiesModel*>(ui->dynamicPropertiesView->model());
        emit setEnabledAddButton(dynamicPropertiesModel->connectionView()->model() &&
                       dynamicPropertiesModel->connectionView()->selectedModelNodes().count() == 1);
    } else if (currentTab() == BackendTab) {
        emit setEnabledAddButton(true);
        emit setEnabledRemoveButton(ui->backendView->selectionModel()->hasSelection());
    }
}

QTableView *ConnectionViewWidget::connectionTableView() const
{
    return ui->connectionView;
}

QTableView *ConnectionViewWidget::bindingTableView() const
{
    return ui->bindingView;
}

QTableView *ConnectionViewWidget::dynamicPropertiesTableView() const
{
    return ui->dynamicPropertiesView;
}

QTableView *ConnectionViewWidget::backendView() const
{
    return ui->backendView;
}

void ConnectionViewWidget::handleTabChanged(int)
{
    invalidateButtonStatus();
}

void ConnectionViewWidget::removeButtonClicked()
{
    if (currentTab() == ConnectionTab) {
        if (ui->connectionView->selectionModel()->selectedRows().isEmpty())
            return;
        int currentRow =  ui->connectionView->selectionModel()->selectedRows().constFirst().row();
        auto connectionModel = qobject_cast<ConnectionModel*>(ui->connectionView->model());
        if (connectionModel) {
            connectionModel->deleteConnectionByRow(currentRow);
        }
    } else if (currentTab() == BindingTab) {
        if (ui->bindingView->selectionModel()->selectedRows().isEmpty())
            return;
        int currentRow =  ui->bindingView->selectionModel()->selectedRows().constFirst().row();
        auto bindingModel = qobject_cast<BindingModel*>(ui->bindingView->model());
        if (bindingModel) {
            bindingModel->deleteBindindByRow(currentRow);
        }
    } else if (currentTab() == DynamicPropertiesTab) {
        if (ui->dynamicPropertiesView->selectionModel()->selectedRows().isEmpty())
            return;
        int currentRow =  ui->dynamicPropertiesView->selectionModel()->selectedRows().constFirst().row();
        auto dynamicPropertiesModel = qobject_cast<DynamicPropertiesModel*>(ui->dynamicPropertiesView->model());
        if (dynamicPropertiesModel)
            dynamicPropertiesModel->deleteDynamicPropertyByRow(currentRow);
    }  else if (currentTab() == BackendTab) {
        int currentRow =  ui->backendView->selectionModel()->selectedRows().constFirst().row();
        auto backendModel = qobject_cast<BackendModel*>(ui->backendView->model());
        if (backendModel)
            backendModel->deletePropertyByRow(currentRow);
    }

    invalidateButtonStatus();
}

void ConnectionViewWidget::addButtonClicked()
{

    if (currentTab() == ConnectionTab) {
        auto connectionModel = qobject_cast<ConnectionModel*>(ui->connectionView->model());
        if (connectionModel) {
            connectionModel->addConnection();
        }
    } else if (currentTab() == BindingTab) {
        auto bindingModel = qobject_cast<BindingModel*>(ui->bindingView->model());
        if (bindingModel) {
            bindingModel->addBindingForCurrentNode();
        }

    } else if (currentTab() == DynamicPropertiesTab) {
        auto dynamicPropertiesModel = qobject_cast<DynamicPropertiesModel*>(ui->dynamicPropertiesView->model());
        if (dynamicPropertiesModel)
            dynamicPropertiesModel->addDynamicPropertyForCurrentNode();
    }  else if (currentTab() == BackendTab) {
        auto backendModel = qobject_cast<BackendModel*>(ui->backendView->model());
        if (backendModel)
            backendModel->addNewBackend();
    }

    invalidateButtonStatus();
}

void ConnectionViewWidget::editorForConnection()
{
    QObject::connect(m_connectionEditor, &QmlDesigner::ActionEditor::accepted,
                     [&]() {
        if (m_connectionEditor->hasModelIndex()) {
            ConnectionModel *connectionModel = qobject_cast<ConnectionModel *>(ui->connectionView->model());
            if (connectionModel->connectionView()->isWidgetEnabled()
                && (connectionModel->rowCount() > m_connectionEditor->modelIndex().row())) {
                connectionModel->connectionView()
                    ->executeInTransaction("ConnectionView::setSignal", [this, connectionModel]() {
                        SignalHandlerProperty signalHandler
                            = connectionModel->signalHandlerPropertyForRow(
                                m_connectionEditor->modelIndex().row());
                        signalHandler.setSource(m_connectionEditor->connectionValue());
                    });
            }
            m_connectionEditor->resetModelIndex();
        }

        m_connectionEditor->hideWidget();
    });
    QObject::connect(m_connectionEditor, &QmlDesigner::ActionEditor::rejected,
                     [&]() {
        m_connectionEditor->resetModelIndex();
        m_connectionEditor->hideWidget();
    });
}

void ConnectionViewWidget::editorForBinding()
{
    QObject::connect(m_bindingEditor, &QmlDesigner::BindingEditor::accepted,
                     [&]() {
        BindingModel *bindingModel = qobject_cast<BindingModel *>(bindingTableView()->model());
        QString newValue = m_bindingEditor->bindingValue().trimmed();

        if (m_bindingIndex.isValid()) {
            if (bindingModel->connectionView()->isWidgetEnabled()
                && (bindingModel->rowCount() > m_bindingIndex.row())) {
                bindingModel->connectionView()->executeInTransaction(
                    "ConnectionView::setBindingProperty", [this, bindingModel, newValue]() {
                        BindingProperty property = bindingModel->bindingPropertyForRow(
                            m_bindingIndex.row());

                        if (property.isValid()) {
                            if (property.isBindingProperty()) {
                                if (property.isDynamic()) {
                                    property
                                        .setDynamicTypeNameAndExpression(property.dynamicTypeName(),
                                                                         newValue);
                                } else {
                                    property.setExpression(newValue);
                                }
                            }
                        }
                    });
            }
        }

        m_bindingIndex = QModelIndex();
        m_bindingEditor->hideWidget();
    });
    QObject::connect(m_bindingEditor, &QmlDesigner::BindingEditor::rejected,
                     [&]() {
        m_bindingIndex = QModelIndex(); //invalidating index
        m_bindingEditor->hideWidget();
    });
}

void ConnectionViewWidget::editorForDynamic()
{
    QObject::connect(m_dynamicEditor, &QmlDesigner::BindingEditor::accepted,
                     [&]() {
        DynamicPropertiesModel *propertiesModel = qobject_cast<DynamicPropertiesModel *>(dynamicPropertiesTableView()->model());
        QString newValue = m_dynamicEditor->bindingValue().trimmed();

        if (m_dynamicIndex.isValid()) {
            if (propertiesModel->connectionView()->isWidgetEnabled()
                && (propertiesModel->rowCount() > m_dynamicIndex.row())) {
                propertiesModel->connectionView()->executeInTransaction(
                    "ConnectionView::setBinding", [this, propertiesModel, newValue]() {
                        AbstractProperty abProp = propertiesModel->abstractPropertyForRow(
                            m_dynamicIndex.row());

                        if (abProp.isValid()) {
                            if (abProp.isBindingProperty()) {
                                BindingProperty property = abProp.toBindingProperty();
                                property.setDynamicTypeNameAndExpression(property.dynamicTypeName(),
                                                                         newValue);
                            }

                            //if it's a variant property, then we remove it and replace with binding
                            else if (abProp.isVariantProperty()) {
                                VariantProperty property = abProp.toVariantProperty();
                                PropertyName name = property.name();
                                TypeName type = property.dynamicTypeName();

                                BindingProperty newProperty = propertiesModel
                                                                  ->replaceVariantWithBinding(name);
                                if (newProperty.isValid()) {
                                    newProperty.setDynamicTypeNameAndExpression(type, newValue);
                                }
                            }
                        }
                    });
            }
        }

        m_dynamicIndex = QModelIndex();
        m_dynamicEditor->hideWidget();
    });
    QObject::connect(m_dynamicEditor, &QmlDesigner::BindingEditor::rejected,
                     [&]() {
        m_dynamicIndex = QModelIndex(); //invalidating index
        m_dynamicEditor->hideWidget();
    });
}

void ConnectionViewWidget::bindingTableViewSelectionChanged(const QModelIndex &current, const QModelIndex & /*previous*/)
{
    if (currentTab() == BindingTab) {
        if (current.isValid()) {
            emit setEnabledRemoveButton(true);
        } else {
            emit setEnabledRemoveButton(false);
        }
    }
}

void ConnectionViewWidget::connectionTableViewSelectionChanged(const QModelIndex &current, const QModelIndex & /*previous*/)
{
    if (currentTab() == ConnectionTab) {
        if (current.isValid()) {
            emit setEnabledRemoveButton(true);
        } else {
            emit setEnabledRemoveButton(false);
        }
    }
}

void ConnectionViewWidget::dynamicPropertiesTableViewSelectionChanged(const QModelIndex &current, const QModelIndex & /*previous*/)
{
    if (currentTab() == DynamicPropertiesTab) {
        if (current.isValid()) {
            emit setEnabledRemoveButton(true);
        } else {
            emit setEnabledRemoveButton(false);
        }
    }
}

void ConnectionViewWidget::backendTableViewSelectionChanged(const QModelIndex &current, const QModelIndex & /*revious*/)
{
    if (currentTab() == BackendTab) {
        if (current.isValid()) {
            emit setEnabledRemoveButton(true);
        } else {
            emit setEnabledRemoveButton(false);
        }
    }

}

} // namespace Internal

} // namespace QmlDesigner
