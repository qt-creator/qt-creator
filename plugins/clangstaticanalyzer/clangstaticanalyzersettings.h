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

#ifndef CLANGSTATICANALYZERSETTINGS_H
#define CLANGSTATICANALYZERSETTINGS_H

#include <QString>

namespace ClangStaticAnalyzer {
namespace Internal {

class ClangStaticAnalyzerSettings
{
public:
    static ClangStaticAnalyzerSettings *instance();

    void writeSettings() const;

    QString defaultClangExecutable() const;
    QString clangExecutable(bool *isSet = nullptr) const;
    void setClangExecutable(const QString &exectuable);

    int simultaneousProcesses() const;
    void setSimultaneousProcesses(int processes);

private:
    ClangStaticAnalyzerSettings();
    void readSettings();

    QString m_clangExecutable;
    int m_simultaneousProcesses;
};

} // namespace Internal
} // namespace ClangStaticAnalyzer

#endif // CLANGSTATICANALYZERSETTINGS_H
