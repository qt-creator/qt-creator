/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
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
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/
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
    explicit PathChooser(QWidget *parent = 0);
    virtual ~PathChooser();

    bool isValid() const;
    QString errorMessage() const;

    QString path() const;

    // Returns the suggested label title when used in a form layout
    static QString label();

    static bool validatePath(const QString &path, QString *errorMessage = 0);

    // Return the home directory, which needs some fixing under Windows.
    static QString homePath();

signals:
    void validChanged();
    void changed();
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
