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

#include "externaltoolconfig.h"
#include "ui_externaltoolconfig.h"

#include <utils/algorithm.h>
#include <utils/hostosinfo.h>
#include <utils/macroexpander.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/fancylineedit.h>
#include <utils/environmentdialog.h>

#include <coreplugin/coreconstants.h>
#include <coreplugin/variablechooser.h>

#include <QDialogButtonBox>
#include <QTextStream>
#include <QMimeData>
#include <QMenu>
#include <QPlainTextEdit>

using namespace Core;
using namespace Core::Internal;


static const Qt::ItemFlags TOOLSMENU_ITEM_FLAGS = Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDropEnabled;
static const Qt::ItemFlags CATEGORY_ITEM_FLAGS = Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDropEnabled | Qt::ItemIsEditable;
static const Qt::ItemFlags TOOL_ITEM_FLAGS = Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsEditable;

// #pragma mark -- ExternalToolModel

ExternalToolModel::ExternalToolModel(QObject *parent)
    : QAbstractItemModel(parent)
{
}

ExternalToolModel::~ExternalToolModel()
{
    QMapIterator<QString, QList<ExternalTool *> > it(m_tools);
    while (it.hasNext()) {
        it.next();
        qDeleteAll(it.value());
    }
}

Qt::DropActions ExternalToolModel::supportedDropActions() const
{
    return Qt::MoveAction;
}

int ExternalToolModel::columnCount(const QModelIndex &parent) const
{
    bool categoryFound;
    categoryForIndex(parent, &categoryFound);
    if (!parent.isValid() || toolForIndex(parent) || categoryFound)
        return 1;
    return 0;
}

QVariant ExternalToolModel::data(const QModelIndex &index, int role) const
{
    if (ExternalTool *tool = toolForIndex(index))
        return data(tool, role);
    bool found;
    QString category = categoryForIndex(index, &found);
    if (found)
        return data(category, role);
    return QVariant();
}

QVariant ExternalToolModel::data(ExternalTool *tool, int role) const
{
    switch (role) {
    case Qt::DisplayRole:
    case Qt::EditRole:
        return tool->displayName();
    default:
        break;
    }
    return QVariant();
}

QVariant ExternalToolModel::data(const QString &category, int role) const
{
    switch (role) {
    case Qt::DisplayRole:
    case Qt::EditRole:
        return category.isEmpty() ? tr("Uncategorized") : category;
    case Qt::ToolTipRole:
        return category.isEmpty() ? tr("Tools that will appear directly under the External Tools menu.") : QVariant();
    default:
        break;
    }
    return QVariant();
}

QMimeData *ExternalToolModel::mimeData(const QModelIndexList &indexes) const
{
    if (indexes.isEmpty())
        return 0;
    QModelIndex modelIndex = indexes.first();
    ExternalTool *tool = toolForIndex(modelIndex);
    QTC_ASSERT(tool, return 0);
    bool found;
    QString category = categoryForIndex(modelIndex.parent(), &found);
    QTC_ASSERT(found, return 0);
    QMimeData *md = new QMimeData();
    QByteArray ba;
    QDataStream stream(&ba, QIODevice::WriteOnly);
    stream << category << m_tools.value(category).indexOf(tool);
    md->setData(QLatin1String("application/qtcreator-externaltool-config"), ba);
    return md;
}

bool ExternalToolModel::dropMimeData(const QMimeData *data,
                                     Qt::DropAction action,
                                     int row,
                                     int column,
                                     const QModelIndex &parent)
{
    Q_UNUSED(column)
    if (action != Qt::MoveAction || !data)
        return false;
    bool found;
    QString toCategory = categoryForIndex(parent, &found);
    QTC_ASSERT(found, return false);
    QByteArray ba = data->data(QLatin1String("application/qtcreator-externaltool-config"));
    if (ba.isEmpty())
        return false;
    QDataStream stream(&ba, QIODevice::ReadOnly);
    QString category;
    int pos = -1;
    stream >> category;
    stream >> pos;
    QList<ExternalTool *> &items = m_tools[category];
    QTC_ASSERT(pos >= 0 && pos < items.count(), return false);
    beginRemoveRows(index(m_tools.keys().indexOf(category), 0), pos, pos);
    ExternalTool *tool = items.takeAt(pos);
    endRemoveRows();
    if (row < 0)
        row = m_tools.value(toCategory).count();
    beginInsertRows(index(m_tools.keys().indexOf(toCategory), 0), row, row);
    m_tools[toCategory].insert(row, tool);
    endInsertRows();
    return true;
}

QStringList ExternalToolModel::mimeTypes() const
{
    return QStringList("application/qtcreator-externaltool-config");
}

QModelIndex ExternalToolModel::index(int row, int column, const QModelIndex &parent) const
{
    if (column == 0 && parent.isValid()) {
        bool found;
        QString category = categoryForIndex(parent, &found);
        if (found) {
            QList<ExternalTool *> items = m_tools.value(category);
            if (row < items.count())
                return createIndex(row, 0, items.at(row));
        }
    } else if (column == 0 && row < m_tools.keys().count()) {
        return createIndex(row, 0);
    }
    return QModelIndex();
}

QModelIndex ExternalToolModel::parent(const QModelIndex &child) const
{
    if (ExternalTool *tool = toolForIndex(child)) {
        int categoryIndex = 0;
        QMapIterator<QString, QList<ExternalTool *> > it(m_tools);
        while (it.hasNext()) {
            it.next();
            if (it.value().contains(tool))
                return index(categoryIndex, 0);
            ++categoryIndex;
        }
    }
    return QModelIndex();
}

int ExternalToolModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return m_tools.keys().count();
    if (toolForIndex(parent))
        return 0;
    bool found;
    QString category = categoryForIndex(parent, &found);
    if (found)
        return m_tools.value(category).count();

    return 0;
}

Qt::ItemFlags ExternalToolModel::flags(const QModelIndex &index) const
{
    if (toolForIndex(index))
        return TOOL_ITEM_FLAGS;
    bool found;
    QString category = categoryForIndex(index, &found);
    if (found) {
        if (category.isEmpty())
            return TOOLSMENU_ITEM_FLAGS;
        return CATEGORY_ITEM_FLAGS;
    }
    return 0;
}

bool ExternalToolModel::setData(const QModelIndex &modelIndex, const QVariant &value, int role)
{
    if (role != Qt::EditRole)
        return false;
    QString string = value.toString();
    if (ExternalTool *tool = toolForIndex(modelIndex)) {
        if (string.isEmpty() || tool->displayName() == string)
            return false;
        // rename tool
        tool->setDisplayName(string);
        emit dataChanged(modelIndex, modelIndex);
        return true;
    } else {
        bool found;
        QString category = categoryForIndex(modelIndex, &found);
        if (found) {
            if (string.isEmpty() || m_tools.contains(string))
                return false;
            // rename category
            QList<QString> categories = m_tools.keys();
            int previousIndex = categories.indexOf(category);
            categories.removeAt(previousIndex);
            categories.append(string);
            Utils::sort(categories);
            int newIndex = categories.indexOf(string);
            if (newIndex != previousIndex) {
                // we have same parent so we have to do special stuff for beginMoveRows...
                int beginMoveRowsSpecialIndex = (previousIndex < newIndex ? newIndex + 1 : newIndex);
                beginMoveRows(QModelIndex(), previousIndex, previousIndex, QModelIndex(), beginMoveRowsSpecialIndex);
            }
            QList<ExternalTool *> items = m_tools.take(category);
            m_tools.insert(string, items);
            if (newIndex != previousIndex)
                endMoveRows();
            return true;
        }
    }
    return false;
}

void ExternalToolModel::setTools(const QMap<QString, QList<ExternalTool *> > &tools)
{
    beginResetModel();
    m_tools = tools;
    endResetModel();
}

QMap<QString, QList<ExternalTool *> > ExternalToolModel::tools() const
{
    return m_tools;
}

ExternalTool *ExternalToolModel::toolForIndex(const QModelIndex &index) const
{
    return static_cast<ExternalTool *>(index.internalPointer());
}

QString ExternalToolModel::categoryForIndex(const QModelIndex &index, bool *found) const
{
    if (index.isValid() && !index.parent().isValid() && index.column() == 0 && index.row() >= 0) {
        const QList<QString> &keys = m_tools.keys();
        if (index.row() < keys.count()) {
            if (found) *found = true;
            return keys.at(index.row());
        }
    }
    if (found) *found = false;
    return QString();
}

void ExternalToolModel::revertTool(const QModelIndex &modelIndex)
{
    ExternalTool *tool = toolForIndex(modelIndex);
    QTC_ASSERT(tool, return);
    QTC_ASSERT(tool->preset() && !tool->preset()->fileName().isEmpty(), return);
    ExternalTool *resetTool = new ExternalTool(tool->preset().data());
    resetTool->setPreset(tool->preset());
    (*tool) = (*resetTool);
    delete resetTool;
    emit dataChanged(modelIndex, modelIndex);
}

QModelIndex ExternalToolModel::addCategory()
{
    const QString &categoryBase = tr("New Category");
    QString category = categoryBase;
    int count = 0;
    while (m_tools.contains(category)) {
        ++count;
        category = categoryBase + QString::number(count);
    }
    QList<QString> categories = m_tools.keys();
    categories.append(category);
    Utils::sort(categories);
    int pos = categories.indexOf(category);

    beginInsertRows(QModelIndex(), pos, pos);
    m_tools.insert(category, QList<ExternalTool *>());
    endInsertRows();
    return index(pos, 0);
}

QModelIndex ExternalToolModel::addTool(const QModelIndex &atIndex)
{
    bool found;
    QString category = categoryForIndex(atIndex, &found);
    if (!found)
        category = categoryForIndex(atIndex.parent(), &found);

    ExternalTool *tool = new ExternalTool;
    tool->setDisplayCategory(category);
    tool->setDisplayName(tr("New Tool"));
    tool->setDescription(tr("This tool prints a line of useful text"));
    //: Sample external tool text
    const QString text = tr("Useful text");
    if (Utils::HostOsInfo::isWindowsHost()) {
        tool->setExecutables(QStringList(QLatin1String("cmd")));
        tool->setArguments(QLatin1String("/c echo ") + text);
    } else {
        tool->setExecutables(QStringList(QLatin1String("echo")));
        tool->setArguments(text);
    }

    int pos;
    QModelIndex parent;
    if (atIndex.parent().isValid()) {
        pos = atIndex.row() + 1;
        parent = atIndex.parent();
    } else {
        pos = m_tools.value(category).count();
        parent = atIndex;
    }
    beginInsertRows(parent, pos, pos);
    m_tools[category].insert(pos, tool);
    endInsertRows();
    return index(pos, 0, parent);
}

void ExternalToolModel::removeTool(const QModelIndex &modelIndex)
{
    ExternalTool *tool = toolForIndex(modelIndex);
    QTC_ASSERT(tool, return);
    QTC_ASSERT(!tool->preset(), return);
    // remove the tool and the tree item
    int categoryIndex = 0;
    QMutableMapIterator<QString, QList<ExternalTool *> > it(m_tools);
    while (it.hasNext()) {
        it.next();
        QList<ExternalTool *> &items = it.value();
        int pos = items.indexOf(tool);
        if (pos != -1) {
            beginRemoveRows(index(categoryIndex, 0), pos, pos);
            items.removeAt(pos);
            endRemoveRows();
            break;
        }
        ++categoryIndex;
    }
    delete tool;
}

// #pragma mark -- ExternalToolConfig

ExternalToolConfig::ExternalToolConfig(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ExternalToolConfig),
    m_model(new ExternalToolModel(this))
{
    ui->setupUi(this);
    ui->toolTree->setModel(m_model);
    ui->toolTree->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);

    connect(ui->toolTree->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &ExternalToolConfig::handleCurrentChanged);

    auto chooser = new VariableChooser(this);
    chooser->addSupportedWidget(ui->executable->lineEdit());
    chooser->addSupportedWidget(ui->arguments);
    chooser->addSupportedWidget(ui->workingDirectory->lineEdit());
    chooser->addSupportedWidget(ui->inputText);

    connect(ui->description, &QLineEdit::editingFinished,
            this, &ExternalToolConfig::updateCurrentItem);
    connect(ui->executable, &Utils::PathChooser::editingFinished,
            this, &ExternalToolConfig::updateCurrentItem);
    connect(ui->executable, &Utils::PathChooser::browsingFinished,
            this, &ExternalToolConfig::updateCurrentItem);
    connect(ui->arguments, &QLineEdit::editingFinished, this, &ExternalToolConfig::updateCurrentItem);
    connect(ui->arguments, &QLineEdit::editingFinished,
            this, &ExternalToolConfig::updateEffectiveArguments);
    connect(ui->workingDirectory, &Utils::PathChooser::editingFinished,
            this, &ExternalToolConfig::updateCurrentItem);
    connect(ui->workingDirectory, &Utils::PathChooser::browsingFinished,
            this, &ExternalToolConfig::updateCurrentItem);
    connect(ui->environmentButton, &QAbstractButton::clicked,
            this, &ExternalToolConfig::editEnvironmentChanges);
    connect(ui->outputBehavior, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated),
            this, &ExternalToolConfig::updateCurrentItem);
    connect(ui->errorOutputBehavior, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated),
            this, &ExternalToolConfig::updateCurrentItem);
    connect(ui->modifiesDocumentCheckbox, &QAbstractButton::clicked,
            this, &ExternalToolConfig::updateCurrentItem);
    connect(ui->inputText, &QPlainTextEdit::textChanged, this, &ExternalToolConfig::updateCurrentItem);

    connect(ui->revertButton, &QAbstractButton::clicked, this, &ExternalToolConfig::revertCurrentItem);
    connect(ui->removeButton, &QAbstractButton::clicked, this, &ExternalToolConfig::removeTool);

    QMenu *menu = new QMenu(ui->addButton);
    ui->addButton->setMenu(menu);
    QAction *addTool = new QAction(tr("Add Tool"), this);
    menu->addAction(addTool);
    connect(addTool, &QAction::triggered, this, &ExternalToolConfig::addTool);
    QAction *addCategory = new QAction(tr("Add Category"), this);
    menu->addAction(addCategory);
    connect(addCategory, &QAction::triggered, this, &ExternalToolConfig::addCategory);

    showInfoForItem(QModelIndex());

}

ExternalToolConfig::~ExternalToolConfig()
{
    delete ui;
}

void ExternalToolConfig::setTools(const QMap<QString, QList<ExternalTool *> > &tools)
{
    QMap<QString, QList<ExternalTool *> > toolsCopy;
    QMapIterator<QString, QList<ExternalTool *> > it(tools);
    while (it.hasNext()) {
        it.next();
        QList<ExternalTool *> itemCopy;
        foreach (ExternalTool *tool, it.value())
            itemCopy.append(new ExternalTool(tool));
        toolsCopy.insert(it.key(), itemCopy);
    }
    if (!toolsCopy.contains(QString()))
        toolsCopy.insert(QString(), QList<ExternalTool *>());
    m_model->setTools(toolsCopy);
    ui->toolTree->expandAll();
}

void ExternalToolConfig::handleCurrentChanged(const QModelIndex &now, const QModelIndex &previous)
{
    updateItem(previous);
    showInfoForItem(now);
}

void ExternalToolConfig::updateButtons(const QModelIndex &index)
{
    ExternalTool *tool = m_model->toolForIndex(index);
    if (!tool) {
        ui->removeButton->setEnabled(false);
        ui->revertButton->setEnabled(false);
        return;
    }
    if (!tool->preset()) {
        ui->removeButton->setEnabled(true);
        ui->revertButton->setEnabled(false);
    } else {
        ui->removeButton->setEnabled(false);
        ui->revertButton->setEnabled((*tool) != (*(tool->preset())));
    }
}

void ExternalToolConfig::updateCurrentItem()
{
    QModelIndex index = ui->toolTree->selectionModel()->currentIndex();
    updateItem(index);
    updateButtons(index);
}

void ExternalToolConfig::updateItem(const QModelIndex &index)
{
    ExternalTool *tool = m_model->toolForIndex(index);
    if (!tool)
        return;
    tool->setDescription(ui->description->text());
    QStringList executables = tool->executables();
    if (executables.size() > 0)
        executables[0] = ui->executable->rawPath();
    else
        executables << ui->executable->rawPath();
    tool->setExecutables(executables);
    tool->setArguments(ui->arguments->text());
    tool->setWorkingDirectory(ui->workingDirectory->rawPath());
    tool->setEnvironment(m_environment);
    tool->setOutputHandling((ExternalTool::OutputHandling)ui->outputBehavior->currentIndex());
    tool->setErrorHandling((ExternalTool::OutputHandling)ui->errorOutputBehavior->currentIndex());
    tool->setModifiesCurrentDocument(ui->modifiesDocumentCheckbox->checkState());
    tool->setInput(ui->inputText->toPlainText());
}

void ExternalToolConfig::showInfoForItem(const QModelIndex &index)
{
    updateButtons(index);
    ExternalTool *tool = m_model->toolForIndex(index);
    if (!tool) {
        ui->description->clear();
        ui->executable->setPath(QString());
        ui->arguments->clear();
        ui->workingDirectory->setPath(QString());
        ui->inputText->clear();
        ui->infoWidget->setEnabled(false);
        m_environment.clear();
        return;
    }
    ui->infoWidget->setEnabled(true);
    ui->description->setText(tool->description());
    ui->executable->setPath(tool->executables().isEmpty() ? QString() : tool->executables().first());
    ui->arguments->setText(tool->arguments());
    ui->workingDirectory->setPath(tool->workingDirectory());
    ui->outputBehavior->setCurrentIndex((int)tool->outputHandling());
    ui->errorOutputBehavior->setCurrentIndex((int)tool->errorHandling());
    ui->modifiesDocumentCheckbox->setChecked(tool->modifiesCurrentDocument());
    m_environment = tool->environment();

    bool blocked = ui->inputText->blockSignals(true);
    ui->inputText->setPlainText(tool->input());
    ui->inputText->blockSignals(blocked);

    ui->description->setCursorPosition(0);
    ui->arguments->setCursorPosition(0);
    updateEnvironmentLabel();
    updateEffectiveArguments();
}

QMap<QString, QList<ExternalTool *> > ExternalToolConfig::tools() const
{
    return m_model->tools();
}

void ExternalToolConfig::apply()
{
    QModelIndex index = ui->toolTree->selectionModel()->currentIndex();
    updateItem(index);
    updateButtons(index);
}

void ExternalToolConfig::revertCurrentItem()
{
    QModelIndex index = ui->toolTree->selectionModel()->currentIndex();
    m_model->revertTool(index);
    showInfoForItem(index);
}

void ExternalToolConfig::addTool()
{
    QModelIndex currentIndex = ui->toolTree->selectionModel()->currentIndex();
    if (!currentIndex.isValid()) // default to Uncategorized
        currentIndex = m_model->index(0, 0);
    QModelIndex index = m_model->addTool(currentIndex);
    ui->toolTree->selectionModel()->setCurrentIndex(index, QItemSelectionModel::Clear);
    ui->toolTree->selectionModel()->setCurrentIndex(index, QItemSelectionModel::SelectCurrent);
    ui->toolTree->edit(index);
}

void ExternalToolConfig::removeTool()
{
    QModelIndex currentIndex = ui->toolTree->selectionModel()->currentIndex();
    ui->toolTree->selectionModel()->setCurrentIndex(QModelIndex(), QItemSelectionModel::Clear);
    m_model->removeTool(currentIndex);
}

void ExternalToolConfig::addCategory()
{
    QModelIndex index = m_model->addCategory();
    ui->toolTree->selectionModel()->setCurrentIndex(index, QItemSelectionModel::Clear);
    ui->toolTree->selectionModel()->setCurrentIndex(index, QItemSelectionModel::SelectCurrent);
    ui->toolTree->edit(index);
}

void ExternalToolConfig::updateEffectiveArguments()
{
    ui->arguments->setToolTip(Utils::globalMacroExpander()->expandProcessArgs(ui->arguments->text()));
}

void ExternalToolConfig::editEnvironmentChanges()
{
    bool ok;
    const QString placeholderText = Utils::HostOsInfo::isWindowsHost()
            ? tr("PATH=C:\\dev\\bin;${PATH}")
            : tr("PATH=/opt/bin:${PATH}");
    const QList<Utils::EnvironmentItem> newItems =
            Utils::EnvironmentDialog::getEnvironmentItems(&ok, ui->environmentLabel,
                                                          m_environment,
                                                          placeholderText);
    if (!ok)
        return;

    m_environment = newItems;
    updateEnvironmentLabel();
}

void ExternalToolConfig::updateEnvironmentLabel()
{
    QString shortSummary = Utils::EnvironmentItem::toStringList(m_environment).join(QLatin1String("; "));
    QFontMetrics fm(ui->environmentLabel->font());
    shortSummary = fm.elidedText(shortSummary, Qt::ElideRight, ui->environmentLabel->width());
    ui->environmentLabel->setText(shortSummary.isEmpty() ? tr("No changes to apply.") : shortSummary);
}
