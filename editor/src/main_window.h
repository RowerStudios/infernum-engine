#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include "ui_element.h"
#include <QDir>
#include <QJsonObject>
#include <QMap>
#include <QMainWindow>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVector>

class QTreeWidget;
class QTreeWidgetItem;
class QMenu;
class QProcess;
class QComboBox;
class QDockWidget;
class QCloseEvent;
class code_editor;
class property_editor;
class viewport_widget;

class main_window : public QMainWindow {
  Q_OBJECT
public:
  explicit main_window(QWidget *parent = nullptr);

  void load_project(const QString &folder);
  QString project_folder() const { return d_project_folder; }

private slots:
  void object_selected(QTreeWidgetItem *current, QTreeWidgetItem *previous);
  void show_about();
  void on_property_changed(const QString &key, const QVariant &value);
  void on_element_selected(int index);
  void on_element_moved(int index);
  void save_project();
  void show_explorer_menu(const QPoint &position);
  void show_viewport_menu(int index, const QPoint &global_position);
  void delete_selected();
  void delete_element(int index);
  void duplicate_element(int index);
  void rename_element(int index);
  void play_game();
  void stop_game();
  void on_code_file_changed(int index);
  void save_active();
  void create_new_scene();
  void switch_scene(const QString &name);
  void add_new_object(const QString &type);
  void add_new_object_submenu(QMenu *parent);
  QVector<ui_element> &current_elements();

private:
  void build_explorer();
  void build_viewport();
  void build_inspector();
  void build_code_panel();
  void build_menus();
  void refresh_explorer();
  void refresh_code_files();
  void open_code_file(const QString &path);
  bool save_code();
  bool maybe_save_code();
  void update_code_title();
  void select_element(int index);
  bool read_properties_file(const QString &path, QJsonObject &root);
  void start_build();
  void launch_game();
  QString repo_root() const;
  QString engine_path() const;
  void keyPressEvent(QKeyEvent *event) override;
  void closeEvent(QCloseEvent *event) override;

  QTreeWidget *d_explorer = nullptr;
  viewport_widget *d_viewport = nullptr;
  property_editor *d_inspector = nullptr;
  code_editor *d_code_editor = nullptr;
  QComboBox *d_code_files = nullptr;
  QDockWidget *d_code_dock = nullptr;
  QProcess *d_build_process = nullptr;
  QProcess *d_game_process = nullptr;
  QString d_project_folder;
  QMap<QString, QVector<ui_element>> d_scenes;
  QStringList d_scene_order;
  QStringList d_broken_scenes;
  QString d_current_scene;
  QString d_add_scene;
  int d_add_after = -1;
  QMap<QString, QVariant> d_settings;
  int d_selected = -1;
};

#endif
