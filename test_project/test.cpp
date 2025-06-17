#include <iostream>
#include <QDebug>

void testFunction() {
    std::cout << "This is a test function" << std::endl;
    qDebug() << "Qt debug message";
    qWarning() << "Qt warning message";
}

int main() {
    testFunction();
    return 0;
}
