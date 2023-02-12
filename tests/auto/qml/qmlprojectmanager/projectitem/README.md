# QmlProject ProjectItem Tests

## Content

This test bundle covers following functionalities of QmlProjectItem class;

* **Getter Functions**: Tests if getter functions are returning correct types with correct values
* **Setter Functions**: Tests if setter functions are updating the internal JSON object as expected
* **Converter Functions:** Tests if QmlProjectToJson and JsonToQmlProject functions are working as expected
* **File Filter Functions:** Tests if file filters are initialized properly

## Data set folder structure

The current folder hierarchy is as following;

```text
| data
| -> converter
|   | -> test-set-1
|   |   | -> testfile.qmlproject
|   |   | -> testfile.qmltojson
|   |   | -> testfile.jsontoqml
|   | -> test-set-2
|   |   | -> testfile.qmlproject
|   |   | -> testfile.qmltojson
|   |   | -> testfile.jsontoqml
|   | -> test-set-..
|   | -> test-set-..
| -> getter-setter
|   | -> testfile-1.qmlproject
|   | -> testfile-2.qmlproject
| -> file-filters
|   | -> test-set-1
|   | -> test-set-...
```

## Further information

Please see [data/README.md](data/README.md) for more information on the test set content.

## Contribution

Please update;

* This README whenever you change the test content
* [data/README.md](data/README.md) whenever you update the `data` folder
