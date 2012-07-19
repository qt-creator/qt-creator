/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

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
    explicit TabSettingsWidget(QWidget *parent = 0);
    ~TabSettingsWidget();

    TabSettings tabSettings() const;

    void setFlat(bool on);
    QString searchKeywords() const;

public slots:
    void setTabSettings(const TextEditor::TabSettings& s);

signals:
    void settingsChanged(const TextEditor::TabSettings &);

protected:
    void changeEvent(QEvent *e);

private slots:
    void slotSettingsChanged();

private:
    Internal::Ui::TabSettingsWidget *ui;
};

} // namespace TextEditor
#endif // TEXTEDITOR_TABSETTINGSWIDGET_H
