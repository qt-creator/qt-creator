/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "cppcodestylesettings.h"

#include "cppcodestylepreferences.h"
#include "cpptoolsconstants.h"
#include "cpptoolssettings.h"

#include <projectexplorer/editorconfiguration.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projecttree.h>

#include <cplusplus/Overview.h>

#include <utils/qtcassert.h>
#include <utils/settingsutils.h>

static const char groupPostfix[] = "IndentSettings";
static const char indentBlockBracesKey[] = "IndentBlockBraces";
static const char indentBlockBodyKey[] = "IndentBlockBody";
static const char indentClassBracesKey[] = "IndentClassBraces";
static const char indentEnumBracesKey[] = "IndentEnumBraces";
static const char indentNamespaceBracesKey[] = "IndentNamespaceBraces";
static const char indentNamespaceBodyKey[] = "IndentNamespaceBody";
static const char indentAccessSpecifiersKey[] = "IndentAccessSpecifiers";
static const char indentDeclarationsRelativeToAccessSpecifiersKey[] = "IndentDeclarationsRelativeToAccessSpecifiers";
static const char indentFunctionBodyKey[] = "IndentFunctionBody";
static const char indentFunctionBracesKey[] = "IndentFunctionBraces";
static const char indentSwitchLabelsKey[] = "IndentSwitchLabels";
static const char indentStatementsRelativeToSwitchLabelsKey[] = "IndentStatementsRelativeToSwitchLabels";
static const char indentBlocksRelativeToSwitchLabelsKey[] = "IndentBlocksRelativeToSwitchLabels";
static const char indentControlFlowRelativeToSwitchLabelsKey[] = "IndentControlFlowRelativeToSwitchLabels";
static const char bindStarToIdentifierKey[] = "BindStarToIdentifier";
static const char bindStarToTypeNameKey[] = "BindStarToTypeName";
static const char bindStarToLeftSpecifierKey[] = "BindStarToLeftSpecifier";
static const char bindStarToRightSpecifierKey[] = "BindStarToRightSpecifier";
static const char extraPaddingForConditionsIfConfusingAlignKey[] = "ExtraPaddingForConditionsIfConfusingAlign";
static const char alignAssignmentsKey[] = "AlignAssignments";
static const char shortGetterNameKey[] = "ShortGetterName";

using namespace CppTools;

// ------------------ CppCodeStyleSettingsWidget

CppCodeStyleSettings::CppCodeStyleSettings() :
    indentBlockBraces(false)
  , indentBlockBody(true)
  , indentClassBraces(false)
  , indentEnumBraces(false)
  , indentNamespaceBraces(false)
  , indentNamespaceBody(false)
  , indentAccessSpecifiers(false)
  , indentDeclarationsRelativeToAccessSpecifiers(true)
  , indentFunctionBody(true)
  , indentFunctionBraces(false)
  , indentSwitchLabels(false)
  , indentStatementsRelativeToSwitchLabels(true)
  , indentBlocksRelativeToSwitchLabels(false)
  , indentControlFlowRelativeToSwitchLabels(true)
  , bindStarToIdentifier(true)
  , bindStarToTypeName(false)
  , bindStarToLeftSpecifier(false)
  , bindStarToRightSpecifier(false)
  , extraPaddingForConditionsIfConfusingAlign(true)
  , alignAssignments(false)
  , preferGetterNameWithoutGetPrefix(true)
{
}

void CppCodeStyleSettings::toSettings(const QString &category, QSettings *s) const
{
    Utils::toSettings(QLatin1String(groupPostfix), category, s, this);
}

void CppCodeStyleSettings::fromSettings(const QString &category, const QSettings *s)
{
    *this = CppCodeStyleSettings(); // Assign defaults
    Utils::fromSettings(QLatin1String(groupPostfix), category, s, this);
}

void CppCodeStyleSettings::toMap(const QString &prefix, QVariantMap *map) const
{
    map->insert(prefix + QLatin1String(indentBlockBracesKey), indentBlockBraces);
    map->insert(prefix + QLatin1String(indentBlockBodyKey), indentBlockBody);
    map->insert(prefix + QLatin1String(indentClassBracesKey), indentClassBraces);
    map->insert(prefix + QLatin1String(indentEnumBracesKey), indentEnumBraces);
    map->insert(prefix + QLatin1String(indentNamespaceBracesKey), indentNamespaceBraces);
    map->insert(prefix + QLatin1String(indentNamespaceBodyKey), indentNamespaceBody);
    map->insert(prefix + QLatin1String(indentAccessSpecifiersKey), indentAccessSpecifiers);
    map->insert(prefix + QLatin1String(indentDeclarationsRelativeToAccessSpecifiersKey), indentDeclarationsRelativeToAccessSpecifiers);
    map->insert(prefix + QLatin1String(indentFunctionBodyKey), indentFunctionBody);
    map->insert(prefix + QLatin1String(indentFunctionBracesKey), indentFunctionBraces);
    map->insert(prefix + QLatin1String(indentSwitchLabelsKey), indentSwitchLabels);
    map->insert(prefix + QLatin1String(indentStatementsRelativeToSwitchLabelsKey), indentStatementsRelativeToSwitchLabels);
    map->insert(prefix + QLatin1String(indentBlocksRelativeToSwitchLabelsKey), indentBlocksRelativeToSwitchLabels);
    map->insert(prefix + QLatin1String(indentControlFlowRelativeToSwitchLabelsKey), indentControlFlowRelativeToSwitchLabels);
    map->insert(prefix + QLatin1String(bindStarToIdentifierKey), bindStarToIdentifier);
    map->insert(prefix + QLatin1String(bindStarToTypeNameKey), bindStarToTypeName);
    map->insert(prefix + QLatin1String(bindStarToLeftSpecifierKey), bindStarToLeftSpecifier);
    map->insert(prefix + QLatin1String(bindStarToRightSpecifierKey), bindStarToRightSpecifier);
    map->insert(prefix + QLatin1String(extraPaddingForConditionsIfConfusingAlignKey), extraPaddingForConditionsIfConfusingAlign);
    map->insert(prefix + QLatin1String(alignAssignmentsKey), alignAssignments);
    map->insert(prefix + QLatin1String(shortGetterNameKey), preferGetterNameWithoutGetPrefix);
}

void CppCodeStyleSettings::fromMap(const QString &prefix, const QVariantMap &map)
{
    indentBlockBraces = map.value(prefix + QLatin1String(indentBlockBracesKey),
                                indentBlockBraces).toBool();
    indentBlockBody = map.value(prefix + QLatin1String(indentBlockBodyKey),
                                indentBlockBody).toBool();
    indentClassBraces = map.value(prefix + QLatin1String(indentClassBracesKey),
                                indentClassBraces).toBool();
    indentEnumBraces = map.value(prefix + QLatin1String(indentEnumBracesKey),
                                indentEnumBraces).toBool();
    indentNamespaceBraces = map.value(prefix + QLatin1String(indentNamespaceBracesKey),
                                indentNamespaceBraces).toBool();
    indentNamespaceBody = map.value(prefix + QLatin1String(indentNamespaceBodyKey),
                                indentNamespaceBody).toBool();
    indentAccessSpecifiers = map.value(prefix + QLatin1String(indentAccessSpecifiersKey),
                                indentAccessSpecifiers).toBool();
    indentDeclarationsRelativeToAccessSpecifiers = map.value(prefix + QLatin1String(indentDeclarationsRelativeToAccessSpecifiersKey),
                                indentDeclarationsRelativeToAccessSpecifiers).toBool();
    indentFunctionBody = map.value(prefix + QLatin1String(indentFunctionBodyKey),
                                indentFunctionBody).toBool();
    indentFunctionBraces = map.value(prefix + QLatin1String(indentFunctionBracesKey),
                                indentFunctionBraces).toBool();
    indentSwitchLabels = map.value(prefix + QLatin1String(indentSwitchLabelsKey),
                                indentSwitchLabels).toBool();
    indentStatementsRelativeToSwitchLabels = map.value(prefix + QLatin1String(indentStatementsRelativeToSwitchLabelsKey),
                                indentStatementsRelativeToSwitchLabels).toBool();
    indentBlocksRelativeToSwitchLabels = map.value(prefix + QLatin1String(indentBlocksRelativeToSwitchLabelsKey),
                                indentBlocksRelativeToSwitchLabels).toBool();
    indentControlFlowRelativeToSwitchLabels = map.value(prefix + QLatin1String(indentControlFlowRelativeToSwitchLabelsKey),
                                indentControlFlowRelativeToSwitchLabels).toBool();
    bindStarToIdentifier = map.value(prefix + QLatin1String(bindStarToIdentifierKey),
                                bindStarToIdentifier).toBool();
    bindStarToTypeName = map.value(prefix + QLatin1String(bindStarToTypeNameKey),
                                bindStarToTypeName).toBool();
    bindStarToLeftSpecifier = map.value(prefix + QLatin1String(bindStarToLeftSpecifierKey),
                                bindStarToLeftSpecifier).toBool();
    bindStarToRightSpecifier = map.value(prefix + QLatin1String(bindStarToRightSpecifierKey),
                                bindStarToRightSpecifier).toBool();
    extraPaddingForConditionsIfConfusingAlign = map.value(prefix + QLatin1String(extraPaddingForConditionsIfConfusingAlignKey),
                                extraPaddingForConditionsIfConfusingAlign).toBool();
    alignAssignments = map.value(prefix + QLatin1String(alignAssignmentsKey),
                                alignAssignments).toBool();
    preferGetterNameWithoutGetPrefix = map.value(prefix + QLatin1String(shortGetterNameKey),
                                preferGetterNameWithoutGetPrefix).toBool();
}

bool CppCodeStyleSettings::equals(const CppCodeStyleSettings &rhs) const
{
    return indentBlockBraces == rhs.indentBlockBraces
           && indentBlockBody == rhs.indentBlockBody
           && indentClassBraces == rhs.indentClassBraces
           && indentEnumBraces == rhs.indentEnumBraces
           && indentNamespaceBraces == rhs.indentNamespaceBraces
           && indentNamespaceBody == rhs.indentNamespaceBody
           && indentAccessSpecifiers == rhs.indentAccessSpecifiers
           && indentDeclarationsRelativeToAccessSpecifiers == rhs.indentDeclarationsRelativeToAccessSpecifiers
           && indentFunctionBody == rhs.indentFunctionBody
           && indentFunctionBraces == rhs.indentFunctionBraces
           && indentSwitchLabels == rhs.indentSwitchLabels
           && indentStatementsRelativeToSwitchLabels == rhs.indentStatementsRelativeToSwitchLabels
           && indentBlocksRelativeToSwitchLabels == rhs.indentBlocksRelativeToSwitchLabels
           && indentControlFlowRelativeToSwitchLabels == rhs.indentControlFlowRelativeToSwitchLabels
           && bindStarToIdentifier == rhs.bindStarToIdentifier
           && bindStarToTypeName == rhs.bindStarToTypeName
           && bindStarToLeftSpecifier == rhs.bindStarToLeftSpecifier
           && bindStarToRightSpecifier == rhs.bindStarToRightSpecifier
           && extraPaddingForConditionsIfConfusingAlign == rhs.extraPaddingForConditionsIfConfusingAlign
           && alignAssignments == rhs.alignAssignments
           && preferGetterNameWithoutGetPrefix == rhs.preferGetterNameWithoutGetPrefix
           ;
}

CppCodeStyleSettings CppCodeStyleSettings::currentProjectCodeStyle()
{
    ProjectExplorer::Project *project = ProjectExplorer::ProjectTree::currentProject();
    if (!project)
        return currentGlobalCodeStyle();

    ProjectExplorer::EditorConfiguration *editorConfiguration = project->editorConfiguration();
    QTC_ASSERT(editorConfiguration, return currentGlobalCodeStyle());

    TextEditor::ICodeStylePreferences *codeStylePreferences
        = editorConfiguration->codeStyle(Constants::CPP_SETTINGS_ID);
    QTC_ASSERT(codeStylePreferences, return currentGlobalCodeStyle());

    CppCodeStylePreferences *cppCodeStylePreferences
        = dynamic_cast<CppCodeStylePreferences *>(codeStylePreferences);
    QTC_ASSERT(cppCodeStylePreferences, return currentGlobalCodeStyle());

    return cppCodeStylePreferences->currentCodeStyleSettings();
}

CppCodeStyleSettings CppCodeStyleSettings::currentGlobalCodeStyle()
{
    CppCodeStylePreferences *cppCodeStylePreferences = CppToolsSettings::instance()->cppCodeStyle();
    QTC_ASSERT(cppCodeStylePreferences, return CppCodeStyleSettings());

    return cppCodeStylePreferences->currentCodeStyleSettings();
}

static void configureOverviewWithCodeStyleSettings(CPlusPlus::Overview &overview,
                                                   const CppCodeStyleSettings &settings)
{
    overview.starBindFlags = CPlusPlus::Overview::StarBindFlags(0);
    if (settings.bindStarToIdentifier)
        overview.starBindFlags |= CPlusPlus::Overview::BindToIdentifier;
    if (settings.bindStarToTypeName)
        overview.starBindFlags |= CPlusPlus::Overview::BindToTypeName;
    if (settings.bindStarToLeftSpecifier)
        overview.starBindFlags |= CPlusPlus::Overview::BindToLeftSpecifier;
    if (settings.bindStarToRightSpecifier)
        overview.starBindFlags |= CPlusPlus::Overview::BindToRightSpecifier;
}

CPlusPlus::Overview CppCodeStyleSettings::currentProjectCodeStyleOverview()
{
    CPlusPlus::Overview overview;
    configureOverviewWithCodeStyleSettings(overview, currentProjectCodeStyle());
    return overview;
}

CPlusPlus::Overview CppCodeStyleSettings::currentGlobalCodeStyleOverview()
{
    CPlusPlus::Overview overview;
    configureOverviewWithCodeStyleSettings(overview, currentGlobalCodeStyle());
    return overview;
}
