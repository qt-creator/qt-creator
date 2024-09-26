// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

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
