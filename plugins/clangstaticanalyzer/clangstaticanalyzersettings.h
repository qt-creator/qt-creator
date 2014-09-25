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

#ifndef CLANGSTATICANALYZERSETTINGS_H
#define CLANGSTATICANALYZERSETTINGS_H

#include <QObject>
#include <QString>

namespace ClangStaticAnalyzer {
namespace Internal {

class ClangStaticAnalyzerSettings : public QObject
{
    Q_OBJECT

public:
    static ClangStaticAnalyzerSettings *instance();

    void writeSettings() const;

    QString clangExecutable() const;
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
