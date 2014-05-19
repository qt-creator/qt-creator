/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef CPPTOOLS_H
#define CPPTOOLS_H

#include "cpptools_global.h"
#include "stringtable.h"

#include <projectexplorer/projectexplorer.h>

#include <QSharedPointer>

QT_BEGIN_NAMESPACE
class QFileInfo;
class QDir;
QT_END_NAMESPACE

namespace CppTools {

class CppToolsSettings;

namespace Internal {

class CppModelManager;
struct CppFileSettings;
class CppCodeModelSettings;

class CppToolsPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "CppTools.json")

public:
    CppToolsPlugin();
    ~CppToolsPlugin();

    static CppToolsPlugin *instance();
    static const QStringList &headerSearchPaths();
    static const QStringList &sourceSearchPaths();
    static const QStringList &headerPrefixes();
    static const QStringList &sourcePrefixes();
    static void clearHeaderSourceCache();

    bool initialize(const QStringList &arguments, QString *errorMessage);
    void extensionsInitialized();
    ShutdownFlag aboutToShutdown();

    QSharedPointer<CppCodeModelSettings> codeModelSettings() const;

    static StringTable &stringTable();

public slots:
    void switchHeaderSource();
    void switchHeaderSourceInNextSplit();

#ifdef WITH_TESTS
private slots:
    // Init/Cleanup methods implemented in cppheadersource_test.cpp
    void initTestCase();
    void cleanupTestCase();

    void test_codegen_public_in_empty_class();
    void test_codegen_public_in_nonempty_class();
    void test_codegen_public_before_protected();
    void test_codegen_private_after_protected();
    void test_codegen_protected_in_nonempty_class();
    void test_codegen_protected_between_public_and_private();
    void test_codegen_qtdesigner_integration();
    void test_codegen_definition_empty_class();
    void test_codegen_definition_first_member();
    void test_codegen_definition_last_member();
    void test_codegen_definition_middle_member();
    void test_codegen_definition_middle_member_surrounded_by_undefined();
    void test_codegen_definition_member_specific_file();

    void test_completion_basic_1();

    void test_completion_template_function_data();
    void test_completion_template_function();

    void test_completion_data();
    void test_completion();

    void test_completion_member_access_operator_data();
    void test_completion_member_access_operator();

    void test_completion_prefix_first_QTCREATORBUG_8737();
    void test_completion_prefix_first_QTCREATORBUG_9236();

    void test_format_pointerdeclaration_in_simpledeclarations();
    void test_format_pointerdeclaration_in_simpledeclarations_data();
    void test_format_pointerdeclaration_in_controlflowstatements();
    void test_format_pointerdeclaration_in_controlflowstatements_data();
    void test_format_pointerdeclaration_multiple_declarators();
    void test_format_pointerdeclaration_multiple_declarators_data();
    void test_format_pointerdeclaration_multiple_matches();
    void test_format_pointerdeclaration_multiple_matches_data();
    void test_format_pointerdeclaration_macros();
    void test_format_pointerdeclaration_macros_data();

    void test_cppsourceprocessor_includes_resolvedUnresolved();
    void test_cppsourceprocessor_includes_cyclic();
    void test_cppsourceprocessor_includes_allDiagnostics();
    void test_cppsourceprocessor_macroUses();

    void test_functionutils_virtualFunctions();
    void test_functionutils_virtualFunctions_data();

    void test_modelmanager_paths_are_clean();
    void test_modelmanager_framework_headers();
    void test_modelmanager_refresh_also_includes_of_project_files();
    void test_modelmanager_refresh_several_times();
    void test_modelmanager_refresh_test_for_changes();
    void test_modelmanager_refresh_added_and_purge_removed();
    void test_modelmanager_refresh_timeStampModified_if_sourcefiles_change();
    void test_modelmanager_refresh_timeStampModified_if_sourcefiles_change_data();
    void test_modelmanager_snapshot_after_two_projects();
    void test_modelmanager_extraeditorsupport_uiFiles();
    void test_modelmanager_gc_if_last_cppeditor_closed();
    void test_modelmanager_dont_gc_opened_files();
    void test_modelmanager_defines_per_project();
    void test_modelmanager_defines_per_project_pch();
    void test_modelmanager_defines_per_editor();

    void test_cpplocatorfilters_CppLocatorFilter();
    void test_cpplocatorfilters_CppLocatorFilter_data();
    void test_cpplocatorfilters_CppCurrentDocumentFilter();

    void test_builtinsymbolsearcher();
    void test_builtinsymbolsearcher_data();

    void test_headersource_data();
    void test_headersource();

    void test_typehierarchy_data();
    void test_typehierarchy();

    void test_includeGroups_detectIncludeGroupsByNewLines();
    void test_includeGroups_detectIncludeGroupsByIncludeDir();
    void test_includeGroups_detectIncludeGroupsByIncludeType();
#endif

private:
    QSharedPointer<CppFileSettings> m_fileSettings;
    QSharedPointer<CppCodeModelSettings> m_codeModelSettings;
    CppToolsSettings *m_settings;
    StringTable m_stringTable;
};

} // namespace Internal
} // namespace CppTools

#endif // CPPTOOLS_H
