// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0
#include "mcuoutputmodel.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildstep.h>
#include <projectexplorer/deployconfiguration.h>

#include <utils/id.h>

#include <projectexplorer/projectmanager.h>
#include <qmlprojectmanager/buildsystem/qmlbuildsystem.h>

#include <QRegularExpression>
#include <QString>
#include <QStringBuilder>
#include <QStringView>

#include <projectexplorer/projectexplorer.h>

static QString coloredString(const QString &text, const QString &color)
{
    if (color == "0" || color.isEmpty())
        return text;

    static const std::unordered_map<QString, QString> mcuSupportedColors = {
        {"31", "#ff0000"},
        {"32", "#00ff00"},
        {"33", "#ffff00"},
        {"34", "#0000ff"},
        {"36", "#00ffff"},
    };

    if (auto iter = mcuSupportedColors.find(color); iter != mcuSupportedColors.end())
        return QStringLiteral("<span style=\"color:%1\">%2</span>").arg(iter->second, text);

    return text;
}

static QString poorMansAnsiToHtml(QString s)
{
    s = s.toHtmlEscaped();
    s.replace(QStringLiteral("\r\n"), QStringLiteral("<br/>"));
    s.replace(QLatin1Char('\n'), QStringLiteral("<br/>"));

    QString out;
    const QRegularExpression re(R"((?:\x1B\[([0-9;]*)m)?([^\x1B]+))");
    auto it = re.globalMatch(s);
    while (it.hasNext()) {
        const QRegularExpressionMatch m = it.next();
        const QString color = m.captured(1);
        const QString text = m.captured(2);
        out += coloredString(text, color);
    }
    return out;
}

McuOutputModel::McuOutputModel(QObject *parent)
    : QObject(parent)
    , m_text()
{
    QObject::connect(
        ProjectExplorer::ProjectManager::instance(),
        &ProjectExplorer::ProjectManager::startupProjectChanged,
        [this]() {
            if (auto buildSystem = QmlProjectManager::QmlBuildSystem::getStartupBuildSystem()) {
                setup(buildSystem->buildConfiguration());
                setMcu(buildSystem->qtForMCUs());
            }
        });
}

bool McuOutputModel::mcu() const
{
    return m_mcu;
}

QString McuOutputModel::text() const
{
    return m_text;
}

void McuOutputModel::setMcu(bool mcu)
{
    clear();
    m_mcu = mcu;
    emit mcuChanged(m_mcu);
}

void McuOutputModel::setText(const QString &text)
{
    m_text += poorMansAnsiToHtml(text);
    emit textChanged(m_text);
}

void McuOutputModel::clear()
{
    m_text.clear();
    emit textChanged(m_text);
}

void McuOutputModel::setup(ProjectExplorer::BuildConfiguration *bc)
{
    clear();

    ProjectExplorer::DeployConfiguration *deployConfiguration = bc->activeDeployConfiguration();
    ProjectExplorer::BuildStepList *stepList = deployConfiguration->stepList();
    ProjectExplorer::BuildStep *step = stepList->firstStepWithId("QmlProject.Mcu.DeployStep");

    if (step) {
        step->disconnect(this);
        connect(step, &ProjectExplorer::BuildStep::addOutput, this, &McuOutputModel::addOutput);
    }
}

void McuOutputModel::addOutput(const QString &str, ProjectExplorer::BuildStep::OutputFormat fmt)
{
    std::string format;
    switch (fmt) {
    case ProjectExplorer::BuildStep::OutputFormat::Stdout:
        format = "Stdout";
        break;
    case ProjectExplorer::BuildStep::OutputFormat::Stderr:
        format = "Stderr";
        break;
    case ProjectExplorer::BuildStep::OutputFormat::NormalMessage:
        format = "NormalMessage";
        break;
    case ProjectExplorer::BuildStep::OutputFormat::ErrorMessage:
        format = "ErrorMessage";
        break;
    };

    if (fmt == ProjectExplorer::BuildStep::OutputFormat::NormalMessage) {
        if (str.contains("Starting") && str.contains("qmlprojectexporter"))
            clear();
    }

    if (fmt == ProjectExplorer::BuildStep::OutputFormat::Stdout
        || fmt == ProjectExplorer::BuildStep::OutputFormat::Stderr) {
        setText(str);
    }
}
