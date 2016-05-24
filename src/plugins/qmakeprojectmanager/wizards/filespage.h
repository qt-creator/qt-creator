/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

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
