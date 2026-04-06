#include "bloom.h"

#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#include <GL/gl.h>
#endif

typedef struct showcase_state
{
    bloom_bool show_debug;
    bloom_bool checkbox_value;
    bloom_bool toggle_value;
    bloom_bool radio_active;

    bloom_f32 slider_f;
    bloom_i32 slider_i;
    bloom_f32 slider_bar;
    bloom_i32 slider_bar_i;

    char text_input[128];
    char text_area[512];

    bloom_i32 int_value;
    bloom_f32 float_value;
    bloom_f64 precise_value;
    bloom_i32 int_scrub;
    bloom_f32 float_scrub;
    bloom_i32 int_min;
    bloom_i32 int_max;
    bloom_f32 float_min;
    bloom_f32 float_max;

    bloom_i32 combo_selected;
    bloom_i32 choice_selected;
    bloom_bool action_primary_pressed;

    char filter_buf[64];
    bloom_i32 filter_selected;

    bloom_bool multi_selected[6];

    bloom_f32 color3[3];
    bloom_f32 color4[4];

    bloom_f32 progress;
    bloom_bool tree_open;

    bloom_f32 split_left;
    bloom_f32 split_right;

    bloom_u32 demo_image_tex;
} showcase_state;

static void showcase_init(showcase_state *state)
{
    memset(state, 0, sizeof(*state));
    state->checkbox_value = BLOOM_TRUE;
    state->toggle_value = BLOOM_TRUE;
    state->radio_active = BLOOM_TRUE;
    state->slider_f = 0.45f;
    state->slider_i = 42;
    state->slider_bar = 0.7f;
    state->slider_bar_i = 33;
    state->int_value = 10;
    state->float_value = 3.14f;
    state->precise_value = 9.8125;
    state->int_scrub = 25;
    state->float_scrub = 0.25f;
    state->int_min = 2;
    state->int_max = 8;
    state->float_min = 0.2f;
    state->float_max = 0.9f;
    state->combo_selected = 0;
    state->choice_selected = 0;
    state->filter_selected = 0;
    state->progress = 0.0f;
    state->tree_open = BLOOM_TRUE;
    state->split_left = 300.0f;
    state->split_right = 300.0f;

    strcpy(state->text_input, "Hello Bloom");
    strcpy(state->text_area, "This is a multiline text area.\nEdit me and try keyboard shortcuts.");
    strcpy(state->filter_buf, "");
}

static void showcase_draw(showcase_state *state)
{
    static const char *combo_items[] = {"Alpha", "Beta", "Gamma", "Delta"};
    static const char *choice_items[] = {"One", "Two", "Three", "Four"};
    static const char *multi_items[] = {"Red", "Green", "Blue", "Orange", "Cyan", "Magenta"};
    bloom_slider_args slider_args = BLOOM_SLIDER_ARGS_DEFAULT;
    bloom_toggle_args toggle_args = BLOOM_TOGGLE_ARGS_DEFAULT;
    bloom_combo_args combo_args = BLOOM_COMBO_ARGS_DEFAULT;
    char value_line[64];
    bloom_i32 i;

    bloom->set_next_window_pos(28.0f, 24.0f);
    bloom->set_next_window_size(1240.0f, 760.0f);

    if (bloom->begin("Bloom Widget Showcase"))
    {
        bloom->text("Text Widgets");
        bloom->text_wrapped("This demo intentionally touches every widget API so library consumers have a one-stop integration sample.");
        bloom->text_colored("Colored text sample", bloom->rgba(255, 180, 80, 255));
        bloom->text_disabled("Disabled style text");
        bloom->separator();

        bloom->text("Buttons and Choice Widgets");
        if (bloom->button("Button"))
        {
            state->progress += 0.05f;
        }
        bloom->same_line();
        bloom->button_sized("Button Sized", 140.0f, 30.0f);
        bloom->same_line();
        bloom->button_mini("Mini");
        bloom->same_line();
        bloom->button_ghost("Ghost", 92.0f, 30.0f);
        bloom->same_line();
        bloom->button_direction("Dir", BLOOM_DIRECTION_RIGHT);

        bloom->choice_strip("Choice Strip", choice_items, 4, &state->choice_selected);
        bloom->list_bullet("Bullet List Entry");
        bloom->separator();

        bloom->text("Boolean Widgets");
        bloom->checkbox("Checkbox", &state->checkbox_value);
        bloom->toggle("Toggle", &state->toggle_value);
        bloom->toggle_ex("Toggle Ex", &state->toggle_value, &toggle_args);
        if (bloom->radio_button("Radio Button", state->radio_active))
        {
            state->radio_active = !state->radio_active;
        }
        bloom->separator();

        bloom->text("Slider Widgets");
        bloom->slider_float("Slider Float", &state->slider_f, 0.0f, 1.0f);
        bloom->slider_int("Slider Int", &state->slider_i, 0, 100);
        bloom->slider_float_ex("Slider Float Ex", &state->slider_f, 0.0f, 1.0f, &slider_args);
        bloom->slider_int_ex("Slider Int Ex", &state->slider_i, 0, 100, &slider_args);
        bloom->slider_float_bar("Slider Float Bar", &state->slider_bar, 0.0f, 1.0f, &slider_args);
        bloom->slider_int_bar("Slider Int Bar", &state->slider_bar_i, 0, 100, &slider_args);
        bloom->slider_float_tall("Slider Float Tall", &state->slider_f, 0.0f, 1.0f, 90.0f);
        bloom->slider_int_tall("Slider Int Tall", &state->slider_i, 0, 100, 90.0f);
        bloom->separator();

        bloom->text("Input Widgets");
        bloom->text_input("Text Input", state->text_input, (bloom_u32)sizeof(state->text_input));
        bloom->text_area("Text Area", state->text_area, (bloom_u32)sizeof(state->text_area), 4);
        bloom->int_field("Int Field", &state->int_value, 1);
        bloom->float_field("Float Field", &state->float_value, 0.1f);
        bloom->precise_field("Precise Field", &state->precise_value, 0.0625);
        bloom->value_field("Value Field", &state->precise_value, BLOOM_VALUE_KIND_DOUBLE, 0.125);
        bloom->int_scrub("Int Scrub", &state->int_scrub, -100, 100, 0.5f);
        bloom->float_scrub("Float Scrub", &state->float_scrub, -1.0f, 1.0f, 0.01f);
        bloom->int_span("Int Span", &state->int_min, &state->int_max, 1);
        bloom->float_span("Float Span", &state->float_min, &state->float_max, 0.01f);
        bloom->separator();

        bloom->text("Selection Widgets");
        if (bloom->combo_begin("Combo", combo_items[state->combo_selected]))
        {
            for (i = 0; i < 4; ++i)
            {
                if (bloom->combo_item(combo_items[i], state->combo_selected == i))
                {
                    state->combo_selected = i;
                }
            }
            bloom->combo_end();
        }

        if (bloom->combo_begin_ex("Combo Ex", combo_items[state->combo_selected], 5))
        {
            for (i = 0; i < 4; ++i)
            {
                if (bloom->combo_item(combo_items[i], state->combo_selected == i))
                {
                    state->combo_selected = i;
                }
            }
            bloom->combo_end();
        }

        if (bloom->combo_begin_args("Combo Args", combo_items[state->combo_selected], &combo_args))
        {
            for (i = 0; i < 4; ++i)
            {
                if (bloom->combo_item(combo_items[i], state->combo_selected == i))
                {
                    state->combo_selected = i;
                }
            }
            bloom->combo_end();
        }

        if (bloom->action_split_begin("Action Split", &state->action_primary_pressed, 4))
        {
            bloom->action_split_item("Do Thing");
            bloom->action_split_item("Do Other Thing");
            bloom->action_split_item("Export");
            bloom->action_split_end();
        }

        if (bloom->filter_select_begin("Filter Select", combo_items[state->filter_selected],
                                       state->filter_buf, (bloom_u32)sizeof(state->filter_buf), 6))
        {
            for (i = 0; i < 4; ++i)
            {
                if (bloom->filter_select_item(combo_items[i], state->filter_selected == i))
                {
                    state->filter_selected = i;
                }
            }
            bloom->filter_select_end();
        }

        if (bloom->filter_select_begin_args("Filter Select Args", combo_items[state->filter_selected],
                                            state->filter_buf, (bloom_u32)sizeof(state->filter_buf), &combo_args))
        {
            for (i = 0; i < 4; ++i)
            {
                if (bloom->filter_select_item(combo_items[i], state->filter_selected == i))
                {
                    state->filter_selected = i;
                }
            }
            bloom->filter_select_end();
        }

        if (bloom->multi_select_begin("Multi Select", "Choose multiple", 6))
        {
            for (i = 0; i < 6; ++i)
            {
                bloom_bool selected = state->multi_selected[i];
                if (bloom->multi_select_item(multi_items[i], selected))
                {
                    state->multi_selected[i] = !state->multi_selected[i];
                }
            }
            bloom->multi_select_end();
        }

        if (bloom->multi_select_begin_args("Multi Select Args", "Choose multiple", &combo_args))
        {
            for (i = 0; i < 6; ++i)
            {
                bloom_bool selected = state->multi_selected[i];
                if (bloom->multi_select_item(multi_items[i], selected))
                {
                    state->multi_selected[i] = !state->multi_selected[i];
                }
            }
            bloom->multi_select_end();
        }

        bloom->separator();
        bloom->text("Color Widgets");
        bloom->color_edit3("Color Edit3", state->color3);
        bloom->color_edit4("Color Edit4", state->color4);
        bloom->color_edit_rgb("Color Edit RGB", state->color3, BLOOM_COLOR_FLAGS_NONE);
        bloom->color_edit_rgba("Color Edit RGBA", state->color4, BLOOM_COLOR_FLAGS_NONE);
        bloom->color_pick_rgb("Color Pick RGB", state->color3, BLOOM_COLOR_FLAGS_NONE);
        bloom->color_pick_rgba("Color Pick RGBA", state->color4, BLOOM_COLOR_FLAGS_NONE);
        bloom->color_swatch("Color Swatch", state->color4, 120.0f, 18.0f);

        bloom->separator();
        bloom->text("Table and Tree Widgets");
        bloom->begin_table("Table", 3);
        bloom->table_header("Name");
        bloom->table_header("Type");
        bloom->table_header("Value");

        bloom->table_next_row();
        bloom->table_next_column();
        bloom->text("slider_f");
        bloom->table_next_column();
        bloom->text("float");
        bloom->table_next_column();
        snprintf(value_line, sizeof(value_line), "%.3f", state->slider_f);
        bloom->text(value_line);

        bloom->table_next_row();
        bloom->table_next_column();
        bloom->text("int_value");
        bloom->table_next_column();
        bloom->text("int");
        bloom->table_next_column();
        bloom->text("editable above");
        bloom->end_table();

        if (bloom->tree_node("Tree Node"))
        {
            bloom->text("Nested text item");
            bloom->tree_pop();
        }

        bloom->collapsing_header("Collapsing Header");
        bloom->separator();

        bloom->text("Utility Widgets (hover for tooltip)");
        bloom->tooltip("Tooltip shows on hover of the preceding widget");

        state->progress += 0.002f;
        if (state->progress > 1.0f)
        {
            state->progress = 0.0f;
        }
        bloom->progress_bar(state->progress, 220.0f, 18.0f);

        if (bloom->hyperlink("https://github.com/bloom-ui/bloom"))
        {
            state->show_debug = !state->show_debug;
        }

        bloom->spinner("Spinner", 10.0f);

        if (state->demo_image_tex != 0)
        {
            bloom->image(state->demo_image_tex, 48.0f, 48.0f);
            bloom->image_button("Image Button", state->demo_image_tex, 48.0f, 48.0f);
        }

        bloom->text_selectable("Selectable text: drag to inspect behavior");

        bloom->separator();
        bloom->text("Flow and Splitter Widgets");
        bloom->begin_flow();
        bloom->button("Flow A");
        bloom->button("Flow B");
        bloom->button("Flow C");
        bloom->button("Flow D");
        bloom->end_flow();

        bloom->splitter(BLOOM_TRUE, 5.0f,
                        &state->split_left, &state->split_right,
                        160.0f, 160.0f);

        bloom->show_debug_overlay(state->show_debug);
        bloom->end();
    }
}

int main(void)
{
    bloom_context *ctx;
    bloom_platform_window *window;
    bloom_render_backend *backend;
    bloom_platform_config config;
    bloom_font *default_font;
    showcase_state state;
    bloom_f64 previous_time;

    showcase_init(&state);

    ctx = bloom->create_context();
    if (!ctx)
    {
        fprintf(stderr, "Failed to create bloom context.\n");
        return 1;
    }

    bloom->set_context(ctx);

    config.title = "Bloom Widgets Showcase";
    config.width = 1280;
    config.height = 800;
    config.flags = BLOOM_PLATFORM_WINDOW_FLAG_DEFAULT;
    config.opacity = 1.0f;
    config.resizable = BLOOM_TRUE;
    config.transparent = BLOOM_FALSE;
    config.topmost = BLOOM_FALSE;
    config.borderless = BLOOM_FALSE;

    window = bloom->platform.create(&config);
    if (!window)
    {
        fprintf(stderr, "Failed to create window.\n");
        bloom->destroy_context(ctx);
        return 1;
    }

    backend = bloom->render.create_opengl_backend();
    if (!backend || !backend->init(backend))
    {
        fprintf(stderr, "Failed to initialize OpenGL backend.\n");
        if (backend)
        {
            bloom->render.destroy_opengl_backend(backend);
        }
        bloom->platform.destroy(window);
        bloom->destroy_context(ctx);
        return 1;
    }

    default_font = bloom->get_default_font();
    if (default_font && default_font->atlas_pixels)
    {
        default_font->texture_id = backend->create_texture(
            backend,
            default_font->atlas_width,
            default_font->atlas_height,
            default_font->atlas_pixels);
    }
    state.demo_image_tex = default_font ? default_font->texture_id : 0;

    previous_time = bloom->platform.get_time();

    while (bloom->platform.poll(window))
    {
        bloom_i32 width = 0;
        bloom_i32 height = 0;
        bloom_f64 now = bloom->platform.get_time();
        bloom_f32 dt = (bloom_f32)(now - previous_time);
        previous_time = now;

        bloom->platform.get_size(window, &width, &height);
        bloom->set_display_size((bloom_f32)width, (bloom_f32)height);
        bloom->set_delta_time(dt > 0.0f ? dt : 1.0f / 60.0f);

#ifdef _WIN32
        glViewport(0, 0, width, height);
        glClearColor(0.10f, 0.12f, 0.14f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
#endif

        bloom->begin_frame();
        showcase_draw(&state);
        bloom->end_frame();

        backend->render(backend, bloom->get_draw_list(), (bloom_f32)width, (bloom_f32)height);
        bloom->platform.swap(window);
    }

    if (default_font && default_font->texture_id != 0)
    {
        backend->destroy_texture(backend, default_font->texture_id);
        default_font->texture_id = 0;
    }

    bloom->render.destroy_opengl_backend(backend);
    bloom->platform.destroy(window);
    bloom->destroy_context(ctx);

    return 0;
}

// xmake f --shared=y
// xmake f --d3d11=y