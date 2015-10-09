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

class Tag {
public:
    explicit Tag(const QString &qualified_name)
        : _qualified_name(qualified_name)
    {
    }

    Tag(const QString &qualified_name, const Parameters &parameters)
        : _qualified_name(qualified_name),
          _parameters(parameters)
    {
    }

    const QString &getQualifiedName() const { return _qualified_name; }

    Parameters getParameters() const { return _parameters; }

private:
    QString _qualified_name;
    Parameters _parameters;
};


template<class T>
class Object :
        public Tag
{
public:
    Object(const QString &qualified_name, T *object)
        : Tag(qualified_name),
          _object(object)
    {
    }

    Object(const QString &qualified_name, T *object, const Parameters &parameters)
        : Tag(qualified_name, parameters),
          _object(object)
    {
    }

    T *getObject() const { return _object; }

private:
    T *_object;
};


inline Tag tag(const QString &qualified_name)
{
    return Tag(qualified_name);
}

inline Tag tag(const QString &qualified_name, const Parameters &parameters)
{
    return Tag(qualified_name, parameters);
}

inline Tag tag(const char *qualified_name)
{
    return Tag(QLatin1String(qualified_name));
}

inline Tag tag(const char *qualified_name, const Parameters &parameters)
{
    return Tag(QLatin1String(qualified_name), parameters);
}

template<class T>
inline Object<T> tag(T &object)
{
    return Object<T>(get_type_uid<T>(), &object);
}

template<class T>
inline Object<T> tag(T &object, const Parameters &parameters)
{
    return Object<T>(get_type_uid<T>(), &object, parameters);
}

template<class T>
inline Object<T> tag(const QString &qualified_name, T &object)
{
    return Object<T>(qualified_name, &object);
}

template<class T>
inline Object<T> tag(const QString &qualified_name, T &object, const Parameters &parameters)
{
    return Object<T>(qualified_name, &object, parameters);
}


class End {
public:
    explicit End()
    {
    }

    explicit End(const Parameters &parameters)
        : _parameters(parameters)
    {
    }

    Parameters getParameters() const { return _parameters; }

private:
    Parameters _parameters;
};

inline End end()
{
    return End();
}

inline End end(const Parameters &parameters)
{
    return End(parameters);
}

}

#endif // QARK_TAG_H
