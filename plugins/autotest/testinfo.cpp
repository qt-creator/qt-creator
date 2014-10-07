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

} // namespace Internal
} // namespace Autotest
