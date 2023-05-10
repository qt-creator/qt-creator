// Copyright (C) 2016 Lorenz Haas
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/aspects.h>
#include <utils/filepath.h>

#include <QCoreApplication>
#include <QDir>
#include <QHash>
#include <QMap>
#include <QObject>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QVector>

#include <memory>

QT_BEGIN_NAMESPACE
class QRegularExpression;
class QVersionNumber;
QT_END_NAMESPACE

namespace Core { class IDocument; }

namespace Beautifier::Internal {

class VersionUpdater;

class AbstractSettings : public Utils::AspectContainer
{
    Q_OBJECT

public:
    explicit AbstractSettings(const QString &name, const QString &ending);
    ~AbstractSettings() override;

    void read();
    void save();

    virtual QString documentationFilePath() const = 0;
    virtual void createDocumentationFile() const;
    virtual QStringList completerWords();

    QStringList styles() const;
    QString style(const QString &key) const;
    bool styleExists(const QString &key) const;
    bool styleIsReadOnly(const QString &key);
    void setStyle(const QString &key, const QString &value);
    void removeStyle(const QString &key);
    void replaceStyle(const QString &oldKey, const QString &newKey, const QString &value);
    virtual QString styleFileName(const QString &key) const;

    Utils::FilePath command() const;
    void setCommand(const Utils::FilePath &cmd);
    QVersionNumber version() const;

    QString supportedMimeTypesAsString() const;
    void setSupportedMimeTypes(const QString &mimes);
    bool isApplicable(const Core::IDocument *document) const;

    QStringList options();
    QString documentation(const QString &option) const;

signals:
    void supportedMimeTypesChanged();

protected:
    void setVersionRegExp(const QRegularExpression &versionRegExp);

    QMap<QString, QString> m_styles;
    QMap<QString, QVariant> m_settings;
    QString m_ending;
    QDir m_styleDir;

    void readDocumentation();
    virtual void readStyles();

private:
    QString m_name;
    std::unique_ptr<VersionUpdater> m_versionUpdater;
    QStringList m_stylesToRemove;
    QSet<QString> m_changedStyles;
    Utils::FilePath m_command;
    QHash<QString, int> m_options;
    QStringList m_docu;
    QStringList m_supportedMimeTypes;
};

} // Beautifier::Internal
