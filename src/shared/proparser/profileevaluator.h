// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmake_global.h"
#include "qmakeglobals.h"
#include "qmakeevaluator.h"
#include "proitems.h"

#include <QHash>
#include <QStringList>

QT_BEGIN_NAMESPACE

class QMakeVfs;
class QMakeParser;
class QMakeEvaluator;
class QMakeHandler;

class QMAKE_EXPORT ProFileEvaluator
{
public:
    enum TemplateType {
        TT_Unknown = 0,
        TT_Application,
        TT_StaticLibrary,
        TT_SharedLibrary,
        TT_Script,
        TT_Aux,
        TT_Subdirs
    };

    struct SourceFile {
        QString fileName;
        int proFileId;
    };

    // Call this from a concurrency-free context
    static void initialize();

    ProFileEvaluator(QMakeGlobals *option, QMakeParser *parser, QMakeVfs *vfs,
                     QMakeHandler *handler);
    ~ProFileEvaluator();

    ProFileEvaluator::TemplateType templateType() const;
#ifdef PROEVALUATOR_CUMULATIVE
    void setCumulative(bool on); // Default is false
#endif
    void setExtraVars(const QHash<QString, QStringList> &extraVars);
    void setExtraConfigs(const QStringList &extraConfigs);
    void setOutputDir(const QString &dir); // Default is empty

    bool loadNamedSpec(const QString &specDir, bool hostSpec);

    bool accept(ProFile *pro, QMakeEvaluator::LoadFlags flags = QMakeEvaluator::LoadAll);

    bool contains(const QString &variableName) const;
    QString value(const QString &variableName) const;
    QStringList values(const QString &variableName) const;
    QVector<SourceFile> fixifiedValues(
            const QString &variable, const QString &baseDirectory, const QString &buildDirectory,
            bool expandWildcards) const;
    QStringList absolutePathValues(const QString &variable, const QString &baseDirectory) const;
    QVector<SourceFile> absoluteFileValues(
            const QString &variable, const QString &baseDirectory, const QStringList &searchDirs,
            QHash<ProString, bool> *handled, QSet<QString> &directoriesWithWildcards) const;
    QString propertyValue(const QString &val) const;
    static QStringList sourcesToFiles(const QVector<SourceFile> &sources);
    QStringList featureRoots() const;

private:
    QMakeEvaluator *d;
    QMakeVfs *m_vfs;
};

Q_DECLARE_TYPEINFO(ProFileEvaluator::SourceFile, Q_MOVABLE_TYPE);

QT_END_NAMESPACE
