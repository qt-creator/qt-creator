/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QMLJSCODESTYLESETTINGSPAGE_H
#define QMLJSCODESTYLESETTINGSPAGE_H

#include <coreplugin/dialogs/ioptionspage.h>
#include <QWidget>
#include <QPointer>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace TextEditor {
    class FontSettings;
    class TabSettings;
    class CodeStyleEditor;
    class ICodeStylePreferences;
}

namespace QmlJSTools {
namespace Internal {

namespace Ui { class QmlJSCodeStyleSettingsPage; }

class QmlJSCodeStylePreferencesWidget : public QWidget
{
    Q_OBJECT

public:
    explicit QmlJSCodeStylePreferencesWidget(QWidget *parent = 0);
    ~QmlJSCodeStylePreferencesWidget();

    void setPreferences(TextEditor::ICodeStylePreferences *preferences);

private slots:
    void decorateEditor(const TextEditor::FontSettings &fontSettings);
    void setVisualizeWhitespace(bool on);
    void slotSettingsChanged();
    void updatePreview();

private:
    TextEditor::ICodeStylePreferences *m_preferences;
    Ui::QmlJSCodeStyleSettingsPage *m_ui;
};


class QmlJSCodeStyleSettingsPage : public Core::IOptionsPage
{
    Q_OBJECT

public:
    explicit QmlJSCodeStyleSettingsPage(QWidget *parent = 0);

    QWidget *widget();
    void apply();
    void finish();

private:
    TextEditor::ICodeStylePreferences *m_pageTabPreferences;
    QPointer<TextEditor::CodeStyleEditor> m_widget;
};

} // namespace Internal
} // namespace CppTools

#endif // QMLJSCODESTYLESETTINGSPAGE_H
