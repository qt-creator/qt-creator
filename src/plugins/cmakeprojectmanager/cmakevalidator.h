/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef CMAKEVALIDATOR_H
#define CMAKEVALIDATOR_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <texteditor/codeassist/keywordscompletionassist.h>

QT_FORWARD_DECLARE_CLASS(QProcess)

namespace CMakeProjectManager {
namespace Internal {

class CMakeValidator : public QObject
{
    Q_OBJECT
public:
    CMakeValidator();
    ~CMakeValidator();

    enum State { Invalid, RunningBasic, RunningFunctionList, RunningFunctionDetails, ValidFunctionDetails };
    void cancel();
    bool isValid() const;

    void setCMakeExecutable(const QString &executable);
    QString cmakeExecutable() const;
    bool hasCodeBlocksMsvcGenerator() const;
    bool hasCodeBlocksNinjaGenerator() const;
    TextEditor::Keywords keywords();
private slots:
    void finished(int exitCode);

private:
    void finishStep();
    void startNextStep();
    bool startProcess(const QStringList &args);
    void parseFunctionOutput(const QByteArray &output);
    void parseFunctionDetailsOutput(const QByteArray &output);
    QString formatFunctionDetails(const QString &command, const QByteArray &args);

    State m_state;
    QProcess *m_process;
    bool m_hasCodeBlocksMsvcGenerator;
    bool m_hasCodeBlocksNinjaGenerator;
    QString m_version;
    QString m_executable;

    QMap<QString, QStringList> m_functionArgs;
    QStringList m_variables;
    QStringList m_functions;
};

} // namespace Internal
} // namespace CMakeProjectManager

#endif // CMAKEVALIDATOR_H
