// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "datataglocatorfilter.h"

#include "qttestframework.h"

#include "../autotesttr.h"
#include "../testtreeitem.h"

#include <coreplugin/editormanager/editormanager.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>

#include <solutions/tasking/tasktree.h>

using namespace Core;
using namespace Tasking;
using namespace Utils;

namespace Autotest::Internal {

static void linkAcceptor(const Link &link)
{
    if (link.hasValidTarget())
        EditorManager::openEditorAt(link);
}

using LinkAcceptor = std::function<void(const Link &)>;

static LocatorMatcherTasks dataTagMatchers(const LinkAcceptor &acceptor)
{
    const auto onSetup = [acceptor] {
        const LocatorStorage &storage = *LocatorStorage::storage();
        const QString input = storage.input();
        const TestTreeItem *qtTestRoot = theQtTestFramework().rootNode();
        if (!qtTestRoot)
            return;

        LocatorFilterEntries entries;
        qtTestRoot->forAllChildItems([&entries, &input, acceptor = acceptor](TestTreeItem *it) {
            if (it->type() != TestTreeItem::TestDataTag)
                return;
            if (it->name().contains(input)) {
                LocatorFilterEntry entry;
                entry.displayName = it->data(0, Qt::DisplayRole).toString();
                {
                    const TestTreeItem *parent = it->parentItem();
                    if (QTC_GUARD(parent)) {
                        const TestTreeItem *grandParent = parent->parentItem();
                        if (QTC_GUARD(grandParent))
                            entry.displayExtra = grandParent->name() + "::" + parent->name();
                    }
                }
                entry.linkForEditor = std::make_optional(it->data(0, LinkRole).value<Link>());
                entry.acceptor = [link = entry.linkForEditor, acceptor = acceptor] {
                    if (link)
                        acceptor(*link);
                    return AcceptResult();
                };
                entries.append(entry);
            }
        });
        storage.reportOutput(entries);
    };
    return {Sync(onSetup)};
}

DataTagLocatorFilter::DataTagLocatorFilter()
{
    setId("Locate Qt Test data tags");
    setDisplayName(Tr::tr("Locate Qt Test data tags"));
    setDescription(Tr::tr("Locates Qt Test data tags found inside the active project."));
    setDefaultShortcutString("qdt");
    setPriority(Medium);
    using namespace ProjectExplorer;
    QObject::connect(ProjectManager::instance(), &ProjectManager::startupProjectChanged,
                     this, [this] { setEnabled(ProjectManager::startupProject()); });
    setEnabled(ProjectManager::startupProject());
}

LocatorMatcherTasks DataTagLocatorFilter::matchers()
{
    return dataTagMatchers(&linkAcceptor);
}

} // namespace Autotest::Internal
