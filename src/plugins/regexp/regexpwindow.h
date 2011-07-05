/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef REGEXPWINDOW_H
#define REGEXPWINDOW_H

#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE
class QLabel;
class QComboBox;
class QCheckBox;
class QLineEdit;
QT_END_NAMESPACE

namespace RegExp {
namespace Internal {

struct Settings;

class RegExpWindow : public QWidget
{
    Q_OBJECT

public:
    RegExpWindow(QWidget *parent = 0);

    Settings settings() const;
    void setSettings(const Settings &s);

protected:
    void contextMenuEvent(QContextMenuEvent *);

private slots:
    void refresh();
    void enterEscaped();

private:
    QLabel *patternLabel;
    QLabel *escapedPatternLabel;
    QLabel *syntaxLabel;
    QLabel *textLabel;
    QComboBox *patternComboBox;
    QLineEdit *escapedPatternLineEdit;
    QComboBox *textComboBox;
    QCheckBox *caseSensitiveCheckBox;
    QCheckBox *minimalCheckBox;
    QComboBox *syntaxComboBox;

    QLabel *indexLabel;
    QLabel *matchedLengthLabel;
    QLineEdit *indexEdit;
    QLineEdit *matchedLengthEdit;

    enum { MaxCaptures = 6 };
    QLabel *captureLabels[MaxCaptures];
    QLineEdit *captureEdits[MaxCaptures];
};

} // namespace Internal
} // namespace RegExp

#endif // REGEXPWINDOW_H
