*Protect software from erosion with static code analysis, architecture analysis, and code-smells-detection.*

# Prevent code erosion

Connect to an Axivion dashboard server from Qt Creator to view results of code analysis.

> **_Note:_** To use the Axivion extension, you must be connected to an Axivion
dashboard server.

# View inline annotations in editor

The editor shows found issues as inline annotations if the project matches the
currently open one and the respective file is part of the project. Hover the
mouse over an annotation to bring up a tooltip with a short description of the
issue.

![Annotation popup](https://doc.qt.io/qtcreator/images/qtcreator-axivion-annotation.webp)

Select ![](https://doc.qt.io/qtcreator/images/info.png) to view detailed
information about the issue in the `Issue Details` view.

![Axivion sidebar view](https://doc.qt.io/qtcreator/images/qtcreator-axivion-view-rule.webp)

To hide inline annotations, go to `Analyze > Axivion` and clear
![](https://doc.qt.io/qtcreator/images/warning.png).

# View issues

To inspect issues found during the analyses, go to `Analyze > Axivion`.

![Axivion dashboard](https://doc.qt.io/qtcreator/images/qtcreator-axivion-issue-search.webp)

Axivion looks for the following types of issues in the selected project:

| Icon  | Type  | Description  |
|-------|-------|--------------|
| ![AV](https://doc.qt.io/qtcreator/images/axivion-av.png) | AV | Architecture violations, such as hidden dependencies. |
| ![CL](https://doc.qt.io/qtcreator/images/axivion-cl.png) | CL | Clones, such as duplicates and similar pieces of code. |
| ![CY](https://doc.qt.io/qtcreator/images/axivion-cy.png) | CY | Cyclic dependencies, such as call, component, and include cycles. |
| ![DE](https://doc.qt.io/qtcreator/images/axivion-de.png) | DE | Dead entities are callable entities in the source code that cannot be reached from the entry points of the system under analysis. |
| ![MV](https://doc.qt.io/qtcreator/images/axivion-mv.png) | MV | Violations of metrics based on lines and tokens, nesting, cyclomatic complexity, control flow, and so on. |
| ![SV](https://doc.qt.io/qtcreator/images/axivion-sv.png) | SV | Style violations, such as deviations from the naming or coding conventions. |
