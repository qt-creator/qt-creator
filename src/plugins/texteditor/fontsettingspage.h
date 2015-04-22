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

#ifndef FONTSETTINGSPAGE_H
#define FONTSETTINGSPAGE_H

#include "texteditor_global.h"

#include "texteditorconstants.h"
#include "colorscheme.h"

#include "texteditoroptionspage.h"

#include <QList>
#include <QString>

QT_BEGIN_NAMESPACE
class QWidget;
class QColor;
QT_END_NAMESPACE

namespace TextEditor {

class Format;
class FontSettings;
namespace Internal { class FontSettingsPagePrivate; }

// GUI description of a format consisting of id (settings key)
// and displayName to be displayed
class TEXTEDITOR_EXPORT FormatDescription
{
public:
    FormatDescription(TextStyle id, const QString &displayName, const QString &tooltipText,
                      const QColor &foreground = Qt::black);
    FormatDescription(TextStyle id, const QString &displayName, const QString &tooltipText,
                      const Format &format);

    TextStyle id() const { return m_id; }

    QString displayName() const
    { return m_displayName; }

    QColor foreground() const;
    QColor background() const;

    const Format &format() const { return m_format; }
    Format &format() { return m_format; }

    QString tooltipText() const
    { return  m_tooltipText; }

private:
    TextStyle m_id;            // Name of the category
    Format m_format;            // Default format
    QString m_displayName;      // Displayed name of the category
    QString m_tooltipText;      // Description text for category
};

typedef QList<FormatDescription> FormatDescriptions;

class TEXTEDITOR_EXPORT FontSettingsPage : public TextEditorOptionsPage
{
    Q_OBJECT

public:
    FontSettingsPage(const FormatDescriptions &fd, Core::Id id, QObject *parent = 0);

    ~FontSettingsPage();

    QWidget *widget();
    void apply();
    void finish();

    void saveSettings();

    const FontSettings &fontSettings() const;

signals:
    void changed(const TextEditor::FontSettings&);

private slots:
    void delayedChange();
    void fontSelected(const QFont &font);
    void fontSizeSelected(const QString &sizeString);
    void fontZoomChanged();
    void antialiasChanged();
    void colorSchemeSelected(int index);
    void openCopyColorSchemeDialog();
    void copyColorScheme(const QString &name);
    void confirmDeleteColorScheme();
    void deleteColorScheme();

private:
    void maybeSaveColorScheme();
    void updatePointSizes();
    QList<int> pointSizesForSelectedFont() const;
    void refreshColorSchemeList();

    Internal::FontSettingsPagePrivate *d_ptr;
};

} // namespace TextEditor

#endif // FONTSETTINGSPAGE_H
