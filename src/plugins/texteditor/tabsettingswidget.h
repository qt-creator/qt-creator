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

#ifndef TEXTEDITOR_TABSETTINGSWIDGET_H
#define TEXTEDITOR_TABSETTINGSWIDGET_H

#include "texteditor_global.h"

#include <QWidget>

namespace TextEditor {

namespace Internal { namespace Ui { class TabSettingsWidget; } }

class TabSettings;

class TEXTEDITOR_EXPORT TabSettingsWidget : public QWidget
{
    Q_OBJECT

public:
    enum CodingStyleLink {
        CppLink,
        QtQuickLink
    };

    explicit TabSettingsWidget(QWidget *parent = 0);
    ~TabSettingsWidget();

    TabSettings tabSettings() const;

    void setFlat(bool on);
    QString searchKeywords() const;
    void setCodingStyleWarningVisible(bool visible);

public slots:
    void setTabSettings(const TextEditor::TabSettings& s);

signals:
    void settingsChanged(const TextEditor::TabSettings &);
    void codingStyleLinkClicked(TextEditor::TabSettingsWidget::CodingStyleLink link);

protected:
    void changeEvent(QEvent *e);

private slots:
    void slotSettingsChanged();
    void codingStyleLinkActivated(const QString &linkString);

private:
    Internal::Ui::TabSettingsWidget *ui;
};

} // namespace TextEditor
#endif // TEXTEDITOR_TABSETTINGSWIDGET_H
