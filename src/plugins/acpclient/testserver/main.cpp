// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "acptestserver.h"

#include <QCommandLineParser>
#include <QCoreApplication>

#include <cstdio>

int main(int argc, char *argv[])
{
    // stdout is a pipe when driven by a client; make sure every protocol line
    // leaves the process immediately.
    setvbuf(stdout, nullptr, _IONBF, 0);

    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("acptestserver");

    QCommandLineParser parser;
    parser.setApplicationDescription(
        "Deterministic ACP agent for testing the Qt Creator AcpClient plugin.");
    parser.addHelpOption();
    parser.addOption({"require-auth", "Require authenticate before session/new succeeds."});
    parser.addOption({"sessions", "Seed <count> sessions and advertise session "
                                  "list/load/delete/close capabilities.", "count"});
    parser.addOption({"permission", "Request permission during session/prompt."});
    parser.addOption({"cancel", "Wait for session/cancel during session/prompt."});
    parser.addOption({"crash-on-prompt", "Exit with code 1 on session/prompt."});
    parser.addOption({"config-options", "Report session config options."});
    parser.addOption({"modes", "Report session modes."});
    parser.addOption({"updates-all", "Stream one session/update of each kind per prompt."});
    parser.addOption({"stderr-noise", "Write noise to stderr."});
    parser.addOption({"invalid-response-on-prompt",
                      "Write a non-JSON line before answering session/prompt."});
    parser.addOption({"chunks", "Number of agent_message_chunk updates per prompt "
                                "(default 3).", "count"});
    parser.process(app);

    AcpTestServer::ServerScenario scenario;
    scenario.requireAuth = parser.isSet("require-auth");
    if (parser.isSet("sessions"))
        scenario.seededSessions = parser.value("sessions").toInt();
    scenario.permission = parser.isSet("permission");
    scenario.waitForCancel = parser.isSet("cancel");
    scenario.crashOnPrompt = parser.isSet("crash-on-prompt");
    scenario.configOptions = parser.isSet("config-options");
    scenario.modes = parser.isSet("modes");
    scenario.allUpdateKinds = parser.isSet("updates-all");
    scenario.stderrNoise = parser.isSet("stderr-noise");
    scenario.invalidResponse = parser.isSet("invalid-response-on-prompt");
    if (parser.isSet("chunks"))
        scenario.chunks = parser.value("chunks").toInt();

    AcpTestServer::Server server(scenario);
    return server.run();
}
