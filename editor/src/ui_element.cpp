#include "ui_element.h"
#include <QColor>
#include <QJsonArray>
#include <QJsonValue>

static QJsonArray color_to_array(const QColor &color) {
  QJsonArray array;
  array.append(color.red());
  array.append(color.green());
  array.append(color.blue());
  array.append(color.alpha());
  return array;
}

static QColor array_to_color(const QJsonValue &value) {
  if (value.isArray()) {
    QJsonArray array = value.toArray();
    if (array.size() >= 3) {
      int r = array.at(0).toInt();
      int g = array.at(1).toInt();
      int b = array.at(2).toInt();
      int a = array.size() >= 4 ? array.at(3).toInt() : 255;
      return QColor(r, g, b, a);
    }
  }
  if (value.isString()) {
    return QColor(value.toString());
  }
  return QColor(0, 0, 0, 255);
}

QMap<QString, QVariant> default_element_properties(const QString &type) {
  QMap<QString, QVariant> props;
  props.insert("position_x", 50.0);
  props.insert("position_y", 50.0);
  props.insert("size_x", 0.0);
  props.insert("size_y", 0.0);
  props.insert("anchor_x", 0.5);
  props.insert("anchor_y", 0.5);
  props.insert("scale", 1.0);
  props.insert("visible", true);
  props.insert("background_color", QColor(0, 0, 0, 255));
  props.insert("background_transparency", 0.5);
  props.insert("border_color", QColor(255, 255, 255, 255));
  props.insert("border_transparency", 1.0);
  props.insert("border_size", 0.0);

  if (type == "text" || type == "button") {
    props.insert("text", type == "text" ? "Label" : "Button");
    props.insert("text_size", 24.0);
    props.insert("text_color", QColor(255, 255, 255, 255));
    props.insert("text_transparency", 0.0);
  }
  if (type == "image" || type == "image_button") {
    props.insert("image", "test");
    props.insert("image_color", QColor(255, 255, 255, 255));
    props.insert("image_transparency", 0.0);
  }
  if (type == "button" || type == "image_button") {
    props.insert("padding_x", 18.0);
    props.insert("padding_y", 10.0);
    props.insert("auto_button_color", true);
    props.insert("hover_color", QColor(52, 52, 62, 255));
    props.insert("hover_transparency", 0.1);
  }
  if (type == "image_button") {
    props.insert("hover_color", QColor(255, 255, 255, 255));
    props.insert("hover_transparency", 0.35);
    props.insert("hover_size", 1.0);
  }
  if (type == "panel") {
    props.insert("size_x", 100.0);
    props.insert("size_y", 100.0);
  }
  return props;
}

ui_element element_from_json(const QJsonObject &object) {
  ui_element element;
  element.name = object.value("name").toString();
  element.type = object.value("type").toString();
  if (element.type.isEmpty()) {
    element.type = "panel";
  }
  if (element.name.isEmpty()) {
    element.name = element.type;
  }
  element.properties = default_element_properties(element.type);

  for (auto it = object.constBegin(); it != object.constEnd(); ++it) {
    const QString &key = it.key();
    if (key == "name" || key == "type") {
      continue;
    }
    QVariant current = element.properties.value(key);
    QJsonValue value = it.value();
    if (current.userType() == QMetaType::QColor) {
      element.properties.insert(key, array_to_color(value));
    } else if (current.userType() == QMetaType::Bool) {
      element.properties.insert(key, value.toBool());
    } else if (current.userType() == QMetaType::Int) {
      element.properties.insert(key, value.toInt());
    } else if (current.userType() == QMetaType::Double) {
      element.properties.insert(key, value.toDouble());
    } else {
      element.properties.insert(key, value.toString());
    }
  }
  return element;
}

QJsonObject element_to_json(const ui_element &element) {
  QJsonObject object;
  object.insert("name", element.name);
  object.insert("type", element.type);
  for (auto it = element.properties.constBegin(); it != element.properties.constEnd(); ++it) {
    const QVariant &value = it.value();
    if (value.userType() == QMetaType::QColor) {
      object.insert(it.key(), color_to_array(value.value<QColor>()));
    } else if (value.userType() == QMetaType::Bool) {
      object.insert(it.key(), value.toBool());
    } else if (value.userType() == QMetaType::Int) {
      object.insert(it.key(), value.toInt());
    } else if (value.userType() == QMetaType::Double) {
      object.insert(it.key(), value.toDouble());
    } else {
      object.insert(it.key(), value.toString());
    }
  }
  return object;
}

QVector<ui_element> default_elements() {
  QVector<ui_element> elements;

  ui_element panel;
  panel.name = "box";
  panel.type = "panel";
  panel.properties = default_element_properties("panel");
  panel.properties.insert("position_x", 50.0);
  panel.properties.insert("position_y", 50.0);
  panel.properties.insert("size_x", 420.0);
  panel.properties.insert("size_y", 260.0);
  elements.append(panel);

  ui_element title;
  title.name = "title_label";
  title.type = "text";
  title.properties = default_element_properties("text");
  title.properties.insert("text", "Inferno");
  title.properties.insert("text_size", 48.0);
  title.properties.insert("position_x", 50.0);
  title.properties.insert("position_y", 30.0);
  elements.append(title);

  ui_element play;
  play.name = "start_button";
  play.type = "button";
  play.properties = default_element_properties("button");
  play.properties.insert("text", "Play");
  play.properties.insert("text_size", 32.0);
  play.properties.insert("position_x", 50.0);
  play.properties.insert("position_y", 70.0);
  play.properties.insert("size_x", 160.0);
  play.properties.insert("size_y", 52.0);
  elements.append(play);

  ui_element icon;
  icon.name = "test_button";
  icon.type = "image_button";
  icon.properties = default_element_properties("image_button");
  icon.properties.insert("image", "test");
  icon.properties.insert("position_x", 80.0);
  icon.properties.insert("position_y", 70.0);
  icon.properties.insert("size_x", 72.0);
  icon.properties.insert("size_y", 72.0);
  elements.append(icon);

  ui_element picture;
  picture.name = "window_image";
  picture.type = "image";
  picture.properties = default_element_properties("image");
  picture.properties.insert("image", "test");
  picture.properties.insert("position_x", 20.0);
  picture.properties.insert("position_y", 30.0);
  picture.properties.insert("size_x", 96.0);
  picture.properties.insert("size_y", 96.0);
  elements.append(picture);

  return elements;
}
