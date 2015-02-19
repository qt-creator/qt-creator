/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd
** All rights reserved.
** For any questions to The Qt Company, please use contact form at
** http://www.qt.io/contact-us
**
** This file is part of the Qt Creator Enterprise Auto Test Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.
**
** If you have questions regarding the use of this file, please use
** contact form at http://www.qt.io/contact-us
**
****************************************************************************/

#include "testinfo.h"

namespace Autotest {
namespace Internal {

TestInfo::TestInfo(const QString &className, const QStringList &functions, unsigned revision,
                   unsigned editorRevision)
    : m_className(className),
      m_functions(functions),
      m_revision(revision),
      m_editorRevision(editorRevision)
{
}

TestInfo::~TestInfo()
{
    m_functions.clear();
}

UnnamedQuickTestInfo::UnnamedQuickTestInfo(const QString &function, const QString &fileName)
    : m_function(function),
      m_fileName(fileName)
{
}

} // namespace Internal
} // namespace Autotest
