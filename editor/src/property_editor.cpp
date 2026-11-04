#include "property_editor.h"
#include <QCheckBox>
#include <QColor>
#include <QColorDialog>
#include <QFont>
#include <QHeaderView>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSizeF>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

static void apply_color_button(QPushButton *button, const QColor &color) {
  bool light = color.red() + color.green() + color.blue() > 380;
  QString foreground = light ? "#000000" : "#ffffff";
  button->setText(color.name());
  button->setStyleSheet(QString("QPushButton { background-color: %1; color: %2;"
                                " border: 1px solid #3a3d40; border-radius: 3px;"
                                " padding: 3px 6px; }")
                            .arg(color.name())
                            .arg(foreground));
}

static QString category_for(const QString &key) {
  if (key == "name" || key == "type") {
    return "Data";
  }
  if (key == "position_x" || key == "position_y" || key == "size_x" ||
      key == "size_y" || key == "anchor_x" || key == "anchor_y" ||
      key == "scale") {
    return "Layout";
  }
  if (key == "text" || key == "text_size" || key == "text_color" ||
      key == "text_transparency") {
    return "Text";
  }
  if (key == "image" || key == "image_color" || key == "image_transparency") {
    return "Image";
  }
  if (key == "visible" || key == "background_color" ||
      key == "background_transparency" || key == "border_color" ||
      key == "border_transparency" || key == "border_size") {
    return "Appearance";
  }
  return "Behavior";
}

static QString variant_to_text(const QVariant &value) {
  if (value.userType() == QMetaType::QSizeF) {
    QSizeF size = value.toSizeF();
    return QString("%1, %2").arg(size.width()).arg(size.height());
  }
  if (value.userType() == QMetaType::QSize) {
    QSize size = value.toSize();
    return QString("%1, %2").arg(size.width()).arg(size.height());
  }
  return value.toString();
}

static QVariant parse_text(const QVariant &original, const QString &text) {
  switch (original.userType()) {
  case QMetaType::QSizeF: {
    QStringList parts = text.split(",");
    if (parts.size() != 2) {
      return QVariant();
    }
    return QSizeF(parts.at(0).trimmed().toDouble(), parts.at(1).trimmed().toDouble());
  }
  case QMetaType::QSize: {
    QStringList parts = text.split(",");
    if (parts.size() != 2) {
      return QVariant();
    }
    return QSize(parts.at(0).trimmed().toInt(), parts.at(1).trimmed().toInt());
  }
  case QMetaType::Int: {
    bool ok = false;
    int parsed = text.toInt(&ok);
    if (!ok) {
      return QVariant();
    }
    return parsed;
  }
  case QMetaType::Double: {
    bool ok = false;
    double parsed = text.toDouble(&ok);
    if (!ok) {
      return QVariant();
    }
    return parsed;
  }
  default:
    return text;
  }
}

property_editor::property_editor(QWidget *parent) : QWidget(parent) {
  d_search = new QLineEdit(this);
  d_search->setPlaceholderText("Filter properties");
  d_search->setClearButtonEnabled(true);
  d_tree = new QTreeWidget(this);
  d_tree->setColumnCount(2);
  d_tree->setHeaderLabels(QStringList() << "Property" << "Value");
  d_tree->header()->setSectionResizeMode(0, QHeaderView::Interactive);
  d_tree->header()->setSectionResizeMode(1, QHeaderView::Stretch);
  d_tree->setColumnWidth(0, 160);
  d_tree->setIndentation(10);
  d_tree->setSelectionMode(QAbstractItemView::NoSelection);
  d_tree->setFocusPolicy(Qt::NoFocus);
  d_tree->setRootIsDecorated(true);
  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setContentsMargins(6, 6, 6, 6);
  layout->setSpacing(6);
  layout->addWidget(d_search);
  layout->addWidget(d_tree);
  setStyleSheet("QLineEdit { background-color: #1b1d1f; color: #e8e8e8;"
                " border: 1px solid #3a3d40; border-radius: 3px; padding: 3px 6px; }"
                "QLineEdit:focus { border: 1px solid #4a8cff; }"
                "QTreeWidget { background-color: #232527; color: #d6d6d6;"
                " border: none; outline: none; }"
                "QTreeWidget::item { height: 24px; border-bottom: 1px solid #2b2d2f; }"
                "QHeaderView::section { background-color: #1b1d1f; color: #9a9a9a;"
                " border: none; border-bottom: 1px solid #3a3d40; padding: 4px 6px; }"
                "QCheckBox { margin-left: 6px; }");
  connect(d_search, &QLineEdit::textChanged, this, &property_editor::filter_rows);
}

void property_editor::set_properties(const QMap<QString, QVariant> &values) {
  d_values = values;
  d_tree->clear();
  for (auto it = d_values.constBegin(); it != d_values.constEnd(); ++it) {
    add_property_row(category_for(it.key()), it.key(), it.key(), it.value());
  }
  if (d_tree->topLevelItemCount() > 0) {
    d_tree->expandAll();
  }
}

QVariant property_editor::property_value(const QString &key) const {
  return d_values.value(key);
}

// Return all properties as a map
QMap<QString, QVariant> property_editor::properties() const {
  return d_values;
}

void property_editor::update_value(const QString &key, const QVariant &value) {
  if (!d_values.contains(key)) {
    return;
  }
  d_values.insert(key, value);
  for (int i = 0; i < d_tree->topLevelItemCount(); ++i) {
    QTreeWidgetItem *category = d_tree->topLevelItem(i);
    for (int j = 0; j < category->childCount(); ++j) {
      QTreeWidgetItem *item = category->child(j);
      if (item->data(0, Qt::UserRole).toString() != key) {
        continue;
      }
      QWidget *editor = d_tree->itemWidget(item, 1);
      if (QLineEdit *line_edit = qobject_cast<QLineEdit *>(editor)) {
        if (!line_edit->hasFocus()) {
          QSignalBlocker blocker(line_edit);
          line_edit->setText(variant_to_text(value));
        }
      } else if (QPushButton *button = qobject_cast<QPushButton *>(editor)) {
        if (value.userType() == QMetaType::QColor) {
          apply_color_button(button, value.value<QColor>());
        }
      } else if (QCheckBox *box = qobject_cast<QCheckBox *>(editor)) {
        if (value.userType() == QMetaType::Bool) {
          QSignalBlocker blocker(box);
          box->setChecked(value.toBool());
        }
      }
      return;
    }
  }
}

void property_editor::add_property_row(const QString &category, const QString &name,
                                       const QString &key, const QVariant &value) {
  QTreeWidgetItem *parent = nullptr;
  for (int i = 0; i < d_tree->topLevelItemCount(); ++i) {
    if (d_tree->topLevelItem(i)->text(0) == category) {
      parent = d_tree->topLevelItem(i);
      break;
    }
  }
  if (!parent) {
    parent = new QTreeWidgetItem(d_tree);
    parent->setText(0, category);
    parent->setFlags(Qt::ItemIsEnabled);
    QFont font = parent->font(0);
    font.setBold(true);
    parent->setFont(0, font);
    parent->setForeground(0, QColor("#9a9a9a"));
  }
  QTreeWidgetItem *item = new QTreeWidgetItem(parent);
  item->setText(0, name);
  item->setFlags(Qt::ItemIsEnabled);
  item->setData(0, Qt::UserRole, key);
  d_tree->setItemWidget(item, 1, create_editor(key, value));
}

QWidget *property_editor::create_editor(const QString &key, const QVariant &value) {
  if (value.userType() == QMetaType::Bool) {
    QCheckBox *box = new QCheckBox;
    box->setChecked(value.toBool());
    connect(box, &QCheckBox::toggled, this, [this, key](bool checked) {
      d_values.insert(key, checked);
      emit property_changed(key, checked);
    });
    return box;
  }
  if (value.userType() == QMetaType::QColor) {
    QPushButton *button = new QPushButton;
    apply_color_button(button, value.value<QColor>());
    connect(button, &QPushButton::clicked, this, [this, key, button]() {
      QColor current = d_values.value(key).value<QColor>();
      QColor chosen = QColorDialog::getColor(current, this, "Select Color");
      if (chosen.isValid()) {
        d_values.insert(key, chosen);
        apply_color_button(button, chosen);
        emit property_changed(key, chosen);
      }
    });
    return button;
  }
  QLineEdit *edit = new QLineEdit;
  edit->setText(variant_to_text(value));
  if (value.userType() == QMetaType::Double || value.userType() == QMetaType::Int) {
    edit->setAlignment(Qt::AlignRight);
  }
  connect(edit, &QLineEdit::textChanged, this, [this, key](const QString &text) {
    QVariant parsed = parse_text(d_values.value(key), text);
    if (parsed.isValid()) {
      d_values.insert(key, parsed);
      emit property_changed(key, parsed);
    }
  });
  return edit;
}

void property_editor::filter_rows(const QString &text) {
  for (int i = 0; i < d_tree->topLevelItemCount(); ++i) {
    QTreeWidgetItem *category = d_tree->topLevelItem(i);
    int visible = 0;
    for (int j = 0; j < category->childCount(); ++j) {
      QTreeWidgetItem *item = category->child(j);
      bool match = text.isEmpty() || item->text(0).contains(text, Qt::CaseInsensitive);
      item->setHidden(!match);
      if (match) {
        visible++;
      }
    }
    category->setHidden(visible == 0);
  }
}
