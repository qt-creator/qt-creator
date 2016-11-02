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
        const ProFile *proFile;
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
            const QString &variable, const QString &baseDirectory, const QString &buildDirectory) const;
    QStringList absolutePathValues(const QString &variable, const QString &baseDirectory) const;
    QVector<SourceFile> absoluteFileValues(
            const QString &variable, const QString &baseDirectory, const QStringList &searchDirs,
            QHash<ProString, bool> *handled) const;
    QString propertyValue(const QString &val) const;
    static QStringList sourcesToFiles(const QVector<SourceFile> &sources);

private:
    QMakeEvaluator *d;
    QMakeVfs *m_vfs;
};

Q_DECLARE_TYPEINFO(ProFileEvaluator::SourceFile, Q_MOVABLE_TYPE);

QT_END_NAMESPACE
