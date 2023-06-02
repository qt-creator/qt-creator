// Copyright (C) 2022 The Qt Company Ltd
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "objectsmapeditorwidget.h"

#include "deletesymbolicnamedialog.h"
#include "objectsmapdocument.h"
#include "objectsmaptreeitem.h"
#include "propertyitemdelegate.h"
#include "squishmessages.h"
#include "squishtr.h"
#include "symbolnameitemdelegate.h"

#include <coreplugin/icore.h>

#include <utils/algorithm.h>
#include <utils/layoutbuilder.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>

#include <QApplication>
#include <QClipboard>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QItemSelection>
#include <QItemSelectionModel>
#include <QLabel>
#include <QMenu>
#include <QMimeData>
#include <QModelIndex>
#include <QPushButton>
#include <QRegularExpression>
#include <QStackedLayout>
#include <QTreeView>

namespace Squish {
namespace Internal {

const char objectsMapObjectMimeType[] = "application/vnd.qtcreator.objectsmapobject";
const char objectsMapPropertyMimeType[] = "application/vnd.qtcreator.objectsmapproperty";

ObjectsMapEditorWidget::ObjectsMapEditorWidget(ObjectsMapDocument *document, QWidget *parent)
    : QWidget(parent)
    , m_document(document)
{
    initUi();
    initializeConnections();
    initializeContextMenus();
}

void ObjectsMapEditorWidget::initUi()
{
    setGeometry(0, 0, 550, 585);

    m_filterLineEdit = new Utils::FancyLineEdit(this);
    m_filterLineEdit->setFiltering(true);

    m_symbolicNamesTreeView = new QTreeView(this);

    m_newSymbolicName = new QPushButton(Tr::tr("New"));
    m_removeSymbolicName = new QPushButton(Tr::tr("Remove"));
    m_removeSymbolicName->setEnabled(false);

    m_propertiesLabel = new QLabel(this);
    m_propertiesLabel->setWordWrap(true);

    QWidget *validPropertiesWidget = new QWidget(this);
    m_propertiesTree = new QTreeView(this);
    m_propertiesTree->setIndentation(20);
    m_propertiesTree->setRootIsDecorated(false);
    m_propertiesTree->setUniformRowHeights(true);
    m_propertiesTree->setItemsExpandable(false);
    m_propertiesTree->setExpandsOnDoubleClick(false);

    m_newProperty = new QPushButton(Tr::tr("New"), this);
    m_newProperty->setEnabled(false);
    m_removeProperty = new QPushButton(Tr::tr("Remove"), this);
    m_removeProperty->setEnabled(false);
    m_jumpToSymbolicName = new QPushButton(this);
    m_jumpToSymbolicName->setEnabled(false);
    m_jumpToSymbolicName->setIcon(QIcon(":/squish/images/jumpTo.png"));
    m_jumpToSymbolicName->setToolTip(Tr::tr("Jump to Symbolic Name"));

    QWidget *invalidPropertiesWidget = new QWidget(this);
    m_propertiesLineEdit = new QLineEdit(this);

    m_stackedLayout = new QStackedLayout;
    m_stackedLayout->addWidget(validPropertiesWidget);
    m_stackedLayout->addWidget(invalidPropertiesWidget);

    using namespace Layouting;

    Row {
        m_propertiesTree,
        Column {
            m_newProperty,
            m_removeProperty,
            m_jumpToSymbolicName,
            st
        }
    }.attachTo(validPropertiesWidget);

    Column {
        m_propertiesLineEdit,
        st
    }.attachTo(invalidPropertiesWidget);

    Column {
        new QLabel("<b>" + Tr::tr("Symbolic Names") + "</b>"),
        m_filterLineEdit,
        Row {
            m_symbolicNamesTreeView,
            Column {
                m_newSymbolicName,
                m_removeSymbolicName,
                st,
            }
        },
        m_propertiesLabel,
        m_stackedLayout
    }.attachTo(this);

    m_objMapFilterModel = new ObjectsMapSortFilterModel(m_document->model(), this);
    m_objMapFilterModel->setDynamicSortFilter(true);
    m_symbolicNamesTreeView->setModel(m_objMapFilterModel);
    m_symbolicNamesTreeView->setSortingEnabled(true);
    m_symbolicNamesTreeView->setHeaderHidden(true);
    SymbolNameItemDelegate *symbolDelegate = new SymbolNameItemDelegate(this);
    m_symbolicNamesTreeView->setItemDelegate(symbolDelegate);
    m_symbolicNamesTreeView->setContextMenuPolicy(Qt::CustomContextMenu);

    PropertyItemDelegate *propertyDelegate = new PropertyItemDelegate(this);
    m_propertiesTree->setItemDelegate(propertyDelegate);
    m_propertiesTree->setContextMenuPolicy(Qt::CustomContextMenu);
    m_propertiesSortModel = new PropertiesSortModel(this);
    m_propertiesTree->setModel(m_propertiesSortModel);
    m_propertiesTree->setSortingEnabled(true);
    m_propertiesTree->header()->setSortIndicatorShown(false);
    m_propertiesTree->header()->setSectionsClickable(false);
    m_propertiesTree->header()->setSectionsMovable(false);

    setPropertiesDisplayValid(true);
}

void ObjectsMapEditorWidget::initializeConnections()
{
    connect(m_filterLineEdit,
            &Utils::FancyLineEdit::filterChanged,
            this,
            [this](const QString &filter) {
                m_objMapFilterModel->setFilterFixedString(filter);
                QItemSelectionModel *selectionModel = m_symbolicNamesTreeView->selectionModel();
                if (selectionModel->hasSelection())
                    m_symbolicNamesTreeView->scrollTo(selectionModel->selectedIndexes().first());
            });
    connect(m_document->model(),
            &ObjectsMapModel::requestSelection,
            this,
            &ObjectsMapEditorWidget::onSelectionRequested);
    connect(m_symbolicNamesTreeView->selectionModel(),
            &QItemSelectionModel::selectionChanged,
            this,
            &ObjectsMapEditorWidget::onObjectSelectionChanged);
    connect(m_jumpToSymbolicName,
            &QPushButton::clicked,
            this,
            &ObjectsMapEditorWidget::onJumpToSymbolicNameClicked);
    connect(m_symbolicNamesTreeView,
            &QTreeView::customContextMenuRequested,
            this,
            [this](const QPoint &pos) {
                m_symbolicNamesCtxtMenu->exec(m_symbolicNamesTreeView->mapToGlobal(pos));
            });
    connect(m_propertiesTree,
            &QTreeView::customContextMenuRequested,
            this,
            [this](const QPoint &pos) {
                m_propertiesCtxtMenu->exec(m_propertiesTree->mapToGlobal(pos));
            });
    connect(m_propertiesLineEdit,
            &QLineEdit::textChanged,
            this,
            &ObjectsMapEditorWidget::onPropertiesContentModified);
    connect(m_newProperty,
            &QPushButton::clicked,
            this,
            &ObjectsMapEditorWidget::onNewPropertyTriggered);
    connect(m_removeProperty,
            &QPushButton::clicked,
            this,
            &ObjectsMapEditorWidget::onRemovePropertyTriggered);
    connect(m_newSymbolicName,
            &QPushButton::clicked,
            this,
            &ObjectsMapEditorWidget::onNewSymbolicNameTriggered);
    connect(m_removeSymbolicName,
            &QPushButton::clicked,
            this,
            &ObjectsMapEditorWidget::onRemoveSymbolicNameTriggered);
}

void ObjectsMapEditorWidget::initializeContextMenus()
{
    m_symbolicNamesCtxtMenu = new QMenu(m_symbolicNamesTreeView);
    QAction *cutAction = new QAction(Tr::tr("Cut"), m_symbolicNamesCtxtMenu);
    cutAction->setShortcut(QKeySequence(QKeySequence::Cut));
    connect(cutAction,
            &QAction::triggered,
            this,
            &ObjectsMapEditorWidget::onCutSymbolicNameTriggered);
    QAction *copyAction = new QAction(Tr::tr("Copy"), m_symbolicNamesCtxtMenu);
    copyAction->setShortcut(QKeySequence(QKeySequence::Copy));
    connect(copyAction, &QAction::triggered, this, &ObjectsMapEditorWidget::onCopySymbolTriggered);
    QAction *pasteAction = new QAction(Tr::tr("Paste"), m_symbolicNamesCtxtMenu);
    pasteAction->setShortcut(QKeySequence(QKeySequence::Paste));
    connect(pasteAction,
            &QAction::triggered,
            this,
            &ObjectsMapEditorWidget::onPasteSymbolicNameTriggered);
    QAction *deleteAction = new QAction(Tr::tr("Delete"), m_symbolicNamesCtxtMenu);
    deleteAction->setShortcut(QKeySequence(QKeySequence::Delete));
    connect(deleteAction,
            &QAction::triggered,
            this,
            &ObjectsMapEditorWidget::onRemoveSymbolicNameTriggered);
    QAction *copyRealNameAction = new QAction(Tr::tr("Copy Real Name"), m_symbolicNamesCtxtMenu);
    connect(copyRealNameAction,
            &QAction::triggered,
            this,
            &ObjectsMapEditorWidget::onCopyRealNameTriggered);

    m_symbolicNamesCtxtMenu->addAction(cutAction);
    m_symbolicNamesCtxtMenu->addAction(copyAction);
    m_symbolicNamesCtxtMenu->addAction(pasteAction);
    m_symbolicNamesCtxtMenu->addAction(deleteAction);
    m_symbolicNamesCtxtMenu->addAction(copyRealNameAction);

    m_propertiesCtxtMenu = new QMenu(m_propertiesTree);
    cutAction = new QAction(Tr::tr("Cut"), m_propertiesCtxtMenu);
    cutAction->setShortcut(QKeySequence(QKeySequence::Cut));
    connect(cutAction, &QAction::triggered, this, &ObjectsMapEditorWidget::onCutPropertyTriggered);
    copyAction = new QAction(Tr::tr("Copy"), m_propertiesCtxtMenu);
    copyAction->setShortcut(QKeySequence(QKeySequence::Copy));
    connect(copyAction, &QAction::triggered, this, &ObjectsMapEditorWidget::onCopyPropertyTriggered);
    pasteAction = new QAction(Tr::tr("Paste"), m_propertiesCtxtMenu);
    pasteAction->setShortcut(QKeySequence(QKeySequence::Paste));
    connect(pasteAction,
            &QAction::triggered,
            this,
            &ObjectsMapEditorWidget::onPastePropertyTriggered);
    deleteAction = new QAction(Tr::tr("Delete"), m_propertiesCtxtMenu);
    deleteAction->setShortcut(QKeySequence(QKeySequence::Delete));
    connect(deleteAction,
            &QAction::triggered,
            this,
            &ObjectsMapEditorWidget::onRemovePropertyTriggered);

    m_propertiesCtxtMenu->addAction(cutAction);
    m_propertiesCtxtMenu->addAction(copyAction);
    m_propertiesCtxtMenu->addAction(pasteAction);
    m_propertiesCtxtMenu->addAction(deleteAction);
}

void ObjectsMapEditorWidget::setPropertiesDisplayValid(bool valid)
{
    static const QString properties = "<b>" + Tr::tr("Properties:") + "</b><br/>";
    static const QString propertiesValidText = properties + Tr::tr(
        "The properties of the Multi Property Name associated with the selected "
        "Symbolic Name. (use \\\\ for a literal \\ in the value)");
    static const QString propertiesInvalidText = properties + Tr::tr(
        "The Hierarchical Name associated with the selected Symbolic Name.");

    m_propertiesLabel->setText(valid ? propertiesValidText : propertiesInvalidText);
    m_stackedLayout->setCurrentIndex(valid ? 0 : 1);
}

void ObjectsMapEditorWidget::onSelectionRequested(const QModelIndex &idx)
{
    QItemSelectionModel *selectionModel = m_symbolicNamesTreeView->selectionModel();
    selectionModel->select(m_objMapFilterModel->mapFromSource(idx),
                           QItemSelectionModel::ClearAndSelect);
    m_symbolicNamesTreeView->scrollTo(selectionModel->selectedIndexes().first());
}

void ObjectsMapEditorWidget::onObjectSelectionChanged(const QItemSelection &selected,
                                                      const QItemSelection & /*deselected*/)
{
    QModelIndexList modelIndexes = selected.indexes();
    if (modelIndexes.isEmpty()) {
        m_propertiesTree->setModel(nullptr);
        m_removeSymbolicName->setEnabled(false);
        m_jumpToSymbolicName->setEnabled(false);
        m_newProperty->setEnabled(false);
        m_removeProperty->setEnabled(false);
        return;
    }
    const QModelIndex &idx = m_objMapFilterModel->mapToSource(modelIndexes.first());
    if (auto item = static_cast<ObjectsMapTreeItem *>(m_document->model()->itemForIndex(idx))) {
        const bool valid = item->isValid();

        if (valid) {
            m_propertiesSortModel->setSourceModel(item->propertiesModel());
            m_propertiesTree->setModel(m_propertiesSortModel);
            connect(m_propertiesTree->selectionModel(),
                    &QItemSelectionModel::selectionChanged,
                    this,
                    &ObjectsMapEditorWidget::onPropertySelectionChanged,
                    Qt::UniqueConnection);
            m_newProperty->setEnabled(true);
            m_removeSymbolicName->setEnabled(true);
            m_jumpToSymbolicName->setEnabled(false);
            m_removeProperty->setEnabled(false);
        } else {
            m_propertiesLineEdit->setText(QLatin1String(item->propertiesContent()));
            m_propertiesLineEdit->setCursorPosition(0);
        }
        setPropertiesDisplayValid(valid);
    }
}

void ObjectsMapEditorWidget::onPropertySelectionChanged(const QItemSelection &selected,
                                                        const QItemSelection & /*deselected*/)
{
    QModelIndexList modelIndexes = selected.indexes();
    if (modelIndexes.isEmpty()) {
        m_jumpToSymbolicName->setEnabled(false);
        m_removeProperty->setEnabled(false);
    } else {
        const QModelIndex current = modelIndexes.first();
        if (current.isValid()) {
            m_removeProperty->setEnabled(true);
            const QString is = current.sibling(current.row(), 1).data().toString();
            m_jumpToSymbolicName->setEnabled(is == Property::OPERATOR_IS);
        }
    }
}

void ObjectsMapEditorWidget::onPropertiesContentModified(const QString &text)
{
    if (!m_propertiesLineEdit->isModified())
        return;

    const QModelIndexList selected = m_symbolicNamesTreeView->selectionModel()->selectedIndexes();
    if (selected.isEmpty())
        return;

    const QModelIndex &idx = m_objMapFilterModel->mapToSource(selected.first());
    if (auto item = static_cast<ObjectsMapTreeItem *>(m_document->model()->itemForIndex(idx)))
        item->setPropertiesContent(text.toUtf8().trimmed());
}

void ObjectsMapEditorWidget::onJumpToSymbolicNameClicked()
{
    QModelIndexList selectedIndexes = m_propertiesTree->selectionModel()->selectedIndexes();
    if (selectedIndexes.isEmpty())
        return;

    if (auto model = qobject_cast<PropertiesModel *>(m_propertiesSortModel->sourceModel())) {
        const QModelIndex propIdx = m_propertiesSortModel->mapToSource(selectedIndexes.first());
        Utils::TreeItem *item = model->itemForIndex(propIdx);

        const QString symbolicName = item->data(2, Qt::DisplayRole).toString();
        if (Utils::TreeItem *found = m_document->model()->findItem(symbolicName)) {
            const QModelIndex idx = m_document->model()->indexForItem(found);
            QItemSelectionModel *selectionModel = m_symbolicNamesTreeView->selectionModel();
            selectionModel->select(m_objMapFilterModel->mapFromSource(idx),
                                   QItemSelectionModel::ClearAndSelect);
            m_symbolicNamesTreeView->scrollTo(m_objMapFilterModel->mapFromSource(idx));
        }
    }
}

static QString generateName(const QStringList &names, const QString &nameTmpl, int start = 1)
{
    int value;
    for (value = start; names.contains(nameTmpl + QString::number(value)); ++value)
        ;
    return nameTmpl + QString::number(value);
}

void ObjectsMapEditorWidget::onNewPropertyTriggered()
{
    static QString nameTemplate = "New";

    PropertiesModel *propertiesModel = qobject_cast<PropertiesModel *>(
        m_propertiesSortModel->sourceModel());

    Utils::TreeItem *root = propertiesModel->rootItem();

    QStringList propertyNames;
    propertyNames.reserve(root->childCount());
    root->forChildrenAtLevel(1, [&propertyNames](Utils::TreeItem *child) {
        propertyNames.append(static_cast<PropertyTreeItem *>(child)->property().m_name);
    });

    Property property;
    property.m_name = generateName(propertyNames, nameTemplate);
    PropertyTreeItem *propertyItem = new PropertyTreeItem(property);
    m_propertiesTree->clearSelection();
    propertiesModel->addNewProperty(propertyItem);
    const QModelIndex srcIdx = propertiesModel->indexForItem(propertyItem);
    m_propertiesTree->edit(m_propertiesSortModel->mapFromSource(srcIdx));
}

void ObjectsMapEditorWidget::onRemovePropertyTriggered()
{
    if (PropertyTreeItem *item = selectedPropertyItem()) {
        auto model = qobject_cast<PropertiesModel *>(m_propertiesSortModel->sourceModel());
        model->removeProperty(item);
    }
}

void ObjectsMapEditorWidget::onNewSymbolicNameTriggered()
{
    static QString nameTemplate = ":NewName";

    ObjectsMapModel *objMapModel = qobject_cast<ObjectsMapModel *>(
        m_objMapFilterModel->sourceModel());
    const QStringList objNames = objMapModel->allSymbolicNames();
    ObjectsMapTreeItem *objMapItem = new ObjectsMapTreeItem(generateName(objNames, nameTemplate),
                                                            Qt::ItemIsEnabled | Qt::ItemIsSelectable
                                                                | Qt::ItemIsEditable);

    objMapItem->initPropertyModelConnections(m_document->model());

    m_symbolicNamesTreeView->clearSelection();
    objMapModel->addNewObject(objMapItem);
    const QModelIndex idx = m_objMapFilterModel->mapFromSource(
        objMapModel->indexForItem(objMapItem));
    m_symbolicNamesTreeView->scrollTo(idx, QAbstractItemView::PositionAtCenter);
    // make sure PropertiesTree is updated as well
    onObjectSelectionChanged(QItemSelection(idx, idx), QItemSelection());
    m_symbolicNamesTreeView->edit(idx);
}

void ObjectsMapEditorWidget::onRemoveSymbolicNameTriggered()
{
    ObjectsMapModel *objMapModel = qobject_cast<ObjectsMapModel *>(
        m_objMapFilterModel->sourceModel());
    const QModelIndexList &selected = m_symbolicNamesTreeView->selectionModel()->selectedIndexes();
    QTC_ASSERT(!selected.isEmpty(), return );

    const QModelIndex idx = selected.first();
    const QString symbolicName = idx.data().toString();
    // if symbol has children it is the window or container for these
    bool hasReference = m_objMapFilterModel->hasChildren(idx);

    QMap<QString, PropertyList> objects;
    objMapModel->forAllItems([&objects](ObjectsMapTreeItem *item) {
        if (item->parent())
            objects.insert(item->data(0, Qt::DisplayRole).toString(), item->properties());
    });

    hasReference |= Utils::anyOf(objects, [&symbolicName](const PropertyList &props) {
        return Utils::anyOf(props, [&symbolicName](const Property &p) {
            return p.m_value == symbolicName && p.isRelativeWidget();
        });
    });

    DeleteSymbolicNameDialog::Result result = DeleteSymbolicNameDialog::RemoveNames;
    QString newReference;
    if (hasReference) {
        DeleteSymbolicNameDialog dialog(symbolicName, objects.keys(), Core::ICore::dialogParent());
        if (dialog.exec() != QDialog::Accepted)
            return;
        result = dialog.result();
        newReference = dialog.selectedSymbolicName();
    } else {
        // Squish does not ask for removing objects without references, but we prefer to do it
        const QString detail = Tr::tr("Do you really want to remove \"%1\"?").arg(symbolicName);
        if (SquishMessages::simpleQuestion(Tr::tr("Remove Symbolic Name"), detail) != QMessageBox::Yes)
            return;
    }

    switch (result) {
    case DeleteSymbolicNameDialog::ResetReference:
        objMapModel->removeSymbolicNameResetReferences(symbolicName, newReference);
        break;
    case DeleteSymbolicNameDialog::InvalidateNames:
        objMapModel->removeSymbolicNameInvalidateReferences(m_objMapFilterModel->mapToSource(idx));
        break;
    case DeleteSymbolicNameDialog::RemoveNames:
        objMapModel->removeSymbolicName(m_objMapFilterModel->mapToSource(idx));
        break;
    }
}

void ObjectsMapEditorWidget::onCopySymbolTriggered()
{
    ObjectsMapTreeItem *item = selectedObjectItem();
    if (!item)
        return;

    const QModelIndex idx = m_document->model()->indexForItem(item);
    const QString &symbolicName = idx.data().toString();
    QMimeData *data = new QMimeData;
    data->setText(symbolicName);
    data->setData(objectsMapObjectMimeType, item->propertiesToByteArray());
    QApplication::clipboard()->setMimeData(data);
}

void ObjectsMapEditorWidget::onPasteSymbolicNameTriggered()
{
    const QMimeData *data = QApplication::clipboard()->mimeData();
    if (!data)
        return;

    QString symbolicName = data->text();
    if (symbolicName.isEmpty())
        return;
    if (symbolicName.at(0) != ObjectsMapTreeItem::COLON)
        symbolicName.prepend(ObjectsMapTreeItem::COLON);
    else if (symbolicName.size() == 1)
        return;

    // if name is not valid at all refuse to do anything
    const QRegularExpression validName("^:[^\t\n\r\f\b\v\a]+$");
    if (!validName.match(symbolicName).hasMatch())
        return;

    if (auto objMapModel = qobject_cast<ObjectsMapModel *>(m_objMapFilterModel->sourceModel())) {
        QStringList usedSymbolicNames = objMapModel->allSymbolicNames();
        // check if name is valid and if not, try to get a new one
        if (usedSymbolicNames.contains(symbolicName))
            symbolicName = ambiguousNameDialog(symbolicName, usedSymbolicNames, false);

        if (symbolicName.isEmpty())
            return;

        ObjectsMapTreeItem *objMapItem = new ObjectsMapTreeItem(symbolicName,
                                                                Qt::ItemIsEnabled
                                                                    | Qt::ItemIsSelectable
                                                                    | Qt::ItemIsEditable);

        // if it's our mime data insert a symbolic name including properties
        if (data->hasFormat(objectsMapObjectMimeType)) {
            QByteArray properties = data->data(objectsMapObjectMimeType);
            if (!properties.isEmpty())
                objMapItem->setPropertiesContent(properties);
        }

        objMapItem->initPropertyModelConnections(m_document->model());
        objMapModel->addNewObject(objMapItem);
        const QModelIndex idx = m_objMapFilterModel->mapFromSource(
            objMapModel->indexForItem(objMapItem));
        m_symbolicNamesTreeView->scrollTo(idx, QAbstractItemView::PositionAtCenter);
        m_symbolicNamesTreeView->selectionModel()->select(QItemSelection(idx, idx),
                                                          QItemSelectionModel::ClearAndSelect);
    }
}

void ObjectsMapEditorWidget::onCopyRealNameTriggered()
{
    if (ObjectsMapTreeItem *item = selectedObjectItem())
        Utils::setClipboardAndSelection(QLatin1String(item->propertiesToByteArray()));
}

void ObjectsMapEditorWidget::onCutSymbolicNameTriggered()
{
    onCopySymbolTriggered();
    onRemoveSymbolicNameTriggered();
}

void ObjectsMapEditorWidget::onCopyPropertyTriggered()
{
    PropertyTreeItem *item = selectedPropertyItem();
    if (!item)
        return;

    QMimeData *data = new QMimeData;
    data->setText(item->property().toString());
    data->setData(objectsMapPropertyMimeType, item->property().toString().toUtf8());
    QApplication::clipboard()->setMimeData(data);
}

void ObjectsMapEditorWidget::onCutPropertyTriggered()
{
    onCopyPropertyTriggered();
    onRemovePropertyTriggered();
}

void ObjectsMapEditorWidget::onPastePropertyTriggered()
{
    const QMimeData *data = QApplication::clipboard()->mimeData();
    // we only handle our own copied mime data
    if (!data || !data->hasFormat(objectsMapPropertyMimeType))
        return;
    if (auto sortModel = qobject_cast<PropertiesSortModel *>(m_propertiesTree->model())) {
        Property property = Property(data->data(objectsMapPropertyMimeType));
        if (property.m_name.isEmpty())
            return;

        auto propertiesModel = qobject_cast<PropertiesModel *>(sortModel->sourceModel());
        const QStringList &usedProperties = propertiesModel->allPropertyNames();
        if (usedProperties.contains(property.m_name)) {
            property.m_name = ambiguousNameDialog(property.m_name, usedProperties, true);
            if (property.m_name.isEmpty())
                return;
        }
        PropertyTreeItem *propertyItem = new PropertyTreeItem(property);
        propertiesModel->addNewProperty(propertyItem);
    }
}

QString ObjectsMapEditorWidget::ambiguousNameDialog(const QString &original,
                                                    const QStringList &usedNames,
                                                    bool isProperty)
{
    QTC_ASSERT(!original.isEmpty(), return QString());

    QDialog dialog(this);
    dialog.setModal(true);
    dialog.setWindowTitle(isProperty ? Tr::tr("Ambiguous Property Name")
                                     : Tr::tr("Ambiguous Symbolic Name"));
    QVBoxLayout *layout = new QVBoxLayout;
    QLabel label(Tr::tr("%1 \"%2\" already exists. Specify a unique name.")
                     .arg(isProperty ? Tr::tr("Property") : Tr::tr("Symbolic Name"))
                     .arg(original));
    layout->addWidget(&label);
    Utils::FancyLineEdit *validator;
    if (isProperty)
        validator = new ValidatingPropertyNameLineEdit(usedNames, &dialog);
    else
        validator = new ValidatingContainerNameLineEdit(usedNames, &dialog);

    layout->addWidget(validator);
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
                                                           | QDialogButtonBox::Cancel,
                                                       &dialog);
    layout->addWidget(buttonBox);
    connect(validator,
            &ValidatingPropertyNameLineEdit::validChanged,
            buttonBox->button(QDialogButtonBox::Ok),
            &QPushButton::setEnabled);
    connect(buttonBox->button(QDialogButtonBox::Ok),
            &QPushButton::clicked,
            &dialog,
            &QDialog::accept);
    connect(buttonBox->button(QDialogButtonBox::Cancel),
            &QPushButton::clicked,
            &dialog,
            &QDialog::reject);

    QString validName(original);
    if (isProperty) {
        validName[0] = validName[0].toUpper();
        validName = Tr::tr("CopyOf") + validName;
    }
    // make sure the name is unique
    if (usedNames.contains(validName))
        validName = generateName(usedNames, validName, 2);

    validator->setText(validName);
    dialog.setLayout(layout);
    if (dialog.exec() == QDialog::Accepted && validator->isValid())
        return validator->text();
    return QString();
}

ObjectsMapTreeItem *ObjectsMapEditorWidget::selectedObjectItem() const
{
    const QModelIndexList &selected = m_symbolicNamesTreeView->selectionModel()->selectedIndexes();
    QTC_ASSERT(!selected.isEmpty(), return nullptr);

    if (auto proxyModel = qobject_cast<QSortFilterProxyModel *>(m_symbolicNamesTreeView->model())) {
        if (auto model = qobject_cast<ObjectsMapModel *>(proxyModel->sourceModel())) {
            const QModelIndex idx = m_objMapFilterModel->mapToSource(selected.first());
            return static_cast<ObjectsMapTreeItem *>(model->itemForIndex(idx));
        }
    }
    return nullptr;
}

PropertyTreeItem *ObjectsMapEditorWidget::selectedPropertyItem() const
{
    auto propertiesModel = qobject_cast<PropertiesModel *>(m_propertiesSortModel->sourceModel());
    const QModelIndexList &selectedIndexes = m_propertiesTree->selectionModel()->selectedIndexes();
    QTC_ASSERT(!selectedIndexes.isEmpty(), return nullptr);

    const QModelIndex idx = m_propertiesSortModel->mapToSource(selectedIndexes.first());
    return static_cast<PropertyTreeItem *>(propertiesModel->itemForIndex(idx));
}

} // namespace Internal
} // namespace Squish
