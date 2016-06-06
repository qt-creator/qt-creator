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

#include "testtreeitem.h"
#include "testtreemodel.h"

#include <coreplugin/id.h>
#include <cplusplus/CppDocument.h>
#include <cpptools/cppmodelmanager.h>
#include <qmljs/qmljsdocument.h>

namespace Autotest {
namespace Internal {

class TestParseResult
{
public:
    explicit TestParseResult(const Core::Id &id) : frameworkId(id) {}
    virtual ~TestParseResult() { qDeleteAll(children); }

    virtual TestTreeItem *createTestTreeItem() const = 0;

    QVector<TestParseResult *> children;
    Core::Id frameworkId;
    TestTreeItem::Type itemType = TestTreeItem::Root;
    QString displayName;
    QString fileName;
    QString proFile;
    QString name;
    unsigned line = 0;
    unsigned column = 0;
};

using TestParseResultPtr = QSharedPointer<TestParseResult>;

class ITestParser
{
public:
    virtual ~ITestParser() { }
    virtual void init(const QStringList &filesToParse) = 0;
    virtual bool processDocument(QFutureInterface<TestParseResultPtr> futureInterface,
                                 const QString &fileName) = 0;
    void setId(const Core::Id &id) { m_id = id; }
    Core::Id id() const { return m_id; }

private:
    Core::Id m_id;
};

class CppParser : public ITestParser
{
public:
    void init(const QStringList &filesToParse) override
    {
        Q_UNUSED(filesToParse);
        m_cppSnapshot = CppTools::CppModelManager::instance()->snapshot();
    }

    static bool selectedForBuilding(const QString &fileName)
    {
        QList<CppTools::ProjectPart::Ptr> projParts =
                CppTools::CppModelManager::instance()->projectPart(fileName);

        return projParts.size() && projParts.at(0)->selectedForBuilding;
    }

protected:
    CPlusPlus::Snapshot m_cppSnapshot;
};

} // namespace Internal
} // namespace Autotest

Q_DECLARE_METATYPE(Autotest::Internal::TestParseResultPtr)
