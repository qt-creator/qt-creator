# Qt Creator

Qt Creator is a cross-platform, integrated development environment (IDE) for
application developers to create applications for multiple desktop, embedded,
and mobile device platforms.

* [Qt Creator Documentation](https://doc.qt.io/qtcreator/index.html)
* [Overview of the Qt Creator IDE](https://doc.qt.io/qtcreator/creator-overview.html)
* [Extending Qt Creator Manual](https://doc.qt.io/qtcreator-extending/)
* [Building Qt Creator from source](BUILDING.md)
* [Contributing](CONTRIBUTING.md)

## Supported Platforms

The standalone binary packages support the following platforms:

| Platform        | Architecture    | Minimum version    |
| --------------- | --------------- | ------------------ |
| Windows         | x86_64          | Windows 10         |
| Windows         | ARM64           | Windows 11         |
| (K)Ubuntu Linux | x86_64          | 22.04 (glibc 2.34) |
| (K)Ubuntu Linux | arm64           | 24.04 (glibc 2.39) |
| macOS           | x86_64, arm64   | 14.4               |

When you compile Qt Creator yourself, the Qt version that you build with
determines the supported platforms.

## Getting Started

Install Qt Creator with the Qt Online Installer or a standalone binary package,
and see [Getting Started](https://doc.qt.io/qtcreator/creator-getting-started.html)
in the documentation to learn how to use it.

To build Qt Creator yourself, see [BUILDING.md](BUILDING.md).

## Contributing

Qt Creator is developed using the Gerrit code review tool. See
[CONTRIBUTING.md](CONTRIBUTING.md) for how to submit patches, and
[TESTING.md](TESTING.md) for how to run the tests.

## Licenses and Attributions

Qt Creator is available under commercial licenses from The Qt Company,
and under the GNU General Public License version 3,
annotated with The Qt Company GPL Exception 1.0.
See [LICENSE.GPL3-EXCEPT](LICENSES/LICENSE.GPL3-EXCEPT) for the details.

For more information about the third-party components that Qt Creator
includes, see the
[Acknowledgements section in the documentation](https://doc.qt.io/qtcreator/creator-acknowledgements.html).
