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

#include <QWidget>

QT_BEGIN_NAMESPACE
class QStringList;
QT_END_NAMESPACE

namespace Designer {
namespace Internal {

struct NewClassWidgetPrivate;

class NewClassWidget : public QWidget
{
    Q_OBJECT
    Q_ENUMS(ClassType)

public:
    enum ClassType { NoClassType,
                     ClassInheritsQObject,
                     ClassInheritsQWidget,
                     ClassInheritsQDeclarativeItem,
                     ClassInheritsQQuickItem,
                     SharedDataClass
                   };
    explicit NewClassWidget(QWidget *parent = nullptr);
    ~NewClassWidget() override;

    QString className() const;
    QString baseClassName() const;
    QString sourceFileName() const;
    QString headerFileName() const;
    QString formFileName() const;
    QString path() const;
    QString sourceExtension() const;
    QString headerExtension() const;
    QString formExtension() const;

    bool isValid(QString *error = nullptr) const;

    QStringList files() const;

signals:
    void validChanged();
    void activated();

public slots:

    /**
     * The name passed into the new class widget will be reformatted to be a
     * valid class name.
     */
    void setClassName(const QString &suggestedName);
    void setPath(const QString &path);
    void setSourceExtension(const QString &e);
    void setHeaderExtension(const QString &e);
    void setLowerCaseFiles(bool v);
    void setClassType(ClassType ct);
    void setNamesDelimiter(const QString &delimiter);

    /**
     * Suggest a class name from the base class by stripping the leading 'Q'
     * character. This will happen automagically if the base class combo
     * changes until the class line edited is manually edited.
     */
    void suggestClassNameFromBase();

private:
    void slotUpdateFileNames(const QString &t);
    void slotValidChanged();
    void slotActivated();
    void classNameEdited();

    QString fixSuffix(const QString &suffix);
    NewClassWidgetPrivate *d;
};

} // namespace Internal
} // namespace Designer
