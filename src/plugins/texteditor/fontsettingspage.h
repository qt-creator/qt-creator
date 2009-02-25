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

#ifndef FONTSETTINGSPAGE_H
#define FONTSETTINGSPAGE_H

#include "texteditor_global.h"

#include "fontsettings.h"

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
