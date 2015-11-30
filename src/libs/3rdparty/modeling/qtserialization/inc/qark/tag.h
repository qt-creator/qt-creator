/***************************************************************************
**
** Copyright (C) 2015 Jochen Becher
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QARK_TAG_H
#define QARK_TAG_H

#include "typeregistry.h"
#include "parameters.h"

#include <QString>

namespace qark {

class Tag
{
public:
    explicit Tag(const QString &qualifiedName)
        : m_qualifiedName(qualifiedName)
    {
    }

    Tag(const QString &qualifiedName, const Parameters &parameters)
        : m_qualifiedName(qualifiedName),
          m_parameters(parameters)
    {
    }

    const QString &qualifiedName() const { return m_qualifiedName; }
    Parameters parameters() const { return m_parameters; }

private:
    QString m_qualifiedName;
    Parameters m_parameters;
};

template<class T>
class Object :  public Tag
{
public:
    Object(const QString &qualifiedName, T *object)
        : Tag(qualifiedName),
          m_object(object)
    {
    }

    Object(const QString &qualifiedName, T *object, const Parameters &parameters)
        : Tag(qualifiedName, parameters),
          m_object(object)
    {
    }

    T *object() const { return m_object; }

private:
    T *m_object = 0;
};

inline Tag tag(const QString &qualifiedName)
{
    return Tag(qualifiedName);
}

inline Tag tag(const QString &qualifiedName, const Parameters &parameters)
{
    return Tag(qualifiedName, parameters);
}

inline Tag tag(const char *qualifiedName)
{
    return Tag(QLatin1String(qualifiedName));
}

inline Tag tag(const char *qualifiedName, const Parameters &parameters)
{
    return Tag(QLatin1String(qualifiedName), parameters);
}

template<class T>
inline Object<T> tag(T &object)
{
    return Object<T>(typeUid<T>(), &object);
}

template<class T>
inline Object<T> tag(T &object, const Parameters &parameters)
{
    return Object<T>(typeUid<T>(), &object, parameters);
}

template<class T>
inline Object<T> tag(const QString &qualifiedName, T &object)
{
    return Object<T>(qualifiedName, &object);
}

template<class T>
inline Object<T> tag(const QString &qualifiedName, T &object, const Parameters &parameters)
{
    return Object<T>(qualifiedName, &object, parameters);
}

class End
{
public:
    explicit End()
    {
    }

    explicit End(const Parameters &parameters)
        : m_parameters(parameters)
    {
    }

    Parameters parameters() const { return m_parameters; }

private:
    Parameters m_parameters;
};

inline End end()
{
    return End();
}

inline End end(const Parameters &parameters)
{
    return End(parameters);
}

} // namespace qark

#endif // QARK_TAG_H
