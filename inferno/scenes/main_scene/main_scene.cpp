#include "main_scene.h"
#include "engine.h"
#include "game.h"
#include "ui.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_render.h>
#include <cstdio>

static ui_scene scene_ui;
static bool ui_ready = false;
static button* start_button = nullptr;
static tween move_tween;
static tween scale_up;
static tween scale_down;

void main_scene(SDL_Renderer* scene_renderer)
{
    if (!scene_renderer) {
        printf(">> scene has no renderer.. ERROR\n");
        return;
    }
    if (!ui_ready) {
        play_sound("lfs");
        scene_ui.load(scene_renderer, "game_properties.json", "main_scene");
        engine_ui_element* element = scene_ui.find("start_button");

        if (element && element->kind == ui_kind::button) {
            start_button = &element->button_data;
            move_tween = create_tween(
                *start_button,
                tween_info(0.2f, easing_style::bounce, easing_direction::in_out),
                { { "x", 50.0f },
                    { "y", 50.0f } });
            scale_up = create_tween(
                *start_button,
                tween_info(0.2f, easing_style::cubic, easing_direction::in_out),
                { { "scale", 1.1f } });
            scale_down = create_tween(
                *start_button,
                tween_info(0.2f, easing_style::cubic, easing_direction::in_out),
                { { "scale", 1.0f } });
        }

        ui_ready = true;
    }
    SDL_Color background = { 12, 12, 16, 255 };
    render_frame(scene_renderer, 100.0f, 100.0f, 50.0f, 50.0f, background);
    if (start_button) {
        if (start_button->clicked) {
            play_sound("press");
            printf("clciked!");
            load_scene("lol");
        }
        if (start_button->m_enter) {
            play_sound("hover");
            scale_up.play();
        }
        if (start_button->m_leave) {
            scale_down.play();
        }
    }

    scene_ui.render(scene_renderer);
}
