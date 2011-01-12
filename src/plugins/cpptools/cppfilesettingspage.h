/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef CPPSETTINGSPAGE_H
#define CPPSETTINGSPAGE_H

#include <coreplugin/dialogs/ioptionspage.h>
#include <QtCore/QPointer>
#include <QtCore/QSharedPointer>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE
namespace Ui {
    class CppFileSettingsPage;
}
class QSettings;
QT_END_NAMESPACE

namespace CppTools {
namespace Internal {

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
};

inline bool operator==(const CppFileSettings &s1, const CppFileSettings &s2) { return s1.equals(s2); }
inline bool operator!=(const CppFileSettings &s1, const CppFileSettings &s2) { return !s1.equals(s2); }

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
    Q_DISABLE_COPY(CppFileSettingsPage)

public:
    explicit CppFileSettingsPage(QSharedPointer<CppFileSettings> &settings,
                                 QObject *parent = 0);
    virtual ~CppFileSettingsPage();

    virtual QString id() const;
    virtual QString displayName() const;
    virtual QString category() const;
    virtual QString displayCategory() const;
    virtual QIcon categoryIcon() const;

    virtual QWidget *createPage(QWidget *parent);
    virtual void apply();
    virtual void finish() { }
    virtual bool matches(const QString &) const;

private:
    const QSharedPointer<CppFileSettings> m_settings;
    QPointer<CppFileSettingsWidget> m_widget;
    QString m_searchKeywords;
};

} // namespace Internal
} // namespace CppTools

#endif // CPPSETTINGSPAGE_H
