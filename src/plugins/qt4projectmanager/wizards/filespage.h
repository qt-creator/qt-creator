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

#ifndef FILESPAGE_H
#define FILESPAGE_H

#include <QtGui/QWizard>

QT_BEGIN_NAMESPACE
class QLabel;
QT_END_NAMESPACE

namespace Core {
namespace Utils {
class NewClassWidget;
} // namespace Utils
} // namespace Core

namespace Qt4ProjectManager {
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

    void setSuffixes(const QString &header, const QString &source,  const QString &form = QString());

public slots:
    void setBaseClassName(const QString &);
    void setNamespacesEnabled(bool b);
    void setBaseClassInputVisible(bool visible);
    void setBaseClassChoices(const QStringList &choices);
    void setFormFileInputVisible(bool visible);
    void setFormInputCheckable(bool checkable);
    void setFormInputChecked(bool checked);


private:
    Core::Utils::NewClassWidget *m_newClassWidget;
    QLabel *m_errorLabel;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // FILESPAGE_H
