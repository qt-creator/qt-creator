/****************************************************************************
**
** Copyright (C) 2020 Denis Shienkov <denis.shienkov@gmail.com>
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <QVariant>

#include <memory>

namespace BareMetal {
namespace Gen {
namespace Xml {

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

} // namespace Xml
} // namespace Gen
} // namespace BareMetal
