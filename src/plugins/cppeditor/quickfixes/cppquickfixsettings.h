// Copyright (C) 2020 Leander Schulten <Leander.Schulten@rwth-aachen.de>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QString>
#include <QStringList>

#include <optional>
#include <vector>

namespace Utils { class QtcSettings; }

namespace CppEditor {

class CppQuickFixSettings
{
public:
    static CppQuickFixSettings *instance()
    {
        static CppQuickFixSettings settings(true);
        return &settings;
    }
    struct CustomTemplate
    {
        bool operator==(const CustomTemplate &b) const
        {
            return types == b.types
                && equalComparison == b.equalComparison
                && returnExpression == b.returnExpression
                && returnType == b.returnType
                && assignment == b.assignment;
        }
        QStringList types;
        QString equalComparison;
        QString returnExpression;
        QString returnType;
        QString assignment;
    };

    struct GetterSetterTemplate
    {
        QString equalComparison = "<cur> == <new>";
        QString returnExpression = "<cur>";
        QString assignment = "<cur> = <new>";
        const static inline QString TYPE_PATTERN = "<type>";
        const static inline QString TEMPLATE_PARAMETER_PATTERN = "<T>";
        std::optional<QString> returnTypeTemplate;
        void replacePlaceholders(QString currentValueVariableName, QString newValueVariableName);
    };

    enum class FunctionLocation {
        InsideClass,
        OutsideClass,
        CppFile,
    };
    enum class MissingNamespaceHandling {
        CreateMissing,
        AddUsingDirective,
        RewriteType, // e.g. change classname to namespacename::classname in cpp file
    };

    explicit CppQuickFixSettings(bool loadGlobalSettings = false);

    void loadGlobalSettings();
    void loadSettingsFrom(Utils::QtcSettings *);
    void saveSettingsTo(Utils::QtcSettings *);
    void saveAsGlobalSettings();
    void setDefaultSettings();

    static QString memberBaseName(
        const QString &name, const std::optional<QString> &baseNameTemplate = {});
    static QString replaceNamePlaceholders(const QString &nameTemplate, const QString &name,
                                           const std::optional<QString> &memberName);
    bool isValueType(QString type) const;
    GetterSetterTemplate findGetterSetterTemplate(QString fullyQualifiedType) const;

    QString getGetterName(const QString &propertyName, const QString &memberName) const
    {
        return replaceNamePlaceholders(getterNameTemplate, propertyName, memberName);
    }
    QString getSetterName(const QString &propertyName, const QString &memberName) const
    {
        return replaceNamePlaceholders(setterNameTemplate, propertyName, memberName);
    }
    QString getSignalName(const QString &propertyName, const QString &memberName) const
    {
        return replaceNamePlaceholders(signalNameTemplate, propertyName, memberName);
    }
    QString getResetName(const QString &propertyName, const QString &memberName) const
    {
        return replaceNamePlaceholders(resetNameTemplate, propertyName, memberName);
    }
    QString getSetterParameterName(const QString &propertyName, const QString &memberName) const
    {
        return replaceNamePlaceholders(setterParameterNameTemplate, propertyName, memberName);
    }
    QString getMemberVariableName(const QString &propertyName) const
    {
        return replaceNamePlaceholders(memberVariableNameTemplate, propertyName, {});
    }
    FunctionLocation determineGetterLocation(int lineCount) const;
    FunctionLocation determineSetterLocation(int lineCount) const;
    bool createMissingNamespacesinCppFile() const
    {
        return cppFileNamespaceHandling == MissingNamespaceHandling::CreateMissing;
    }
    bool addUsingNamespaceinCppFile() const
    {
        return cppFileNamespaceHandling == MissingNamespaceHandling::AddUsingDirective;
    }
    bool rewriteTypesinCppFile() const
    {
        return cppFileNamespaceHandling == MissingNamespaceHandling::RewriteType;
    }

public:
    int getterOutsideClassFrom = 0;
    int getterInCppFileFrom = 1;
    int setterOutsideClassFrom = 0;
    int setterInCppFileFrom = 1;
    QString getterAttributes;                 // e.g. [[nodiscard]]
    QString getterNameTemplate = R"js(memberName === name ? "get" + name[0].toUpperCase() + name.slice(1) : name)js";
    QString setterNameTemplate = R"js("set" + name[0].toUpperCase() + name.slice(1))js";
    QString setterParameterNameTemplate = R"js("new" + name[0].toUpperCase() + name.slice(1))js";
    QString signalNameTemplate = R"js(name + "Changed")js";
    QString resetNameTemplate = R"js("reset" + name[0].toUpperCase() + name.slice(1))js";
    bool signalWithNewValue = false;
    bool setterAsSlot = false;
    MissingNamespaceHandling cppFileNamespaceHandling = MissingNamespaceHandling::CreateMissing;
    QString memberVariableNameTemplate = R"js("m_" + name)js";
    QString nameFromMemberVariableTemplate;
    QStringList valueTypes; // if contains use value. Ignores namespaces and template parameters
    bool returnByConstRef = false;
    bool useAuto = true;
    std::vector<CustomTemplate> customTemplates;
};
} // namespace CppEditor
