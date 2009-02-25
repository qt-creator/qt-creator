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

#ifndef PATHCHOOSER_H
#define PATHCHOOSER_H

#include "utils_global.h"

#include <QtGui/QWidget>

namespace Core {
namespace Utils {

struct PathChooserPrivate;

/* A Control that let's the user choose a path, consisting of a QLineEdit and
 * a "Browse" button. Has some validation logic for embedding into
 * QWizardPage. */

class QWORKBENCH_UTILS_EXPORT PathChooser : public QWidget
{
    Q_DISABLE_COPY(PathChooser)
    Q_OBJECT
    Q_PROPERTY(QString path READ path WRITE setPath DESIGNABLE true)

public:
    static const char * const browseButtonLabel;

    explicit PathChooser(QWidget *parent = 0);
    virtual ~PathChooser();

    enum Kind {
        Directory,
        File,
        Command,
        // ,Any
    };

    // Default is <Directory>
    void setExpectedKind(Kind expected);

    void setPromptDialogTitle(const QString &title);

    void setInitialBrowsePathBackup(const QString &path);

    bool isValid() const;
    QString errorMessage() const;

    QString path() const;

    // Returns the suggested label title when used in a form layout
    static QString label();

    bool validatePath(const QString &path, QString *errorMessage = 0);

    // Return the home directory, which needs some fixing under Windows.
    static QString homePath();

private:
    // Returns overridden title or the one from <title>
    QString makeDialogTitle(const QString &title);

signals:
    void validChanged();
    void changed();
    void beforeBrowsing();
    void browsingFinished();
    void returnPressed();

public slots:
    void setPath(const QString &);

private slots:
    void slotBrowse();

private:
    PathChooserPrivate *m_d;
};

} // namespace Utils
} // namespace Core

#endif // PATHCHOOSER_H
