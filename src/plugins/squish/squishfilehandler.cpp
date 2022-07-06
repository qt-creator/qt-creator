/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator Squish plugin.
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

#include "squishfilehandler.h"
#include "opensquishsuitesdialog.h"
#include "squishconstants.h"
#include "squishtesttreemodel.h"
#include "squishtools.h"
#include "squishutils.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <utils/qtcassert.h>

#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>

namespace Squish {
namespace Internal {

static SquishFileHandler *m_instance = nullptr;

SquishFileHandler::SquishFileHandler(QObject *parent)
    : QObject(parent)
{
    m_instance = this;
}

SquishFileHandler *SquishFileHandler::instance()
{
    if (!m_instance)
        m_instance = new SquishFileHandler;

    return m_instance;
}

SquishTestTreeItem *createTestTreeItem(const QString &name,
                                       const QString &filePath,
                                       const QStringList &cases)
{
    SquishTestTreeItem *item = new SquishTestTreeItem(name, SquishTestTreeItem::SquishSuite);
    item->setFilePath(filePath);
    for (const QString &testCase : cases) {
        SquishTestTreeItem *child = new SquishTestTreeItem(QFileInfo(testCase).dir().dirName(),
                                                           SquishTestTreeItem::SquishTestCase);
        child->setFilePath(testCase);
        item->appendChild(child);
    }
    return item;
}

void SquishFileHandler::modifySuiteItem(const QString &suiteName,
                                        const QString &filePath,
                                        const QStringList &cases)
{
    SquishTestTreeItem *item = createTestTreeItem(suiteName, filePath, cases);
    // TODO update file watcher
    m_suites.insert(suiteName, filePath);
    emit suiteTreeItemModified(item, suiteName);
}

void SquishFileHandler::openTestSuites()
{
    OpenSquishSuitesDialog dialog;
    dialog.exec();
    QMessageBox::StandardButton replaceSuite = QMessageBox::NoButton;
    const QStringList chosenSuites = dialog.chosenSuites();
    for (const QString &suite : chosenSuites) {
        const QDir suiteDir(suite);
        const QString suiteName = suiteDir.dirName();
        const QStringList cases = SquishUtils::validTestCases(suite);
        const QFileInfo suiteConf(suiteDir, "suite.conf");
        if (m_suites.contains(suiteName)) {
            if (replaceSuite == QMessageBox::YesToAll) {
                modifySuiteItem(suiteName, suiteConf.absoluteFilePath(), cases);
            } else if (replaceSuite != QMessageBox::NoToAll) {
                replaceSuite
                    = QMessageBox::question(Core::ICore::dialogParent(),
                                            tr("Suite Already Open"),
                                            tr("A test suite with the name \"%1\" is already open."
                                               "\nClose the opened test suite and replace it "
                                               "with the new one?")
                                                .arg(suiteName),
                                            QMessageBox::Yes | QMessageBox::YesToAll
                                                | QMessageBox::No | QMessageBox::NoToAll,
                                            QMessageBox::No);
                if (replaceSuite == QMessageBox::YesToAll || replaceSuite == QMessageBox::Yes)
                    modifySuiteItem(suiteName, suiteConf.absoluteFilePath(), cases);
            }
        } else {
            SquishTestTreeItem *item = createTestTreeItem(suiteName,
                                                          suiteConf.absoluteFilePath(),
                                                          cases);
            // TODO add file watcher
            m_suites.insert(suiteName, suiteConf.absoluteFilePath());
            emit testTreeItemCreated(item);
        }
    }
    emit suitesOpened();
}

void SquishFileHandler::closeTestSuite(const QString &suiteName)
{
    if (!m_suites.contains(suiteName))
        return;

    // TODO close respective editors if there are any
    // TODO remove file watcher
    m_suites.remove(suiteName);
    emit suiteTreeItemRemoved(suiteName);
}

void SquishFileHandler::closeAllTestSuites()
{
    // TODO close respective editors if there are any
    // TODO remove file watcher
    const QStringList &suiteNames = m_suites.keys();
    m_suites.clear();
    for (const QString &suiteName : suiteNames)
        emit suiteTreeItemRemoved(suiteName);
}

void SquishFileHandler::runTestCase(const QString &suiteName, const QString &testCaseName)
{
    QTC_ASSERT(!suiteName.isEmpty() && !testCaseName.isEmpty(), return );

    if (SquishTools::instance()->state() != SquishTools::Idle)
        return;

    const QDir suitePath = QFileInfo(m_suites.value(suiteName)).absoluteDir();
    if (!suitePath.exists() || !suitePath.isReadable()) {
        QMessageBox::critical(Core::ICore::dialogParent(),
                              tr("Test Suite Path Not Accessible"),
                              tr("The path \"%1\" does not exist or is not accessible.\n"
                                 "Refusing to run test case \"%2\".")
                                  .arg(QDir::toNativeSeparators(suitePath.absolutePath()))
                                  .arg(testCaseName));
        return;
    }

    SquishTools::instance()->runTestCases(suitePath.absolutePath(), QStringList(testCaseName));
}

void SquishFileHandler::runTestSuite(const QString &suiteName)
{
    QTC_ASSERT(!suiteName.isEmpty(), return );

    if (SquishTools::instance()->state() != SquishTools::Idle)
        return;

    const QString suiteConf = m_suites.value(suiteName);
    const QDir suitePath = QFileInfo(suiteConf).absoluteDir();
    if (!suitePath.exists() || !suitePath.isReadable()) {
        QMessageBox::critical(Core::ICore::dialogParent(),
                              tr("Test Suite Path Not Accessible"),
                              tr("The path \"%1\" does not exist or is not accessible.\n"
                                 "Refusing to run test cases.")
                                  .arg(QDir::toNativeSeparators(suitePath.absolutePath())));
        return;
    }

    QStringList testCases = SquishTestTreeModel::instance()->getSelectedSquishTestCases(suiteConf);
    SquishTools::instance()->runTestCases(suitePath.absolutePath(), testCases);
}

void addAllEntriesRecursively(SquishTestTreeItem *item)
{
    QDir folder(item->filePath());

    const QFileInfoList entries = folder.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot);
    for (const QFileInfo &info : entries) {
        const QString &path = info.absoluteFilePath();
        // TODO improve this later? Squish refuses directories containing Squish test suites
        const bool isDir = info.isDir();
        if (!info.isFile() && !isDir)
            continue;

        SquishTestTreeItem *child
            = new SquishTestTreeItem(info.fileName(),
                                     isDir ? SquishTestTreeItem::SquishSharedFolder
                                           : SquishTestTreeItem::SquishSharedFile);
        child->setFilePath(path);

        if (info.isDir())
            addAllEntriesRecursively(child);

        item->appendChild(child);
    }
}

void SquishFileHandler::addSharedFolder()
{
    const QString &chosen = QFileDialog::getExistingDirectory(Core::ICore::dialogParent(),
                                                              tr("Select Global Script Folder"));
    if (chosen.isEmpty())
        return;

    if (m_sharedFolders.contains(chosen))
        return;

    m_sharedFolders.append(chosen);
    SquishTestTreeItem *item = new SquishTestTreeItem(chosen,
                                                      SquishTestTreeItem::SquishSharedFolder);
    item->setFilePath(chosen);
    addAllEntriesRecursively(item);
    emit testTreeItemCreated(item);
}

bool SquishFileHandler::removeSharedFolder(const QString &folder)
{
    if (m_sharedFolders.contains(folder))
        return m_sharedFolders.removeOne(folder);

    return false;
}

void SquishFileHandler::removeAllSharedFolders()
{
    m_sharedFolders.clear();
}

void SquishFileHandler::openObjectsMap(const QString &suiteName)
{
    QTC_ASSERT(!suiteName.isEmpty(), return );

    const Utils::FilePath objectsMapPath = Utils::FilePath::fromString(
        SquishUtils::objectsMapPath(m_suites.value(suiteName)));
    if (!objectsMapPath.isEmpty() && objectsMapPath.exists()) {
        if (!Core::EditorManager::openEditor(objectsMapPath, Constants::OBJECTSMAP_EDITOR_ID)) {
            QMessageBox::critical(Core::ICore::dialogParent(),
                                  tr("Error"),
                                  tr("Failed to open objects.map file at \"%1\".")
                                      .arg(objectsMapPath.toUserOutput()));
        }
    }
}

} // namespace Internal
} // namespace Squish
