/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef EXTERNALTOOL_H
#define EXTERNALTOOL_H

#include "icore.h"
#include "core_global.h"
#include "actionmanager/command.h"
#include "actionmanager/actioncontainer.h"

#include <utils/qtcprocess.h>

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QProcess>
#include <QtCore/QTextCodec>

namespace Core {
namespace Internal {

class ExternalTool : public QObject
{
    Q_OBJECT

public:
    enum OutputHandling {
        Ignore,
        ShowInPane,
        ReplaceSelection,
        ReloadDocument
    };

    ExternalTool();
    explicit ExternalTool(const ExternalTool *other);
    ~ExternalTool();

    QString id() const;
    QString description() const;
    QString displayName() const;
    void setDisplayName(const QString &name);
    QString displayCategory() const;
    int order() const;
    OutputHandling outputHandling() const;
    OutputHandling errorHandling() const;

    QStringList executables() const;
    QString arguments() const;
    QString input() const;
    QString workingDirectory() const;

    static ExternalTool *createFromXml(const QByteArray &xml, QString *errorMessage = 0, const QString &locale = QString());

    // if the display name is different from the one in the original xml
    bool isDisplayNameChanged() const { return m_isDisplayNameChanged; }

    // ignores changed state
    bool operator==(const ExternalTool &other);
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
    OutputHandling m_outputHandling;
    OutputHandling m_errorHandling;
    bool m_isDisplayNameChanged;
};

class ExternalToolRunner : public QObject
{
    Q_OBJECT
public:
    ExternalToolRunner(const ExternalTool *tool);
    ~ExternalToolRunner();

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
    QString m_resolvedExecutable;
    QString m_resolvedArguments;
    QString m_resolvedInput;
    QString m_resolvedWorkingDirectory;
    Utils::QtcProcess *m_process;
    QTextCodec *m_outputCodec;
    QTextCodec::ConverterState m_outputCodecState;
    QTextCodec::ConverterState m_errorCodecState;
    QString m_processOutput;
    QString m_expectedFileName;
};

} // Internal

class CORE_EXPORT ExternalToolManager : public QObject
{
    Q_OBJECT

public:
    static ExternalToolManager *instance() { return m_instance; }

    ExternalToolManager(Core::ICore *core);
    ~ExternalToolManager();

    QMap<QString, QList<Internal::ExternalTool *> > toolsByCategory() const;
    QMap<QString, Internal::ExternalTool *> toolsById() const;

    void setToolsByCategory(const QMap<QString, QList<Internal::ExternalTool *> > &tools);

signals:
    void replaceSelectionRequested(const QString &text);

private slots:
    void menuActivated();

private:
    void initialize();
    void parseDirectory(const QString &directory,
                        QMap<QString, QMultiMap<int, Internal::ExternalTool*> > *categoryMenus,
                        QMap<QString, Internal::ExternalTool *> *tools,
                        bool ignoreDuplicates = false);
    void readSettings(const QMap<QString, Internal::ExternalTool *> &tools,
                      QMap<QString, QMultiMap<int, Internal::ExternalTool*> > *categoryPriorityMap);
    void writeSettings();

    static ExternalToolManager *m_instance;
    Core::ICore *m_core;
    QMap<QString, Internal::ExternalTool *> m_tools;
    QMap<QString, QList<Internal::ExternalTool *> > m_categoryMap;
    QMap<QString, QAction *> m_actions;
    QMap<QString, ActionContainer *> m_containers;

    // for sending the replaceSelectionRequested signal
    friend class Core::Internal::ExternalToolRunner;
};

} // Core

Q_DECLARE_METATYPE(Core::Internal::ExternalTool *)

#endif // EXTERNALTOOL_H
