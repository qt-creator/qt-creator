/**************************************************************************
**
** Copyright (c) 2014 Hugues Delorme
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
#ifndef CLONEOPTIONSPANEL_H
#define CLONEOPTIONSPANEL_H

#include <QWidget>

namespace Bazaar {
namespace Internal {

namespace Ui { class CloneOptionsPanel; }

class CloneOptionsPanel : public QWidget
{
    Q_OBJECT

public:
    CloneOptionsPanel(QWidget *parent = 0);
    ~CloneOptionsPanel();

    bool isUseExistingDirectoryOptionEnabled() const;
    bool isStackedOptionEnabled() const;
    bool isStandAloneOptionEnabled() const;
    bool isBindOptionEnabled() const;
    bool isSwitchOptionEnabled() const;
    bool isHardLinkOptionEnabled() const;
    bool isNoTreeOptionEnabled() const;
    QString revision() const;

private:
    Ui::CloneOptionsPanel *m_ui;
};

} // namespace Internal
} // namespace Bazaar

#endif // CLONEOPTIONSPANEL_H
