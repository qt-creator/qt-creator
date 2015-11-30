/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef EXTERNALTOOL_H
#define EXTERNALTOOL_H

#include <utils/fileutils.h>
#include <utils/environment.h>

#include <QObject>
#include <QStringList>
#include <QProcess>
#include <QSharedPointer>
#include <QTextCodec>
#include <QMetaType>

namespace Utils { class QtcProcess; }
namespace Core {
namespace Internal {

class ExternalTool : public QObject
{
    Q_OBJECT

public:
    enum OutputHandling {
        Ignore,
        ShowInPane,
        ReplaceSelection
    };

    ExternalTool();
    explicit ExternalTool(const ExternalTool *other);
    ~ExternalTool();

    QString id() const;
    QString description() const;
    QString displayName() const;
    QString displayCategory() const;
    int order() const;
    OutputHandling outputHandling() const;
    OutputHandling errorHandling() const;
    bool modifiesCurrentDocument() const;

    QStringList executables() const;
    QString arguments() const;
    QString input() const;
    QString workingDirectory() const;
    QList<Utils::EnvironmentItem> environment() const;

    void setFileName(const QString &fileName);
    void setPreset(QSharedPointer<ExternalTool> preset);
    QString fileName() const;
    // all tools that are preset (changed or unchanged) have the original value here:
    QSharedPointer<ExternalTool> preset() const;

    static ExternalTool *createFromXml(const QByteArray &xml, QString *errorMessage = 0, const QString &locale = QString());
    static ExternalTool *createFromFile(const QString &fileName, QString *errorMessage = 0,
                                        const QString &locale = QString());

    bool save(QString *errorMessage = 0) const;

    bool operator==(const ExternalTool &other) const;
    bool operator!=(const ExternalTool &other) const { return !((*this) == other); }
    ExternalTool &operator=(const ExternalTool &other);

    void setId(const QString &id);
    void setDisplayCategory(const QString &category);
    void setDisplayName(const QString &name);
    void setDescription(const QString &description);
    void setOutputHandling(OutputHandling handling);
    void setErrorHandling(OutputHandling handling);
    void setModifiesCurrentDocument(bool modifies);
    void setExecutables(const QStringList &executables);
    void setArguments(const QString &arguments);
    void setInput(const QString &input);
    void setWorkingDirectory(const QString &workingDirectory);
    void setEnvironment(const QList<Utils::EnvironmentItem> &items);

private:
    QString m_id;
    QString m_description;
    QString m_displayName;
    QString m_displayCategory;
    int m_order;
    QStringList m_executables;
    QString m_arguments;
    QString m_input;
    QString m_workingDirectory;
    QList<Utils::EnvironmentItem> m_environment;
    OutputHandling m_outputHandling;
    OutputHandling m_errorHandling;
    bool m_modifiesCurrentDocument;

    QString m_fileName;
    QString m_presetFileName;
    QSharedPointer<ExternalTool> m_presetTool;
};

class ExternalToolRunner : public QObject
{
    Q_OBJECT
public:
    ExternalToolRunner(const ExternalTool *tool);
    ~ExternalToolRunner();

    bool hasError() const;
    QString errorString() const;

private slots:
    void started();
    void finished(int exitCode, QProcess::ExitStatus status);
    void error(QProcess::ProcessError error);
    void readStandardOutput();
    void readStandardError();

private:
    void run();
    bool resolve();

    const ExternalTool *m_tool; // is a copy of the tool that was passed in
    Utils::FileName m_resolvedExecutable;
    QString m_resolvedArguments;
    QString m_resolvedInput;
    QString m_resolvedWorkingDirectory;
    Utils::Environment m_resolvedEnvironment;
    Utils::QtcProcess *m_process;
    QTextCodec *m_outputCodec;
    QTextCodec::ConverterState m_outputCodecState;
    QTextCodec::ConverterState m_errorCodecState;
    QString m_processOutput;
    QString m_expectedFileName;
    bool m_hasError;
    QString m_errorString;
};

} // Internal
} // Core

Q_DECLARE_METATYPE(Core::Internal::ExternalTool *)

#endif // EXTERNALTOOL_H
