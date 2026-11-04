#include "main_window.h"
#include "code_editor.h"
#include "property_editor.h"
#include "viewport_widget.h"
#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QComboBox>
#include <QCoreApplication>
#include <QDebug>
#include <QDirIterator>
#include <QDockWidget>
#include <QFile>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonValue>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QIcon>
#include <QSize>
#include <QToolBar>
#include <QMessageBox>
#include <QProcess>
#include <QRegularExpression>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSplitter>
#include <QStatusBar>
#include <QTextDocument>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

static QByteArray strip_comments(const QByteArray &data) {
  QByteArray result;
  bool in_string = false;
  bool escaped = false;
  for (int i = 0; i < data.size(); ++i) {
    char c = data.at(i);
    if (in_string) {
      result.append(c);
      if (escaped) {
        escaped = false;
      } else if (c == '\\') {
        escaped = true;
      } else if (c == '"') {
        in_string = false;
      }
      continue;
    }
    if (c == '"') {
      in_string = true;
      result.append(c);
    } else if (c == '/' && i + 1 < data.size() && data.at(i + 1) == '/') {
      while (i < data.size() && data.at(i) != '\n') {
        ++i;
      }
      result.append('\n');
    } else if (c == '/' && i + 1 < data.size() && data.at(i + 1) == '*') {
      i += 2;
      while (i + 1 < data.size() && !(data.at(i) == '*' && data.at(i + 1) == '/')) {
        ++i;
      }
      ++i;
    } else {
      result.append(c);
    }
  }
  return result;
}

static QIcon icon_for_element_type(const QString &type) {
  if (type == "panel") {
    return QIcon(":/icons/box.png");
  }
  if (type == "text") {
    return QIcon(":/icons/type.png");
  }
  if (type == "image") {
    return QIcon(":/icons/image.png");
  }
  if (type == "button") {
    return QIcon(":/icons/mouse-pointer.png");
  }
  if (type == "image_button") {
    return QIcon(":/icons/image.png");
  }
  return QIcon();
}

static QVector<QJsonObject> parse_objects(const QByteArray &data) {
  QVector<QJsonObject> objects;
  int depth = 0;
  int start = -1;
  bool in_string = false;
  bool escaped = false;
  for (int i = 0; i < data.size(); ++i) {
    char c = data.at(i);
    if (in_string) {
      if (escaped) {
        escaped = false;
      } else if (c == '\\') {
        escaped = true;
      } else if (c == '"') {
        in_string = false;
      }
      continue;
    }
    if (c == '"') {
      in_string = true;
    } else if (c == '{') {
      if (depth == 0) {
        start = i;
      }
      ++depth;
    } else if (c == '}') {
      --depth;
      if (depth == 0 && start >= 0) {
        QJsonParseError error;
        QJsonDocument document = QJsonDocument::fromJson(data.mid(start, i - start + 1), &error);
        if (error.error == QJsonParseError::NoError && document.isObject()) {
          objects.append(document.object());
        }
        start = -1;
      }
    }
  }
  return objects;
}

main_window::main_window(QWidget *parent) : QMainWindow(parent) {
  setWindowTitle("Infernum Engine Editor");
  setWindowIcon(QIcon(":/icons/layers.png"));
  resize(1280, 800);
  setStyleSheet("QMainWindow { background-color: #1b1d1f; }"
                "QMenuBar { background-color: #1b1d1f; color: #d6d6d6; }"
                "QMenuBar::item:selected { background-color: #2d3033; }"
                "QMenu { background-color: #232527; color: #d6d6d6;"
                " border: 1px solid #3a3d40; }"
                "QMenu::item:selected { background-color: #2d3033; }"
                "QStatusBar { background-color: #1b1d1f; color: #8a8a8a; }"
                "QTreeWidget#explorer { background-color: #232527; color: #d6d6d6;"
                " border: none; outline: none; }"
                "QTreeWidget#explorer::item { height: 22px; }"
                "QTreeWidget#explorer::item:selected { background-color: #2d3033; }"
                "QDockWidget { color: #d6d6d6; }"
                "QDockWidget::title { background-color: #1b1d1f; padding: 4px; }"
                "QPlainTextEdit#code { background-color: #17191b; color: #d6d6d6;"
                " border: none; selection-background-color: #2d3033; }"
                "QComboBox { background-color: #232527; color: #d6d6d6;"
                " border: 1px solid #3a3d40; padding: 2px 6px; }"
                "QComboBox QAbstractItemView { background-color: #232527;"
                " color: #d6d6d6; selection-background-color: #2d3033; }"
                "QPushButton { background-color: #2d3033; color: #d6d6d6;"
                " border: 1px solid #3a3d40; padding: 4px 10px; }"
                "QPushButton:hover { background-color: #3a3d40; }"
                "QToolBar { background-color: #1b1d1f; border: none;"
                " spacing: 4px; padding: 4px; }"
                "QToolBar QToolButton { background-color: transparent;"
                " border: none; border-radius: 3px; padding: 4px; }"
                "QToolBar QToolButton:hover { background-color: #2d3033; }"
                "QSplitter::handle { background-color: #101214; }");

  build_explorer();
  build_inspector();
  build_viewport();
  build_code_panel();
  build_menus();

  QSplitter *left_split = new QSplitter(Qt::Horizontal, this);
  left_split->addWidget(d_explorer);
  left_split->addWidget(d_viewport);
  left_split->setStretchFactor(0, 0);
  left_split->setStretchFactor(1, 1);
  left_split->setSizes({240, 660});

  QSplitter *main_split = new QSplitter(Qt::Horizontal, this);
  main_split->addWidget(left_split);
  main_split->addWidget(d_inspector);
  main_split->setStretchFactor(0, 1);
  main_split->setStretchFactor(1, 0);
  main_split->setSizes({900, 360});

  setCentralWidget(main_split);
  resizeDocks({d_code_dock}, {320}, Qt::Vertical);
  statusBar()->showMessage("Infernum Engine Editor");
}

void main_window::build_explorer() {
  d_explorer = new QTreeWidget(this);
  d_explorer->setObjectName("explorer");
  d_explorer->setHeaderHidden(true);
  d_explorer->setColumnCount(1);
  d_explorer->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(d_explorer, &QTreeWidget::currentItemChanged, this, &main_window::object_selected);
  connect(d_explorer, &QTreeWidget::customContextMenuRequested, this, &main_window::show_explorer_menu);

  QAction *delete_shortcut = new QAction(this);
  delete_shortcut->setShortcut(QKeySequence(Qt::Key_Delete));
  delete_shortcut->setShortcutContext(Qt::WidgetWithChildrenShortcut);
  connect(delete_shortcut, &QAction::triggered, this, &main_window::delete_selected);
  d_explorer->addAction(delete_shortcut);
}

void main_window::build_inspector() {
  d_inspector = new property_editor(this);
  d_inspector->setMinimumWidth(300);
  connect(d_inspector, &property_editor::property_changed, this, &main_window::on_property_changed);
}

void main_window::build_viewport() {
  d_viewport = new viewport_widget(this);
  connect(d_viewport, &viewport_widget::element_selected, this, &main_window::on_element_selected);
  connect(d_viewport, &viewport_widget::element_moved, this, &main_window::on_element_moved);
  connect(d_viewport, &viewport_widget::element_context_menu, this, &main_window::show_viewport_menu);

  QAction *delete_shortcut = new QAction(this);
  delete_shortcut->setShortcut(QKeySequence(Qt::Key_Delete));
  delete_shortcut->setShortcutContext(Qt::WidgetWithChildrenShortcut);
  connect(delete_shortcut, &QAction::triggered, this, &main_window::delete_selected);
  d_viewport->addAction(delete_shortcut);
}

void main_window::build_code_panel() {
  d_code_dock = new QDockWidget("Code", this);
  d_code_dock->setObjectName("code_dock");
  d_code_dock->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea);

  QWidget *container = new QWidget(d_code_dock);
  QVBoxLayout *layout = new QVBoxLayout(container);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);

  QWidget *bar = new QWidget(container);
  QHBoxLayout *bar_layout = new QHBoxLayout(bar);
  bar_layout->setContentsMargins(6, 4, 6, 4);
  bar_layout->setSpacing(6);

  QLabel *label = new QLabel("File:", bar);
  d_code_files = new QComboBox(bar);
  d_code_files->setMinimumWidth(320);
  QPushButton *save_button = new QPushButton("Save", bar);
  save_button->setIcon(QIcon(":/icons/save.png"));
  QPushButton *reload_button = new QPushButton("Reload", bar);
  reload_button->setIcon(QIcon(":/icons/rotate-cw.png"));

  bar_layout->addWidget(label);
  bar_layout->addWidget(d_code_files, 1);
  bar_layout->addWidget(save_button);
  bar_layout->addWidget(reload_button);

  d_code_editor = new code_editor(container);

  layout->addWidget(bar);
  layout->addWidget(d_code_editor, 1);
  d_code_dock->setWidget(container);
  addDockWidget(Qt::BottomDockWidgetArea, d_code_dock);

  connect(d_code_files, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &main_window::on_code_file_changed);
  connect(save_button, &QPushButton::clicked, this, [this]() { save_code(); });
  connect(reload_button, &QPushButton::clicked, this, [this]() {
    if (!maybe_save_code()) {
      return;
    }
    QString path = d_code_editor->file_path();
    if (!path.isEmpty()) {
      open_code_file(path);
    }
  });
  connect(d_code_editor, &code_editor::modification_changed, this,
          [this](bool) { update_code_title(); });
}

void main_window::build_menus() {
  QMenu *file_menu = menuBar()->addMenu("File");

  QAction *new_scene_action = file_menu->addAction("New Scene…");
  new_scene_action->setIcon(QIcon(":/icons/file.png"));
  new_scene_action->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_N));
  connect(new_scene_action, &QAction::triggered, this, &main_window::create_new_scene);

  QAction *save_action = file_menu->addAction("Save");
  save_action->setShortcut(QKeySequence::Save);
  save_action->setIcon(QIcon(":/icons/save.png"));
  connect(save_action, &QAction::triggered, this, &main_window::save_active);

  QAction *reload_action = file_menu->addAction("Reload");
  reload_action->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_R));
  reload_action->setIcon(QIcon(":/icons/rotate-cw.png"));
  connect(reload_action, &QAction::triggered, this, [this]() {
    if (!maybe_save_code()) {
      return;
    }
    if (!d_project_folder.isEmpty()) {
      load_project(d_project_folder);
    }
  });

  file_menu->addSeparator();
  QAction *quit_action = file_menu->addAction("Quit");
  quit_action->setShortcut(QKeySequence::Quit);
  quit_action->setIcon(QIcon(":/icons/x.png"));
  connect(quit_action, &QAction::triggered, this, &main_window::close);

  QMenu *edit_menu = menuBar()->addMenu("Edit");
  QAction *undo_action = edit_menu->addAction("Undo");
  undo_action->setEnabled(false);
  QAction *redo_action = edit_menu->addAction("Redo");
  redo_action->setEnabled(false);
  edit_menu->addSeparator();
  QAction *delete_action = edit_menu->addAction("Delete");
  delete_action->setIcon(QIcon(":/icons/trash-2.png"));
  connect(delete_action, &QAction::triggered, this, &main_window::delete_selected);

  QMenu *view_menu = menuBar()->addMenu("View");
  view_menu->addAction("Explorer");
  view_menu->addAction("Properties");
  QAction *code_action = view_menu->addAction("Code");
  code_action->setCheckable(true);
  code_action->setChecked(true);
  code_action->setIcon(QIcon(":/icons/code.png"));
  connect(code_action, &QAction::toggled, this, [this](bool visible) {
    if (d_code_dock) {
      d_code_dock->setVisible(visible);
    }
  });

  QMenu *test_menu = menuBar()->addMenu("Test");
  QAction *play_action = test_menu->addAction("Play");
  play_action->setShortcut(QKeySequence(Qt::Key_F5));
  play_action->setIcon(QIcon(":/icons/play.png"));
  connect(play_action, &QAction::triggered, this, &main_window::play_game);
  QAction *stop_action = test_menu->addAction("Stop");
  stop_action->setShortcut(QKeySequence(Qt::SHIFT | Qt::Key_F5));
  stop_action->setIcon(QIcon(":/icons/square.png"));
  connect(stop_action, &QAction::triggered, this, &main_window::stop_game);

  QMenu *help_menu = menuBar()->addMenu("Help");
  QAction *about_action = help_menu->addAction("About Infernum Engine");
  about_action->setIcon(QIcon(":/icons/info.png"));
  connect(about_action, &QAction::triggered, this, &main_window::show_about);

  QToolBar *toolbar = addToolBar("Infernum");
  toolbar->setObjectName("main_toolbar");
  toolbar->setIconSize(QSize(16, 16));
  toolbar->setToolButtonStyle(Qt::ToolButtonIconOnly);
  toolbar->addAction(save_action);
  toolbar->addAction(reload_action);
  toolbar->addSeparator();
  toolbar->addAction(play_action);
  toolbar->addAction(stop_action);
  toolbar->addSeparator();
  toolbar->addAction(code_action);
}

bool main_window::read_properties_file(const QString &path, QJsonObject &root) {
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly)) {
    return false;
  }
  QByteArray data = strip_comments(file.readAll());
  file.close();

  QJsonParseError error;
  QJsonDocument document = QJsonDocument::fromJson(data, &error);
  if (error.error == QJsonParseError::NoError && document.isObject()) {
    root = document.object();
    return true;
  }

  QVector<QJsonObject> objects = parse_objects(data);
  if (objects.isEmpty()) {
    return false;
  }
  for (const QJsonObject &object : objects) {
    for (auto it = object.constBegin(); it != object.constEnd(); ++it) {
      root.insert(it.key(), it.value());
    }
  }
  return true;
}

void main_window::load_project(const QString &folder) {
  d_project_folder = folder;
  d_scenes.clear();
  d_scene_order.clear();
  d_current_scene.clear();
  d_settings.clear();
  d_selected = -1;

  QJsonObject root;
  QString properties_path = QDir(folder).filePath("game_properties.json");
  if (read_properties_file(properties_path, root)) {
    for (auto it = root.constBegin(); it != root.constEnd(); ++it) {
      if (it.key() == "ui" || it.key() == "scenes") {
        continue;
      }
      d_settings.insert(it.key(), it.value().toVariant());
    }
  } else {
    statusBar()->showMessage(QString("could not read %1 - using defaults")
                                 .arg(properties_path), 4000);
  }

  QString scenes_dir = QDir(folder).filePath("scenes");
  QDir scene_directory(scenes_dir);
  const QStringList scene_folders =
      scene_directory.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
  int element_count = 0;
  d_broken_scenes.clear();
  for (const QString &scene_name : scene_folders) {
    const QString scene_path = QDir(scenes_dir).filePath(scene_name + "/properties.json");
    if (!QFile::exists(scene_path)) {
      continue;
    }
    QJsonObject scene_root;
    if (!read_properties_file(scene_path, scene_root)) {
      d_broken_scenes.append(scene_name);
      d_scenes.insert(scene_name, {});
      d_scene_order.append(scene_name);
      continue;
    }
    QVector<ui_element> elements;
    for (const QJsonValue &value : scene_root.value("ui").toArray()) {
      if (value.isObject()) {
        elements.append(element_from_json(value.toObject()));
      }
    }
    d_scenes.insert(scene_name, elements);
    d_scene_order.append(scene_name);
    element_count += elements.size();
  }

  if (d_scene_order.isEmpty() && read_properties_file(properties_path, root)) {
    QJsonValue scenes_value = root.value("scenes");
    if (scenes_value.isObject()) {
      const QJsonObject scenes = scenes_value.toObject();
      for (auto it = scenes.constBegin(); it != scenes.constEnd(); ++it) {
        QVector<ui_element> elements;
        for (const QJsonValue &value : it.value().toArray()) {
          if (value.isObject()) {
            elements.append(element_from_json(value.toObject()));
          }
        }
        d_scenes.insert(it.key(), elements);
        d_scene_order.append(it.key());
        element_count += elements.size();
      }
    } else {
      QVector<ui_element> elements;
      for (const QJsonValue &value : root.value("ui").toArray()) {
        if (value.isObject()) {
          elements.append(element_from_json(value.toObject()));
        }
      }
      if (!elements.isEmpty()) {
        QString scene_name = d_settings.value("main_scene", "main_scene").toString();
        d_scenes.insert(scene_name, elements);
        d_scene_order.append(scene_name);
        element_count += elements.size();
      }
    }
  }
  d_scene_order.sort();
  if (!d_scene_order.isEmpty()) {
    statusBar()->showMessage(QString("loaded %1 scenes, %2 elements")
                                 .arg(d_scene_order.size())
                                 .arg(element_count), 4000);
  }

  if (d_scene_order.isEmpty()) {
    d_scene_order.append("main_scene");
    d_scenes.insert("main_scene", default_elements());
  }
  d_current_scene = d_scene_order.first();

  QString assets_dir = d_settings.value("assets_dir", "./assets").toString();
  if (QDir::isRelativePath(assets_dir)) {
    assets_dir = QDir(folder).filePath(assets_dir);
  }
  QString assets_base = QFileInfo(assets_dir).absolutePath();
  d_viewport->set_asset_base_path(assets_base);
  d_viewport->set_elements(&d_scenes[d_current_scene]);

  refresh_explorer();
  if (!d_scenes[d_current_scene].isEmpty()) {
    select_element(0);
  } else {
    d_selected = -1;
    d_viewport->set_selected(-1);
    d_inspector->set_properties(QMap<QString, QVariant>());
  }
  setWindowTitle(QString("Infernum Engine Editor - %1").arg(QDir(folder).dirName()));
  refresh_code_files();
}

void main_window::refresh_explorer() {
  QSignalBlocker blocker(d_explorer);
  d_explorer->clear();

  QTreeWidgetItem *workspace = new QTreeWidgetItem(d_explorer);
  workspace->setText(0, "Workspace");
  workspace->setIcon(0, QIcon(":/icons/folder.png"));

  for (const QString &scene_name : d_scene_order) {
    QTreeWidgetItem *scene_item = new QTreeWidgetItem(workspace);
    scene_item->setText(0, scene_name);
    scene_item->setIcon(0, QIcon(":/icons/code.png"));
    if (scene_name == d_settings.value("main_scene", "main_scene").toString()) {
      QFont font = scene_item->font(0);
      font.setBold(true);
      scene_item->setFont(0, font);
    }
    scene_item->setData(0, Qt::UserRole, scene_name);
    scene_item->setData(0, Qt::UserRole + 1, -1);

    const QVector<ui_element> &elements = d_scenes.value(scene_name);
    for (int i = 0; i < elements.size(); ++i) {
      QTreeWidgetItem *item = new QTreeWidgetItem(scene_item);
      item->setText(0, elements.at(i).name);
      item->setIcon(0, icon_for_element_type(elements.at(i).type));
      item->setData(0, Qt::UserRole, scene_name);
      item->setData(0, Qt::UserRole + 1, i);
    }
  }

  d_explorer->expandAll();
}

void main_window::switch_scene(const QString &name) {
  if (!d_scenes.contains(name) || name == d_current_scene) {
    return;
  }
  d_current_scene = name;
  d_selected = -1;
  d_viewport->set_elements(&d_scenes[d_current_scene]);
  d_viewport->set_selected(-1);
  d_inspector->set_properties(QMap<QString, QVariant>());
  d_viewport->refresh();
}

QVector<ui_element> &main_window::current_elements() {
  return d_scenes[d_current_scene];
}

void main_window::select_element(int index) {
  QVector<ui_element> &elements = current_elements();
  if (index < 0 || index >= elements.size()) {
    d_selected = -1;
    d_viewport->set_selected(-1);
    d_inspector->set_properties(QMap<QString, QVariant>());
    return;
  }

  d_selected = index;
  d_viewport->set_selected(index);
  d_inspector->set_properties(elements.at(index).properties);

  QSignalBlocker blocker(d_explorer);
  QTreeWidgetItem *root = d_explorer->topLevelItem(0);
  if (root) {
    for (int s = 0; s < root->childCount(); ++s) {
      QTreeWidgetItem *scene_item = root->child(s);
      if (scene_item->data(0, Qt::UserRole).toString() != d_current_scene) {
        continue;
      }
      for (int i = 0; i < scene_item->childCount(); ++i) {
        if (scene_item->child(i)->data(0, Qt::UserRole + 1).toInt() == index) {
          d_explorer->setCurrentItem(scene_item->child(i));
          break;
        }
      }
      break;
    }
  }

  statusBar()->showMessage(QString("selected %1 (%2)")
                               .arg(elements.at(index).name)
                               .arg(elements.at(index).type), 3000);
}

void main_window::object_selected(QTreeWidgetItem *current, QTreeWidgetItem *previous) {
  Q_UNUSED(previous);
  if (!current) {
    return;
  }
  QString scene = current->data(0, Qt::UserRole).toString();
  int index = current->data(0, Qt::UserRole + 1).toInt();
  if (index < 0) {
    if (!scene.isEmpty() && scene != d_current_scene) {
      switch_scene(scene);
      refresh_explorer();
      QSignalBlocker blocker(d_explorer);
      QTreeWidgetItem *root = d_explorer->topLevelItem(0);
      if (root) {
        for (int s = 0; s < root->childCount(); ++s) {
          if (root->child(s)->data(0, Qt::UserRole).toString() == scene) {
            d_explorer->setCurrentItem(root->child(s));
            break;
          }
        }
      }
      statusBar()->showMessage(QString("scene %1").arg(scene), 3000);
    } else {
      d_selected = -1;
      d_viewport->set_selected(-1);
      d_inspector->set_properties(QMap<QString, QVariant>());
    }
    return;
  }
  if (scene != d_current_scene) {
    switch_scene(scene);
    refresh_explorer();
  }
  select_element(index);
}

void main_window::on_property_changed(const QString &key, const QVariant &value) {
  if (d_selected < 0 || d_selected >= current_elements().size()) {
    return;
  }
  current_elements()[d_selected].properties.insert(key, value);
  d_viewport->refresh();
  statusBar()->showMessage(QString("%1 = %2").arg(key).arg(value.toString()), 2000);
}

void main_window::on_element_selected(int index) {
  select_element(index);
}

void main_window::on_element_moved(int index) {
  if (index < 0 || index >= current_elements().size()) {
    return;
  }
  d_inspector->update_value("position_x", current_elements().at(index).properties.value("position_x"));
  d_inspector->update_value("position_y", current_elements().at(index).properties.value("position_y"));
  statusBar()->showMessage(QString("%1: x=%2 y=%3")
                               .arg(current_elements().at(index).name)
                               .arg(current_elements().at(index).properties.value("position_x").toDouble(), 0, 'f', 2)
                               .arg(current_elements().at(index).properties.value("position_y").toDouble(), 0, 'f', 2), 2000);
}

void main_window::refresh_code_files() {
  if (!d_code_files || !d_code_editor) {
    return;
  }
  QString current = d_code_editor->file_path();
  QString scenes_dir = QDir(d_project_folder).filePath("scenes");
  QStringList paths;
  QDirIterator iterator(scenes_dir, {"*.cpp", "*.h", "*.hpp", "*.c"},
                        QDir::Files, QDirIterator::Subdirectories);
  while (iterator.hasNext()) {
    paths.append(iterator.next());
  }
  paths.sort();

  QSignalBlocker blocker(d_code_files);
  d_code_files->clear();
  for (const QString &path : paths) {
    d_code_files->addItem(QDir(d_project_folder).relativeFilePath(path), path);
  }

  QString target = current;
  if (target.isEmpty()) {
    target = QDir(scenes_dir).filePath("main_scene/main_scene.cpp");
  }
  int index = d_code_files->findData(target);
  if (index < 0 && d_code_files->count() > 0) {
    index = 0;
  }
  if (index >= 0) {
    d_code_files->setCurrentIndex(index);
    open_code_file(d_code_files->itemData(index).toString());
  } else {
    d_code_editor->clear();
    update_code_title();
  }
}

void main_window::open_code_file(const QString &path) {
  if (!d_code_editor || path.isEmpty()) {
    return;
  }
  if (!d_code_editor->load_file(path)) {
    statusBar()->showMessage(QString("could not open %1").arg(path), 5000);
    return;
  }
  int index = d_code_files->findData(path);
  if (index >= 0) {
    QSignalBlocker blocker(d_code_files);
    d_code_files->setCurrentIndex(index);
  }
  update_code_title();
  statusBar()->showMessage(QString("editing %1").arg(path), 3000);
}

void main_window::on_code_file_changed(int index) {
  if (index < 0 || !d_code_files || !d_code_editor) {
    return;
  }
  QString path = d_code_files->itemData(index).toString();
  if (path == d_code_editor->file_path()) {
    return;
  }
  if (!maybe_save_code()) {
    QSignalBlocker blocker(d_code_files);
    int old_index = d_code_files->findData(d_code_editor->file_path());
    d_code_files->setCurrentIndex(old_index >= 0 ? old_index : 0);
    return;
  }
  open_code_file(path);
}

void main_window::save_active() {
  QWidget *focus = QApplication::focusWidget();
  bool code_focused =
      d_code_editor && (focus == d_code_editor || d_code_editor->isAncestorOf(focus));
  if (code_focused) {
    save_code();
    return;
  }
  if (d_code_editor && d_code_editor->document()->isModified()) {
    if (!save_code()) {
      return;
    }
  }
  save_project();
}

bool main_window::save_code() {
  if (!d_code_editor || d_code_editor->file_path().isEmpty()) {
    return true;
  }
  if (!d_code_editor->save_file()) {
    statusBar()->showMessage(
        QString("could not save %1").arg(d_code_editor->file_path()), 8000);
    return false;
  }
  update_code_title();
  statusBar()->showMessage(
      QString("saved %1").arg(d_code_editor->file_path()), 3000);
  return true;
}

bool main_window::maybe_save_code() {
  if (!d_code_editor || !d_code_editor->document()->isModified()) {
    return true;
  }
  QMessageBox::StandardButton answer = QMessageBox::warning(
      this, "Unsaved code", "Save changes to the current file?",
      QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
  if (answer == QMessageBox::Save) {
    return save_code();
  }
  if (answer == QMessageBox::Cancel) {
    return false;
  }
  return true;
}

void main_window::update_code_title() {
  if (!d_code_dock || !d_code_editor) {
    return;
  }
  QString title = "Code";
  if (!d_code_editor->file_path().isEmpty()) {
    title = QFileInfo(d_code_editor->file_path()).fileName();
  }
  if (d_code_editor->document()->isModified()) {
    title += " *";
  }
  d_code_dock->setWindowTitle(title);
}

void main_window::closeEvent(QCloseEvent *event) {
  if (!maybe_save_code()) {
    event->ignore();
    return;
  }
  QMainWindow::closeEvent(event);
}

void main_window::save_project() {
  if (d_project_folder.isEmpty()) {
    return;
  }

  QJsonObject root;
  for (auto it = d_settings.constBegin(); it != d_settings.constEnd(); ++it) {
    const QVariant &value = it.value();
    if (value.userType() == QMetaType::Bool) {
      root.insert(it.key(), value.toBool());
    } else if (value.userType() == QMetaType::Int) {
      root.insert(it.key(), value.toInt());
    } else if (value.userType() == QMetaType::Double) {
      root.insert(it.key(), value.toDouble());
    } else if (value.userType() == QMetaType::QString) {
      root.insert(it.key(), value.toString());
    } else {
      root.insert(it.key(), QJsonValue::fromVariant(value));
    }
  }

  QString path = QDir(d_project_folder).filePath("game_properties.json");
  QFile file(path);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    statusBar()->showMessage(QString("could not save %1").arg(path), 4000);
    return;
  }
  file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
  file.close();

  QStringList skipped;
  for (const QString &scene_name : d_scene_order) {
    if (d_broken_scenes.contains(scene_name)) {
      skipped.append(scene_name);
      continue;
    }
    const QString scene_dir = QDir(d_project_folder).filePath("scenes/" + scene_name);
    QDir().mkpath(scene_dir);
    QJsonObject scene_root;
    scene_root.insert("name", scene_name);
    QJsonArray ui_array;
    for (const ui_element &element : d_scenes.value(scene_name)) {
      ui_array.append(element_to_json(element));
    }
    scene_root.insert("ui", ui_array);
    const QString scene_path = QDir(scene_dir).filePath("properties.json");
    QFile scene_file(scene_path);
    if (!scene_file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
      skipped.append(scene_name);
      continue;
    }
    scene_file.write(QJsonDocument(scene_root).toJson(QJsonDocument::Indented));
    scene_file.close();
  }
  if (!skipped.isEmpty()) {
    statusBar()->showMessage(
        QString("could not save scene(s): %1 (invalid properties.json?)")
            .arg(skipped.join(", ")), 6000);
    return;
  }
  statusBar()->showMessage(QString("saved %1").arg(path), 4000);
}

void main_window::add_new_object_submenu(QMenu *parent) {
  QMenu *sub = new QMenu("Add new object", parent);
  const struct { const char *label; const char *type; const char *icon; } entries[] = {
      {"Panel", "panel", ":/icons/box.png"},
      {"Text", "text", ":/icons/type.png"},
      {"Image", "image", ":/icons/image.png"},
      {"Button", "button", ":/icons/mouse-pointer.png"},
      {"Image Button", "image_button", ":/icons/image.png"},
  };
  for (const auto &entry : entries) {
    QAction *action = sub->addAction(QIcon(entry.icon), entry.label);
    const QString type = QString::fromLatin1(entry.type);
    connect(action, &QAction::triggered, this, [this, type]() { add_new_object(type); });
  }
  parent->addMenu(sub);
}

void main_window::add_new_object(const QString &type) {
  const QString scene = d_add_scene;
  const int after = d_add_after;
  if (scene.isEmpty() || !d_scenes.contains(scene)) {
    return;
  }
  if (scene != d_current_scene) {
    switch_scene(scene);
    refresh_explorer();
  }
  QVector<ui_element> &elements = current_elements();
  auto name_taken = [&elements](const QString &candidate) {
    for (const ui_element &element : elements) {
      if (element.name == candidate) {
        return true;
      }
    }
    return false;
  };
  QString name = type;
  int suffix = 2;
  while (name_taken(name)) {
    name = QString("%1_%2").arg(type).arg(suffix);
    ++suffix;
  }

  ui_element element;
  element.name = name;
  element.type = type;
  element.properties = default_element_properties(type);

  int insert_at = (after >= 0 && after < elements.size()) ? after + 1 : elements.size();
  elements.insert(insert_at, element);

  d_viewport->set_elements(&d_scenes[d_current_scene]);
  refresh_explorer();
  select_element(insert_at);
  statusBar()->showMessage(QString("added %1 to %2").arg(name, d_current_scene), 3000);
}

void main_window::create_new_scene() {
  if (d_project_folder.isEmpty()) {
    return;
  }
  bool ok = false;
  const QString name = QInputDialog::getText(
                           this, "Create new scene", "Scene name :",
                           QLineEdit::Normal, QString(), &ok)
                           .trimmed();
  if (!ok) {
    return;
  }
  static const QRegularExpression valid_name("^[a-z][a-z0-9_]*$");
  if (!valid_name.match(name).hasMatch()) {
    QMessageBox::warning(this, "incorrect scene name",
                         "use lowercase snake_case: letters, numbers, underscores; "
                         "start with a letter.");
    return;
  }
  if (d_scene_order.contains(name)) {
    QMessageBox::warning(this, "scene exists",
                         "a scene with that name already exists.");
    return;
  }
  const QString scene_dir = QDir(d_project_folder).filePath("scenes/" + name);
  const QString cpp_path = QDir(scene_dir).filePath(name + ".cpp");
  const QString header_path = QDir(scene_dir).filePath(name + ".h");
  const QString properties_path = QDir(scene_dir).filePath("properties.json");
  if (QFile::exists(cpp_path) || QFile::exists(header_path) ||
      QFile::exists(properties_path)) {
    QMessageBox::warning(this, "file exists",
                         QString("scene files already exist for:\n%1").arg(cpp_path));
    return;
  }

  const QString cpp_source =
      QString("#include \"%1.h\"\n"
              "#include \"engine.h\"\n"
              "#include \"ui.h\"\n"
              "#include <SDL3/SDL.h>\n"
              "#include <SDL3/SDL_render.h>\n"
              "#include <cstdio>\n"
              "\n"
              "static ui_scene scene_ui;\n"
              "static bool ui_ready = false;\n"
              "\n"
              "void main_scene(SDL_Renderer *scene_renderer) {\n"
              "  if (!scene_renderer) {\n"
              "    printf(\">> scene has no renderer.. ERROR\\n\");\n"
              "    return;\n"
              "  }\n"
              "  if (!ui_ready) {\n"
              "    scene_ui.load(scene_renderer, \"game_properties.json\", \"%2\");\n"
              "    ui_ready = true;\n"
              "  }\n"
              "  SDL_Color background = {12, 12, 16, 255};\n"
              "  render_frame(scene_renderer, 100.0f, 100.0f, 50.0f, 50.0f, background);\n"
              "  scene_ui.render(scene_renderer);\n"
              "}\n")
          .arg(name, name);

  const QString header_source =
      QString("#pragma once\n"
              "#include <SDL3/SDL.h>\n"
              "extern \"C\" {\n"
              "#ifdef _WIN32\n"
              "__declspec(dllexport)\n"
              "#endif\n"
              "void main_scene(SDL_Renderer *scene_renderer);\n"
              "}\n");

  if (!QDir().mkpath(scene_dir)) {
    QMessageBox::warning(this, "could not create scene",
                         QString("could not create %1").arg(scene_dir));
    return;
  }

  QFile cpp_file(cpp_path);
  if (!cpp_file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    QMessageBox::warning(this, "could not create scene",
                         QString("could not write %1").arg(cpp_path));
    return;
  }
  cpp_file.write(cpp_source.toUtf8());
  cpp_file.close();

  QFile header_file(header_path);
  if (!header_file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    cpp_file.remove();
    QMessageBox::warning(this, "could not create scene",
                         QString("could not write %1").arg(header_path));
    return;
  }
  header_file.write(header_source.toUtf8());
  header_file.close();

  QJsonObject scene_root;
  scene_root.insert("name", name);
  scene_root.insert("ui", QJsonArray());
  QFile properties_file(properties_path);
  if (!properties_file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    cpp_file.remove();
    header_file.remove();
    QMessageBox::warning(this, "could not create scene",
                         QString("could not write %1").arg(properties_path));
    return;
  }
  properties_file.write(QJsonDocument(scene_root).toJson(QJsonDocument::Indented));
  properties_file.close();

  d_scenes.insert(name, {});
  d_scene_order.append(name);
  d_scene_order.sort();
  save_project();
  refresh_explorer();
  switch_scene(name);
  statusBar()->showMessage(
      QString("created scene %1 - press F5 to build it; edit scenes/%1/%1.cpp to use it")
          .arg(name, name, name), 8000);
}

void main_window::show_explorer_menu(const QPoint &position) {
  QTreeWidgetItem *item = d_explorer->itemAt(position);
  if (!item) {
    return;
  }
  const QString scene = item->data(0, Qt::UserRole).toString();
  const int index = item->data(0, Qt::UserRole + 1).toInt();

  QString effective_scene = scene;
  if (effective_scene.isEmpty()) {
    effective_scene = d_current_scene;
  }
  d_add_scene = effective_scene;
  d_add_after = index >= 0 ? index : -1;

  if (index >= 0) {
    if (effective_scene != d_current_scene) {
      switch_scene(effective_scene);
      refresh_explorer();
    }
    select_element(index);
  }

  QMenu menu(this);
  add_new_object_submenu(&menu);

  QAction *rename_action = nullptr;
  QAction *duplicate_action = nullptr;
  QAction *delete_action = nullptr;
  if (index >= 0) {
    menu.addSeparator();
    rename_action = menu.addAction("Rename");
    rename_action->setIcon(QIcon(":/icons/edit-2.png"));
    duplicate_action = menu.addAction("Duplicate");
    duplicate_action->setIcon(QIcon(":/icons/copy.png"));
    menu.addSeparator();
    delete_action = menu.addAction("Delete");
    delete_action->setIcon(QIcon(":/icons/trash-2.png"));
  }

  QAction *chosen = menu.exec(d_explorer->viewport()->mapToGlobal(position));
  if (!chosen) {
    return;
  }
  if (rename_action && chosen == rename_action) {
    rename_element(index);
  } else if (duplicate_action && chosen == duplicate_action) {
    duplicate_element(index);
  } else if (delete_action && chosen == delete_action) {
    delete_element(index);
  }
}

void main_window::show_viewport_menu(int index, const QPoint &global_position) {
  if (index < 0 || index >= current_elements().size()) {
    return;
  }
  d_add_scene = d_current_scene;
  d_add_after = index;

  QMenu menu(this);
  add_new_object_submenu(&menu);
  menu.addSeparator();
  QAction *rename_action = menu.addAction("Rename");
  rename_action->setIcon(QIcon(":/icons/edit-2.png"));
  QAction *duplicate_action = menu.addAction("Duplicate");
  duplicate_action->setIcon(QIcon(":/icons/copy.png"));
  menu.addSeparator();
  QAction *delete_action = menu.addAction("Delete");
  delete_action->setIcon(QIcon(":/icons/trash-2.png"));

  QAction *chosen = menu.exec(global_position);
  if (chosen == rename_action) {
    rename_element(index);
  } else if (chosen == duplicate_action) {
    duplicate_element(index);
  } else if (chosen == delete_action) {
    delete_element(index);
  }
}

void main_window::delete_selected() {
  if (d_selected < 0 || d_selected >= current_elements().size()) {
    return;
  }
  delete_element(d_selected);
}

void main_window::delete_element(int index) {
  if (index < 0 || index >= current_elements().size()) {
    return;
  }

  QString name = current_elements().at(index).name;
  current_elements().remove(index);
  refresh_explorer();

  int next = qMin(index, current_elements().size() - 1);
  if (next >= 0) {
    select_element(next);
  } else {
    d_selected = -1;
    d_viewport->set_selected(-1);
    d_inspector->set_properties(QMap<QString, QVariant>());
  }
  d_viewport->refresh();
  statusBar()->showMessage(QString("deleted %1").arg(name), 3000);
}

void main_window::duplicate_element(int index) {
  if (index < 0 || index >= current_elements().size()) {
    return;
  }

  ui_element copy = current_elements().at(index);
  QString base_name = copy.name;
  QString name = base_name + "_copy";
  int suffix = 2;
  bool taken = true;
  while (taken) {
    taken = false;
    for (const ui_element &element : current_elements()) {
      if (element.name == name) {
        taken = true;
        break;
      }
    }
    if (taken) {
      name = base_name + "_copy" + QString::number(suffix);
      ++suffix;
    }
  }

  copy.name = name;
  current_elements().insert(index + 1, copy);
  refresh_explorer();
  select_element(index + 1);
  d_viewport->refresh();
  statusBar()->showMessage(QString("duplicated %1 as %2").arg(base_name, name), 3000);
}

void main_window::rename_element(int index) {
  if (index < 0 || index >= current_elements().size()) {
    return;
  }

  bool ok = false;
  QString old_name = current_elements().at(index).name;
  QString name = QInputDialog::getText(this, "Rename", "Name:", QLineEdit::Normal,
                                       old_name, &ok).trimmed();
  if (!ok || name.isEmpty()) {
    return;
  }

  bool taken = true;
  QString unique_name = name;
  int suffix = 2;
  while (taken) {
    taken = false;
    for (int i = 0; i < current_elements().size(); ++i) {
      if (i != index && current_elements().at(i).name == unique_name) {
        taken = true;
        break;
      }
    }
    if (taken) {
      unique_name = name + QString::number(suffix);
      ++suffix;
    }
  }

  current_elements()[index].name = unique_name;
  refresh_explorer();
  select_element(index);
  statusBar()->showMessage(QString("renamed %1 to %2").arg(old_name, unique_name), 3000);
}

void main_window::play_game() {
  if (d_build_process || d_game_process) {
    stop_game();
  }
  start_build();
}

void main_window::stop_game() {
  QProcess *game = d_game_process;
  d_game_process = nullptr;
  if (game) {
    game->disconnect(this);
    game->kill();
    game->waitForFinished(2000);
    game->deleteLater();
  }

  QProcess *build = d_build_process;
  d_build_process = nullptr;
  if (build) {
    build->disconnect(this);
    build->kill();
    build->waitForFinished(2000);
    build->deleteLater();
  }
}

QString main_window::repo_root() const {
  QDir dir(QCoreApplication::applicationDirPath());
  dir.cdUp();
  return dir.absolutePath();
}

QString main_window::engine_path() const {
  QString platform = "linux";
#ifdef Q_OS_MACOS
  platform = "macos";
#endif
#ifdef Q_OS_WIN
  platform = "windows";
#endif
  QString exe_name = platform == "windows" ? "Inferno.exe" : "Inferno";
  return QDir(repo_root()).filePath("build/" + platform + "/" + exe_name);
}

void main_window::start_build() {
  if (d_build_process) {
    return;
  }

  if (!save_code()) {
    statusBar()->showMessage("could not save code", 8000);
    return;
  }
  save_project();

  d_build_process = new QProcess(this);
  d_build_process->setWorkingDirectory(repo_root());
  d_build_process->setProcessChannelMode(QProcess::MergedChannels);
  connect(d_build_process, &QProcess::readyReadStandardOutput, this, [this]() {
    if (!d_build_process) {
      return;
    }
    QString output = QString::fromLocal8Bit(d_build_process->readAllStandardOutput()).trimmed();
    if (!output.isEmpty()) {
      statusBar()->showMessage(output.right(200), 4000);
    }
  });
  connect(d_build_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
          this, [this](int exit_code, QProcess::ExitStatus) {
            QProcess *process = d_build_process;
            d_build_process = nullptr;
            QString output;
            if (process) {
              output = QString::fromLocal8Bit(process->readAllStandardOutput());
              process->deleteLater();
            }
            if (exit_code == 0) {
              statusBar()->showMessage("build finished - launching game", 4000);
              launch_game();
            } else {
              statusBar()->showMessage(QString("build failed (exit %1)").arg(exit_code), 8000);
              QString detail = output.trimmed();
              if (detail.isEmpty()) {
                detail = QString("build.sh exited with code %1").arg(exit_code);
              }
              QMessageBox::critical(this, "Build failed", detail.right(4000));
            }
          });
  connect(d_build_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError) {
    QProcess *process = d_build_process;
    if (!process) {
      return;
    }
    d_build_process = nullptr;
    process->disconnect(this);
    process->deleteLater();
    statusBar()->showMessage("could not start build", 8000);
  });

  statusBar()->showMessage("building...", 0);
  d_build_process->start("./build.sh", QStringList() << QDir(d_project_folder).absolutePath());
}

void main_window::launch_game() {
  if (d_game_process) {
    return;
  }

  QString path = engine_path();
  if (!QFileInfo::exists(path)) {
    statusBar()->showMessage(QString("engine not found: %1").arg(path), 8000);
    return;
  }

  d_game_process = new QProcess(this);
  d_game_process->setWorkingDirectory(QFileInfo(path).absolutePath());
  connect(d_game_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
          this, [this](int, QProcess::ExitStatus) {
            QProcess *process = d_game_process;
            d_game_process = nullptr;
            if (process) {
              process->deleteLater();
            }
            statusBar()->showMessage("game stopped", 3000);
          });
  connect(d_game_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError) {
    QProcess *process = d_game_process;
    if (!process) {
      return;
    }
    d_game_process = nullptr;
    process->disconnect(this);
    process->deleteLater();
    statusBar()->showMessage("could not launch game", 8000);
  });

  statusBar()->showMessage("running game", 0);
  d_game_process->start(path, QStringList());
}

void main_window::show_about() {
  QMessageBox::information(
      this, "About",
      "Infernum Engine Editor (IEE) \n  made with passion for \n Inferno"

                           "built with qt 5.");
}

void main_window::keyPressEvent(QKeyEvent *event) {
  if (event->key() == Qt::Key_S && (event->modifiers() & Qt::ControlModifier)) {
    save_active();
    return;
  }
  QMainWindow::keyPressEvent(event);
}
