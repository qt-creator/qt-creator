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

namespace Autotest::Internal {

static void linkAcceptor(const Utils::Link &link)
{
    if (link.hasValidTarget())
        Core::EditorManager::openEditorAt(link);
}

using LinkAcceptor = std::function<void(const Utils::Link &)>;

static Core::LocatorMatcherTasks dataTagMatchers(const LinkAcceptor &acceptor)
{
    using namespace Tasking;

    Storage<Core::LocatorStorage> storage;

    const auto onSetup = [storage, acceptor] {
        const QString input = storage->input();
        const TestTreeItem *qtTestRoot = theQtTestFramework().rootNode();
        if (!qtTestRoot)
            return;

        Core::LocatorFilterEntries entries;
        qtTestRoot->forAllChildItems([&entries, &input, acceptor = acceptor](TestTreeItem *it) {
            if (it->type() != TestTreeItem::TestDataTag)
                return;
            if (it->name().contains(input)) {
                Core::LocatorFilterEntry entry;
                entry.displayName = it->data(0, Qt::DisplayRole).toString();
                {
                    const TestTreeItem *parent = it->parentItem();
                    if (QTC_GUARD(parent)) {
                        const TestTreeItem *grandParent = parent->parentItem();
                        if (QTC_GUARD(grandParent))
                            entry.displayExtra = grandParent->name() + "::" + parent->name();
                    }
                }
                entry.linkForEditor = std::make_optional(it->data(0, LinkRole).value<Utils::Link>());
                entry.acceptor = [link = entry.linkForEditor, acceptor = acceptor] {
                    if (link)
                        acceptor(*link);
                    return Core::AcceptResult();
                };
                entries.append(entry);
            }
        });
        storage->reportOutput(entries);
    };
    return {{Sync(onSetup), storage}};
}

DataTagLocatorFilter::DataTagLocatorFilter()
{
    setId("Locate Qt Test data tags");
    setDisplayName(Tr::tr("Locate Qt Test data tags"));
    setDescription(Tr::tr("Locates a Qt Test data tag found inside the active project."));
    setDefaultShortcutString("qdt");
    setPriority(Medium);
    using namespace ProjectExplorer;
    QObject::connect(ProjectManager::instance(), &ProjectManager::startupProjectChanged,
                     this, [this] { setEnabled(ProjectManager::startupProject()); });
    setEnabled(ProjectManager::startupProject());
}

Core::LocatorMatcherTasks DataTagLocatorFilter::matchers()
{
    return dataTagMatchers(&linkAcceptor);
}

} // namespace Autotest::Internal
