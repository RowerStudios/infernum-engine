#include "main_window.h"
#include <QApplication>
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QString>

int main(int argc, char **argv) {
  QApplication app(argc, argv);

  QString project_folder;
  if (argc > 1) {
    project_folder = QString(argv[1]);
    if (QDir::isRelativePath(project_folder)) {
      project_folder = QDir(QDir::currentPath()).filePath(project_folder);
    }
    if (!QDir(project_folder).exists()) {
      QDir repo_root(QCoreApplication::applicationDirPath() + "/..");
      QString candidate = repo_root.filePath(QString(argv[1]));
      if (QDir(candidate).exists()) {
        project_folder = candidate;
      }
    }
  } else {
    project_folder = QDir(QCoreApplication::applicationDirPath() + "/..").filePath("inferno");
  }

  if (!QDir(project_folder).exists()) {
    qCritical() << "project folder not found:" << project_folder;
    return 1;
  }

  main_window window;
  window.load_project(project_folder);
  window.show();
  return app.exec();
}
