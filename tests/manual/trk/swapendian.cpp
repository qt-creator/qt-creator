

#include <QtCore>

int main(int argc, char *argv[])
{
    QFile file1(argv[1]);
    file1.open(QIODevice::ReadOnly);

    QByteArray ba = file1.readAll();
    file1.close();

    for (int i = 0; i < ba.size(); i += 4) {
        char c = ba[i]; ba[i] = ba[i + 3]; ba[i + 3] = c;
        c = ba[i + 1]; ba[i + 1] = ba[i + 2]; ba[i + 2] = c;
    }

    QFile file2(argv[2]);
    file2.open(QIODevice::WriteOnly);
    file2.write(ba);
    file2.close();
}

