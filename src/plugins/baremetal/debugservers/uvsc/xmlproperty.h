// Copyright (C) 2020 Denis Shienkov <denis.shienkov@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QVariant>

#include <memory>

namespace BareMetal::Gen::Xml {

class INodeVisitor;

class Property
{
    Q_DISABLE_COPY(Property)
public:
    Property() = default;
    explicit Property(QByteArray name, QVariant value);
    virtual ~Property() = default;

    QByteArray name() const { return m_name; }
    void setName(QByteArray name) { m_name = std::move(name); }

    QVariant value() const { return m_value; }
    void setValue(QVariant value) { m_value = std::move(value); }

    template<class T>
    T *appendChild(std::unique_ptr<T> child) {
        const auto p = child.get();
        m_children.push_back(std::move(child));
        return p;
    }

    template<class T, class... Args>
    T *appendChild(Args&&... args) {
        return appendChild(std::make_unique<T>(std::forward<Args>(args)...));
    }

    void appendProperty(QByteArray name, QVariant value = QVariant());
    void appendMultiLineProperty(QByteArray key, QStringList values,
                                 QChar sep = QLatin1Char(','));

    virtual void accept(INodeVisitor *visitor) const;

protected:
    const std::vector<std::unique_ptr<Property>> &children() const
    { return m_children; }

private:
    QByteArray m_name;
    QVariant m_value;
    std::vector<std::unique_ptr<Property>> m_children;
};

} // BareMetal::Gen::Xml
