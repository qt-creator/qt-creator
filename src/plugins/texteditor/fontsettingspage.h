/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef FONTSETTINGSPAGE_H
#define FONTSETTINGSPAGE_H

#include "texteditor_global.h"

#include "fontsettings.h"

#include "texteditoroptionspage.h"

#include <QList>
#include <QString>

QT_BEGIN_NAMESPACE
class QWidget;
class QColor;
QT_END_NAMESPACE

namespace TextEditor {

namespace Internal {
class FontSettingsPagePrivate;
} // namespace Internal

// GUI description of a format consisting of id (settings key)
// and displayName to be displayed
class TEXTEDITOR_EXPORT FormatDescription
{
public:
    FormatDescription(TextStyle id, const QString &displayName,
                      const QColor &foreground = Qt::black);
    FormatDescription(TextStyle id, const QString &displayName,
                      const Format &format);

    TextStyle id() const { return m_id; }

    QString displayName() const
    { return m_displayName; }

    QColor foreground() const;
    QColor background() const;

    const Format &format() const { return m_format; }
    Format &format() { return m_format; }

private:
    TextStyle m_id;            // Name of the category
    QString m_displayName;      // Displayed name of the category
    Format m_format;            // Default format
};

typedef QList<FormatDescription> FormatDescriptions;

class TEXTEDITOR_EXPORT FontSettingsPage : public TextEditorOptionsPage
{
    Q_OBJECT

public:
    FontSettingsPage(const FormatDescriptions &fd,
                     const QString &id,
                     QObject *parent = 0);

    ~FontSettingsPage();

    QWidget *createPage(QWidget *parent);
    void apply();
    void finish();
    bool matches(const QString &) const;

    void saveSettings();

    const FontSettings &fontSettings() const;

signals:
    void changed(const TextEditor::FontSettings&);

private slots:
    void delayedChange();
    void fontFamilySelected(const QString &family);
    void fontSizeSelected(const QString &sizeString);
    void fontZoomChanged();
    void colorSchemeSelected(int index);
    void copyColorScheme();
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
