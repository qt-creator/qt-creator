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

#ifndef TESTTREEITEM_H
#define TESTTREEITEM_H

#include <QList>
#include <QString>

namespace Autotest {
namespace Internal {

class TestTreeItem
{

public:
    enum Type {
        ROOT,
        TEST_CLASS,
        TEST_FUNCTION,
        TEST_DATAFUNCTION,
        TEST_SPECIALFUNCTION
    };

    TestTreeItem(const QString &name, const QString &filePath, Type type, TestTreeItem *parent = 0);
    virtual ~TestTreeItem();
    TestTreeItem(const TestTreeItem& other);

    TestTreeItem *child(int row) const;
    TestTreeItem *parent() const;
    void appendChild(TestTreeItem *child);
    int row() const;
    int childCount() const;
    void removeChildren();
    bool removeChild(int row);
    bool modifyContent(const TestTreeItem *modified);

    const QString name() const { return m_name; }
    void setName(const QString &name) { m_name = name; }
    const QString filePath() const { return m_filePath; }
    void setFilePath(const QString &filePath) { m_filePath = filePath; }
    void setLine(unsigned line) { m_line = line;}
    unsigned line() const { return m_line; }
    void setColumn(unsigned column) { m_column = column; }
    unsigned column() const { return m_column; }
    QString mainFile() const { return m_mainFile; }
    void setMainFile(const QString &mainFile) { m_mainFile = mainFile; }
    void setChecked(const Qt::CheckState checked);
    Qt::CheckState checked() const;
    Type type() const { return m_type; }
    void setParent(TestTreeItem *parent) { m_parent = parent; }
    QList<QString> getChildNames() const;

private:
    void revalidateCheckState();

    QString m_name;
    QString m_filePath;
    Qt::CheckState m_checked;
    Type m_type;
    unsigned m_line;
    unsigned m_column;
    QString m_mainFile;
    TestTreeItem *m_parent;
    QList<TestTreeItem *> m_children;
};

struct TestCodeLocationAndType {
    QString m_fileName;
    unsigned m_line;
    unsigned m_column;
    TestTreeItem::Type m_type;
};

} // namespace Internal
} // namespace Autotest

#endif // TESTTREEITEM_H
