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

#ifndef TESTINFO_H
#define TESTINFO_H

#include <QStringList>

namespace Autotest {
namespace Internal {

class TestInfo
{
public:
    explicit TestInfo(const QString &className, const QStringList &functions = QStringList(),
                      unsigned revision = 0, unsigned editorRevision = 0);

    ~TestInfo();
    const QString testClass() const { return m_className; }
    void setTestClass(const QString &className) { m_className = className; }
    const QStringList testFunctions() const { return m_functions; }
    void setTestFunctions(const QStringList &functions) { m_functions = functions; }
    unsigned revision() const { return m_revision; }
    void setRevision(unsigned revision) { m_revision = revision; }
    unsigned editorRevision() const { return m_editorRevision; }
    void setEditorRevision(unsigned editorRevision) { m_editorRevision = editorRevision; }
    const QString referencingFile() const { return m_referencingFile; }
    void setReferencingFile(const QString &refFile) {m_referencingFile = refFile; }

private:
    QString m_className;
    QStringList m_functions;
    unsigned m_revision;
    unsigned m_editorRevision;
    QString m_referencingFile;
};

} // namespace Internal
} // namespace Autotest

#endif // TESTINFO_H
