/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

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
