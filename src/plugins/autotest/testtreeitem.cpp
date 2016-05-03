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

#include "autotestconstants.h"
#include "autotest_utils.h"
#include "testcodeparser.h"
#include "testconfiguration.h"
#include "testtreeitem.h"
#include "testtreemodel.h"

#include <cplusplus/Icons.h>
#include <projectexplorer/session.h>
#include <texteditor/texteditor.h>
#include <utils/qtcassert.h>

#include <QIcon>

namespace Autotest {
namespace Internal {

TestTreeItem::TestTreeItem(const QString &name, const QString &filePath, Type type)
    : TreeItem( { name } ),
      m_name(name),
      m_filePath(filePath),
      m_type(type),
      m_line(0),
      m_status(NewlyAdded)
{
    m_checked = (m_type == TestCase || m_type == TestFunctionOrSet) ? Qt::Checked : Qt::Unchecked;
}

static QIcon testTreeIcon(TestTreeItem::Type type)
{
    static QIcon icons[] = {
        QIcon(),
        CPlusPlus::Icons::iconForType(CPlusPlus::Icons::ClassIconType),
        CPlusPlus::Icons::iconForType(CPlusPlus::Icons::SlotPrivateIconType),
        QIcon(QLatin1String(":/images/data.png"))
    };

    if (int(type) >= int(sizeof icons / sizeof *icons))
        return icons[2];
    return icons[type];
}

QVariant TestTreeItem::data(int /*column*/, int role) const
{
    switch (role) {
    case Qt::DisplayRole:
        if (m_type == Root && childCount() == 0)
            return QString(m_name + QCoreApplication::translate("TestTreeItem", " (none)"));
        else
            return m_name;
    case Qt::ToolTipRole:
        return m_filePath;
    case Qt::DecorationRole:
        return testTreeIcon(m_type);
    case Qt::CheckStateRole:
        return QVariant();
    case LinkRole: {
        QVariant itemLink;
        itemLink.setValue(TextEditor::TextEditorWidget::Link(m_filePath, m_line, m_column));
        return itemLink;
    }
    case ItalicRole:
        return false;
    case TypeRole:
        return m_type;
    case EnabledRole:
        return true;
    }
    return QVariant();
}

bool TestTreeItem::setData(int /*column*/, const QVariant &data, int role)
{
    if (role == Qt::CheckStateRole) {
        Qt::CheckState old = checked();
        setChecked((Qt::CheckState)data.toInt());
        return checked() != old;
    }
    return false;
}

Qt::ItemFlags TestTreeItem::flags(int /*column*/) const
{
    static const Qt::ItemFlags defaultFlags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    switch (m_type) {
    case Root:
        return Qt::ItemIsEnabled;
    case TestCase:
        return defaultFlags | Qt::ItemIsTristate | Qt::ItemIsUserCheckable;
    case TestFunctionOrSet:
        return defaultFlags | Qt::ItemIsUserCheckable;
    case TestDataFunction:
    case TestSpecialFunction:
    case TestDataTag:
    default:
        return defaultFlags;
    }
}

bool TestTreeItem::modifyTestCaseContent(const QString &name, unsigned line, unsigned column)
{
    bool hasBeenModified = modifyName(name);
    hasBeenModified |= modifyLineAndColumn(line, column);
    return hasBeenModified;
}

bool TestTreeItem::modifyTestFunctionContent(const TestParseResult *result)
{
    bool hasBeenModified = modifyFilePath(result->fileName);
    hasBeenModified |= modifyLineAndColumn(result->line, result->column);
    return hasBeenModified;
}

// TODO pass TestParseResult * to all modifyXYZ() OR remove completely if possible
bool TestTreeItem::modifyDataTagContent(const QString &name, const QString &fileName,
                                        unsigned line, unsigned column)
{
    bool hasBeenModified = modifyFilePath(fileName);
    hasBeenModified |= modifyName(name);
    hasBeenModified |= modifyLineAndColumn(line, column);
    return hasBeenModified;
}

bool TestTreeItem::modifyLineAndColumn(unsigned line, unsigned column)
{
    bool hasBeenModified = false;
    if (m_line != line) {
        m_line = line;
        hasBeenModified = true;
    }
    if (m_column != column) {
        m_column = column;
        hasBeenModified = true;
    }
    return hasBeenModified;
}

void TestTreeItem::setChecked(const Qt::CheckState checkState)
{
    switch (m_type) {
    case TestFunctionOrSet: {
        m_checked = (checkState == Qt::Unchecked ? Qt::Unchecked : Qt::Checked);
        parentItem()->revalidateCheckState();
        break;
    }
    case TestCase: {
        Qt::CheckState usedState = (checkState == Qt::Unchecked ? Qt::Unchecked : Qt::Checked);
        for (int row = 0, count = childCount(); row < count; ++row)
            childItem(row)->setChecked(usedState);
        m_checked = usedState;
    }
    default:
        return;
    }
}

Qt::CheckState TestTreeItem::checked() const
{
    switch (m_type) {
    case TestCase:
    case TestFunctionOrSet:
        return m_checked;
    default:
        return Qt::Unchecked;
    }
}

void TestTreeItem::markForRemoval(bool mark)
{
    m_status = mark ? MarkedForRemoval : Cleared;
}

void TestTreeItem::markForRemovalRecursively(bool mark)
{
    markForRemoval(mark);
    for (int row = 0, count = childCount(); row < count; ++row)
        childItem(row)->markForRemovalRecursively(mark);
}

void TestTreeItem::markForRemovalRecursively(const QString &filePath)
{
    if (m_filePath == filePath) {
        markForRemovalRecursively(true);
    } else {
        for (int row = 0, count = childCount(); row < count; ++row) {
            TestTreeItem *child = childItem(row);
            child->markForRemovalRecursively(filePath);
        }
    }
}

TestTreeItem *TestTreeItem::parentItem() const
{
    return static_cast<TestTreeItem *>(parent());
}

TestTreeItem *TestTreeItem::childItem(int row) const
{
    return static_cast<TestTreeItem *>(child(row));
}

TestTreeItem *TestTreeItem::findChildByName(const QString &name)
{
    return findChildBy([name](const TestTreeItem *other) -> bool {
        return other->name() == name;
    });
}

TestTreeItem *TestTreeItem::findChildByFile(const QString &filePath)
{
    return findChildBy([filePath](const TestTreeItem *other) -> bool {
        return other->filePath() == filePath;
    });
}

TestTreeItem *TestTreeItem::findChildByNameAndFile(const QString &name, const QString &filePath)
{
    return findChildBy([name, filePath](const TestTreeItem *other) -> bool {
        return other->filePath() == filePath && other->name() == name;
    });
}

QList<TestConfiguration *> TestTreeItem::getAllTestConfigurations() const
{
    return QList<TestConfiguration *>();
}

QList<TestConfiguration *> TestTreeItem::getSelectedTestConfigurations() const
{
    return QList<TestConfiguration *>();
}

bool TestTreeItem::lessThan(const TestTreeItem *other, SortMode mode) const
{
    const QString &lhs = data(0, Qt::DisplayRole).toString();
    const QString &rhs = other->data(0, Qt::DisplayRole).toString();

    switch (mode) {
    case Alphabetically:
        if (lhs == rhs)
            return index().row() > other->index().row();
        return lhs > rhs;
    case Naturally: {
        const TextEditor::TextEditorWidget::Link &leftLink =
                data(0, LinkRole).value<TextEditor::TextEditorWidget::Link>();
        const TextEditor::TextEditorWidget::Link &rightLink =
                other->data(0, LinkRole).value<TextEditor::TextEditorWidget::Link>();
        if (leftLink.targetFileName == rightLink.targetFileName) {
            return leftLink.targetLine == rightLink.targetLine
                    ? leftLink.targetColumn > rightLink.targetColumn
                    : leftLink.targetLine > rightLink.targetLine;
        }
        return leftLink.targetFileName > rightLink.targetFileName;
    }
    default:
        return true;
    }
}

void TestTreeItem::revalidateCheckState()
{
    if (childCount() == 0)
        return;
    bool foundChecked = false;
    bool foundUnchecked = false;
    for (int row = 0, count = childCount(); row < count; ++row) {
        TestTreeItem *child = childItem(row);
        switch (child->type()) {
        case TestDataFunction:
        case TestSpecialFunction:
            continue;
        default:
            break;
        }

        foundChecked |= (child->checked() != Qt::Unchecked);
        foundUnchecked |= (child->checked() == Qt::Unchecked);
        if (foundChecked && foundUnchecked) {
            m_checked = Qt::PartiallyChecked;
            return;
        }
    }
    m_checked = (foundUnchecked ? Qt::Unchecked : Qt::Checked);
}

inline bool TestTreeItem::modifyFilePath(const QString &filePath)
{
    if (m_filePath != filePath) {
        m_filePath = filePath;
        return true;
    }
    return false;
}

inline bool TestTreeItem::modifyName(const QString &name)
{
    if (m_name != name) {
        m_name = name;
        return true;
    }
    return false;
}

TestTreeItem *TestTreeItem::findChildBy(CompareFunction compare)
{
    for (int row = 0, count = childCount(); row < count; ++row) {
        TestTreeItem *child = childItem(row);
        if (compare(child))
            return child;
    }
    return 0;
}

AutoTestTreeItem *AutoTestTreeItem::createTestItem(const TestParseResult *result)
{
    AutoTestTreeItem *item = new AutoTestTreeItem(result->displayName, result->fileName,
                                                  result->itemType);
    item->setProFile(result->proFile);
    item->setLine(result->line);
    item->setColumn(result->column);

    foreach (const TestParseResult *funcParseResult, result->children)
        item->appendChild(createTestItem(funcParseResult));
    return item;
}

QVariant AutoTestTreeItem::data(int column, int role) const
{
    switch (role) {
    case Qt::CheckStateRole:
        switch (type()) {
        case Root:
        case TestDataFunction:
        case TestSpecialFunction:
        case TestDataTag:
            return QVariant();
        default:
            return checked();
        }
    case ItalicRole:
        switch (type()) {
        case TestDataFunction:
        case TestSpecialFunction:
            return true;
        default:
            return false;
        }
    }
    return TestTreeItem::data(column, role);
}

bool AutoTestTreeItem::canProvideTestConfiguration() const
{
    switch (type()) {
    case TestCase:
    case TestFunctionOrSet:
    case TestDataTag:
        return true;
    default:
        return false;
    }
}

TestConfiguration *AutoTestTreeItem::testConfiguration() const
{
    ProjectExplorer::Project *project = ProjectExplorer::SessionManager::startupProject();
    QTC_ASSERT(project, return 0);

    QtTestConfiguration *config = 0;
    switch (type()) {
    case TestCase:
        config = new QtTestConfiguration;
        config->setTestCaseCount(childCount());
        config->setProFile(proFile());
        config->setProject(project);
        config->setDisplayName(TestUtils::getCMakeDisplayNameIfNecessary(filePath(), proFile()));
        break;
    case TestFunctionOrSet: {
        TestTreeItem *parent = parentItem();
        config = new QtTestConfiguration();
        config->setTestCases(QStringList(name()));
        config->setProFile(parent->proFile());
        config->setProject(project);
        config->setDisplayName(
                TestUtils::getCMakeDisplayNameIfNecessary(filePath(), parent->proFile()));
        break;
    }
    case TestDataTag:{
        const TestTreeItem *function = parentItem();
        const TestTreeItem *parent = function ? function->parentItem() : 0;
        if (!parent)
            return 0;
        const QString functionWithTag = function->name() + QLatin1Char(':') + name();
        config = new QtTestConfiguration();
        config->setTestCases(QStringList(functionWithTag));
        config->setProFile(parent->proFile());
        config->setProject(project);
        config->setDisplayName(TestUtils::getCMakeDisplayNameIfNecessary(filePath(),
                                                                         parent->proFile()));
        break;
    }
    default:
        return 0;
    }
    return config;
}

QList<TestConfiguration *> AutoTestTreeItem::getAllTestConfigurations() const
{
    QList<TestConfiguration *> result;

    ProjectExplorer::Project *project = ProjectExplorer::SessionManager::startupProject();
    if (!project || type() != Root)
        return result;

    for (int row = 0, count = childCount(); row < count; ++row) {
        const TestTreeItem *child = childItem(row);

        TestConfiguration *tc = new QtTestConfiguration();
        tc->setTestCaseCount(child->childCount());
        tc->setProFile(child->proFile());
        tc->setProject(project);
        tc->setDisplayName(TestUtils::getCMakeDisplayNameIfNecessary(child->filePath(),
                                                                     child->proFile()));
        result << tc;
    }
    return result;
}

QList<TestConfiguration *> AutoTestTreeItem::getSelectedTestConfigurations() const
{
    QList<TestConfiguration *> result;
    ProjectExplorer::Project *project = ProjectExplorer::SessionManager::startupProject();
    if (!project || type() != Root)
        return result;

    QtTestConfiguration *testConfiguration = 0;

    for (int row = 0, count = childCount(); row < count; ++row) {
        const TestTreeItem *child = childItem(row);

        switch (child->checked()) {
        case Qt::Unchecked:
            continue;
        case Qt::Checked:
            testConfiguration = new QtTestConfiguration();
            testConfiguration->setTestCaseCount(child->childCount());
            testConfiguration->setProFile(child->proFile());
            testConfiguration->setProject(project);
            testConfiguration->setDisplayName(
                    TestUtils::getCMakeDisplayNameIfNecessary(child->filePath(), child->proFile()));
            result << testConfiguration;
            continue;
        case Qt::PartiallyChecked:
        default:
            int grandChildCount = child->childCount();
            QStringList testCases;
            for (int grandChildRow = 0; grandChildRow < grandChildCount; ++grandChildRow) {
                const TestTreeItem *grandChild = child->childItem(grandChildRow);
                if (grandChild->checked() == Qt::Checked)
                    testCases << grandChild->name();
            }

            testConfiguration = new QtTestConfiguration();
            testConfiguration->setTestCases(testCases);
            testConfiguration->setProFile(child->proFile());
            testConfiguration->setProject(project);
            testConfiguration->setDisplayName(
                    TestUtils::getCMakeDisplayNameIfNecessary(child->filePath(), child->proFile()));
            result << testConfiguration;
        }
    }

    return result;
}

QuickTestTreeItem *QuickTestTreeItem::createTestItem(const TestParseResult *result)
{
    QuickTestTreeItem *item = new QuickTestTreeItem(result->name, result->fileName,
                                                    result->itemType);
    item->setProFile(result->proFile);
    item->setLine(result->line);
    item->setColumn(result->column);
    foreach (const TestParseResult *funcResult, result->children)
        item->appendChild(createTestItem(funcResult));
    return item;
}

QVariant QuickTestTreeItem::data(int column, int role) const
{
    switch (role) {
    case Qt::DisplayRole:
        if (type() == TestCase && name().isEmpty())
            return QObject::tr(Constants::UNNAMED_QUICKTESTS);
        break;
    case Qt::ToolTipRole:
        if (type() == TestCase && name().isEmpty())
            return QObject::tr("<p>Give all test cases a name to ensure correct behavior "
                               "when running test cases and to be able to select them.</p>");
        break;
    case Qt::CheckStateRole:
        switch (type()) {
        case Root:
        case TestDataFunction:
        case TestSpecialFunction:
        case TestDataTag:
            return QVariant();
        case TestCase:
            return name().isEmpty() ? QVariant() : checked();
        case TestFunctionOrSet:
            return (parentItem() && !parentItem()->name().isEmpty()) ? checked() : QVariant();
        default:
            return checked();
        }

    case ItalicRole:
        switch (type()) {
        case TestDataFunction:
        case TestSpecialFunction:
            return true;
        case TestCase:
            return name().isEmpty();
        case TestFunctionOrSet:
            return parentItem() ? parentItem()->name().isEmpty() : false;
        default:
            return false;
        }
    default:
        break;
    }
    return TestTreeItem::data(column, role);
}

Qt::ItemFlags QuickTestTreeItem::flags(int column) const
{
    // handle unnamed quick tests and their functions
    switch (type()) {
    case TestCase:
        if (name().isEmpty())
            return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
        break;
    case TestFunctionOrSet:
        if (parentItem()->name().isEmpty())
            return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
        break;
    default: {} // avoid warning regarding unhandled enum values
    }
    return TestTreeItem::flags(column);
}

bool QuickTestTreeItem::canProvideTestConfiguration() const
{
    switch (type()) {
    case TestCase:
        return !name().isEmpty();
    case TestFunctionOrSet:
        return !parentItem()->name().isEmpty();
    default:
        return false;
    }
}

TestConfiguration *QuickTestTreeItem::testConfiguration() const
{
    ProjectExplorer::Project *project = ProjectExplorer::SessionManager::startupProject();
    QTC_ASSERT(project, return 0);

    QuickTestConfiguration *config = 0;
    switch (type()) {
    case TestCase: {
        QStringList testFunctions;
        for (int row = 0, count = childCount(); row < count; ++row)
            testFunctions << name() + QLatin1String("::") + childItem(row)->name();
        config = new QuickTestConfiguration;
        config->setTestCases(testFunctions);
        config->setProFile(proFile());
        config->setProject(project);
        break;
    }
    case TestFunctionOrSet: {
        TestTreeItem *parent = parentItem();
        QStringList testFunction(parent->name() + QLatin1String("::") + name());
        config = new QuickTestConfiguration;
        config->setTestCases(testFunction);
        config->setProFile(parent->proFile());
        config->setProject(project);
        break;
    }
    default:
        return 0;
    }
    return config;
}

QList<TestConfiguration *> QuickTestTreeItem::getAllTestConfigurations() const
{
    QList<TestConfiguration *> result;

    ProjectExplorer::Project *project = ProjectExplorer::SessionManager::startupProject();
    if (!project || type() != Root)
        return result;

    QHash<QString, int> foundProFiles;
    for (int row = 0, count = childCount(); row < count; ++row) {
        const TestTreeItem *child = childItem(row);
        // unnamed Quick Tests must be handled separately
        if (child->name().isEmpty()) {
            for (int childRow = 0, ccount = child->childCount(); childRow < ccount; ++ childRow) {
                const TestTreeItem *grandChild = child->childItem(childRow);
                const QString &proFile = grandChild->proFile();
                foundProFiles.insert(proFile, foundProFiles[proFile] + 1);
            }
            continue;
        }
        // named Quick Test
        const QString &proFile = child->proFile();
        foundProFiles.insert(proFile, foundProFiles[proFile] + child->childCount());
    }
    // create TestConfiguration for each project file
    QHash<QString, int>::ConstIterator it = foundProFiles.begin();
    QHash<QString, int>::ConstIterator end = foundProFiles.end();
    for ( ; it != end; ++it) {
        QuickTestConfiguration *tc = new QuickTestConfiguration;
        tc->setTestCaseCount(it.value());
        tc->setProFile(it.key());
        tc->setProject(project);
        result << tc;
    }
    return result;
}

QList<TestConfiguration *> QuickTestTreeItem::getSelectedTestConfigurations() const
{
    QList<TestConfiguration *> result;
    ProjectExplorer::Project *project = ProjectExplorer::SessionManager::startupProject();
    if (!project || type() != Root)
        return result;

    QuickTestConfiguration *tc = 0;
    QHash<QString, QuickTestConfiguration *> foundProFiles;
    // unnamed Quick Tests must be handled first
    if (TestTreeItem *unnamed = unnamedQuickTests()) {
        for (int childRow = 0, ccount = unnamed->childCount(); childRow < ccount; ++ childRow) {
            const TestTreeItem *grandChild = unnamed->childItem(childRow);
            const QString &proFile = grandChild->proFile();
            if (foundProFiles.contains(proFile)) {
                QTC_ASSERT(tc,
                           qWarning() << "Illegal state (unnamed Quick Test listed as named)";
                           return QList<TestConfiguration *>());
                foundProFiles[proFile]->setTestCaseCount(tc->testCaseCount() + 1);
            } else {
                tc = new QuickTestConfiguration;
                tc->setTestCaseCount(1);
                tc->setUnnamedOnly(true);
                tc->setProFile(proFile);
                tc->setProject(project);
                foundProFiles.insert(proFile, tc);
            }
        }
    }

    for (int row = 0, count = childCount(); row < count; ++row) {
        const TestTreeItem *child = childItem(row);
        // unnamed Quick Tests have been handled separately already
        if (child->name().isEmpty())
            continue;

        // named Quick Tests
        switch (child->checked()) {
        case Qt::Unchecked:
            continue;
        case Qt::Checked:
        case Qt::PartiallyChecked:
        default:
            QStringList testFunctions;
            int grandChildCount = child->childCount();
            for (int grandChildRow = 0; grandChildRow < grandChildCount; ++grandChildRow) {
                const TestTreeItem *grandChild = child->childItem(grandChildRow);
                if (grandChild->type() != TestFunctionOrSet)
                    continue;
                testFunctions << child->name() + QLatin1String("::") + grandChild->name();
            }
            if (foundProFiles.contains(child->proFile())) {
                tc = foundProFiles[child->proFile()];
                QStringList oldFunctions(tc->testCases());
                // if oldFunctions.size() is 0 this test configuration is used for at least one
                // unnamed test case
                if (oldFunctions.size() == 0) {
                    tc->setTestCaseCount(tc->testCaseCount() + testFunctions.size());
                    tc->setUnnamedOnly(false);
                } else {
                    oldFunctions << testFunctions;
                    tc->setTestCases(oldFunctions);
                }
            } else {
                tc = new QuickTestConfiguration;
                tc->setTestCases(testFunctions);
                tc->setProFile(child->proFile());
                tc->setProject(project);
                foundProFiles.insert(child->proFile(), tc);
            }
            break;
        }
    }
    QHash<QString, QuickTestConfiguration *>::ConstIterator it = foundProFiles.begin();
    QHash<QString, QuickTestConfiguration *>::ConstIterator end = foundProFiles.end();
    for ( ; it != end; ++it) {
        QuickTestConfiguration *config = it.value();
        if (!config->unnamedOnly())
            result << config;
        else
            delete config;
    }

    return result;
}

bool QuickTestTreeItem::lessThan(const TestTreeItem *other, TestTreeItem::SortMode mode) const
{
    // handle special item (<unnamed>)
    if (name().isEmpty())
        return false;
    if (other->name().isEmpty())
        return true;
    return TestTreeItem::lessThan(other, mode);
}

TestTreeItem *QuickTestTreeItem::unnamedQuickTests() const
{
    if (type() != Root)
        return 0;

    for (int row = 0, count = childCount(); row < count; ++row) {
        TestTreeItem *child = childItem(row);
        if (child->name().isEmpty())
            return child;
    }
    return 0;
}

static QString gtestFilter(GoogleTestTreeItem::TestStates states)
{
    if ((states & GoogleTestTreeItem::Parameterized) && (states & GoogleTestTreeItem::Typed))
        return QLatin1String("*/%1/*.%2");
    if (states & GoogleTestTreeItem::Parameterized)
        return QLatin1String("*/%1.%2/*");
    if (states & GoogleTestTreeItem::Typed)
        return QLatin1String("%1/*.%2");
    return QLatin1String("%1.%2");
}

GoogleTestTreeItem *GoogleTestTreeItem::createTestItem(const TestParseResult *result)
{
    const GoogleTestParseResult *parseResult = static_cast<const GoogleTestParseResult *>(result);
    GoogleTestTreeItem *item = new GoogleTestTreeItem(parseResult->name, parseResult->fileName,
                                                      parseResult->itemType);
    item->setProFile(parseResult->proFile);
    item->setLine(parseResult->line);
    item->setColumn(parseResult->column);

    if (parseResult->parameterized)
        item->setState(Parameterized);
    if (parseResult->typed)
        item->setState(Typed);
    if (parseResult->disabled)
        item->setState(Disabled);
    foreach (const TestParseResult *testSet, parseResult->children)
        item->appendChild(createTestItem(testSet));
    return item;
}

QVariant GoogleTestTreeItem::data(int column, int role) const
{
    switch (role) {
    case Qt::DisplayRole: {
        if (type() == TestTreeItem::Root)
            break;

        const QString &displayName = (m_state & Disabled) ? name().mid(9) : name();
        return QVariant(displayName + nameSuffix());
    }
    case Qt::CheckStateRole:
        switch (type()) {
        case TestCase:
        case TestFunctionOrSet:
            return checked();
        default:
            return QVariant();
        }
    case ItalicRole:
        return false;
    case EnabledRole:
        return !(m_state & Disabled);
    default:
        break;
    }
    return TestTreeItem::data(column, role);
}

TestConfiguration *GoogleTestTreeItem::testConfiguration() const
{
    ProjectExplorer::Project *project = ProjectExplorer::SessionManager::startupProject();
    QTC_ASSERT(project, return 0);

    GoogleTestConfiguration *config = 0;
    switch (type()) {
    case TestCase: {
        const QString &testSpecifier = gtestFilter(state()).arg(name()).arg(QLatin1Char('*'));
        if (int count = childCount()) {
            config = new GoogleTestConfiguration;
            config->setTestCases(QStringList(testSpecifier));
            config->setTestCaseCount(count);
            config->setProFile(proFile());
            config->setProject(project);
            // item has no filePath set - so take it of the first children
            config->setDisplayName(
                    TestUtils::getCMakeDisplayNameIfNecessary(childItem(0)->filePath(), proFile()));
        }
        break;
    }
    case TestFunctionOrSet: {
        GoogleTestTreeItem *parent = static_cast<GoogleTestTreeItem *>(parentItem());
        if (parent)
            return 0;
        const QString &testSpecifier = gtestFilter(parent->state()).arg(parent->name()).arg(name());
        config = new GoogleTestConfiguration;
        config->setTestCases(QStringList(testSpecifier));
        config->setProFile(proFile());
        config->setProject(project);
        config->setDisplayName(
                    TestUtils::getCMakeDisplayNameIfNecessary(filePath(), parent->proFile()));
        break;
    }
    default:
        return 0;
    }
    return config;
}

// used as key inside getAllTestCases()/getSelectedTestCases() for Google Tests
class ProFileWithDisplayName
{
public:
    ProFileWithDisplayName(const QString &file, const QString &name)
        : proFile(file), displayName(name) {}
    QString proFile;
    QString displayName;

    bool operator==(const ProFileWithDisplayName &rhs) const
    {
        return proFile == rhs.proFile && displayName == rhs.displayName;
    }
};

// needed as ProFileWithDisplayName is used as key inside a QHash
bool operator<(const ProFileWithDisplayName &lhs, const ProFileWithDisplayName &rhs)
{
    return lhs.proFile == rhs.proFile ? lhs.displayName < rhs.displayName
                                      : lhs.proFile < rhs.proFile;
}

// needed as ProFileWithDisplayName is used as a key inside a QHash
uint qHash(const ProFileWithDisplayName &lhs)
{
    return ::qHash(lhs.proFile) ^ ::qHash(lhs.displayName);
}

QList<TestConfiguration *> GoogleTestTreeItem::getAllTestConfigurations() const
{
    QList<TestConfiguration *> result;

    ProjectExplorer::Project *project = ProjectExplorer::SessionManager::startupProject();
    if (!project || type() != Root)
        return result;

    QHash<ProFileWithDisplayName, int> proFilesWithTestSets;
    for (int row = 0, count = childCount(); row < count; ++row) {
        const GoogleTestTreeItem *child = static_cast<const GoogleTestTreeItem *>(childItem(row));

        const int grandChildCount = child->childCount();
        for (int grandChildRow = 0; grandChildRow < grandChildCount; ++grandChildRow) {
            const TestTreeItem *grandChild = child->childItem(grandChildRow);
            if (grandChild->checked() == Qt::Checked) {
                ProFileWithDisplayName key(grandChild->proFile(),
                    TestUtils::getCMakeDisplayNameIfNecessary(grandChild->filePath(),
                                                              grandChild->proFile()));

                proFilesWithTestSets.insert(key, proFilesWithTestSets[key] + 1);
            }
        }
    }

    QHash<ProFileWithDisplayName, int>::ConstIterator it = proFilesWithTestSets.begin();
    QHash<ProFileWithDisplayName, int>::ConstIterator end = proFilesWithTestSets.end();
    for ( ; it != end; ++it) {
        const ProFileWithDisplayName &key = it.key();
        GoogleTestConfiguration *tc = new GoogleTestConfiguration;
        tc->setTestCaseCount(it.value());
        tc->setProFile(key.proFile);
        tc->setDisplayName(key.displayName);
        tc->setProject(project);
        result << tc;
    }

    return result;
}

QList<TestConfiguration *> GoogleTestTreeItem::getSelectedTestConfigurations() const
{
    QList<TestConfiguration *> result;
    ProjectExplorer::Project *project = ProjectExplorer::SessionManager::startupProject();
    if (!project || type() != Root)
        return result;

    QHash<ProFileWithDisplayName, QStringList> proFilesWithCheckedTestSets;
    for (int row = 0, count = childCount(); row < count; ++row) {
        const GoogleTestTreeItem *child = static_cast<const GoogleTestTreeItem *>(childItem(row));
        if (child->checked() == Qt::Unchecked)
            continue;

        int grandChildCount = child->childCount();
        for (int grandChildRow = 0; grandChildRow < grandChildCount; ++grandChildRow) {
            const TestTreeItem *grandChild = child->childItem(grandChildRow);
            if (grandChild->checked() == Qt::Checked) {
                ProFileWithDisplayName key(grandChild->proFile(),
                    TestUtils::getCMakeDisplayNameIfNecessary(grandChild->filePath(),
                                                              grandChild->proFile()));

                proFilesWithCheckedTestSets[key].append(
                            gtestFilter(child->state()).arg(child->name()).arg(grandChild->name()));
            }
        }
    }

    QHash<ProFileWithDisplayName, QStringList>::ConstIterator it = proFilesWithCheckedTestSets.begin();
    QHash<ProFileWithDisplayName, QStringList>::ConstIterator end = proFilesWithCheckedTestSets.end();
    for ( ; it != end; ++it) {
        const ProFileWithDisplayName &key = it.key();
        GoogleTestConfiguration *tc = new GoogleTestConfiguration;
        tc->setTestCases(it.value());
        tc->setProFile(key.proFile);
        tc->setDisplayName(key.displayName);
        tc->setProject(project);
        result << tc;
    }

    return result;
}

bool GoogleTestTreeItem::modifyTestSetContent(const GoogleTestParseResult *result)
{
    bool hasBeenModified = modifyLineAndColumn(result->line, result->column);
    GoogleTestTreeItem::TestStates states = result->disabled ? GoogleTestTreeItem::Disabled
                                                             : GoogleTestTreeItem::Enabled;
    if (m_state != states) {
        m_state = states;
        hasBeenModified = true;
    }
    return hasBeenModified;
}

TestTreeItem *GoogleTestTreeItem::findChildByNameStateAndFile(const QString &name,
                                                              GoogleTestTreeItem::TestStates state,
                                                              const QString &proFile)
{
    return findChildBy([name, state, proFile](const TestTreeItem *other) -> bool {
        const GoogleTestTreeItem *gtestItem = static_cast<const GoogleTestTreeItem *>(other);
        return other->proFile() == proFile
                && other->name() == name
                && gtestItem->state() == state;
    });
}

QString GoogleTestTreeItem::nameSuffix() const
{
    static QString markups[] = { QCoreApplication::translate("GoogleTestTreeItem", "parameterized"),
                                 QCoreApplication::translate("GoogleTestTreeItem", "typed") };
    QString suffix;
    if (m_state & Parameterized)
        suffix =  QLatin1String(" [") + markups[0];
    if (m_state & Typed)
        suffix += (suffix.isEmpty() ? QLatin1String(" [") : QLatin1String(", ")) + markups[1];
    if (!suffix.isEmpty())
        suffix += QLatin1Char(']');
    return suffix;
}

} // namespace Internal
} // namespace Autotest
