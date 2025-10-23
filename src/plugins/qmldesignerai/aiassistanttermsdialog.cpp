// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "aiassistanttermsdialog.h"

#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>

namespace QmlDesigner {

AiAssistantTermsDialog::AiAssistantTermsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("AI Assistant Terms and Conditions"));
    resize(500, 400);

    setLayout(new QVBoxLayout(this));

    auto *label = new QLabel(tr("Please read and accept the Terms and Conditions:"), this);
    layout()->addWidget(label);

    m_termsText = new QTextEdit(this);
    m_termsText->setReadOnly(true);
    m_termsText->setHtml(R"(<h2 style="text-align:center;">Appendix for Qt AI Services</h2>
<p style="text-align:center;"><strong>Version 2025-06</strong><br>
Compliant with Qt License Agreement 4, Frame Agreement 2023-06, or later</p>

<h3>1. Introduction</h3>
<p>
These Terms of Service (“Terms”) govern the access and use of <strong>Qt Design Studio AI Assistant</strong> (as defined below).
These Terms supplement the Agreement between Customer (“Customer”) and <strong>The Qt Company</strong>
to provide terms specific to the use of Qt AI. By using Qt AI, Customer agrees to be bound by these Terms.
If Customer does not wish to accept these Terms, Customer should not use Qt AI.
</p>

<p><strong>1.1.</strong> These Terms govern the use of AI services provided by The Qt Company designed to enhance,
improve, or automate Customer’s experience with the Services (including Licensed Software) through AI offerings.
This may include machine learning, natural language processing, computer vision, data analytics,
and integration or use of third-party large language models.</p>

<p><strong>1.2.</strong> Qt AI may provide functionality that interfaces with third-party AI offerings (e.g., LLMs)
to obtain output or suggestions for Customer. Customer is solely responsible for selecting the information to be shared with Qt AI
and ensuring Customer’s ability to share any information with The Qt Company, including Qt AI.</p>

<h3>2. Definitions</h3>
<p><strong>2.1.</strong> Any capitalized terms not defined in this Section 2 have the meaning set forth in the Agreement.</p>
<p><strong>2.2.</strong> “Agreement” means a valid legal document, including its appendices, signed by both parties,
based on which the Services by The Qt Company are provided, and which specifies further rights and obligations between the parties.</p>
<p><strong>2.3.</strong> “AI Assistant” means a software development or test development assistant application as defined in 2.7 and 2.8.</p>
<p><strong>2.4.</strong> “Inputs” means prompts, documentation, code, images, or any other materials Customer provides for use with Qt AI.</p>
<p><strong>2.5.</strong> “Model” means tuning, models, software, documentation, or similar offerings that may be utilized in conjunction with Qt AI.
Any Models are provided separately from Qt AI and are subject to separate applicable terms and conditions.
Customer’s use of Models is at Customer’s own risk.</p>
<p><strong>2.6.</strong> “Outputs” means code, text, images, audio, suggestions, or other content generated via use of Qt AI,
but does not include the intellectual property of The Qt Company (e.g., Licensed Software or Qt Community Edition).</p>
<p><strong>2.7.</strong> “Qt AI” means services offered by The Qt Company that integrate or otherwise use artificial intelligence,
operated and/or developed either by The Qt Company or by third parties.</p>
<p><strong>2.8.</strong> “Qt AI Assistant” is an optional plug-in extension to the Qt Creator IDE that connects to third-party
Large Language Models (“LLMs”) to generate or explain code, write test cases, and provide other developer assistance.</p>
<p><strong>2.9.</strong> “Squish AI Assistant” is an optional plug-in extension to the Squish IDE that connects to third-party LLMs
to suggest improvements, maintain test cases, and explain code or errors.</p>
<p><strong>2.10.</strong> “Third-Party Software” is as defined in the Agreement and includes any third-party LLM(s)
used in connection with Qt AI, based on the selection of The Qt Company or the Customer.</p>

<h3>3. Use of Qt AI</h3>
<p><strong>3.1.</strong> To use Qt AI, Customer must have an Agreement and a valid active subscription license for applicable Licensed Software.
User restrictions apply as per the Agreement.</p>
<p><strong>3.2.</strong> Use of AI Assistant further requires that Customer connect to at least one LLM.
Such LLM may be operated by Customer or a third party.
Customer is responsible for ensuring compliance with the terms of any such LLM.</p>
<p><strong>3.3.</strong> The accuracy and suitability of Outputs must be confirmed by the Customer.
The Qt Company disclaims all warranties regarding the correctness or fitness of Outputs.</p>

<h3>4. Prohibited Use</h3>
<p>Customer must not use Qt AI in any way that violates applicable laws
or infringes any intellectual property rights of third parties or The Qt Company.</p>

<h3>5. Limitations</h3>
<p>Customer may not use an LLM that claims ownership of or licenses The Qt Company’s intellectual property.
Use of any LLM or other Third-Party Software must comply with its own license terms.</p>

<h3>6. Intellectual Property Rights and Ownership</h3>
<p><strong>6.1. Ownership of The Qt Company.</strong> The Qt Company owns all proprietary and intellectual property rights
related to Qt AI, including system-generated data. Use of Qt AI transfers no ownership to the Customer.</p>

<p><strong>6.2. Ownership of Customer.</strong> Customer retains all rights to its Inputs and data.</p>
<div style="margin-left:1em;">
  <p><strong>6.2.1.</strong> Customer owns its Inputs and Outputs, except where explicitly set forth otherwise.</p>
  <p><strong>6.2.2.</strong> By submitting Inputs, Customer warrants that it has the right to do so.</p>
  <p><strong>6.2.3.</strong> Outputs generated for Customer are considered Customer’s property,
  but may contain Qt Company intellectual property or third-party components subject to separate licenses.</p>
</div>

<p><strong>6.3. Feedback.</strong> Customer grants The Qt Company perpetual rights to use any feedback or suggestions.</p>
<p><strong>6.4. Access to Data.</strong> The Qt Company may access Inputs and Outputs solely to provide Qt AI and Support,
and will not use Customer’s data to train models without written consent.</p>

<h3>7. Indemnification</h3>
<p><strong>7.1.</strong> The Qt Company will indemnify Customer for third-party claims alleging that Qt AI
infringes intellectual property rights (excluding any LLM or Third-Party Software).</p>
<p><strong>7.2.</strong> Customer shall indemnify The Qt Company against claims arising from misuse of Qt AI,
violation of third-party rights, or breaches of these Terms.</p>
<p><strong>7.3–7.6.</strong> Additional provisions detail notification, defense, and limitation of indemnity obligations.</p>

<h3>8. Risks and Disclaimers</h3>
<p><strong>8.1.</strong> Qt AI and related support are provided “as is.”</p>
<p><strong>8.2.</strong> The Qt Company makes no warranties, express or implied, regarding uninterrupted use, accuracy,
or freedom from harmful components.</p>
<p><strong>8.3.</strong> Outputs should not be relied upon without independent verification.</p>
<p><strong>8.4.</strong> These disclaimers do not apply where prohibited by law.</p>

<h3>9. Limitation of Liability</h3>
<p><strong>9.1.</strong> Except in cases of gross negligence, intentional misconduct, or IP violations,
neither party is liable for indirect, consequential, or punitive damages.</p>
<p><strong>9.2.</strong> Aggregate liability is limited to the total fees paid under the Agreement,
except where otherwise required by law.</p>

<h3>10. Miscellaneous</h3>
<p>These Terms are an integral and inseparable part of the Agreement.
Except as modified herein, all provisions of the Agreement remain applicable.</p>)");

    layout()->addWidget(m_termsText);

    m_agreeCheckbox = new QCheckBox(tr("I have read and agree to the Terms and Conditions"), this);
    layout()->addWidget(m_agreeCheckbox);
    connect(m_agreeCheckbox, &QCheckBox::stateChanged, this, [this](int state) {
        m_acceptButton->setEnabled(state == Qt::Checked);
    });

    auto *buttonLayout = new QHBoxLayout(this);
    m_acceptButton = new QPushButton(tr("Accept"), this);
    m_rejectButton = new QPushButton(tr("Reject"), this);

    m_acceptButton->setEnabled(false);

    connect(m_acceptButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(m_rejectButton, &QPushButton::clicked, this, &QDialog::reject);

    buttonLayout->addStretch();
    buttonLayout->addWidget(m_acceptButton);
    buttonLayout->addWidget(m_rejectButton);
    static_cast<QVBoxLayout *>(layout())->addLayout(buttonLayout);
}

} // namespace QmlDesigner
