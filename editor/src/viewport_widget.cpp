#include "viewport_widget.h"
#include "ui.h"
#include <SDL3/SDL.h>
#include <QColor>
#include <QContextMenuEvent>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPen>
#include <QResizeEvent>
#include <QShowEvent>
#include <QRectF>
#include <cstdio>
#include <memory>
#include <string>
#include <json/json.h>

using namespace std;

static void set_scene_position(engine_ui_element &element, float x, float y) {
  switch (element.kind) {
    case ui_kind::panel:
      element.panel_data.properties.position_x = x;
      element.panel_data.properties.position_y = y;
      break;
    case ui_kind::text:
      element.text_data.properties.position_x = x;
      element.text_data.properties.position_y = y;
      break;
    case ui_kind::image:
      element.image_data.properties.position_x = x;
      element.image_data.properties.position_y = y;
      break;
    case ui_kind::button:
      element.button_data.properties.position_x = x;
      element.button_data.properties.position_y = y;
      break;
    case ui_kind::image_button:
      element.image_button_data.properties.position_x = x;
      element.image_button_data.properties.position_y = y;
      break;
  }
}

viewport_widget::viewport_widget(QWidget *parent) : QWidget(parent) {
  setMinimumSize(400, 300);
  setFocusPolicy(Qt::StrongFocus);
  setMouseTracking(true);
  setAttribute(Qt::WA_OpaquePaintEvent, true);
  setAutoFillBackground(false);
  setStyleSheet("background-color: #17191b;");
}

viewport_widget::~viewport_widget() {
  delete d_scene;
  if (d_renderer) {
    SDL_DestroyRenderer(d_renderer);
  }
  if (d_sdl_window) {
    SDL_DestroyWindow(d_sdl_window);
  }
}

void viewport_widget::set_elements(QVector<ui_element> *elements) {
  d_elements = elements;
  refresh();
}

void viewport_widget::set_asset_base_path(const QString &base) {
  QString path = base;
  if (!path.endsWith('/')) {
    path += '/';
  }
  d_asset_base_path = path;
  ::set_asset_base_path(path.toStdString());
  refresh();
}

void viewport_widget::set_selected(int index) {
  d_selected = index;
  render_frame();
}

QRectF viewport_widget::canvas_rect() const {
  const double ratio = 800.0 / 600.0;
  double available_width = width();
  double available_height = height();
  double canvas_width = available_width;
  double canvas_height = canvas_width / ratio;
  if (canvas_height > available_height) {
    canvas_height = available_height;
    canvas_width = canvas_height * ratio;
  }
  return QRectF((available_width - canvas_width) / 2.0,
                (available_height - canvas_height) / 2.0,
                canvas_width, canvas_height);
}

static QRectF logical_rect_from_properties(const QMap<QString, QVariant> &properties,
                                           const QString &type) {
  double px = properties.value("position_x", 50.0).toDouble();
  double py = properties.value("position_y", 50.0).toDouble();
  double sx = properties.value("size_x", 0.0).toDouble();
  double sy = properties.value("size_y", 0.0).toDouble();
  double ax = properties.value("anchor_x", 0.5).toDouble();
  double ay = properties.value("anchor_y", 0.5).toDouble();

  if (sx <= 0.0) {
    if (type == "text") {
      double text_size = properties.value("text_size", 24.0).toDouble();
      sx = qMax(40.0, properties.value("text").toString().length() * text_size * 0.6);
    } else if (type == "image" || type == "image_button") {
      sx = 64.0;
    } else if (type == "button") {
      sx = 120.0;
    } else {
      sx = 100.0;
    }
  }
  if (sy <= 0.0) {
    if (type == "text") {
      sy = properties.value("text_size", 24.0).toDouble() * 1.4;
    } else if (type == "image" || type == "image_button") {
      sy = 64.0;
    } else if (type == "button") {
      sy = 40.0;
    } else {
      sy = 100.0;
    }
  }

  double center_x = 800.0 * px / 100.0;
  double center_y = 600.0 * py / 100.0;
  return QRectF(center_x - sx * ax,
                center_y - sy * ay,
                sx, sy);
}

QRectF viewport_widget::element_rect(const ui_element &element) const {
  QRectF canvas = canvas_rect();
  double scale = canvas.width() / 800.0;
  QRectF logical = logical_rect_from_properties(element.properties, element.type);
  return QRectF(canvas.left() + logical.left() * scale,
                canvas.top() + logical.top() * scale,
                logical.width() * scale,
                logical.height() * scale);
}

QRectF viewport_widget::logical_element_rect(const ui_element &element) const {
  return logical_rect_from_properties(element.properties, element.type);
}

int viewport_widget::hit_test(const QPoint &pos) const {
  if (!d_elements) {
    return -1;
  }
  for (int i = d_elements->size() - 1; i >= 0; --i) {
    const ui_element &element = d_elements->at(i);
    if (!element.properties.value("visible", true).toBool()) {
      continue;
    }
    if (element_rect(element).contains(pos)) {
      return i;
    }
  }
  return -1;
}

bool viewport_widget::ensure_offscreen() {
  if (d_renderer) {
    return true;
  }
  if (!SDL_Init(SDL_INIT_VIDEO)) {
    printf(">> sdl cannot be started.. ERROR\n");
    return false;
  }

  SDL_Window *window = SDL_CreateWindow("Infernum Preview (offscreen)", 800, 600,
                                        SDL_WINDOW_HIDDEN);
  if (!window) {
    printf(">> cannot create offscreen sdl window: %s.. ERROR\n", SDL_GetError());
    return false;
  }
  d_sdl_window = window;
  d_renderer = SDL_CreateRenderer(window, "software");
  if (!d_renderer) {
    printf(">> cannot create software renderer: %s.. ERROR\n", SDL_GetError());
    return false;
  }
  SDL_SetRenderLogicalPresentation(d_renderer, 800, 600,
                                   SDL_LOGICAL_PRESENTATION_LETTERBOX);
  return true;
}

bool viewport_widget::rebuild_scene() {
  if (!d_renderer || !d_elements) {
    return false;
  }

  QJsonArray array;
  for (const ui_element &element : *d_elements) {
    array.append(element_to_json(element));
  }
  QJsonObject object;
  object.insert("ui", array);
  QByteArray data = QJsonDocument(object).toJson(QJsonDocument::Compact);

  Json::CharReaderBuilder builder;
  Json::Value root;
  string errors;
  unique_ptr<Json::CharReader> reader(builder.newCharReader());
  if (!reader->parse(data.constData(), data.constData() + data.size(),
                     &root, &errors)) {
    printf(">> cannot build preview json: %s\n", errors.c_str());
    return false;
  }

  delete d_scene;
  d_scene = new ui_scene();
  if (!d_scene->load_root(d_renderer, root)) {
    delete d_scene;
    d_scene = nullptr;
    d_scene_ready = false;
    return false;
  }
  d_scene_ready = true;
  return true;
}

void viewport_widget::render_frame() {
  if (!d_renderer) {
    return;
  }

  SDL_SetRenderLogicalPresentation(d_renderer, 800, 600,
                                   SDL_LOGICAL_PRESENTATION_LETTERBOX);
  SDL_SetRenderDrawColor(d_renderer, 12, 12, 16, 255);
  SDL_RenderClear(d_renderer);

  if (d_scene_ready && d_scene) {
    d_scene->render(d_renderer);
  }

  if (d_selected >= 0 && d_elements && d_selected < d_elements->size()) {
    const ui_element &element = d_elements->at(d_selected);
    if (element.properties.value("visible", true).toBool()) {
      QRectF box = logical_element_rect(element);
      SDL_FRect outline = {static_cast<float>(box.left()) - 2.0f,
                           static_cast<float>(box.top()) - 2.0f,
                           static_cast<float>(box.width()) + 4.0f,
                           static_cast<float>(box.height()) + 4.0f};
      SDL_SetRenderDrawColor(d_renderer, 255, 204, 0, 255);
      SDL_RenderRect(d_renderer, &outline);
    }
  }

  SDL_Surface *surface = SDL_RenderReadPixels(d_renderer, nullptr);
  if (surface) {
    SDL_Surface *working = nullptr;
    if (surface->format == SDL_PIXELFORMAT_ARGB8888) {
      working = surface;
    } else {
      working = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_ARGB8888);
    }
    if (working) {
      d_image = QImage(static_cast<const uchar *>(working->pixels),
                       working->w, working->h, working->pitch,
                       QImage::Format_ARGB32)
                    .copy();
      if (working != surface) {
        SDL_DestroySurface(working);
      }
    }
    SDL_DestroySurface(surface);
  }
  update();
}

void viewport_widget::refresh() {
  if (!ensure_offscreen()) {
    return;
  }
  d_scene_ready = false;
  if (rebuild_scene()) {
    render_frame();
  }
}

void viewport_widget::paintEvent(QPaintEvent *event) {
  Q_UNUSED(event);
  QPainter painter(this);
  painter.fillRect(rect(), QColor("#17191b"));

  QRectF canvas = canvas_rect();
  painter.fillRect(canvas, QColor("#0c0c10"));
  if (!d_image.isNull()) {
    painter.drawImage(canvas, d_image);
  }
  painter.setPen(QPen(QColor("#3a3d40"), 1));
  painter.drawRect(canvas);
}

void viewport_widget::mousePressEvent(QMouseEvent *event) {
  if (event->button() != Qt::LeftButton) {
    return;
  }
  int index = hit_test(event->pos());
  if (index >= 0) {
    d_selected = index;
    d_dragging = true;
    d_last_pos = event->pos();
    emit element_selected(index);
  } else {
    d_selected = -1;
  }
  render_frame();
}

void viewport_widget::mouseMoveEvent(QMouseEvent *event) {
  if (!d_dragging || d_selected < 0 || !d_elements) {
    return;
  }
  QRectF canvas = canvas_rect();
  if (canvas.width() <= 0.0 || canvas.height() <= 0.0) {
    return;
  }
  QPoint delta = event->pos() - d_last_pos;
  d_last_pos = event->pos();

  ui_element &element = (*d_elements)[d_selected];
  double dx = delta.x() * 100.0 / canvas.width();
  double dy = delta.y() * 100.0 / canvas.height();
  double new_x = element.properties.value("position_x", 50.0).toDouble() + dx;
  double new_y = element.properties.value("position_y", 50.0).toDouble() + dy;
  element.properties.insert("position_x", new_x);
  element.properties.insert("position_y", new_y);

  emit element_moved(d_selected);
  bool updated = false;
  if (d_scene_ready && d_scene) {
    engine_ui_element *live = d_scene->find(element.name.toStdString());
    if (live) {
      set_scene_position(*live, (float)new_x, (float)new_y);
      updated = true;
    }
  }
  if (updated) {
    render_frame();
  } else {
    d_scene_ready = false;
    if (rebuild_scene()) {
      render_frame();
    }
  }
}

void viewport_widget::mouseReleaseEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    d_dragging = false;
  }
}

void viewport_widget::contextMenuEvent(QContextMenuEvent *event) {
  int index = hit_test(event->pos());
  if (index < 0) {
    return;
  }
  d_selected = index;
  emit element_selected(index);
  render_frame();
  emit element_context_menu(index, event->globalPos());
}

void viewport_widget::resizeEvent(QResizeEvent *event) {
  QWidget::resizeEvent(event);
  update();
}

void viewport_widget::showEvent(QShowEvent *event) {
  QWidget::showEvent(event);
  ensure_offscreen();
  render_frame();
}
