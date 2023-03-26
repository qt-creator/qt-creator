// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

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
    T *m_object = nullptr;
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
