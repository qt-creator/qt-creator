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

#include "ioptionspage.h"
#include "ui_externaltoolconfig.h"

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/environmentdialog.h>
#include <utils/fancylineedit.h>
#include <utils/hostosinfo.h>
#include <utils/macroexpander.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <coreplugin/coreconstants.h>
#include <coreplugin/externaltool.h>
#include <coreplugin/externaltoolmanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/variablechooser.h>

#include <QDialogButtonBox>
#include <QDir>
#include <QFileInfo>
#include <QMenu>
#include <QMimeData>
#include <QPlainTextEdit>
#include <QTextStream>

using namespace Utils;

namespace Core {
namespace Internal {

const Qt::ItemFlags TOOLSMENU_ITEM_FLAGS = Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDropEnabled;
const Qt::ItemFlags CATEGORY_ITEM_FLAGS = Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDropEnabled | Qt::ItemIsEditable;
const Qt::ItemFlags TOOL_ITEM_FLAGS = Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsEditable;

class ExternalToolModel final : public QAbstractItemModel
{
    Q_DECLARE_TR_FUNCTIONS(Core::ExternalToolConfig)

public:
    ExternalToolModel() = default;
    ~ExternalToolModel() final;

    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &modelIndex, int role = Qt::DisplayRole) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    Qt::ItemFlags flags(const QModelIndex &modelIndex) const override;
    bool setData(const QModelIndex &modelIndex, const QVariant &value, int role = Qt::EditRole) override;

    QMimeData *mimeData(const QModelIndexList &indexes) const override;
    bool dropMimeData(const QMimeData *data,
                      Qt::DropAction action,
                      int row,
                      int column,
                      const QModelIndex &parent) override;
    QStringList mimeTypes() const override;

    void setTools(const QMap<QString, QList<ExternalTool *> > &tools);
    QMap<QString, QList<ExternalTool *> > tools() const { return m_tools; }

    ExternalTool *toolForIndex(const QModelIndex &modelIndex) const;
    QString categoryForIndex(const QModelIndex &modelIndex, bool *found) const;
    void revertTool(const QModelIndex &modelIndex);
    QModelIndex addCategory();
    QModelIndex addTool(const QModelIndex &atIndex);
    void removeTool(const QModelIndex &modelIndex);
    Qt::DropActions supportedDropActions() const override { return Qt::MoveAction; }

private:
    QVariant data(ExternalTool *tool, int role = Qt::DisplayRole) const;
    QVariant data(const QString &category, int role = Qt::DisplayRole) const;

    QMap<QString, QList<ExternalTool *> > m_tools;
};

ExternalToolModel::~ExternalToolModel()
{
    for (QList<ExternalTool *> &toolInCategory : m_tools)
        qDeleteAll(toolInCategory);
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
        return nullptr;
    QModelIndex modelIndex = indexes.first();
    ExternalTool *tool = toolForIndex(modelIndex);
    QTC_ASSERT(tool, return nullptr);
    bool found;
    QString category = categoryForIndex(modelIndex.parent(), &found);
    QTC_ASSERT(found, return nullptr);
    auto md = new QMimeData();
    QByteArray ba;
    QDataStream stream(&ba, QIODevice::WriteOnly);
    stream << category << m_tools.value(category).indexOf(tool);
    md->setData("application/qtcreator-externaltool-config", ba);
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
    QByteArray ba = data->data("application/qtcreator-externaltool-config");
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
    } else if (column == 0 && row < m_tools.size()) {
        return createIndex(row, 0);
    }
    return QModelIndex();
}

QModelIndex ExternalToolModel::parent(const QModelIndex &child) const
{
    if (ExternalTool *tool = toolForIndex(child)) {
        int categoryIndex = 0;
        for (const QList<ExternalTool *> &toolsInCategory : m_tools) {
            if (toolsInCategory.contains(tool))
                return index(categoryIndex, 0);
            ++categoryIndex;
        }
    }
    return QModelIndex();
}

int ExternalToolModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return m_tools.size();
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
    return {};
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
    auto resetTool = new ExternalTool(tool->preset().data());
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

    auto tool = new ExternalTool;
    tool->setDisplayCategory(category);
    tool->setDisplayName(tr("New Tool"));
    tool->setDescription(tr("This tool prints a line of useful text"));
    //: Sample external tool text
    const QString text = tr("Useful text");
    if (HostOsInfo::isWindowsHost()) {
        tool->setExecutables({"cmd"});
        tool->setArguments("/c echo " + text);
    } else {
        tool->setExecutables({"echo"});
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
    for (QList<ExternalTool *> &items : m_tools) {
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

static void fillBaseEnvironmentComboBox(QComboBox *box)
{
    box->clear();
    box->addItem(ExternalTool::tr("System Environment"), QByteArray());
    for (const EnvironmentProvider &provider : EnvironmentProvider::providers())
        box->addItem(provider.displayName, Id::fromName(provider.id).toSetting());
}

class ExternalToolConfig final : public IOptionsPageWidget
{
    Q_DECLARE_TR_FUNCTIONS(Core::ExternalToolConfig)

public:
    ExternalToolConfig();

    void setTools(const QMap<QString, QList<ExternalTool *> > &tools);
    QMap<QString, QList<ExternalTool *> > tools() const { return m_model.tools(); }

    void apply() final;

private:
    void handleCurrentChanged(const QModelIndex &now, const QModelIndex &previous);
    void showInfoForItem(const QModelIndex &index);
    void updateItem(const QModelIndex &index);
    void revertCurrentItem();
    void updateButtons(const QModelIndex &index);
    void updateCurrentItem();
    void addTool();
    void removeTool();
    void addCategory();
    void updateEffectiveArguments();
    void editEnvironmentChanges();
    void updateEnvironmentLabel();

    Ui::ExternalToolConfig m_ui;
    EnvironmentItems m_environment;
    ExternalToolModel m_model;
};

ExternalToolConfig::ExternalToolConfig()
{
    m_ui.setupUi(this);
    m_ui.executable->setExpectedKind(PathChooser::ExistingCommand);
    m_ui.scrollArea->viewport()->setAutoFillBackground(false);
    m_ui.scrollAreaWidgetContents->setAutoFillBackground(false);
    m_ui.toolTree->setModel(&m_model);
    m_ui.toolTree->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);

    connect(m_ui.toolTree->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &ExternalToolConfig::handleCurrentChanged);

    auto chooser = new VariableChooser(this);
    chooser->addSupportedWidget(m_ui.executable->lineEdit());
    chooser->addSupportedWidget(m_ui.arguments);
    chooser->addSupportedWidget(m_ui.workingDirectory->lineEdit());
    chooser->addSupportedWidget(m_ui.inputText);

    fillBaseEnvironmentComboBox(m_ui.baseEnvironment);

    connect(m_ui.description, &QLineEdit::editingFinished,
            this, &ExternalToolConfig::updateCurrentItem);
    connect(m_ui.executable, &PathChooser::editingFinished,
            this, &ExternalToolConfig::updateCurrentItem);
    connect(m_ui.executable, &PathChooser::browsingFinished,
            this, &ExternalToolConfig::updateCurrentItem);
    connect(m_ui.arguments, &QLineEdit::editingFinished,
            this, &ExternalToolConfig::updateCurrentItem);
    connect(m_ui.arguments, &QLineEdit::editingFinished,
            this, &ExternalToolConfig::updateEffectiveArguments);
    connect(m_ui.workingDirectory, &PathChooser::editingFinished,
            this, &ExternalToolConfig::updateCurrentItem);
    connect(m_ui.workingDirectory, &PathChooser::browsingFinished,
            this, &ExternalToolConfig::updateCurrentItem);
    connect(m_ui.environmentButton, &QAbstractButton::clicked,
            this, &ExternalToolConfig::editEnvironmentChanges);
    connect(m_ui.outputBehavior, QOverload<int>::of(&QComboBox::activated),
            this, &ExternalToolConfig::updateCurrentItem);
    connect(m_ui.errorOutputBehavior, QOverload<int>::of(&QComboBox::activated),
            this, &ExternalToolConfig::updateCurrentItem);
    connect(m_ui.modifiesDocumentCheckbox, &QAbstractButton::clicked,
            this, &ExternalToolConfig::updateCurrentItem);
    connect(m_ui.inputText, &QPlainTextEdit::textChanged,
            this, &ExternalToolConfig::updateCurrentItem);

    connect(m_ui.revertButton, &QAbstractButton::clicked,
            this, &ExternalToolConfig::revertCurrentItem);
    connect(m_ui.removeButton, &QAbstractButton::clicked,
            this, &ExternalToolConfig::removeTool);

    auto menu = new QMenu(m_ui.addButton);
    m_ui.addButton->setMenu(menu);
    auto addTool = new QAction(tr("Add Tool"), this);
    menu->addAction(addTool);
    connect(addTool, &QAction::triggered, this, &ExternalToolConfig::addTool);
    auto addCategory = new QAction(tr("Add Category"), this);
    menu->addAction(addCategory);
    connect(addCategory, &QAction::triggered, this, &ExternalToolConfig::addCategory);

    showInfoForItem(QModelIndex());

    setTools(ExternalToolManager::toolsByCategory());
}

void ExternalToolConfig::setTools(const QMap<QString, QList<ExternalTool *> > &tools)
{
    QMap<QString, QList<ExternalTool *> > toolsCopy;
    for (auto it = tools.cbegin(), end = tools.cend(); it != end; ++it) {
        QList<ExternalTool *> itemCopy;
        for (ExternalTool *tool : it.value())
            itemCopy.append(new ExternalTool(tool));
        toolsCopy.insert(it.key(), itemCopy);
    }
    if (!toolsCopy.contains(QString()))
        toolsCopy.insert(QString(), QList<ExternalTool *>());
    m_model.setTools(toolsCopy);
    m_ui.toolTree->expandAll();
}

void ExternalToolConfig::handleCurrentChanged(const QModelIndex &now, const QModelIndex &previous)
{
    updateItem(previous);
    showInfoForItem(now);
}

void ExternalToolConfig::updateButtons(const QModelIndex &index)
{
    ExternalTool *tool = m_model.toolForIndex(index);
    if (!tool) {
        m_ui.removeButton->setEnabled(false);
        m_ui.revertButton->setEnabled(false);
        return;
    }
    if (!tool->preset()) {
        m_ui.removeButton->setEnabled(true);
        m_ui.revertButton->setEnabled(false);
    } else {
        m_ui.removeButton->setEnabled(false);
        m_ui.revertButton->setEnabled((*tool) != (*(tool->preset())));
    }
}

void ExternalToolConfig::updateCurrentItem()
{
    const QModelIndex index = m_ui.toolTree->selectionModel()->currentIndex();
    updateItem(index);
    updateButtons(index);
}

void ExternalToolConfig::updateItem(const QModelIndex &index)
{
    ExternalTool *tool = m_model.toolForIndex(index);
    if (!tool)
        return;
    tool->setDescription(m_ui.description->text());
    QStringList executables = tool->executables();
    if (executables.size() > 0)
        executables[0] = m_ui.executable->rawPath();
    else
        executables << m_ui.executable->rawPath();
    tool->setExecutables(executables);
    tool->setArguments(m_ui.arguments->text());
    tool->setWorkingDirectory(m_ui.workingDirectory->rawPath());
    tool->setBaseEnvironmentProviderId(Id::fromSetting(m_ui.baseEnvironment->currentData()));
    tool->setEnvironmentUserChanges(m_environment);
    tool->setOutputHandling(ExternalTool::OutputHandling(m_ui.outputBehavior->currentIndex()));
    tool->setErrorHandling(ExternalTool::OutputHandling(m_ui.errorOutputBehavior->currentIndex()));
    tool->setModifiesCurrentDocument(m_ui.modifiesDocumentCheckbox->checkState());
    tool->setInput(m_ui.inputText->toPlainText());
}

void ExternalToolConfig::showInfoForItem(const QModelIndex &index)
{
    updateButtons(index);
    ExternalTool *tool = m_model.toolForIndex(index);
    if (!tool) {
        m_ui.description->clear();
        m_ui.executable->setPath(QString());
        m_ui.arguments->clear();
        m_ui.workingDirectory->setPath(QString());
        m_ui.inputText->clear();
        m_ui.infoWidget->setEnabled(false);
        m_environment.clear();
        return;
    }
    m_ui.infoWidget->setEnabled(true);
    m_ui.description->setText(tool->description());
    m_ui.executable->setPath(tool->executables().isEmpty() ? QString()
                                                          : tool->executables().constFirst());
    m_ui.arguments->setText(tool->arguments());
    m_ui.workingDirectory->setPath(tool->workingDirectory());
    m_ui.outputBehavior->setCurrentIndex(int(tool->outputHandling()));
    m_ui.errorOutputBehavior->setCurrentIndex(int(tool->errorHandling()));
    m_ui.modifiesDocumentCheckbox->setChecked(tool->modifiesCurrentDocument());
    const int baseEnvironmentIndex = m_ui.baseEnvironment->findData(
        tool->baseEnvironmentProviderId().toSetting());
    m_ui.baseEnvironment->setCurrentIndex(std::max(0, baseEnvironmentIndex));
    m_environment = tool->environmentUserChanges();

    {
        QSignalBlocker blocker(m_ui.inputText);
        m_ui.inputText->setPlainText(tool->input());
    }

    m_ui.description->setCursorPosition(0);
    m_ui.arguments->setCursorPosition(0);
    updateEnvironmentLabel();
    updateEffectiveArguments();
}

static QString getUserFilePath(const QString &proposalFileName)
{
    const QDir resourceDir(ICore::userResourcePath());
    if (!resourceDir.exists(QLatin1String("externaltools")))
        resourceDir.mkpath(QLatin1String("externaltools"));
    const QFileInfo fi(proposalFileName);
    const QString &suffix = QLatin1Char('.') + fi.completeSuffix();
    const QString &newFilePath = ICore::userResourcePath()
            + QLatin1String("/externaltools/") + fi.baseName();
    int count = 0;
    QString tryPath = newFilePath + suffix;
    while (QFile::exists(tryPath)) {
        if (++count > 15)
            return QString();
        // add random number
        const int number = qrand() % 1000;
        tryPath = newFilePath + QString::number(number) + suffix;
    }
    return tryPath;
}

static QString idFromDisplayName(const QString &displayName)
{
    QString id = displayName;
    id.remove(QRegularExpression("&(?!&)"));
    QChar *c = id.data();
    while (!c->isNull()) {
        if (!c->isLetterOrNumber())
            *c = QLatin1Char('_');
        ++c;
    }
    return id;
}

static QString findUnusedId(const QString &proposal, const QMap<QString, QList<ExternalTool *> > &tools)
{
    int number = 0;
    QString result;
    bool found = false;
    do {
        result = proposal + (number > 0 ? QString::number(number) : QString::fromLatin1(""));
        ++number;
        found = false;
        for (auto it = tools.cbegin(), end = tools.cend(); it != end; ++it) {
            foreach (ExternalTool *tool, it.value()) {
                if (tool->id() == result) {
                    found = true;
                    break;
                }
            }
        }
    } while (found);
    return result;
}

void ExternalToolConfig::apply()
{
    QModelIndex index = m_ui.toolTree->selectionModel()->currentIndex();
    updateItem(index);
    updateButtons(index);

    QMap<QString, ExternalTool *> originalTools = ExternalToolManager::toolsById();
    QMap<QString, QList<ExternalTool *> > newToolsMap = tools();
    QMap<QString, QList<ExternalTool *> > resultMap;
    for (auto it = newToolsMap.cbegin(), end = newToolsMap.cend(); it != end; ++it) {
        QList<ExternalTool *> items;
        foreach (ExternalTool *tool, it.value()) {
            ExternalTool *toolToAdd = nullptr;
            if (ExternalTool *originalTool = originalTools.take(tool->id())) {
                // check if it has different category and is custom tool
                if (tool->displayCategory() != it.key() && !tool->preset())
                    tool->setDisplayCategory(it.key());
                // check if the tool has changed
                if ((*originalTool) == (*tool)) {
                    toolToAdd = originalTool;
                } else {
                    // case 1: tool is changed preset
                    if (tool->preset() && (*tool) != (*(tool->preset()))) {
                        // check if we need to choose a new file name
                        if (tool->preset()->fileName() == tool->fileName()) {
                            const QString &fileName = FilePath::fromString(tool->preset()->fileName()).fileName();
                            const QString &newFilePath = getUserFilePath(fileName);
                            // TODO error handling if newFilePath.isEmpty() (i.e. failed to find a unused name)
                            tool->setFileName(newFilePath);
                        }
                        // TODO error handling
                        tool->save();
                    // case 2: tool is previously changed preset but now same as preset
                    } else if (tool->preset() && (*tool) == (*(tool->preset()))) {
                        // check if we need to delete the changed description
                        if (originalTool->fileName() != tool->preset()->fileName()
                                && QFile::exists(originalTool->fileName())) {
                            // TODO error handling
                            QFile::remove(originalTool->fileName());
                        }
                        tool->setFileName(tool->preset()->fileName());
                        // no need to save, it's the same as the preset
                    // case 3: tool is custom tool
                    } else {
                        // TODO error handling
                        tool->save();
                    }

                     // 'tool' is deleted by config page, 'originalTool' is deleted by setToolsByCategory
                    toolToAdd = new ExternalTool(tool);
                }
            } else {
                // new tool. 'tool' is deleted by config page
                QString id = idFromDisplayName(tool->displayName());
                id = findUnusedId(id, newToolsMap);
                tool->setId(id);
                // TODO error handling if newFilePath.isEmpty() (i.e. failed to find a unused name)
                tool->setFileName(getUserFilePath(id + QLatin1String(".xml")));
                // TODO error handling
                tool->save();
                toolToAdd = new ExternalTool(tool);
            }
            items.append(toolToAdd);
        }
        if (!items.isEmpty())
            resultMap.insert(it.key(), items);
    }
    // Remove tools that have been deleted from the settings (and are no preset)
    foreach (ExternalTool *tool, originalTools) {
        QTC_ASSERT(!tool->preset(), continue);
        // TODO error handling
        QFile::remove(tool->fileName());
    }

    ExternalToolManager::setToolsByCategory(resultMap);
}

void ExternalToolConfig::revertCurrentItem()
{
    QModelIndex index = m_ui.toolTree->selectionModel()->currentIndex();
    m_model.revertTool(index);
    showInfoForItem(index);
}

void ExternalToolConfig::addTool()
{
    QModelIndex currentIndex = m_ui.toolTree->selectionModel()->currentIndex();
    if (!currentIndex.isValid()) // default to Uncategorized
        currentIndex = m_model.index(0, 0);
    QModelIndex index = m_model.addTool(currentIndex);
    m_ui.toolTree->selectionModel()->setCurrentIndex(index, QItemSelectionModel::Clear);
    m_ui.toolTree->selectionModel()->setCurrentIndex(index, QItemSelectionModel::SelectCurrent);
    m_ui.toolTree->edit(index);
}

void ExternalToolConfig::removeTool()
{
    QModelIndex currentIndex = m_ui.toolTree->selectionModel()->currentIndex();
    m_ui.toolTree->selectionModel()->setCurrentIndex(QModelIndex(), QItemSelectionModel::Clear);
    m_model.removeTool(currentIndex);
}

void ExternalToolConfig::addCategory()
{
    QModelIndex index = m_model.addCategory();
    m_ui.toolTree->selectionModel()->setCurrentIndex(index, QItemSelectionModel::Clear);
    m_ui.toolTree->selectionModel()->setCurrentIndex(index, QItemSelectionModel::SelectCurrent);
    m_ui.toolTree->edit(index);
}

void ExternalToolConfig::updateEffectiveArguments()
{
    m_ui.arguments->setToolTip(Utils::globalMacroExpander()->expandProcessArgs(m_ui.arguments->text()));
}

void ExternalToolConfig::editEnvironmentChanges()
{
    const QString placeholderText = HostOsInfo::isWindowsHost()
            ? tr("PATH=C:\\dev\\bin;${PATH}")
            : tr("PATH=/opt/bin:${PATH}");
    const auto newItems = EnvironmentDialog::getEnvironmentItems(m_ui.environmentLabel,
                                                                        m_environment,
                                                                        placeholderText);
    if (newItems) {
        m_environment = *newItems;
        updateEnvironmentLabel();
    }
}

void ExternalToolConfig::updateEnvironmentLabel()
{
    QString shortSummary = EnvironmentItem::toStringList(m_environment).join("; ");
    QFontMetrics fm(m_ui.environmentLabel->font());
    shortSummary = fm.elidedText(shortSummary, Qt::ElideRight, m_ui.environmentLabel->width());
    m_ui.environmentLabel->setText(shortSummary.isEmpty() ? tr("No changes to apply.") : shortSummary);
}

// ToolSettingsPage

ToolSettings::ToolSettings()
{
    setId(Constants::SETTINGS_ID_TOOLS);
    setDisplayName(ExternalToolConfig::tr("External Tools"));
    setCategory(Constants::SETTINGS_CATEGORY_CORE);
    setWidgetCreator([] { return new ExternalToolConfig; });
}

} // Internal
} // Core
