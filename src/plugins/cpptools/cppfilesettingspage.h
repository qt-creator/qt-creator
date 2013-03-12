/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#ifndef CPPFILESETTINGSPAGE_H
#define CPPFILESETTINGSPAGE_H

#include <coreplugin/dialogs/ioptionspage.h>

#include <QPointer>
#include <QSharedPointer>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace CppTools {
namespace Internal {

namespace Ui { class CppFileSettingsPage; }

struct CppFileSettings
{
    CppFileSettings();

    QString headerSuffix;
    QString sourceSuffix;
    bool lowerCaseFiles;
    QString licenseTemplatePath;

    void toSettings(QSettings *) const;
    void fromSettings(QSettings *);
    bool applySuffixesToMimeDB();

    // Convenience to return a license template completely formatted.
    // Currently made public in
    static QString licenseTemplate(const QString &file = QString(), const QString &className = QString());

    bool equals(const CppFileSettings &rhs) const;
    bool operator==(const CppFileSettings &s) const { return equals(s); }
    bool operator!=(const CppFileSettings &s) const { return !equals(s); }
};

class CppFileSettingsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CppFileSettingsWidget(QWidget *parent = 0);
    virtual ~CppFileSettingsWidget();

    CppFileSettings settings() const;
    void setSettings(const CppFileSettings &s);

    QString searchKeywords() const;

private slots:
    void slotEdit();

private:
    inline QString licenseTemplatePath() const;
    inline void setLicenseTemplatePath(const QString &);

    Ui::CppFileSettingsPage *m_ui;
};

class CppFileSettingsPage : public Core::IOptionsPage
{
public:
    explicit CppFileSettingsPage(QSharedPointer<CppFileSettings> &settings,
                                 QObject *parent = 0);

    QWidget *createPage(QWidget *parent);
    void apply();
    void finish() { }
    bool matches(const QString &) const;

private:
    const QSharedPointer<CppFileSettings> m_settings;
    QPointer<CppFileSettingsWidget> m_widget;
    QString m_searchKeywords;
};

} // namespace Internal
} // namespace CppTools

#endif // CPPFILESETTINGSPAGE_H
