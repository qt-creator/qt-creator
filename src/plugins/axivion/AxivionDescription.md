*Protect software from erosion with static code analysis, architecture analysis, and code-smells-detection.*

# Prevent code erosion

Connect to an Axivion dashboard server from Qt Creator to view results of code analysis.

> **_NOTE:_** Enable the Axivion plugin to use it.

To use the plugin, you must set up a project in the Axivion dashboard server and link to it from Qt Creator. You can then see found issues in the Edit mode, issues in the Axivion dashboard, and issue details in the Axivion sidebar view.

The editor shows found issues as inline annotations. Hover the mouse over an annotation to bring up a tool tip with a short description of the issue.

![Annotation popup](https://doc.qt.io/qtcreator/images/qtcreator-axivion-annotation.webp)

Select ![](https://doc.qt.io/qtcreator/images/info.png) to view detailed information about the issue in the Axivion sidebar view.

![Axivion sidebar view](https://doc.qt.io/qtcreator/images/qtcreator-axivion-view-rule.webp)

# View issue counts

To view the issue counts, select  (Show Dashboard) in the Axivion dashboard.

![Axivion dashboard](https://doc.qt.io/qtcreator/images/qtcreator-axivion-view.webp)

The Axivion dashboard lists the numbers of the following types of issues that Axivion found in the linked project:


| Icon  | Type  | Description  |
|-------|-------|--------------|
| ![AV](https://doc.qt.io/qtcreator/images/axivion-av.png) | AV | Architecture violations, such as hidden dependencies. |
| ![CL](https://doc.qt.io/qtcreator/images/axivion-cl.png) | CL | Clones, such as duplicates and similar pieces of code. |
| ![CY](https://doc.qt.io/qtcreator/images/axivion-cy.png) | CY | Cyclic dependencies, such as call, component, and include cycles. |
| ![DE](https://doc.qt.io/qtcreator/images/axivion-de.png) | DE | Dead entities are callable entities in the source code that cannot be reached from the entry points of the system under analysis. |
| ![MV](https://doc.qt.io/qtcreator/images/axivion-mv.png) | MV | Violations of metrics based on lines and tokens, nesting, cyclomatic complexity, control flow, and so on. |
| ![SV](https://doc.qt.io/qtcreator/images/axivion-sv.png) | SV | Style violations, such as deviations from the naming or coding conventions. |

To clear the view, select ![clear](https://doc.qt.io/qtcreator/images/clean_pane_small.png) (Clear).

To view issues, select ![seach](https://doc.qt.io/qtcreator/images/zoom.png) (Search for Issues).

# Filter issues

![Issues in Axivion view](https://doc.qt.io/qtcreator/images/qtcreator-axivion-issue-search.webp)

To filter issues, select:

 - The icon of an issue type.
 - Two analyzed versions to compare. Select EMPTY to see issues from the version you select in the right-side version box.
 -  to see only added issues.
 -  to see only removed issues.
 - The owner of the issue. Select ANYBODY to see all issues, NOBODY to see issues that are not associated with a user, or a user name to see issues owned by a particular user.
 - Path patterns to show issues in the files in the directories that match the pattern.
The information you see depends on the issue type. Double-click an issue to see more information about it in the Axivion sidebar view.

# Jump to issues in the editor

Typically, the details for cycles and clones show several paths. To view the issues in the editor:

Click in a location column (that shows a file or line) to open the respective location (if it can be found).
Click in other columns to open the first link in the issue details. Usually, it leads to the Left location or Source location.
The easiest way to jump to the Right location is to select the link in the details or in the Right Path or Target Path column.
