/**************************************************************************
**
** Copyright (c) 2014 Lorenz Haas
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef BEAUTIFIER_ABSTRACTSETTINGS_H
#define BEAUTIFIER_ABSTRACTSETTINGS_H

#include <QCoreApplication>
#include <QDir>
#include <QHash>
#include <QMap>
#include <QSet>
#include <QString>
#include <QStringList>

namespace Beautifier {
namespace Internal {

class AbstractSettings
{
    Q_DECLARE_TR_FUNCTIONS(AbstractSettings)

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
    QString styleFileName(const QString &key) const;

    QString command() const;
    void setCommand(const QString &command);

    QStringList options();
    QString documentation(const QString &option) const;

protected:
    QMap<QString, QString> m_styles;
    QMap<QString, QVariant> m_settings;

    void readDocumentation();

private:
    QString m_name;
    QString m_ending;
    QDir m_styleDir;
    QStringList m_stylesToRemove;
    QSet<QString> m_changedStyles;
    QString m_command;
    QHash<QString, int> m_options;
    QStringList m_docu;
};

} // namespace Internal
} // namespace Beautifier

#endif // BEAUTIFIER_ABSTRACTSETTINGS_H
