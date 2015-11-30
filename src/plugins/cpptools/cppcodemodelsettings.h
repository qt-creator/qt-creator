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

#ifndef CPPTOOLS_CPPCODEMODELSETTINGS_H
#define CPPTOOLS_CPPCODEMODELSETTINGS_H

#include "cpptools_global.h"

#include <QObject>
#include <QStringList>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace CppTools {

class CPPTOOLS_EXPORT CppCodeModelSettings : public QObject
{
    Q_OBJECT

public:
    enum PCHUsage {
        PchUse_None = 1,
        PchUse_BuildSystem = 2
    };

public:
    void fromSettings(QSettings *s);
    void toSettings(QSettings *s);

public:
    bool useClangCodeModel() const;
    void setUseClangCodeModel(bool useClangCodeModel);

    static QStringList defaultExtraClangOptions();
    QStringList extraClangOptions() const;
    void setExtraClangOptions(const QStringList &extraClangOptions);

    PCHUsage pchUsage() const;
    void setPCHUsage(PCHUsage pchUsage);

public: // for tests
    void emitChanged();

signals:
    void changed();

private:
    bool m_isClangCodeModelAvailable = false;
    bool m_useClangCodeModel = false;
    QStringList m_extraClangOptions;
    PCHUsage m_pchUsage = PchUse_None;
};

} // namespace CppTools

#endif // CPPTOOLS_CPPCODEMODELSETTINGS_H
