/**************************************************************************
**
** Copyright (C) 2015 Lorenz Haas
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

#ifndef BEAUTIFIER_UNCRUSTIFY_H
#define BEAUTIFIER_UNCRUSTIFY_H

#include "../beautifierabstracttool.h"
#include "../command.h"

QT_FORWARD_DECLARE_CLASS(QAction)

namespace Beautifier {
namespace Internal {

class BeautifierPlugin;

namespace Uncrustify {

class UncrustifySettings;

class Uncrustify : public BeautifierAbstractTool
{
    Q_OBJECT

public:
    explicit Uncrustify(BeautifierPlugin *parent = 0);
    virtual ~Uncrustify();
    bool initialize() override;
    void updateActions(Core::IEditor *editor) override;
    QList<QObject *> autoReleaseObjects() override;

private slots:
    void formatFile();
    void formatSelectedText();

private:
    BeautifierPlugin *m_beautifierPlugin;
    QAction *m_formatFile;
    QAction *m_formatRange;
    UncrustifySettings *m_settings;
    QString configurationFile() const;
    Command command(const QString &cfgFile, bool fragment = false) const;
};

} // namespace Uncrustify
} // namespace Internal
} // namespace Beautifier

#endif // BEAUTIFIER_UNCRUSTIFY_H
