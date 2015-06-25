/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd
** All rights reserved.
** For any questions to The Qt Company, please use contact form at http://www.qt.io/contact-us
**
** This file is part of the Qt Enterprise ClangStaticAnalyzer Add-on.
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
#ifndef CLANGSTATICANALYZERPROJECTSETTINGS_H
#define CLANGSTATICANALYZERPROJECTSETTINGS_H

#include <projectexplorer/project.h>
#include <utils/fileutils.h>

#include <QList>
#include <QObject>

namespace ClangStaticAnalyzer {
namespace Internal {
class Diagnostic;

class SuppressedDiagnostic
{
public:
    SuppressedDiagnostic(const Utils::FileName &filePath, const QString &description,
                         const QString &contextKind, const QString &context, int uniquifier)
        : filePath(filePath)
        , description(description)
        , contextKind(contextKind)
        , context(context)
        , uniquifier(uniquifier)
    {
    }

    SuppressedDiagnostic(const Diagnostic &diag);

    Utils::FileName filePath; // Relative for files in project, absolute otherwise.
    QString description;
    QString contextKind;
    QString context;
    int uniquifier;
};

inline bool operator==(const SuppressedDiagnostic &d1, const SuppressedDiagnostic &d2)
{
    return d1.filePath == d2.filePath && d1.description == d2.description
            && d1.contextKind == d2.contextKind && d1.context == d2.context
            && d1.uniquifier == d2.uniquifier;
}

typedef QList<SuppressedDiagnostic> SuppressedDiagnosticsList;

class ProjectSettings : public QObject
{
    Q_OBJECT
public:
    ProjectSettings(ProjectExplorer::Project *project);

    SuppressedDiagnosticsList suppressedDiagnostics() const { return m_suppressedDiagnostics; }
    void addSuppressedDiagnostic(const SuppressedDiagnostic &diag);
    void removeSuppressedDiagnostic(const SuppressedDiagnostic &diag);
    void removeAllSuppressedDiagnostics();

signals:
    void suppressedDiagnosticsChanged();

private:
    void load();
    void store();

    ProjectExplorer::Project * const m_project;
    SuppressedDiagnosticsList m_suppressedDiagnostics;
};

} // namespace Internal
} // namespace ClangStaticAnalyzer

#endif // Include guard.
