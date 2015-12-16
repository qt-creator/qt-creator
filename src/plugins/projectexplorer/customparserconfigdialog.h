/****************************************************************************
**
** Copyright (C) 2015 Andre Hartmann.
** Contact: aha_1980@gmx.de
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

#ifndef CUSTOMPARSERCONFIGDIALOG_H
#define CUSTOMPARSERCONFIGDIALOG_H

#include "customparser.h"

#include <QDialog>

QT_BEGIN_NAMESPACE
class QLineEdit;
QT_END_NAMESPACE

namespace ProjectExplorer {
namespace Internal {

namespace Ui { class CustomParserConfigDialog; }

class CustomParserConfigDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CustomParserConfigDialog(QDialog *parent = 0);
    ~CustomParserConfigDialog();

    void setExampleSettings();
    void setSettings(const CustomParserSettings &settings);
    CustomParserSettings settings() const;
    void setErrorPattern(const QString &errorPattern);
    QString errorPattern() const;
    void setErrorFileNameCap(int errorFileNameCap);
    int errorFileNameCap() const;
    void setErrorLineNumberCap(int errorLineNumberCap);
    int errorLineNumberCap() const;
    void setErrorMessageCap(int errorMessageCap);
    int errorMessageCap() const;
    void setErrorChannel(CustomParserExpression::CustomParserChannel errorChannel);
    CustomParserExpression::CustomParserChannel errorChannel() const;
    void setErrorExample(const QString &errorExample);
    QString errorExample() const;
    void setWarningPattern(const QString &warningPattern);
    QString warningPattern() const;
    void setWarningFileNameCap(int warningFileNameCap);
    int warningFileNameCap() const;
    void setWarningLineNumberCap(int warningLineNumberCap);
    int warningLineNumberCap() const;
    void setWarningMessageCap(int warningMessageCap);
    int warningMessageCap() const;
    void setWarningChannel(CustomParserExpression::CustomParserChannel warningChannel);
    CustomParserExpression::CustomParserChannel warningChannel() const;
    void setWarningExample(const QString &warningExample);
    QString warningExample() const;

    bool isDirty() const;

private slots:
    void changed();

private:
    bool checkPattern(QLineEdit *pattern, const QString &outputText,
                      QString *errorMessage, QRegularExpressionMatch *match);

    Ui::CustomParserConfigDialog *ui;
    bool m_dirty;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // CUSTOMPARSERCONFIGDIALOG_H
