/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef CPPSETTINGSPAGE_H
#define CPPSETTINGSPAGE_H

#include <coreplugin/dialogs/ioptionspage.h>
#include <QtCore/QPointer>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE
namespace Ui {
    class CppFileSettingsPage;
}
class QSettings;
QT_END_NAMESPACE

namespace CppTools {
namespace Internal {

struct CppFileSettings {
    CppFileSettings();

    QString headerSuffix;
    QString sourceSuffix;
    bool lowerCaseFiles;

    void toSettings(QSettings *) const;
    void fromSettings(QSettings *);
    void applySuffixesToMimeDB();

    bool equals(const CppFileSettings &rhs) const;
};

inline bool operator==(const CppFileSettings &s1, const CppFileSettings &s2) { return s1.equals(s2); }
inline bool operator!=(const CppFileSettings &s1, const CppFileSettings &s2) { return !s1.equals(s2); }

class CppFileSettingsWidget : public QWidget {
    Q_OBJECT
public:
    explicit CppFileSettingsWidget(QWidget *parent = 0);
    virtual ~CppFileSettingsWidget();

    CppFileSettings settings() const;
    void setSettings(const CppFileSettings &s);

private:
    Ui::CppFileSettingsPage *m_ui;
};

class CppFileSettingsPage : public Core::IOptionsPage
{
public:
    explicit CppFileSettingsPage(QObject *parent = 0);
    virtual ~CppFileSettingsPage();

    virtual QString id() const;
    virtual QString trName() const;
    virtual QString category() const;
    virtual QString trCategory() const;

    virtual QWidget *createPage(QWidget *parent);
    virtual void apply();
    virtual void finish() { }

private:
    QPointer<CppFileSettingsWidget> m_widget;
    CppFileSettings m_settings;
};

} // namespace Internal
} // namespace CppTools

#endif // CPPSETTINGSPAGE_H
