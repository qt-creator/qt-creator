/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd
** All rights reserved.
** For any questions to The Qt Company, please use contact form at http://www.qt.io/contact-us
**
** This file is part of the Qt Enterprise LicenseChecker Add-on.
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
    bool isValid() const;

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
