/****************************************************************************
**
** Copyright (C) 2014 Digia Plc
** All rights reserved.
** For any questions to Digia, please use contact form at http://qt.digia.com
**
** This file is part of the Qt Creator Enterprise Auto Test Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.
**
** If you have questions regarding the use of this file, please use
** contact form at http://qt.digia.com
**
****************************************************************************/

#include "testcodeparser.h"
#include "testtreeitem.h"
#include "testtreemodel.h"

#include <cpptools/cppmodelmanager.h>

#include <projectexplorer/buildtargetinfo.h>
#include <projectexplorer/environmentaspect.h>
#include <projectexplorer/localapplicationrunconfiguration.h>
#include <projectexplorer/project.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>

#include <texteditor/texteditor.h>

#include <utils/fileutils.h>

#include <QIcon>

namespace Autotest {
namespace Internal {

TestTreeModel::TestTreeModel(QObject *parent) :
    QAbstractItemModel(parent),
    m_rootItem(new TestTreeItem(QString(), QString(), TestTreeItem::ROOT)),
    m_autoTestRootItem(new TestTreeItem(tr("Auto Tests"), QString(), TestTreeItem::ROOT, m_rootItem)),
//    m_quickTestRootItem(new TestTreeItem(tr("Qt Quick Tests"), QString(), TestTreeItem::ROOT, m_rootItem)),
    m_parser(new TestCodeParser(this))
{
    m_rootItem->appendChild(m_autoTestRootItem);
//    m_rootItem->appendChild(m_quickTestRootItem);
    m_parser->updateTestTree();

//    CppTools::CppModelManagerInterface *cppMM = CppTools::CppModelManagerInterface::instance();
//    if (cppMM) {
//        // replace later on by
//        // cppMM->registerAstProcessor([this](const CplusPlus::Document::Ptr &doc,
//        //                             const CPlusPlus::Snapshot &snapshot) {
//        //        checkForQtTestStuff(doc, snapshot);
//        //      });
//        connect(cppMM, SIGNAL(documentUpdated(CPlusPlus::Document::Ptr)),
//                this, SLOT(checkForQtTestStuff(CPlusPlus::Document::Ptr)),
//                Qt::DirectConnection);

//    }
}

static TestTreeModel *m_instance = 0;

TestTreeModel *TestTreeModel::instance()
{
    if (!m_instance)
        m_instance = new TestTreeModel;
    return m_instance;
}

TestTreeModel::~TestTreeModel()
{
    delete m_rootItem;
    m_instance = 0;
}

QModelIndex TestTreeModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    TestTreeItem *parentItem = parent.isValid()
            ? static_cast<TestTreeItem *>(parent.internalPointer())
            : m_rootItem;

    TestTreeItem *childItem = parentItem->child(row);
    return childItem ? createIndex(row, column, childItem) : QModelIndex();
}

QModelIndex TestTreeModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    TestTreeItem *childItem = static_cast<TestTreeItem *>(index.internalPointer());
    TestTreeItem *parentItem = childItem->parent();

    if (parentItem == m_rootItem)
        return QModelIndex();

    return createIndex(parentItem->row(), 0, parentItem);
}

bool TestTreeModel::hasChildren(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return true;

    TestTreeItem *item = static_cast<TestTreeItem *>(parent.internalPointer());
    return item->childCount() > 0;
}

int TestTreeModel::rowCount(const QModelIndex &parent) const
{
    TestTreeItem *parentItem = parent.isValid()
            ? static_cast<TestTreeItem *>(parent.internalPointer()) : m_rootItem;
    return parentItem->childCount();
}

int TestTreeModel::columnCount(const QModelIndex &) const
{
    return 1;
}

static QIcon testTreeIcon(TestTreeItem::Type type)
{
    static QIcon icons[3] = {
        QIcon(),
        QIcon(QLatin1String(":/images/class.png")),
        QIcon(QLatin1String(":/images/func.png"))
    };
    return icons[type];
}

QVariant TestTreeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    TestTreeItem *item = static_cast<TestTreeItem *>(index.internalPointer());
    if (!item)
        return QVariant();

    if (role == Qt::DisplayRole) {
        if ((item == m_autoTestRootItem && m_autoTestRootItem->childCount() == 0)
                /*|| (item == m_quickTestRootItem && m_quickTestRootItem->childCount() == 0)*/) {
            return QString(item->name() + tr(" (none)"));
        } else {
            return item->name();
        }

        return QVariant(); // TODO ?
    }
    switch(role) {
    case Qt::ToolTipRole:
        return item->filePath();
    case Qt::DecorationRole:
        return testTreeIcon(item->type());
    case Qt::CheckStateRole:
        if (item->type() == TestTreeItem::ROOT)
            return QVariant();
        return item->checked();
    case LinkRole:
        QVariant itemLink;
        TextEditor::TextEditorWidget::Link link(item->filePath(), item->line(), item->column());
        itemLink.setValue(link);
        return itemLink;
    }

    // TODO ?
    return QVariant();
}

bool TestTreeModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid())
        return false;

    if (role == Qt::CheckStateRole) {
        TestTreeItem *item = static_cast<TestTreeItem *>(index.internalPointer());
        Qt::CheckState old = item->checked();
        item->setChecked((Qt::CheckState)value.toInt());
        if (item->checked() != old) {
            switch(item->type()) {
            case TestTreeItem::TEST_CLASS:
                emit dataChanged(index, index);
                if (item->childCount() > 0) {
                    emit dataChanged(index.child(0, 0), index.child(item->childCount() - 1, 0));
                }
                break;
            case TestTreeItem::TEST_FUNCTION:
                emit dataChanged(index, index);
                emit dataChanged(index.parent(), index.parent());
                break;
            default: // avoid warning regarding unhandled enum member
                break;
            }
            return true;
        }
    }
    return false;
}

Qt::ItemFlags TestTreeModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable;

    TestTreeItem *item = static_cast<TestTreeItem *>(index.internalPointer());
    switch(item->type()) {
    case TestTreeItem::TEST_CLASS:
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsTristate | Qt::ItemIsUserCheckable;
    case TestTreeItem::TEST_FUNCTION:
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable;
    case TestTreeItem::ROOT:
    default:
        return Qt::ItemIsEnabled;
    }
}

bool TestTreeModel::removeRows(int row, int count, const QModelIndex &parent)
{
    if (!parent.isValid())
        return false;
    TestTreeItem *parentItem = static_cast<TestTreeItem *>(parent.internalPointer());
    if (!parentItem)
        return false;

    bool subItemsSuccess = true;
    bool itemSuccess = true;
    for (int i = row + count - 1;  i >= row; --i) {
        QModelIndex child = index(i, 0, parent);
        subItemsSuccess &= removeRows(0, rowCount(child), child);
        beginRemoveRows(parent, i, i);
        itemSuccess &= parentItem->removeChild(i);
        endRemoveRows();
    }
    return subItemsSuccess && itemSuccess;
}

bool TestTreeModel::hasTests() const
{
    return m_autoTestRootItem->childCount() > 0 /*|| m_quickTestRootItem->childCount() > 0*/;
}

static void addProjectInformation(TestConfiguration *config, const QString &filePath)
{
    QString targetFile;
    QString targetName;
    QString workDir;
    QString proFile;
    Utils::Environment env;
    ProjectExplorer::Project *project = 0;
    CppTools::CppModelManager *cppMM = CppTools::CppModelManager::instance();
    QList<CppTools::ProjectPart::Ptr> projParts = cppMM->projectPart(filePath);
    if (!projParts.empty()) {
        proFile = projParts.at(0)->projectFile;
        project = projParts.at(0)->project; // necessary to grab this here? or should this be the current active startup project anyway?
    }
    if (project) {
        if (auto target = project->activeTarget()) {
            ProjectExplorer::BuildTargetInfoList appTargets = target->applicationTargets();
            foreach (ProjectExplorer::BuildTargetInfo bti, appTargets.list) {
                if (bti.isValid() && bti.projectFilePath.toString() == proFile) {
                    targetFile = bti.targetFilePath.toString();
                    targetName = bti.targetName;
                    break;
                }
            }

            QList<ProjectExplorer::RunConfiguration *> rcs = target->runConfigurations();
            foreach (ProjectExplorer::RunConfiguration *rc, rcs) {
                if (ProjectExplorer::LocalApplicationRunConfiguration *localRunConfiguration
                        = qobject_cast<ProjectExplorer::LocalApplicationRunConfiguration *>(rc)) {
                    if (localRunConfiguration->executable() == targetFile) {
                        workDir = Utils::FileUtils::normalizePathName(
                                    localRunConfiguration->workingDirectory());
                        ProjectExplorer::EnvironmentAspect *envAsp
                                = localRunConfiguration->extraAspect<ProjectExplorer::EnvironmentAspect>();
                        env = envAsp->environment();
                        break;
                    }
                }
            }
        }
    }
    config->setTargetFile(targetFile);
    config->setTargetName(targetName);
    config->setWorkingDirectory(workDir);
    config->setProFile(proFile);
    config->setEnvironment(env);
    config->setProject(project);
}

QList<TestConfiguration *> TestTreeModel::getAllTestCases() const
{
    QList<TestConfiguration *> result;

    int count = m_autoTestRootItem->childCount();
    for (int row = 0; row < count; ++row) {
        TestTreeItem *child = m_autoTestRootItem->child(row);

        TestConfiguration *tc = new TestConfiguration(child->name(), QStringList(),
                                                      child->childCount());
        addProjectInformation(tc, child->filePath());
        result << tc;
    }
    return result;
}

QList<TestConfiguration *> TestTreeModel::getSelectedTests() const
{
    QList<TestConfiguration *> result;
    TestConfiguration *tc;

    int count = m_autoTestRootItem->childCount();
    for (int row = 0; row < count; ++row) {
        TestTreeItem *child = m_autoTestRootItem->child(row);

        switch (child->checked()) {
        case Qt::Unchecked:
            continue;
        case Qt::Checked:
            tc = new TestConfiguration(child->name(), QStringList(), child->childCount());
            addProjectInformation(tc, child->filePath());
            result << tc;
            continue;
        case Qt::PartiallyChecked:
        default:
            QString childName = child->name();
            int grandChildCount = child->childCount();
            QStringList testCases;
            for (int grandChildRow = 0; grandChildRow < grandChildCount; ++grandChildRow) {
                const TestTreeItem *grandChild = child->child(grandChildRow);
                if (grandChild->checked() == Qt::Checked)
                    testCases << grandChild->name();
            }

            tc = new TestConfiguration(childName, testCases);
            addProjectInformation(tc, child->filePath());
            result << tc;
        }
    }
    return result;
}

void TestTreeModel::modifyAutoTestSubtree(int row, TestTreeItem *newItem)
{
    static QVector<int> modificationRoles = QVector<int>() << Qt::DisplayRole
                                                           << Qt::ToolTipRole << LinkRole;
    QModelIndex toBeModifiedIndex = index(0, 0).child(row, 0);
    if (!toBeModifiedIndex.isValid())
        return;
    TestTreeItem *toBeModifiedItem = static_cast<TestTreeItem *>(toBeModifiedIndex.internalPointer());
    if (toBeModifiedItem->modifyContent(newItem))
        emit dataChanged(toBeModifiedIndex, toBeModifiedIndex, modificationRoles);

    // process sub-items as well...
    int childCount = toBeModifiedItem->childCount();
    int newChildCount = newItem->childCount();

    // for keeping the CheckState on modifications
    QHash<QString, Qt::CheckState> originalItems;
    for (int row = 0; row < childCount; ++row) {
        const TestTreeItem *child = toBeModifiedItem->child(row);
        originalItems.insert(child->name(), child->checked());
    }

    if (childCount <= newChildCount) {
        for (int row = 0; row < childCount; ++row) {
            QModelIndex child = toBeModifiedIndex.child(row, 0);
            TestTreeItem *toBeModifiedChild = toBeModifiedItem->child(row);
            TestTreeItem *modifiedChild = newItem->child(row);
            if (toBeModifiedChild->modifyContent(modifiedChild)) {
                emit dataChanged(child, child, modificationRoles);
            }
            if (originalItems.contains(toBeModifiedChild->name())) {
                    Qt::CheckState state = originalItems.value(toBeModifiedChild->name());
                    if (state != toBeModifiedChild->checked()) {
                        toBeModifiedChild->setChecked(state);
                        emit dataChanged(child, child, QVector<int>() << Qt::CheckStateRole);
                }
            } else { // newly added (BAD: happens for renaming as well)
                toBeModifiedChild->setChecked(Qt::Checked);
                emit dataChanged(child, child, QVector<int>() << Qt::CheckStateRole);
            }
        }
        if (childCount < newChildCount) { // add aditional items
            for (int row = childCount; row < newChildCount; ++row) {
                TestTreeItem *newChild = newItem->child(row);
                TestTreeItem *toBeAdded = new TestTreeItem(newChild->name(), newChild->filePath(),
                                                           newChild->type(), toBeModifiedItem);
                toBeAdded->setLine(newChild->line());
                beginInsertRows(toBeModifiedIndex, row, row);
                toBeModifiedItem->appendChild(toBeAdded);
                endInsertRows();
                if (originalItems.contains(toBeAdded->name())
                        && originalItems.value(toBeAdded->name()) != Qt::Checked)
                    toBeAdded->setChecked(originalItems.value(toBeAdded->name()));
            }
        }
    } else {
        for (int row = 0; row < newChildCount; ++row) {
            QModelIndex child = toBeModifiedIndex.child(row, 0);
            TestTreeItem *toBeModifiedChild = toBeModifiedItem->child(row);
            TestTreeItem *modifiedChild = newItem->child(row);
            if (toBeModifiedChild->modifyContent(modifiedChild))
                emit dataChanged(child, child, modificationRoles);
            if (originalItems.contains(toBeModifiedChild->name())) {
                    Qt::CheckState state = originalItems.value(toBeModifiedChild->name());
                    if (state != toBeModifiedChild->checked()) {
                        toBeModifiedChild->setChecked(state);
                        emit dataChanged(child, child, QVector<int>() << Qt::CheckStateRole);
                }
            } else { // newly added (BAD: happens for renaming as well)
                toBeModifiedChild->setChecked(Qt::Checked);
                emit dataChanged(child, child, QVector<int>() << Qt::CheckStateRole);
            }
        } // remove rest of the items
        removeRows(newChildCount, childCount - newChildCount, toBeModifiedIndex);
    }
    emit testTreeModelChanged();
}

void TestTreeModel::removeAutoTestSubtreeByFilePath(const QString &file)
{
    QModelIndex atRootIndex = index(0, 0);
    int count = rowCount(atRootIndex);
    for (int row = 0; row < count; ++row) {
        QModelIndex childIndex = atRootIndex.child(row, 0);
        TestTreeItem *childItem = static_cast<TestTreeItem *>(childIndex.internalPointer());
        if (file == childItem->filePath()) {
            removeRow(row, atRootIndex);
            break;
        }
    }
    emit testTreeModelChanged();
}

void TestTreeModel::addAutoTest(TestTreeItem *newItem)
{
    beginInsertRows(index(0, 0), m_autoTestRootItem->childCount(), m_autoTestRootItem->childCount());
    m_autoTestRootItem->appendChild(newItem);
    endInsertRows();
    emit testTreeModelChanged();
}

void TestTreeModel::removeAllAutoTests()
{
    beginResetModel();
    m_autoTestRootItem->removeChildren();
    endResetModel();
    emit testTreeModelChanged();
}

} // namespace Internal
} // namespace Autotest
