#include <QCoreApplication>
#include <QLoggingCategory>
#include <QDebug>

Q_LOGGING_CATEGORY(logTest, "org.deepin.dde.GrandSearch.Test")

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    qCInfo(logTest) << "Hello, DLogCover minimal test!";
    return 0;
} 