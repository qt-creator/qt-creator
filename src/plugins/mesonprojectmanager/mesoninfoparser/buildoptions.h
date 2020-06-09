/****************************************************************************
**
** Copyright (C) 2020 Alexis Jeandet.
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
#include <utils/fileutils.h>
#include <utils/optional.h>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QString>
#include <QVariant>
#include <QWidget>
namespace MesonProjectManager {
namespace Internal {

struct ComboData
{
    QString value() const { return m_choices.at(m_currentIndex != -1 ? m_currentIndex : 0); }
    void setValue(const QString &value) { m_currentIndex = m_choices.indexOf(value); }
    ComboData(const QStringList &choices, const QString &value)
        : m_choices{choices}
    {
        setValue(value);
    }
    ComboData() = default;
    const QStringList &choices() const { return m_choices; }
    int currentIndex() const { return m_currentIndex; }

private:
    QStringList m_choices;
    int m_currentIndex = 0;
};

struct FeatureData : ComboData
{
    FeatureData()
        : ComboData({"enabled", "disabled", "auto"}, "disabled")
    {}
    FeatureData(const QString &value)
        : ComboData({"enabled", "disabled", "auto"}, value)
    {}
};

} // namespace Internal
} // namespace MesonProjectManager
Q_DECLARE_METATYPE(MesonProjectManager::Internal::FeatureData);
Q_DECLARE_METATYPE(MesonProjectManager::Internal::ComboData);

namespace MesonProjectManager {
namespace Internal {

struct BuildOption
{
    enum class Type { integer, string, feature, combo, array, boolean, unknown };
    const QString name;
    const QString section;
    const QString description;
    const Utils::optional<QString> subproject;
    virtual ~BuildOption() {}
    virtual QVariant value() const = 0;
    virtual QString valueStr() const = 0;
    virtual void setValue(const QVariant &) = 0;
    virtual Type type() const = 0;
    virtual BuildOption *copy() const = 0;
    inline QString fullName() const
    {
        if (subproject)
            return QString("%1:%2").arg(*subproject).arg(name);
        return name;
    }
    inline virtual QString mesonArg() const
    {
        return QString("-D%1=%2").arg(fullName()).arg(valueStr());
    }
    BuildOption(const QString &name, const QString &section, const QString &description)
        : name{name.contains(":") ? name.split(":").last() : name}
        , section{section}
        , description{description}
        , subproject{name.contains(":") ? Utils::optional<QString>(name.split(":").first())
                                        : Utils::nullopt}
    {}
}; // namespace Internal

struct IntegerBuildOption final : BuildOption
{
    QVariant value() const override { return m_currentValue; }
    QString valueStr() const override { return QString::number(m_currentValue); }
    void setValue(const QVariant &value) override { m_currentValue = value.toInt(); }
    Type type() const override { return Type::integer; }
    BuildOption *copy() const override { return new IntegerBuildOption{*this}; }
    IntegerBuildOption(const QString &name,
                       const QString &section,
                       const QString &description,
                       const QVariant &value)
        : BuildOption(name, section, description)
        , m_currentValue{value.toInt()}
    {}

protected:
    int m_currentValue;
};

struct StringBuildOption : BuildOption
{
    QVariant value() const override { return m_currentValue; }
    QString valueStr() const override { return m_currentValue; }
    void setValue(const QVariant &value) override { m_currentValue = value.toString(); }
    Type type() const override { return Type::string; }
    BuildOption *copy() const override { return new StringBuildOption{*this}; }

    StringBuildOption(const QString &name,
                      const QString &section,
                      const QString &description,
                      const QVariant &value)
        : BuildOption(name, section, description)
        , m_currentValue{value.toString()}
    {}

protected:
    QString m_currentValue;
};

struct FeatureBuildOption : BuildOption
{
    QVariant value() const override { return QVariant::fromValue(m_currentValue); }
    QString valueStr() const override { return m_currentValue.value(); }
    void setValue(const QVariant &value) override { m_currentValue.setValue(value.toString()); }
    Type type() const override { return Type::feature; }
    BuildOption *copy() const override { return new FeatureBuildOption{*this}; }
    FeatureBuildOption(const QString &name,
                       const QString &section,
                       const QString &description,
                       const QVariant &value)
        : BuildOption(name, section, description)
    {
        setValue(value);
    }

protected:
    FeatureData m_currentValue;
};

struct ComboBuildOption : BuildOption
{
    const QStringList &choices() const { return m_currentValue.choices(); }
    QVariant value() const override { return QVariant::fromValue(m_currentValue); }
    QString valueStr() const override { return m_currentValue.value(); }
    void setValue(const QVariant &value) override { m_currentValue.setValue(value.toString()); }
    Type type() const override { return Type::combo; }
    BuildOption *copy() const override { return new ComboBuildOption{*this}; }
    ComboBuildOption(const QString &name,
                     const QString &section,
                     const QString &description,
                     const QStringList &choices,
                     const QVariant &value)
        : BuildOption(name, section, description)
        , m_currentValue{choices, value.toString()}
    {}

protected:
    ComboData m_currentValue;
};

static inline QStringList quoteAll(const QStringList &values)
{
    QStringList quoted;
    std::transform(std::cbegin(values),
                   std::cend(values),
                   std::back_inserter(quoted),
                   [](const QString &v) {
                       if (v.front() == '\'' && v.back() == '\'')
                           return v;
                       return QString("\'%1\'").arg(v);
                   });
    return quoted;
}
struct ArrayBuildOption : BuildOption
{
    QVariant value() const override { return m_currentValue; }
    QString valueStr() const override { return m_currentValue.join(" "); }
    void setValue(const QVariant &value) override
    {
        m_currentValue = quoteAll(value.toStringList());
    }
    Type type() const override { return Type::array; }
    BuildOption *copy() const override { return new ArrayBuildOption{*this}; }
    inline virtual QString mesonArg() const override
    {
        return QString("-D%1=[%2]").arg(fullName()).arg(quoteAll(m_currentValue).join(','));
    }
    ArrayBuildOption(const QString &name,
                     const QString &section,
                     const QString &description,
                     const QVariant &value)
        : BuildOption(name, section, description)
        , m_currentValue{quoteAll(value.toStringList())}
    {}

protected:
    QStringList m_currentValue;
};

struct BooleanBuildOption : BuildOption
{
    QVariant value() const override { return m_currentValue; }
    QString valueStr() const override
    {
        return m_currentValue ? QString{"true"} : QString{"false"};
    }
    void setValue(const QVariant &value) override { m_currentValue = value.toBool(); }
    Type type() const override { return Type::boolean; }
    BuildOption *copy() const override { return new BooleanBuildOption{*this}; }

    BooleanBuildOption(const QString &name,
                       const QString &section,
                       const QString &description,
                       const QVariant &value)
        : BuildOption(name, section, description)
    {
        setValue(value);
    }

protected:
    bool m_currentValue;
};

struct UnknownBuildOption : BuildOption
{
    QVariant value() const override { return {"Unknown option"}; }
    QString valueStr() const override { return {"Unknown option"}; }
    void setValue(const QVariant &) override {}
    Type type() const override { return Type::unknown; }
    BuildOption *copy() const override { return new UnknownBuildOption{*this}; }

    UnknownBuildOption(const QString &name, const QString &section, const QString &description)
        : BuildOption(name, section, description)
    {}
};

using BuildOptionsList = std::vector<std::unique_ptr<BuildOption>>;

} // namespace Internal
} // namespace MesonProjectManager
