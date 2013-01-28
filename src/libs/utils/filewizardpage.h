/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef FILEWIZARDPAGE_H
#define FILEWIZARDPAGE_H

#include "utils_global.h"

#include <QWizardPage>

namespace Utils {

struct FileWizardPagePrivate;

class QTCREATOR_UTILS_EXPORT FileWizardPage : public QWizardPage
{
    Q_OBJECT
    Q_PROPERTY(QString path READ path WRITE setPath DESIGNABLE true)
    Q_PROPERTY(QString fileName READ fileName WRITE setFileName DESIGNABLE true)

public:
    explicit FileWizardPage(QWidget *parent = 0);
    virtual ~FileWizardPage();

    QString fileName() const;
    QString path() const;

    virtual bool isComplete() const;

    void setFileNameLabel(const QString &label);
    void setPathLabel(const QString &label);

    bool forceFirstCapitalLetterForFileName() const;
    void setForceFirstCapitalLetterForFileName(bool b);

    // Validate a base name entry field (potentially containing extension)
    static bool validateBaseName(const QString &name, QString *errorMessage = 0);

signals:
    void activated();
    void pathChanged();

public slots:
    void setPath(const QString &path);
    void setFileName(const QString &name);

private slots:
    void slotValidChanged();
    void slotActivated();

protected:
    virtual void changeEvent(QEvent *e);

private:
    FileWizardPagePrivate *d;
};

} // namespace Utils

#endif // FILEWIZARDPAGE_H
