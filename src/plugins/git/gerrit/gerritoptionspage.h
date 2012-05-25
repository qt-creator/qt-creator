/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef GERRIT_INTERNAL_GERRITOPTIONSPAGE_H
#define GERRIT_INTERNAL_GERRITOPTIONSPAGE_H

#include <vcsbase/vcsbaseoptionspage.h>

#include <QWidget>
#include <QSharedPointer>
#include <QWeakPointer>

QT_BEGIN_NAMESPACE
class QLineEdit;
class QSpinBox;
class QCheckBox;
QT_END_NAMESPACE

namespace Utils {
class PathChooser;
}
namespace Gerrit {
namespace Internal {

class GerritParameters;

class GerritOptionsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit GerritOptionsWidget(QWidget *parent = 0);

    GerritParameters parameters() const;
    void setParameters(const GerritParameters &);

private:
    QLineEdit *m_hostLineEdit;
    QLineEdit *m_userLineEdit;
    Utils::PathChooser *m_sshChooser;
    QSpinBox *m_portSpinBox;
    QLineEdit *m_additionalQueriesLineEdit;
    QCheckBox *m_httpsCheckBox;
};

class GerritOptionsPage : public VcsBase::VcsBaseOptionsPage
{
    Q_OBJECT

public:
    GerritOptionsPage(const QSharedPointer<GerritParameters> &p,
                      QObject *parent = 0);

    static QString optionsId();

    QWidget *createPage(QWidget *parent);
    void apply();
    void finish() { }
    bool matches(const QString &) const;

private:
    const QSharedPointer<GerritParameters> &m_parameters;
    QWeakPointer<GerritOptionsWidget> m_widget;
};

} // namespace Internal
} // namespace Gerrit

#endif // GERRIT_INTERNAL_GERRITOPTIONSPAGE_H
