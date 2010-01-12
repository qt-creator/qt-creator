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

#ifndef MODIFICATIONGROUPTOKEN_H
#define MODIFICATIONGROUPTOKEN_H

#include <corelib_global.h>

namespace QmlDesigner {

namespace Internal {
    class ModelRewriter;
    class ModelPrivate;
}

/*!
\class QmlDesigner::ModificationGroupToken
\ingroup CoreModel
\brief TBD
*/

class CORESHARED_EXPORT ModificationGroupToken
{
    friend class Internal::ModelRewriter;
    friend class Internal::ModelPrivate;

public:
    ModificationGroupToken(): m_depth(0), m_uniqueNumber(-1) {}

    unsigned depth() const { return m_depth; }
    bool isValid() const { return m_uniqueNumber != -1; }

    bool operator==(const ModificationGroupToken& token) const { return m_uniqueNumber == token.m_uniqueNumber; }
    bool operator!=(const ModificationGroupToken& token) const { return m_uniqueNumber != token.m_uniqueNumber; }

private:
    void makeInvalid() { m_uniqueNumber = -1; }
    ModificationGroupToken(unsigned depth);

private:
    static long uniqueNumberCounter;

    unsigned m_depth;
    long m_uniqueNumber;
};

}

#endif // MODIFICATIONGROUPTOKEN_H
