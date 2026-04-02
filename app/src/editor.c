#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_RESIZE2_IMPLEMENTATION
#define STBIR_ASSERT(x) ((void)0)

#include "editor.h"
#include <stb_image.h>
#include <stb_image_write.h>
#include <stb_image_resize2.h>
#include <yaml.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char output_fmt[8];
    int width, height, channels;
    char edge[8];
} editor_task_t;

static int parse_yaml_mem(const char *yaml_buf, size_t yaml_len, editor_task_t *task) {
    FILE *f = fmemopen((void*)yaml_buf, yaml_len, "rb");
    if (!f) {
        fprintf(stderr, "OOM: could not open YAML memory\n");
        return 1;
    }
    yaml_parser_t parser;
    yaml_event_t event;
    yaml_parser_initialize(&parser);
    yaml_parser_set_input_file(&parser, f);

    int in_commands = 0, in_resize = 0;
    char key[64] = {0};
    while (yaml_parser_parse(&parser, &event)) {
        switch (event.type) {
        case YAML_SCALAR_EVENT: {
            const char *val = (const char *)event.data.scalar.value;
            if (key[0]) {
                if (strcmp(key, "output") == 0) {
                    strncpy(task->output_fmt, val, sizeof(task->output_fmt));
                } else if (in_resize) {
                    if (strcmp(key, "width") == 0)      task->width = atoi(val);
                    else if (strcmp(key, "height") == 0) task->height = atoi(val);
                    else if (strcmp(key, "channels") == 0) task->channels = atoi(val);
                    else if (strcmp(key, "edge") == 0) strncpy(task->edge, val, sizeof(task->edge));
                }
                key[0] = 0;
            } else if (strcmp(val, "output") == 0) {
                strncpy(key, "output", sizeof(key));
            } else if (strcmp(val, "commands") == 0) {
                in_commands = 1;
            } else if (strcmp(val, "resize") == 0) {
                in_resize = 1;
            } else if (in_resize) {
                strncpy(key, val, sizeof(key));
            }
            break;
        }
        case YAML_MAPPING_END_EVENT:
            if (in_resize) in_resize = 0;
            break;
        case YAML_STREAM_END_EVENT:
            goto done;
        default:
            break;
        }
        yaml_event_delete(&event);
    }
done:
    yaml_parser_delete(&parser);
    fclose(f);
    return 0;
}

static void file_write_callback(void *context, void *data, int size) {
    fwrite(data, 1, size, (FILE*)context);
}

int editor_process(
    const unsigned char *image, size_t image_len,
    const char *yaml, size_t yaml_len,
    FILE *outfile
) {
    editor_task_t task = {0};
    if (parse_yaml_mem(yaml, yaml_len, &task)) {
        fprintf(stderr, "Failed to parse YAML\n");
        return 1;
    }
    if (!*task.output_fmt) {
        fprintf(stderr, "YAML missing output format\n");
        return 2;
    }

    int width, height, channels;
    unsigned char *data = stbi_load_from_memory(image, image_len, &width, &height, &channels, 0);
    if (!data) {
        fprintf(stderr, "Failed to load image from buffer\n");
        return 3;
    }
    fprintf(stderr, "Loaded image: %dx%d (%d channels)\n", width, height, channels);

    int out_w = task.width > 0 ? task.width : width;
    int out_h = task.height > 0 ? task.height : height;
    int out_c = task.channels > 0 ? task.channels : channels;
    unsigned char *out_img = malloc(out_w * out_h * out_c);
    if (!out_img) {
        fprintf(stderr, "OOM allocating out_img\n");
        stbi_image_free(data);
        return 4;
    }

    int edge_mode = STBIR_EDGE_CLAMP;
    if (strcmp(task.edge, "wrap") == 0) edge_mode = STBIR_EDGE_WRAP;
    else if (strcmp(task.edge, "reflect") == 0) edge_mode = STBIR_EDGE_REFLECT;

    int pixel_layout = (out_c == 4) ? STBIR_RGBA : STBIR_RGB;

    stbir_resize(
        data, width, height, 0,
        out_img, out_w, out_h, 0,
        pixel_layout,
        STBIR_TYPE_UINT8,
        edge_mode,
        STBIR_FILTER_DEFAULT
    );

    int wrote = 0;
    if (strcmp(task.output_fmt, "jpg") == 0 || strcmp(task.output_fmt, "jpeg") == 0) {
        wrote = stbi_write_jpg_to_func(
            file_write_callback, outfile, out_w, out_h, out_c, out_img, 90);
    } else if (strcmp(task.output_fmt, "png") == 0) {
        wrote = stbi_write_png_to_func(
            file_write_callback, outfile, out_w, out_h, out_c, out_img, out_w * out_c);
    } else if (strcmp(task.output_fmt, "bmp") == 0) {
        wrote = stbi_write_bmp_to_func(
            file_write_callback, outfile, out_w, out_h, out_c, out_img);
    } else {
        fprintf(stderr, "Unknown output format: %s\n", task.output_fmt);
        stbi_image_free(data);
        free(out_img);
        return 5;
    }
    if (!wrote) {
        fprintf(stderr, "Image write failed\n");
        stbi_image_free(data);
        free(out_img);
        return 6;
    }
    stbi_image_free(data);
    free(out_img);
    return 0;
}
