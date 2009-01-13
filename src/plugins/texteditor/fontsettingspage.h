/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#ifndef FONTSETTINGSPAGE_H
#define FONTSETTINGSPAGE_H

#include "texteditor_global.h"

#include "fontsettings.h"

#include <coreplugin/icore.h>
#include <coreplugin/dialogs/ioptionspage.h>

#include <QtGui/QColor>
#include <QtGui/QTextCharFormat>
#include <QtCore/QString>
#include <QtCore/QVector>

QT_BEGIN_NAMESPACE
class QWidget;
QT_END_NAMESPACE

namespace TextEditor {

namespace Internal {
class FontSettingsPagePrivate;
} // namespace Internal

// GUI description of a format consisting of name (settings key)
// and trName to be displayed
class TEXTEDITOR_EXPORT FormatDescription
{
public:
    FormatDescription(const QString &name, const QString &trName,
                      const QColor &foreground = Qt::black);

    QString name() const;

    QString trName() const;

    QColor foreground() const;
    void setForeground(const QColor &foreground);

    QColor background() const;

private:
    QString m_name;             // Name of the category
    QString m_trName;           // Displayed name of the category
    Format m_format;            // Default format
};

typedef QList<FormatDescription> FormatDescriptions;

class TEXTEDITOR_EXPORT FontSettingsPage : public Core::IOptionsPage
{
    Q_OBJECT

public:
    FontSettingsPage(const FormatDescriptions &fd,
                     const QString &category,
                     const QString &trCategory,
                     Core::ICore *core,
                     QObject *parent = 0);

    ~FontSettingsPage();

    QString name() const;
    QString category() const;
    QString trCategory() const;

    QWidget *createPage(QWidget *parent);
    void apply();
    void finish();

    const FontSettings &fontSettings() const;

signals:
    void changed(const TextEditor::FontSettings&);

private slots:
    void delayedChange();
    void itemChanged();
    void changeForeColor();
    void changeBackColor();
    void eraseBackColor();
    void checkCheckBoxes();
    void updatePointSizes();
    void updatePreview();

private:
    Internal::FontSettingsPagePrivate *d_ptr;
};

} // namespace TextEditor

#endif // FONTSETTINGSPAGE_H
