/****************************************************************************
**
** Copyright (C) 2016 Lorenz Haas
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <QCoreApplication>
#include <QDir>
#include <QHash>
#include <QMap>
#include <QObject>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QVector>

namespace Core { class IDocument; }

namespace Beautifier {
namespace Internal {

class AbstractSettings : public QObject
{
    Q_OBJECT

public:
    explicit AbstractSettings(const QString &name, const QString &ending);
    virtual ~AbstractSettings();

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

    QString command() const;
    void setCommand(const QString &command);
    int version() const;
    virtual void updateVersion();

    QString supportedMimeTypesAsString() const;
    void setSupportedMimeTypes(const QString &mimes);
    bool isApplicable(const Core::IDocument *document) const;

    QStringList options();
    QString documentation(const QString &option) const;

signals:
    void supportedMimeTypesChanged();

protected:
    QMap<QString, QString> m_styles;
    QMap<QString, QVariant> m_settings;
    int m_version = 0;
    QString m_ending;
    QDir m_styleDir;

    void readDocumentation();
    virtual void readStyles();

private:
    QString m_name;
    QStringList m_stylesToRemove;
    QSet<QString> m_changedStyles;
    QString m_command;
    QHash<QString, int> m_options;
    QStringList m_docu;
    QStringList m_supportedMimeTypes;
};

} // namespace Internal
} // namespace Beautifier
