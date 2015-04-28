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

#ifndef PROJECTINTROPAGE_H
#define PROJECTINTROPAGE_H

#include "utils_global.h"
#include "wizardpage.h"

namespace Utils {

struct ProjectIntroPagePrivate;

class QTCREATOR_UTILS_EXPORT ProjectIntroPage : public WizardPage
{
    Q_OBJECT
    Q_PROPERTY(QString description READ description WRITE setPath DESIGNABLE true)
    Q_PROPERTY(QString path READ path WRITE setPath DESIGNABLE true)
    Q_PROPERTY(QString projectName READ projectName WRITE setProjectName DESIGNABLE true)
    Q_PROPERTY(bool useAsDefaultPath READ useAsDefaultPath WRITE setUseAsDefaultPath DESIGNABLE true)
    Q_PROPERTY(bool forceSubProject READ forceSubProject WRITE setForceSubProject DESIGNABLE true)

public:
    explicit ProjectIntroPage(QWidget *parent = 0);
    virtual ~ProjectIntroPage();

    QString projectName() const;
    QString path() const;
    QString description() const;
    bool useAsDefaultPath() const;

    // Insert an additional control into the form layout for the target.
    void insertControl(int row, QWidget *label, QWidget *control);

    virtual bool isComplete() const;

    bool forceSubProject() const;
    void setForceSubProject(bool force);
    void setProjectList(const QStringList &projectList);
    void setProjectDirectories(const QStringList &directoryList);
    int projectIndex() const;

    static bool validateProjectName(const QString &name, QString *errorMessage /* = 0*/);

signals:
    void activated();

public slots:
    void setPath(const QString &path);
    void setProjectName(const QString &name);
    void setDescription(const QString &description);
    void setUseAsDefaultPath(bool u);

private slots:
    void slotChanged();
    void slotActivated();

private:
    enum StatusLabelMode { Error, Warning, Hint };

    bool validate();
    void displayStatusMessage(StatusLabelMode m, const QString &);
    void hideStatusLabel();

    ProjectIntroPagePrivate *d;
};

} // namespace Utils

#endif // PROJECTINTROPAGE_H
