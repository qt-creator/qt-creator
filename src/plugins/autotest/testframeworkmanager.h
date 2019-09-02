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

#pragma once

#include "itestframework.h"

#include <QHash>
#include <QSharedPointer>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace Core { class Id; }

namespace Autotest {

class TestTreeItem;

namespace Internal {

class TestRunner;
struct TestSettings;

}

class IFrameworkSettings;
class ITestParser;
class ITestSettingsPage;
class TestTreeModel;

class TestFrameworkManager
{
public:
    static TestFrameworkManager *instance();
    virtual ~TestFrameworkManager();
    bool registerTestFramework(ITestFramework *framework);

    void activateFrameworksFromSettings(QSharedPointer<Internal::TestSettings> settings);
    QString frameworkNameForId(const Core::Id &id) const;
    QList<Core::Id> registeredFrameworkIds() const;
    QList<Core::Id> sortedRegisteredFrameworkIds() const;
    QList<Core::Id> sortedActiveFrameworkIds() const;

    TestTreeItem *rootNodeForTestFramework(const Core::Id &frameworkId) const;
    ITestParser *testParserForTestFramework(const Core::Id &frameworkId) const;
    QSharedPointer<IFrameworkSettings> settingsForTestFramework(const Core::Id &frameworkId) const;
    void synchronizeSettings(QSettings *s);
    bool isActive(const Core::Id &frameworkId) const;
    bool groupingEnabled(const Core::Id &frameworkId) const;
    void setGroupingEnabledFor(const Core::Id &frameworkId, bool enabled);
    QString groupingToolTip(const Core::Id &frameworkId) const;
    bool hasActiveFrameworks() const;
    unsigned priority(const Core::Id &frameworkId) const;
private:
    QList<Core::Id> activeFrameworkIds() const;
    explicit TestFrameworkManager();
    QHash<Core::Id, ITestFramework *> m_registeredFrameworks;
    QHash<Core::Id, QSharedPointer<IFrameworkSettings> > m_frameworkSettings;
    QVector<ITestSettingsPage *> m_frameworkSettingsPages;
    TestTreeModel *m_testTreeModel;
    Internal::TestRunner *m_testRunner;

    typedef QHash<Core::Id, ITestFramework *>::ConstIterator FrameworkIterator;
};

} // namespace Autotest
