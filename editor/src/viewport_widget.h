#ifndef VIEWPORT_WIDGET_H
#define VIEWPORT_WIDGET_H

#include "ui_element.h"
#include <QImage>
#include <QPoint>
#include <QRectF>
#include <QString>
#include <QVector>
#include <QWidget>

typedef struct SDL_Renderer SDL_Renderer;
typedef struct SDL_Window SDL_Window;
struct ui_scene;

class viewport_widget : public QWidget {
  Q_OBJECT
public:
  explicit viewport_widget(QWidget *parent = nullptr);
  ~viewport_widget() override;

  void set_elements(QVector<ui_element> *elements);
  void set_asset_base_path(const QString &base);
  void set_selected(int index);
  int selected() const { return d_selected; }
  QRectF canvas_rect() const;
  void refresh();

signals:
  void element_selected(int index);
  void element_moved(int index);
  void element_context_menu(int index, const QPoint &global_position);

protected:
  void paintEvent(QPaintEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void contextMenuEvent(QContextMenuEvent *event) override;
  void resizeEvent(QResizeEvent *event) override;
  void showEvent(QShowEvent *event) override;

private:
  QRectF element_rect(const ui_element &element) const;
  QRectF logical_element_rect(const ui_element &element) const;
  int hit_test(const QPoint &pos) const;
  bool ensure_offscreen();
  bool rebuild_scene();
  void render_frame();

  QVector<ui_element> *d_elements = nullptr;
  QString d_asset_base_path;
  int d_selected = -1;
  bool d_dragging = false;
  QPoint d_last_pos;
  SDL_Window *d_sdl_window = nullptr;
  SDL_Renderer *d_renderer = nullptr;
  ui_scene *d_scene = nullptr;
  bool d_scene_ready = false;
  QImage d_image;
};

#endif
