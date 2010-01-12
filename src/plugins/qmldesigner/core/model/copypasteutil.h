/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef COPYPASTEUTIL_H
#define COPYPASTEUTIL_H

#include <qmljsastfwd_p.h>

#include "import.h"

namespace QmlDesigner {
namespace Internal {

class CopyPasteUtil {
public:
    CopyPasteUtil(const QString &originalSource): m_originalSource(originalSource)
    {}

    Import createImport(QmlJS::AST::UiImport *ast);

protected:
    QString textAt(const QmlJS::AST::SourceLocation &loc) const
    { return m_originalSource.mid(loc.offset, loc.length); }

    QString textAt(const QmlJS::AST::SourceLocation &firstSourceLocation, const QmlJS::AST::SourceLocation &lastSourceLocation) const
    { return m_originalSource.mid(firstSourceLocation.offset, lastSourceLocation.end() - firstSourceLocation.offset); }

    QString originalSource() const { return m_originalSource; }

private:
    QString m_originalSource;
};

} // namespace Internal
} // namespace QmlDesigner

#endif // COPYPASTEUTIL_H
