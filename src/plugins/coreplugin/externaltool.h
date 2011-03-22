/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
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
#include <QtCore/QSharedPointer>
#include <QtCore/QTextCodec>
#include <QtGui/QMenu>

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
    bool m_hasError;
    QString m_errorString;
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
    void openPreferences();

private:
    void initialize();
    void parseDirectory(const QString &directory,
                        QMap<QString, QMultiMap<int, Internal::ExternalTool*> > *categoryMenus,
                        QMap<QString, Internal::ExternalTool *> *tools,
                        bool isPreset = false);
    void readSettings(const QMap<QString, Internal::ExternalTool *> &tools,
                      QMap<QString, QList<Internal::ExternalTool*> > *categoryPriorityMap);
    void writeSettings();

    static ExternalToolManager *m_instance;
    Core::ICore *m_core;
    QMap<QString, Internal::ExternalTool *> m_tools;
    QMap<QString, QList<Internal::ExternalTool *> > m_categoryMap;
    QMap<QString, QAction *> m_actions;
    QMap<QString, ActionContainer *> m_containers;
    QAction *m_configureSeparator;
    QAction *m_configureAction;

    // for sending the replaceSelectionRequested signal
    friend class Core::Internal::ExternalToolRunner;
};

} // Core

Q_DECLARE_METATYPE(Core::Internal::ExternalTool *)

#endif // EXTERNALTOOL_H
