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

#ifndef FILESPAGE_H
#define FILESPAGE_H

#include <QWizard>

QT_BEGIN_NAMESPACE
class QLabel;
QT_END_NAMESPACE

namespace Utils { class NewClassWidget; }

namespace QmakeProjectManager {
namespace Internal {

class FilesPage : public QWizardPage
{
    Q_OBJECT

public:
    explicit FilesPage(QWidget *parent = 0);
    virtual bool isComplete() const;

    QString className() const;
    void setClassName(const QString &suggestedClassName);

    QString baseClassName() const;
    QString sourceFileName() const;
    QString headerFileName() const;
    QString formFileName() const;

    // API of the embedded NewClassWidget
    bool namespacesEnabled() const;
    bool isBaseClassInputVisible() const;
    bool isFormInputVisible() const;
    bool formInputCheckable() const;
    bool formInputChecked() const;
    QStringList baseClassChoices() const;
    bool lowerCaseFiles() const;
    bool isClassTypeComboVisible() const;

    void setSuffixes(const QString &header, const QString &source,  const QString &form = QString());

public slots:
    void setBaseClassName(const QString &);
    void setNamespacesEnabled(bool b);
    void setBaseClassInputVisible(bool visible);
    void setBaseClassChoices(const QStringList &choices);
    void setFormFileInputVisible(bool visible);
    void setFormInputCheckable(bool checkable);
    void setFormInputChecked(bool checked);
    void setLowerCaseFiles(bool l);
    void setClassTypeComboVisible(bool v);

private:
    Utils::NewClassWidget *m_newClassWidget;
    QLabel *m_errorLabel;
};

} // namespace Internal
} // namespace QmakeProjectManager

#endif // FILESPAGE_H
