/****************************************************************************
**
** Copyright (C) 2020 Leander Schulten <Leander.Schulten@rwth-aachen.de>
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

#include <utils/optional.h>

#include <QString>
#include <QStringList>

#include <vector>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

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
        Utils::optional<QString> returnTypeTemplate;
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
    void loadSettingsFrom(QSettings *);
    void saveSettingsTo(QSettings *);
    void saveAsGlobalSettings();
    void setDefaultSettings();

    static QString replaceNamePlaceholders(const QString &nameTemplate, const QString &name);
    bool isValueType(QString type) const;
    GetterSetterTemplate findGetterSetterTemplate(QString fullyQualifiedType) const;

    QString getGetterName(const QString &variableName) const
    {
        return replaceNamePlaceholders(getterNameTemplate, variableName);
    }
    QString getSetterName(const QString &variableName) const
    {
        return replaceNamePlaceholders(setterNameTemplate, variableName);
    }
    QString getSignalName(const QString &variableName) const
    {
        return replaceNamePlaceholders(signalNameTemplate, variableName);
    }
    QString getResetName(const QString &variableName) const
    {
        return replaceNamePlaceholders(resetNameTemplate, variableName);
    }
    QString getSetterParameterName(const QString &variableName) const
    {
        return replaceNamePlaceholders(setterParameterNameTemplate, variableName);
    }
    QString getMemberVariableName(const QString &variableName) const
    {
        return replaceNamePlaceholders(memberVariableNameTemplate, variableName);
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
    QString getterNameTemplate = "<name>";    // or get<Name>
    QString setterNameTemplate = "set<Name>"; // or set_<name> or Set<Name>
    QString setterParameterNameTemplate = "new<Name>";
    QString signalNameTemplate = "<name>Changed";
    QString resetNameTemplate = "reset<Name>";
    bool signalWithNewValue = false;
    bool setterAsSlot = false;
    MissingNamespaceHandling cppFileNamespaceHandling = MissingNamespaceHandling::CreateMissing;
    QString memberVariableNameTemplate = "m_<name>";
    QStringList valueTypes; // if contains use value. Ignores namespaces and template parameters
    std::vector<CustomTemplate> customTemplates;
};
} // namespace CppEditor
