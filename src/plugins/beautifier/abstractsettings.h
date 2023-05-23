// Copyright (C) 2016 Lorenz Haas
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>

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

    Utils::FilePathAspect command;
    Utils::StringAspect supportedMimeTypes;
    Utils::FilePathAspect documentationFilePath;

    QVersionNumber version() const;

    bool isApplicable(const Core::IDocument *document) const;

    QStringList options();
    QString documentation(const QString &option) const;

protected:
    void setVersionRegExp(const QRegularExpression &versionRegExp);

    QMap<QString, QString> m_styles;
    QString m_ending;
    QDir m_styleDir;

    void readDocumentation();
    virtual void readStyles();

private:
    std::unique_ptr<VersionUpdater> m_versionUpdater;
    QStringList m_stylesToRemove;
    QSet<QString> m_changedStyles;
    QHash<QString, int> m_options;
    QStringList m_docu;
    QStringList m_supportedMimeTypes;
};

} // Beautifier::Internal
