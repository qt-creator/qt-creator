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

#ifndef TESTTREEITEM_H
#define TESTTREEITEM_H

#include <utils/treemodel.h>

#include <QList>
#include <QString>
#include <QMetaType>

namespace {
    enum ItemRole {
        LinkRole = Qt::UserRole + 2, // can be removed if AnnotationRole comes back
        ItalicRole, // used only inside the delegate
        TypeRole,
        StateRole
    };
}

namespace Autotest {
namespace Internal {

class TestTreeItem : public Utils::TreeItem
{

public:
    enum Type
    {
        Root,
        TestClass,
        TestFunction,
        TestDataTag,
        TestDataFunction,
        TestSpecialFunction,
        GTestCase,
        GTestCaseParameterized,
        GTestName
    };

    enum TestState
    {
        Enabled         = 0x00,
        Disabled        = 0x01,
    };

    Q_FLAGS(TestState)
    Q_DECLARE_FLAGS(TestStates, TestState)

    TestTreeItem(const QString &name = QString(), const QString &filePath = QString(),
                 Type type = Root);
    virtual ~TestTreeItem();
    TestTreeItem(const TestTreeItem& other);

    virtual QVariant data(int column, int role) const override;
    virtual bool setData(int column, const QVariant &data, int role) override;
    bool modifyContent(const TestTreeItem *modified);

    const QString name() const { return m_name; }
    const QString filePath() const { return m_filePath; }
    void setLine(unsigned line) { m_line = line;}
    unsigned line() const { return m_line; }
    void setColumn(unsigned column) { m_column = column; }
    unsigned column() const { return m_column; }
    QString mainFile() const { return m_mainFile; }
    void setMainFile(const QString &mainFile) { m_mainFile = mainFile; }
    QString referencingFile() const { return m_referencingFile; }
    void setReferencingFile(const QString &referencingFile) { m_referencingFile = referencingFile; }
    void setChecked(const Qt::CheckState checked);
    Qt::CheckState checked() const;
    Type type() const { return m_type; }
    void setState(TestStates states) { m_state = states; }
    TestStates state() const { return m_state; }
    void markForRemoval(bool mark);
    void markForRemovalRecursively(bool mark);
    bool markedForRemoval() const { return m_markedForRemoval; }
    TestTreeItem *parentItem() const;
    TestTreeItem *childItem(int row) const;

private:
    void revalidateCheckState();

    QString m_name;
    QString m_filePath;
    Qt::CheckState m_checked;
    Type m_type;
    unsigned m_line;
    unsigned m_column;
    QString m_mainFile;  // main for Quick tests, project file for gtest
    QString m_referencingFile;
    TestStates m_state;
    bool m_markedForRemoval;
};

struct TestCodeLocationAndType {
    QString m_name;     // tag name for m_type == TEST_DATATAG, file name for other values
    unsigned m_line;
    unsigned m_column;
    TestTreeItem::Type m_type;
    TestTreeItem::TestStates m_state;
};

struct GTestCaseSpec
{
    QString testCaseName;
    bool parameterized;
};

typedef QVector<TestCodeLocationAndType> TestCodeLocationList;

} // namespace Internal
} // namespace Autotest

Q_DECLARE_METATYPE(Autotest::Internal::TestTreeItem *)
Q_DECLARE_METATYPE(Autotest::Internal::TestCodeLocationAndType)

#endif // TESTTREEITEM_H
