/****************************************************************************
**
** Copyright (C) 2014 Denis Shienkov <denis.shienkov@gmail.com>
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
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
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef GDBSERVERPROVIDERCHOOSER_H
#define GDBSERVERPROVIDERCHOOSER_H

#include <QPointer>

#include <QWidget>

QT_BEGIN_NAMESPACE
class QComboBox;
class QPushButton;
QT_END_NAMESPACE

namespace Core { class Id; }

namespace BareMetal {
namespace Internal {

class GdbServerProvider;

class GdbServerProviderChooser : public QWidget
{
    Q_OBJECT

public:
    explicit GdbServerProviderChooser(
            bool useManageButton = true, QWidget *parent = 0);

    QString currentProviderId() const;
    void setCurrentProviderId(const QString &id);

signals:
    void providerChanged();

public slots:
    void populate();

private slots:
    void currentIndexChanged(int index);
    void manageButtonClicked();

protected:
    bool providerMatches(const GdbServerProvider *) const;
    QString providerText(const GdbServerProvider *) const;

private:
    QPointer<QComboBox> m_chooser;
    QPointer<QPushButton> m_manageButton;
};

} // namespace Internal
} // namespace BareMetal

#endif // GDBSERVERPROVIDERCHOOSER_H
