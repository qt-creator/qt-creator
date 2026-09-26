// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>

#include <QJsonObject>
#include <QJsonValue>

namespace Utils { class Environment; }

namespace Java::Internal {

void setupJavaLanguageServer();

// The java an environment names: the one JAVA_HOME points at before the one
// found by name, since what is found by name need not be a JDK at all.
Utils::FilePath javaIn(const Utils::Environment &environment);

// Whether that java can run anything. An executable file is no proof of a
// JDK: macOS ships /usr/bin/java as a stub that reports that there is no
// runtime, and a java too old for the server fails just as late. Asking
// costs a process, so it is asked once and only where the answer decides
// how a server is started.
bool javaRuns(const Utils::FilePath &java);

// Takes back the Eclipse project the Android support of Qt Creator used to
// write into the directory of a project, which the server reads in place of
// what this plugin tells it.
void removeLegacyEclipseProjectFiles(const Utils::FilePath &projectDirectory);

// The arguments a setting is to be given: those the launcher jar is run with
// where a java runs it, and none at all where the executable starts the
// server by itself or where the jar is one this knows no command line for.
QString jdtlsArguments(const Utils::FilePath &executable, const Utils::FilePath &launcherJar);

// How the server of an installation is to be started: the launcher jar of it
// run by the java given, and the launcher of the installation itself where
// there is no jar, no configuration for this machine or no java.
class JdtlsCommand
{
public:
    Utils::FilePath executable;
    Utils::FilePath launcherJar;
    QString arguments;

    bool isEmpty() const { return executable.isEmpty(); }
};

JdtlsCommand jdtlsCommandIn(const Utils::FilePath &installation, const Utils::FilePath &java);

// The configuration of the user as it stands once another one has come in.
// A client is handed back what it last sent when the server comes up, and
// that says nothing about what the user set, so it leaves it as it was.
QJsonValue userJavaConfiguration(const QJsonValue &incoming,
                                 const QJsonValue &sent,
                                 const QJsonValue &previousUser);

// The configuration to send: what the build system says about the project
// goes in beside what the user set under "java.project", not in place of it.
QJsonValue javaConfigurationWithProject(const QJsonValue &user, const QJsonObject &project);

// What a class path that has just changed is told as. One that has gone away
// is empty lists rather than nothing at all, which the server would read as
// having nothing to say about it.
QJsonObject javaProjectToSend(const QJsonObject &project, const QJsonObject &was);

} // namespace Java::Internal
