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

#ifndef PROJECTINTROPAGE_H
#define PROJECTINTROPAGE_H

#include "utils_global.h"

#include <QtGui/QWizardPage>

namespace Core {
namespace Utils {

struct ProjectIntroPagePrivate;

/* Standard wizard page for a single file letting the user choose name
 * and path. Looks similar to FileWizardPage, but provides additional
 * functionality:
 * - Description label at the top for displaying introductory text
 * - It does on the fly validation (connected to changed()) and displays
 *   warnings/errors in a status label at the bottom (the page is complete
 *   when fully validated, validatePage() is thus not implemented).
 *
 * Note: Careful when changing projectintropage.ui. It must have main
 * geometry cleared and QLayout::SetMinimumSize constraint on the main
 * layout, otherwise, QWizard will squeeze it due to its strange expanding
 * hacks. */

class QWORKBENCH_UTILS_EXPORT ProjectIntroPage : public QWizardPage
{
    Q_OBJECT
    Q_DISABLE_COPY(ProjectIntroPage)
    Q_PROPERTY(QString description READ description WRITE setPath DESIGNABLE true)
    Q_PROPERTY(QString path READ path WRITE setPath DESIGNABLE true)
    Q_PROPERTY(QString name READ  name WRITE setName DESIGNABLE true)
public:
    explicit ProjectIntroPage(QWidget *parent = 0);
    virtual ~ProjectIntroPage();

    QString name() const;
    QString path() const;
    QString description() const;

    // Insert an additional control into the form layout for the target.
    void insertControl(int row, QWidget *label, QWidget *control);

    virtual bool isComplete() const;

    // Validate a project directory name entry field
    static bool validateProjectDirectory(const QString &name, QString *errorMessage);

signals:
    void activated();

public slots:
    void setPath(const QString &path);
    void setName(const QString &name);
    void setDescription(const QString &description);

private slots:
    void slotChanged();
    void slotActivated();

protected:
    virtual void changeEvent(QEvent *e);

private:
    enum StatusLabelMode { Error, Warning, Hint };

    bool validate();
    void displayStatusMessage(StatusLabelMode m, const QString &);
    void hideStatusLabel();

    ProjectIntroPagePrivate *m_d;
};

} // namespace Utils
} // namespace Core

#endif // PROJECTINTROPAGE_H
