// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/generatedfile.h>

#include <QMap>

namespace QmakeProjectManager::Internal {

struct PluginOptions;

struct GenerationParameters {
    QString path;
    QString fileName;
    QString templatePath;
};

class PluginGenerator : public QObject
{
    Q_OBJECT

public:
    static Utils::Result<Core::GeneratedFiles> generatePlugin(const GenerationParameters& p,
                                                              const PluginOptions &options);

private:
    using SubstitutionMap = QMap<QString, QString>;
    static Utils::Result<QString> processTemplate(const QString &tmpl, const SubstitutionMap &substMap);
    static QString cStringQuote(QString s);
};

} // QmakeProjectManager::Internal
