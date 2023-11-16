# Qt QR Code Generator Library

Qt QR Code Generator is a simple C++ class that uses the [qrcodegen](https://github.com/nayuki/QR-Code-generator) library to generate QR codes from QStrings in Qt applications.

[![Screenshot](example/screenshot.png)](example/screenshot.png)

## Usage

1. Copy the *Qt-QrCodeGenerator* folder in your `lib` folder.
2. Include the *Qt-QrCodeGenerator* project include (pri) file using the `include()` qmake function.
3. Use the `QrCodeGenerator` class in your code:

```cpp
#include <QrCodeGenerator>

QrCodeGenerator generator;
QString data = "https://www.example.com";
QImage qrCodeImage = generator.generateQr(data);
```

4. That's all! Check the [example](example) project as a reference for your project if needed.

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.
