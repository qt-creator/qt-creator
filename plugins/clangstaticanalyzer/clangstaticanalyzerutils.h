/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd
** All rights reserved.
** For any questions to The Qt Company, please use contact form at http://www.qt.io/contact-us
**
** This file is part of the Qt Enterprise Qt LicenseChecker Add-on.
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

#ifndef CLANGSTATICANALYZERUTILS_H
#define CLANGSTATICANALYZERUTILS_H

#include <QtGlobal>

QT_BEGIN_NAMESPACE
class QString;
QT_END_NAMESPACE



namespace ClangStaticAnalyzer {
namespace Internal {

class Location;

QString clangExecutable(const QString &fileNameOrPath, bool *isValid);
QString clangExecutableFromSettings(const QString &toolchainType, bool *isValid);

QString createFullLocationString(const ClangStaticAnalyzer::Internal::Location &location);

} // namespace Internal
} // namespace ClangStaticAnalyzer

#endif // CLANGSTATICANALYZERUTILS_H
