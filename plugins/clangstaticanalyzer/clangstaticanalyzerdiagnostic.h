/****************************************************************************
**
** Copyright (C) 2014 Digia Plc
** All rights reserved.
** For any questions to Digia, please use contact form at http://qt.digia.com <http://qt.digia.com/>
**
** This file is part of the Qt Enterprise LicenseChecker Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.
**
** If you have questions regarding the use of this file, please use
** contact form at http://qt.digia.com <http://qt.digia.com/>
**
****************************************************************************/

#ifndef CLANGSTATICANALZYERDIAGNOSTIC_H
#define CLANGSTATICANALZYERDIAGNOSTIC_H

#include <QList>
#include <QMetaType>
#include <QString>

namespace ClangStaticAnalyzer {
namespace Internal {

class Location
{
public:
    Location();
    Location(const QString &filePath, int line, int column);

    bool isValid() const;

    QString filePath;
    int line;
    int column;
};

class ExplainingStep
{
public:
    ExplainingStep();

    bool isValid() const;

    QString message;
    QString extendedMessage;
    Location location;
    QList<Location> ranges;
    int depth;
};

class Diagnostic
{
public:
    QString description;
    QString category;
    QString type;
    QString issueContextKind;
    QString issueContext;
    Location location;
    QList<ExplainingStep> explainingSteps;
};

} // namespace Internal
} // namespace ClangStaticAnalyzer

Q_DECLARE_METATYPE(ClangStaticAnalyzer::Internal::Diagnostic)

#endif // CLANGSTATICANALZYERDIAGNOSTIC_H
