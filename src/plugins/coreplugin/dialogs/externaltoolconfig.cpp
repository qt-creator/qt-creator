// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "externaltoolconfig.h"

#include "ioptionspage.h"
#include "../coreconstants.h"
#include "../coreplugintr.h"
#include "../externaltool.h"
#include "../externaltoolmanager.h"
#include "../icore.h"

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/environmentdialog.h>
#include <utils/fancylineedit.h>
#include <utils/hostosinfo.h>
#include <utils/layoutbuilder.h>
#include <utils/macroexpander.h>
#include <utils/pathchooser.h>
#include <utils/process.h>
#include <utils/qtcassert.h>
#include <utils/variablechooser.h>

#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMimeData>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRandomGenerator>
#include <QScrollArea>
#include <QTextStream>
#include <QTreeView>

using namespace Utils;

namespace Core {
namespace Internal {

const Qt::ItemFlags TOOLSMENU_ITEM_FLAGS = Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDropEnabled;
const Qt::ItemFlags CATEGORY_ITEM_FLAGS = Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDropEnabled | Qt::ItemIsEditable;
const Qt::ItemFlags TOOL_ITEM_FLAGS = Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsEditable;

class ExternalToolModel final : public QAbstractItemModel
{
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

    static ExternalTool *toolForIndex(const QModelIndex &modelIndex);
    QString categoryForIndex(const QModelIndex &modelIndex, bool *found) const;
    void revertTool(const QModelIndex &modelIndex);
    QModelIndex addCategory();
    QModelIndex addTool(const QModelIndex &atIndex);
    void removeTool(const QModelIndex &modelIndex);
    Qt::DropActions supportedDropActions() const override { return Qt::MoveAction; }

private:
    static QVariant data(ExternalTool *tool, int role = Qt::DisplayRole);
    static QVariant data(const QString &category, int role = Qt::DisplayRole);

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

QVariant ExternalToolModel::data(ExternalTool *tool, int role)
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

QVariant ExternalToolModel::data(const QString &category, int role)
{
    switch (role) {
    case Qt::DisplayRole:
    case Qt::EditRole:
        return category.isEmpty() ? Tr::tr("Uncategorized") : category;
    case Qt::ToolTipRole:
        return category.isEmpty() ? Tr::tr("Tools that will appear directly under the External Tools menu.") : QVariant();
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
    return {"application/qtcreator-externaltool-config"};
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

ExternalTool *ExternalToolModel::toolForIndex(const QModelIndex &index)
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
    QTC_ASSERT(tool->preset() && !tool->preset()->filePath().isEmpty(), return);
    auto resetTool = new ExternalTool(tool->preset().data());
    resetTool->setPreset(tool->preset());
    (*tool) = (*resetTool);
    delete resetTool;
    emit dataChanged(modelIndex, modelIndex);
}

QModelIndex ExternalToolModel::addCategory()
{
    const QString &categoryBase = Tr::tr("New Category");
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
    tool->setDisplayName(Tr::tr("New Tool"));
    tool->setDescription(Tr::tr("This tool prints a line of useful text"));
    //: Sample external tool text
    const QString text = Tr::tr("Useful text");
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
    box->addItem(Tr::tr("System Environment"), QByteArray());
    for (const EnvironmentProvider &provider : EnvironmentProvider::providers())
        box->addItem(provider.displayName, Id::fromName(provider.id).toSetting());
}

class ExternalToolConfig final : public IOptionsPageWidget
{
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

    EnvironmentItems m_environment;
    ExternalToolModel m_model;

    QTreeView *m_toolTree;
    QPushButton *m_removeButton;
    QPushButton *m_revertButton;
    QWidget *m_infoWidget;
    QLineEdit *m_description;
    Utils::PathChooser *m_executable;
    QLineEdit *m_arguments;
    Utils::PathChooser *m_workingDirectory;
    QComboBox *m_outputBehavior;
    QLabel *m_environmentLabel;
    QComboBox *m_errorOutputBehavior;
    QCheckBox *m_modifiesDocumentCheckbox;
    QPlainTextEdit *m_inputText;
    QComboBox *m_baseEnvironment;
};

ExternalToolConfig::ExternalToolConfig()
{
    m_toolTree = new QTreeView(this);
    m_toolTree->setDragEnabled(true);
    m_toolTree->setDragDropMode(QAbstractItemView::InternalMove);
    m_toolTree->header()->setVisible(false);
    m_toolTree->header()->setDefaultSectionSize(21);

    auto addButton = new QPushButton(Tr::tr("Add"));
    addButton->setToolTip(Tr::tr("Add tool."));

    m_removeButton = new QPushButton(Tr::tr("Remove"));
    m_removeButton->setToolTip(Tr::tr("Remove tool."));

    m_revertButton = new QPushButton(Tr::tr("Reset"));
    m_revertButton->setToolTip(Tr::tr("Revert tool to default."));

    auto scrollArea = new QScrollArea(this);
    QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    sizePolicy.setHorizontalStretch(10);
    sizePolicy.setVerticalStretch(0);
    scrollArea->setSizePolicy(sizePolicy);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setFrameShadow(QFrame::Plain);
    scrollArea->setLineWidth(0);
    scrollArea->setWidgetResizable(true);

    auto scrollAreaWidgetContents = new QWidget();
    scrollAreaWidgetContents->setGeometry(QRect(0, 0, 396, 444));

    m_infoWidget = new QWidget(scrollAreaWidgetContents);
    QSizePolicy sizePolicy1(QSizePolicy::Preferred, QSizePolicy::Preferred);
    sizePolicy1.setHorizontalStretch(10);
    sizePolicy1.setVerticalStretch(0);
    m_infoWidget->setSizePolicy(sizePolicy1);

    m_description = new QLineEdit(m_infoWidget);

    m_executable = new PathChooser(m_infoWidget);

    m_arguments = new QLineEdit(m_infoWidget);

    m_workingDirectory = new PathChooser(m_infoWidget);

    auto outputLabel = new QLabel(Tr::tr("Output:"));
    outputLabel->setToolTip(Tr::tr("<html><head/><body>\n"
        "<p>What to do with the executable's standard output.\n"
        "<ul><li>Ignore: Do nothing with it.</li><li>Show in General Messages.</li>"
        "<li>Replace selection: Replace the current selection in the current document with it.</li>"
        "</ul></p></body></html>\n"));

    m_outputBehavior = new QComboBox(m_infoWidget);
    m_outputBehavior->addItem(Tr::tr("Ignore"));
    m_outputBehavior->addItem(Tr::tr("Show in General Messages"));
    m_outputBehavior->addItem(Tr::tr("Replace Selection"));

    auto errorOutputLabel = new QLabel(Tr::tr("Error output:"));
    errorOutputLabel->setToolTip(Tr::tr("<html><head><body>\n"
        "<p >What to do with the executable's standard error output.</p>\n"
        "<ul><li>Ignore: Do nothing with it.</li>\n"
        "<li>Show in General Messages.</li>\n"
        "<li>Replace selection: Replace the current selection in the current document with it.</li>\n"
        "</ul></body></html>"));

    m_errorOutputBehavior = new QComboBox(m_infoWidget);
    m_errorOutputBehavior->addItem(Tr::tr("Ignore"));
    m_errorOutputBehavior->addItem(Tr::tr("Show in General Messages"));
    m_errorOutputBehavior->addItem(Tr::tr("Replace Selection"));

    m_environmentLabel = new QLabel(Tr::tr("No changes to apply."));
    m_environmentLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    auto environmentButton = new QPushButton(Tr::tr("Change..."));

    m_modifiesDocumentCheckbox = new QCheckBox(Tr::tr("Modifies current document"));
    m_modifiesDocumentCheckbox->setToolTip(Tr::tr("If the tool modifies the current document, "
        "set this flag to ensure that the document is saved before "
        "running the tool and is reloaded after the tool finished."));

    auto inputLabel = new QLabel(Tr::tr("Input:"));
    inputLabel->setToolTip(Tr::tr("Text to pass to the executable via standard input. Leave "
                                  "empty if the executable should not receive any input."));

    m_inputText = new QPlainTextEdit(m_infoWidget);
    QSizePolicy sizePolicy3(QSizePolicy::Expanding, QSizePolicy::Expanding);
    sizePolicy3.setHorizontalStretch(0);
    sizePolicy3.setVerticalStretch(10);
    m_inputText->setSizePolicy(sizePolicy3);
    m_inputText->setLineWrapMode(QPlainTextEdit::NoWrap);

    m_baseEnvironment = new QComboBox(m_infoWidget);
    m_baseEnvironment->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    scrollArea->setWidget(scrollAreaWidgetContents);

    using namespace Layouting;

    Form {
        Tr::tr("Description:"), m_description, br,
        Tr::tr("Executable:"), m_executable, br,
        Tr::tr("Arguments:"), m_arguments, br,
        Tr::tr("Working directory:"), m_workingDirectory, br,
        outputLabel, m_outputBehavior, br,
        errorOutputLabel, m_errorOutputBehavior, br,
        Tr::tr("Base environment:"), m_baseEnvironment, br,
        Tr::tr("Environment:"),  m_environmentLabel, environmentButton, br,
        empty, m_modifiesDocumentCheckbox, br,
        inputLabel, m_inputText
    }.attachTo(m_infoWidget);

    Column {
        m_infoWidget, noMargin
    }.attachTo(scrollAreaWidgetContents);

    Row {
        Column {
            m_toolTree,
            Row { addButton, m_removeButton, st, m_revertButton }
        },
        scrollArea
    }.attachTo(this);

    m_executable->setExpectedKind(PathChooser::ExistingCommand);
    scrollArea->viewport()->setAutoFillBackground(false);
    scrollAreaWidgetContents->setAutoFillBackground(false);
    m_toolTree->setModel(&m_model);
    m_toolTree->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);

    connect(m_toolTree->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &ExternalToolConfig::handleCurrentChanged);

    auto chooser = new VariableChooser(this);
    chooser->addSupportedWidget(m_executable->lineEdit());
    chooser->addSupportedWidget(m_arguments);
    chooser->addSupportedWidget(m_workingDirectory->lineEdit());
    chooser->addSupportedWidget(m_inputText);

    fillBaseEnvironmentComboBox(m_baseEnvironment);

    connect(m_description, &QLineEdit::editingFinished,
            this, &ExternalToolConfig::updateCurrentItem);
    connect(m_executable, &PathChooser::editingFinished,
            this, &ExternalToolConfig::updateCurrentItem);
    connect(m_executable, &PathChooser::browsingFinished,
            this, &ExternalToolConfig::updateCurrentItem);
    connect(m_arguments, &QLineEdit::editingFinished,
            this, &ExternalToolConfig::updateCurrentItem);
    connect(m_arguments, &QLineEdit::editingFinished,
            this, &ExternalToolConfig::updateEffectiveArguments);
    connect(m_workingDirectory, &PathChooser::editingFinished,
            this, &ExternalToolConfig::updateCurrentItem);
    connect(m_workingDirectory, &PathChooser::browsingFinished,
            this, &ExternalToolConfig::updateCurrentItem);
    connect(environmentButton, &QAbstractButton::clicked,
            this, &ExternalToolConfig::editEnvironmentChanges);
    connect(m_outputBehavior, &QComboBox::activated,
            this, &ExternalToolConfig::updateCurrentItem);
    connect(m_errorOutputBehavior, &QComboBox::activated,
            this, &ExternalToolConfig::updateCurrentItem);
    connect(m_modifiesDocumentCheckbox, &QAbstractButton::clicked,
            this, &ExternalToolConfig::updateCurrentItem);
    connect(m_inputText, &QPlainTextEdit::textChanged,
            this, &ExternalToolConfig::updateCurrentItem);

    connect(m_revertButton, &QAbstractButton::clicked,
            this, &ExternalToolConfig::revertCurrentItem);
    connect(m_removeButton, &QAbstractButton::clicked,
            this, &ExternalToolConfig::removeTool);

    auto menu = new QMenu(addButton);
    addButton->setMenu(menu);
    auto addTool = new QAction(Tr::tr("Add Tool"), this);
    menu->addAction(addTool);
    connect(addTool, &QAction::triggered, this, &ExternalToolConfig::addTool);
    auto addCategory = new QAction(Tr::tr("Add Category"), this);
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
    m_toolTree->expandAll();
}

void ExternalToolConfig::handleCurrentChanged(const QModelIndex &now, const QModelIndex &previous)
{
    updateItem(previous);
    showInfoForItem(now);
}

void ExternalToolConfig::updateButtons(const QModelIndex &index)
{
    const ExternalTool *tool = ExternalToolModel::toolForIndex(index);
    if (!tool) {
        m_removeButton->setEnabled(false);
        m_revertButton->setEnabled(false);
        return;
    }
    if (!tool->preset()) {
        m_removeButton->setEnabled(true);
        m_revertButton->setEnabled(false);
    } else {
        m_removeButton->setEnabled(false);
        m_revertButton->setEnabled((*tool) != (*(tool->preset())));
    }
}

void ExternalToolConfig::updateCurrentItem()
{
    const QModelIndex index = m_toolTree->selectionModel()->currentIndex();
    updateItem(index);
    updateButtons(index);
}

void ExternalToolConfig::updateItem(const QModelIndex &index)
{
    ExternalTool *tool = ExternalToolModel::toolForIndex(index);
    if (!tool)
        return;
    tool->setDescription(m_description->text());
    FilePaths executables = tool->executables();
    if (executables.size() > 0)
        executables[0] = m_executable->rawFilePath();
    else
        executables << m_executable->rawFilePath();
    tool->setExecutables(executables);
    tool->setArguments(m_arguments->text());
    tool->setWorkingDirectory(m_workingDirectory->rawFilePath());
    tool->setBaseEnvironmentProviderId(Id::fromSetting(m_baseEnvironment->currentData()));
    tool->setEnvironmentUserChanges(m_environment);
    tool->setOutputHandling(ExternalTool::OutputHandling(m_outputBehavior->currentIndex()));
    tool->setErrorHandling(ExternalTool::OutputHandling(m_errorOutputBehavior->currentIndex()));
    tool->setModifiesCurrentDocument(m_modifiesDocumentCheckbox->checkState());
    tool->setInput(m_inputText->toPlainText());
}

void ExternalToolConfig::showInfoForItem(const QModelIndex &index)
{
    updateButtons(index);
    const ExternalTool *tool = ExternalToolModel::toolForIndex(index);
    if (!tool) {
        m_description->clear();
        m_executable->setFilePath({});
        m_arguments->clear();
        m_workingDirectory->setFilePath({});
        m_inputText->clear();
        m_infoWidget->setEnabled(false);
        m_environment.clear();
        return;
    }
    m_infoWidget->setEnabled(true);
    m_description->setText(tool->description());
    m_executable->setFilePath(tool->executables().isEmpty() ? FilePath()
                                                               : tool->executables().constFirst());
    m_arguments->setText(tool->arguments());
    m_workingDirectory->setFilePath(tool->workingDirectory());
    m_outputBehavior->setCurrentIndex(int(tool->outputHandling()));
    m_errorOutputBehavior->setCurrentIndex(int(tool->errorHandling()));
    m_modifiesDocumentCheckbox->setChecked(tool->modifiesCurrentDocument());
    const int baseEnvironmentIndex = m_baseEnvironment->findData(
        tool->baseEnvironmentProviderId().toSetting());
    m_baseEnvironment->setCurrentIndex(std::max(0, baseEnvironmentIndex));
    m_environment = tool->environmentUserChanges();

    {
        QSignalBlocker blocker(m_inputText);
        m_inputText->setPlainText(tool->input());
    }

    m_description->setCursorPosition(0);
    m_arguments->setCursorPosition(0);
    updateEnvironmentLabel();
    updateEffectiveArguments();
}

static FilePath getUserFilePath(const QString &proposalFileName)
{
    const FilePath resourceDir(ICore::userResourcePath());
    const FilePath externalToolsDir = resourceDir / "externaltools";
    if (!externalToolsDir.isDir())
        externalToolsDir.createDir();

    const FilePath proposal = FilePath::fromString(proposalFileName);
    const QString suffix = QLatin1Char('.') + proposal.suffix();
    const FilePath newFilePath = externalToolsDir / proposal.baseName();

    int count = 0;
    FilePath tryPath = newFilePath.stringAppended(suffix);
    while (tryPath.exists()) {
        if (++count > 15)
            return {};
        // add random number
        const int number = QRandomGenerator::global()->generate() % 1000;
        tryPath = newFilePath.stringAppended(QString::number(number) + suffix);
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
            const QList<ExternalTool *> tools = it.value();
            for (const ExternalTool *tool : tools) {
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
    QModelIndex index = m_toolTree->selectionModel()->currentIndex();
    updateItem(index);
    updateButtons(index);

    QMap<QString, ExternalTool *> originalTools = ExternalToolManager::toolsById();
    QMap<QString, QList<ExternalTool *> > newToolsMap = tools();
    QMap<QString, QList<ExternalTool *> > resultMap;
    for (auto it = newToolsMap.cbegin(), end = newToolsMap.cend(); it != end; ++it) {
        QList<ExternalTool *> items;
        const QList<ExternalTool *> tools = it.value();
        for (ExternalTool *tool : tools) {
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
                        if (tool->preset()->filePath() == tool->filePath()) {
                            const QString &fileName = tool->preset()->filePath().fileName();
                            const FilePath &newFilePath = getUserFilePath(fileName);
                            // TODO error handling if newFilePath.isEmpty() (i.e. failed to find a unused name)
                            tool->setFilePath(newFilePath);
                        }
                        // TODO error handling
                        tool->save();
                    // case 2: tool is previously changed preset but now same as preset
                    } else if (tool->preset() && (*tool) == (*(tool->preset()))) {
                        // check if we need to delete the changed description
                        if (originalTool->filePath() != tool->preset()->filePath()
                                && originalTool->filePath().exists()) {
                            // TODO error handling
                            originalTool->filePath().removeFile();
                        }
                        tool->setFilePath(tool->preset()->filePath());
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
                tool->setFilePath(getUserFilePath(id + QLatin1String(".xml")));
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
    for (const ExternalTool *tool : std::as_const(originalTools)) {
        QTC_ASSERT(!tool->preset(), continue);
        // TODO error handling
        tool->filePath().removeFile();
    }

    ExternalToolManager::setToolsByCategory(resultMap);
}

void ExternalToolConfig::revertCurrentItem()
{
    QModelIndex index = m_toolTree->selectionModel()->currentIndex();
    m_model.revertTool(index);
    showInfoForItem(index);
}

void ExternalToolConfig::addTool()
{
    QModelIndex currentIndex = m_toolTree->selectionModel()->currentIndex();
    if (!currentIndex.isValid()) // default to Uncategorized
        currentIndex = m_model.index(0, 0);
    QModelIndex index = m_model.addTool(currentIndex);
    m_toolTree->selectionModel()->setCurrentIndex(index, QItemSelectionModel::Clear);
    m_toolTree->selectionModel()->setCurrentIndex(index, QItemSelectionModel::SelectCurrent);
    m_toolTree->edit(index);
}

void ExternalToolConfig::removeTool()
{
    QModelIndex currentIndex = m_toolTree->selectionModel()->currentIndex();
    m_toolTree->selectionModel()->setCurrentIndex(QModelIndex(), QItemSelectionModel::Clear);
    m_model.removeTool(currentIndex);
}

void ExternalToolConfig::addCategory()
{
    QModelIndex index = m_model.addCategory();
    m_toolTree->selectionModel()->setCurrentIndex(index, QItemSelectionModel::Clear);
    m_toolTree->selectionModel()->setCurrentIndex(index, QItemSelectionModel::SelectCurrent);
    m_toolTree->edit(index);
}

void ExternalToolConfig::updateEffectiveArguments()
{
    m_arguments->setToolTip(Utils::globalMacroExpander()->expandProcessArgs(m_arguments->text()));
}

void ExternalToolConfig::editEnvironmentChanges()
{
    const QString placeholderText = HostOsInfo::isWindowsHost()
            ? Tr::tr("PATH=C:\\dev\\bin;${PATH}")
            : Tr::tr("PATH=/opt/bin:${PATH}");
    const auto newItems = EnvironmentDialog::getEnvironmentItems(m_environmentLabel,
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
    QFontMetrics fm(m_environmentLabel->font());
    shortSummary = fm.elidedText(shortSummary, Qt::ElideRight, m_environmentLabel->width());
    m_environmentLabel->setText(shortSummary.isEmpty() ? Tr::tr("No changes to apply.") : shortSummary);
}

// ToolSettingsPage

ToolSettings::ToolSettings()
{
    setId(Constants::SETTINGS_ID_TOOLS);
    setDisplayName(Tr::tr("External Tools"));
    setCategory(Constants::SETTINGS_CATEGORY_CORE);
    setWidgetCreator([] { return new ExternalToolConfig; });
}

} // Internal
} // Core
