/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd
** All rights reserved.
** For any questions to The Qt Company, please use contact form at http://www.qt.io/contact-us
**
** This file is part of the Qt Enterprise Qt Quick Profiler Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.
**
** If you have questions regarding the use of this file, please use
** contact form at http://www.qt.io/contact-us
**
****************************************************************************/
#include "clangstaticanalyzerprojectsettings.h"

#include "clangstaticanalyzerdiagnostic.h"

#include <utils/qtcassert.h>

namespace ClangStaticAnalyzer {
namespace Internal {

static QString suppressedDiagnosticsKey()
{
    return QLatin1String("ClangStaticAnalyzer.SuppressedDiagnostics");
}

static QString suppressedDiagnosticFilePathKey()
{
    return QLatin1String("ClangStaticAnalyzer.SuppressedDiagnosticFilePath");
}

static QString suppressedDiagnosticMessageKey()
{
    return QLatin1String("ClangStaticAnalyzer.SuppressedDiagnosticMessage");
}

static QString suppressedDiagnosticContextKindKey()
{
    return QLatin1String("ClangStaticAnalyzer.SuppressedDiagnosticContextKind");
}

static QString suppressedDiagnosticContextKey()
{
    return QLatin1String("ClangStaticAnalyzer.SuppressedDiagnosticContext");
}

static QString suppressedDiagnosticUniquifierKey()
{
    return QLatin1String("ClangStaticAnalyzer.SuppressedDiagnosticUniquifier");
}

ProjectSettings::ProjectSettings(ProjectExplorer::Project *project) : m_project(project)
{
    load();
    connect(project, &ProjectExplorer::Project::aboutToSaveSettings, this,
            &ProjectSettings::store);
}

void ProjectSettings::addSuppressedDiagnostic(const SuppressedDiagnostic &diag)
{
    QTC_ASSERT(!m_suppressedDiagnostics.contains(diag), return);
    m_suppressedDiagnostics << diag;
    emit suppressedDiagnosticsChanged();
}

void ProjectSettings::removeSuppressedDiagnostic(const SuppressedDiagnostic &diag)
{
    const bool wasPresent = m_suppressedDiagnostics.removeOne(diag);
    QTC_ASSERT(wasPresent, return);
    emit suppressedDiagnosticsChanged();
}

void ProjectSettings::removeAllSuppressedDiagnostics()
{
    m_suppressedDiagnostics.clear();
    emit suppressedDiagnosticsChanged();
}

void ProjectSettings::load()
{
    const QVariantList list = m_project->namedSettings(suppressedDiagnosticsKey()).toList();
    foreach (const QVariant &v, list) {
        const QVariantMap diag = v.toMap();
        const QString fp = diag.value(suppressedDiagnosticFilePathKey()).toString();
        if (fp.isEmpty())
            continue;
        const QString message = diag.value(suppressedDiagnosticMessageKey()).toString();
        if (message.isEmpty())
            continue;
        Utils::FileName fullPath = Utils::FileName::fromString(fp);
        if (fullPath.toFileInfo().isRelative()) {
            fullPath = m_project->projectDirectory();
            fullPath.appendPath(fp);
        }
        if (!fullPath.exists())
            continue;
        const QString contextKind = diag.value(suppressedDiagnosticContextKindKey()).toString();
        const QString context = diag.value(suppressedDiagnosticContextKey()).toString();
        const int uniquifier = diag.value(suppressedDiagnosticUniquifierKey()).toInt();
        m_suppressedDiagnostics << SuppressedDiagnostic(Utils::FileName::fromString(fp), message,
                                                        contextKind, context, uniquifier);
    }
    emit suppressedDiagnosticsChanged();
}

void ProjectSettings::store()
{
    QVariantList list;
    foreach (const SuppressedDiagnostic &diag, m_suppressedDiagnostics) {
        QVariantMap diagMap;
        diagMap.insert(suppressedDiagnosticFilePathKey(), diag.filePath.toString());
        diagMap.insert(suppressedDiagnosticMessageKey(), diag.description);
        diagMap.insert(suppressedDiagnosticContextKindKey(), diag.contextKind);
        diagMap.insert(suppressedDiagnosticContextKey(), diag.context);
        diagMap.insert(suppressedDiagnosticUniquifierKey(), diag.uniquifier);
        list << diagMap;
    }
    m_project->setNamedSettings(suppressedDiagnosticsKey(), list);
}


SuppressedDiagnostic::SuppressedDiagnostic(const Diagnostic &diag)
    : filePath(Utils::FileName::fromString(diag.location.filePath))
    , description(diag.description)
    , contextKind(diag.issueContextKind)
    , context(diag.issueContext)
    , uniquifier(diag.explainingSteps.count())
{
}

} // namespace Internal
} // namespace ClangStaticAnalyzer
