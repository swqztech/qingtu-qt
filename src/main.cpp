#include "mainwindow.h"

#include <QApplication>

int main(int argc, char* argv[]) {
  QApplication app(argc, argv);
  QApplication::setApplicationName(QStringLiteral("qingtu-qt"));
  QApplication::setOrganizationName(QStringLiteral("swqztech"));
  MainWindow w;
  w.show();
  return app.exec();
}
