// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>
#include <QString>
#include <QList>
#include <QMap>

QT_BEGIN_NAMESPACE
QT_END_NAMESPACE

namespace Core { class GeneratedFile; }

namespace QmakeProjectManager {
namespace Internal {

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
    static QList<Core::GeneratedFile> generatePlugin(const GenerationParameters& p,
                                                     const PluginOptions &options,
                                                     QString *errorMessage);

private:
    using SubstitutionMap = QMap<QString, QString>;
    static QString processTemplate(const QString &tmpl, const SubstitutionMap &substMap, QString *errorMessage);
    static QString cStringQuote(QString s);
};

}
}
