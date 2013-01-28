/**************************************************************************
**
** Copyright (C) 2011 - 2013 Research In Motion
**
** Contact: Research In Motion (blackberry-qt@qnx.com)
** Contact: KDAB (info@kdab.com)
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

#ifndef QNX_INTERNAL_BARDESCRIPTORFILEIMAGEWIZARDPAGE_H
#define QNX_INTERNAL_BARDESCRIPTORFILEIMAGEWIZARDPAGE_H

#include <QWizardPage>

namespace Qnx {
namespace Internal {

namespace Ui {
class BarDescriptorFileImageWizardPage;
}

class BarDescriptorFileImageWizardPage : public QWizardPage
{
    Q_OBJECT

public:
    explicit BarDescriptorFileImageWizardPage(QWidget *parent = 0);
    ~BarDescriptorFileImageWizardPage();

    bool isComplete() const;

    QString icon() const;
    QString landscapeSplashScreen() const;
    QString portraitSplashScreen() const;

private slots:
    void validateIcon(const QString &path);
    void validateLandscapeSplashScreen(const QString &path);
    void validatePortraitSplashScreen(const QString &path);

private:
    enum ImageValidationResult {
        Valid,
        CouldNotLoad,
        IncorrectSize
    };

    void updateSplashScreenValidationLabel();

    ImageValidationResult validateImage(const QString &path, const QSize &minimumSize, const QSize &maximumSize);
    QSize imageSize(const QString &path);

    Ui::BarDescriptorFileImageWizardPage *m_ui;

    ImageValidationResult m_iconValidationResult;
    ImageValidationResult m_landscapeSplashScreenValidationResult;
    ImageValidationResult m_portraitSplashScreenValidationResult;
};


} // namespace Internal
} // namespace Qnx
#endif // QNX_INTERNAL_BARDESCRIPTORFILEIMAGEWIZARDPAGE_H
