/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef PROFILEEVALUATOR_H
#define PROFILEEVALUATOR_H

#include "qmake_global.h"
#include "proitems.h"

#include <QHash>
#include <QStringList>

QT_BEGIN_NAMESPACE

class QMakeGlobals;
class QMakeParser;

class QMAKE_EXPORT ProFileEvaluatorHandler
{
public:
    // qmake/project configuration error
    virtual void configError(const QString &msg) = 0;
    // Some error during evaluation
    virtual void evalError(const QString &filename, int lineNo, const QString &msg) = 0;
    // error() and message() from .pro file
    virtual void fileMessage(const QString &msg) = 0;

    enum EvalFileType { EvalProjectFile, EvalIncludeFile, EvalConfigFile, EvalFeatureFile, EvalAuxFile };
    virtual void aboutToEval(ProFile *parent, ProFile *proFile, EvalFileType type) = 0;
    virtual void doneWithEval(ProFile *parent) = 0;
};


class QMAKE_EXPORT ProFileEvaluator
{
    class Private;

public:
    enum TemplateType {
        TT_Unknown = 0,
        TT_Application,
        TT_Library,
        TT_Script,
        TT_Aux,
        TT_Subdirs
    };

    // Call this from a concurrency-free context
    static void initialize();

    ProFileEvaluator(QMakeGlobals *option, QMakeParser *parser, ProFileEvaluatorHandler *handler);
    ~ProFileEvaluator();

    ProFileEvaluator::TemplateType templateType() const;
#ifdef PROEVALUATOR_CUMULATIVE
    void setCumulative(bool on); // Default is true!
#endif
    void setOutputDir(const QString &dir); // Default is empty

    enum LoadFlag {
        LoadProOnly = 0,
        LoadPreFiles = 1,
        LoadPostFiles = 2,
        LoadAll = LoadPreFiles|LoadPostFiles
    };
    Q_DECLARE_FLAGS(LoadFlags, LoadFlag)
    bool accept(ProFile *pro, LoadFlags flags = LoadAll);

    bool contains(const QString &variableName) const;
    QString value(const QString &variableName) const;
    QStringList values(const QString &variableName) const;
    QStringList values(const QString &variableName, const ProFile *pro) const;
    QStringList absolutePathValues(const QString &variable, const QString &baseDirectory) const;
    QStringList absoluteFileValues(
            const QString &variable, const QString &baseDirectory, const QStringList &searchDirs,
            const ProFile *pro) const;
    QString propertyValue(const QString &val) const;

private:
    QString sysrootify(const QString &path, const QString &baseDir) const;

    Private *d;

    friend class QMakeGlobals;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(ProFileEvaluator::LoadFlags)

QT_END_NAMESPACE

#endif // PROFILEEVALUATOR_H
