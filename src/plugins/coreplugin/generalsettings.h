/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef GENERALSETTINGS_H
#define GENERALSETTINGS_H

#include <coreplugin/dialogs/ioptionspage.h>
#include <QtCore/QPointer>
#include <QtGui/QWidget>

namespace Core {
namespace Internal {

namespace Ui {
    class GeneralSettings;
}

class GeneralSettings : public IOptionsPage
{
    Q_OBJECT

public:
    GeneralSettings();

    QString id() const;
    QString trName() const;
    QString category() const;
    QString trCategory() const;
    QWidget* createPage(QWidget *parent);
    void apply();
    void finish();
    virtual bool matches(const QString &) const;

private slots:
    void resetInterfaceColor();
    void resetExternalEditor();
    void resetLanguage();
    void showHelpForExternalEditor();
#ifdef Q_OS_UNIX
#  ifndef Q_OS_MAC
    void showHelpForFileBrowser();
    void resetFileBrowser();
#  endif
    void resetTerminal();
#endif

private:
    void variableHelpDialogCreator(const QString& helpText);
    void fillLanguageBox() const;
    QString language() const;
    void setLanguage(const QString&);
    Ui::GeneralSettings *m_page;
    QString m_searchKeywords;
    QPointer<QWidget> m_dialog;
};

} // namespace Internal
} // namespace Core

#endif // GENERALSETTINGS_H
