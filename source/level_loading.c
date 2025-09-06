#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <math.h>

#include "objects.h"
#include "level_loading.h"

#include "main.h"
#include "math.h"

#include "level.h"
#include "player.h"
#include "filesystem.h"

#include "../libraries/color.h"

#include "groups.h"

#include "bg_01_png.h"
#include "bg_02_png.h"
#include "bg_03_png.h"
#include "bg_04_png.h"
#include "bg_05_png.h"
#include "bg_06_png.h"
#include "bg_07_png.h"

#include "g_01_png.h"
#include "g_02_png.h"
#include "g_03_png.h"
#include "g_04_png.h"
#include "g_05_png.h"
#include "g_06_png.h"
#include "g_07_png.h"

const unsigned char *backgrounds[] = {
    bg_01_png,
    bg_02_png,
    bg_03_png,
    bg_04_png,
    bg_05_png,
    bg_06_png,
    bg_07_png,
};

const unsigned char *grounds[] = {
    g_01_png,
    g_02_png,
    g_03_png,
    g_04_png,
    g_05_png,
    g_06_png,
    g_07_png,
};

struct LoadedLevelInfo level_info;

Section *section_hash[SECTION_HASH_SIZE] = {0};
GFXSection *section_gfx_hash[SECTION_HASH_SIZE] = {0};

GDLayerSortable gfx_player_layer;
GameObject *player_game_object;

GDLayerSortable *sortable_list;

unsigned int section_hash_func(int x, int y) {
    return ((unsigned int)x * 73856093u ^ (unsigned int)y * 19349663u) % SECTION_HASH_SIZE;
}

Section *get_or_create_section(int x, int y) {
    unsigned int h = section_hash_func(x, y);
    Section *sec = section_hash[h];
    while (sec) {
        if (sec->x == x && sec->y == y) return sec;
        sec = sec->next;
    }
    sec = malloc(sizeof(Section));
    sec->objects = malloc(sizeof(GameObject*) * 8);
    sec->object_count = 0;
    sec->object_capacity = 8;
    sec->x = x;
    sec->y = y;
    sec->next = section_hash[h];
    section_hash[h] = sec;
    return sec;
}

GFXSection *get_or_create_gfx_section(int x, int y) {
    unsigned int h = section_hash_func(x, y);
    GFXSection *sec = section_gfx_hash[h];
    while (sec) {
        if (sec->x == x && sec->y == y) return sec;
        sec = sec->next;
    }
    sec = malloc(sizeof(GFXSection));
    sec->layers = malloc(sizeof(GDLayerSortable*) * 8);
    sec->layer_count = 0;
    sec->layer_capacity = 8;
    sec->x = x;
    sec->y = y;
    sec->next = section_gfx_hash[h];
    section_gfx_hash[h] = sec;
    return sec;
}

void free_sections(void) {
    for (int i = 0; i < SECTION_HASH_SIZE; i++) {
        Section *sec = section_hash[i];
        while (sec) {
            Section *next = sec->next;
            free(sec->objects);
            free(sec);
            sec = next;
        }
        section_hash[i] = NULL;
        GFXSection *sec2 = section_gfx_hash[i];
        while (sec2) {
            GFXSection *next = sec2->next;
            free(sec2->layers);
            free(sec2);
            sec2 = next;
        }
        section_gfx_hash[i] = NULL;
    }
}

void assign_object_to_section(GameObject *obj) {
    int sx = (int)(obj->x / SECTION_SIZE);
    int sy = (int)(obj->y / SECTION_SIZE);
    Section *sec = get_or_create_section(sx, sy);
    if (sec->object_count >= sec->object_capacity) {
        sec->object_capacity *= 2;
        sec->objects = realloc(sec->objects, sizeof(GameObject*) * sec->object_capacity);
    }
    sec->objects[sec->object_count++] = obj;
}

void assign_layer_to_section(GDLayerSortable *layer) {
    int sx = (int)(layer->layer->obj->x / GFX_SECTION_SIZE);
    int sy = (int)(layer->layer->obj->y / GFX_SECTION_SIZE);
    GFXSection *sec = get_or_create_gfx_section(sx, sy);
    if (sec->layer_count >= sec->layer_capacity) {
        sec->layer_capacity *= 2;
        sec->layers = realloc(sec->layers, sizeof(GDLayerSortable*) * sec->layer_capacity);
    }
    sec->layers[sec->layer_count++] = layer;
}

void update_object_section(GameObject *obj, float new_x, float new_y) {
    if (!obj) return;

    int old_sx = (int)(obj->x / SECTION_SIZE);
    int old_sy = (int)(obj->y / SECTION_SIZE);
    int new_sx = (int)(new_x / SECTION_SIZE);
    int new_sy = (int)(new_y / SECTION_SIZE);

    int old_gfx_sx = (int)(obj->x / GFX_SECTION_SIZE);
    int old_gfx_sy = (int)(obj->y / GFX_SECTION_SIZE);
    int new_gfx_sx = (int)(new_x / GFX_SECTION_SIZE);
    int new_gfx_sy = (int)(new_y / GFX_SECTION_SIZE);
    
    // Update position
    obj->x = new_x;
    obj->y = new_y;

    if (new_sx != old_sx || new_sy != old_sy) {
        // remove from old
        unsigned int old_h = section_hash_func(old_sx, old_sy);
        Section *sec = section_hash[old_h];
        while (sec) {
            if (sec->x == old_sx && sec->y == old_sy) {
                for (int i = 0; i < sec->object_count; i++) {
                    if (sec->objects[i] == obj) {
                        sec->objects[i] = sec->objects[sec->object_count - 1];
                        sec->object_count--;
                        break;
                    }
                }
                break;
            }
            sec = sec->next;
        }

        // reassign to new
        assign_object_to_section(obj);
    }

    if (new_gfx_sx != old_gfx_sx || new_gfx_sy != old_gfx_sy) {
        unsigned int old_gfx_h = section_hash_func(old_gfx_sx, old_gfx_sy);

        int count = 0;
        GDLayerSortable *layers_to_update[MAX_OBJECT_LAYERS];

        GFXSection *sec_gfx = section_gfx_hash[old_gfx_h];
        while (sec_gfx) {
            if (sec_gfx->x == old_gfx_sx && sec_gfx->y == old_gfx_sy) {
                int i = 0;
                while (i < sec_gfx->layer_count) {
                    // Remove layers from old section
                    if (sec_gfx->layers[i]->layer->obj == obj) {
                        layers_to_update[count++] = sec_gfx->layers[i];
                        // Shift elements down
                        sec_gfx->layers[i] = sec_gfx->layers[sec_gfx->layer_count - 1];
                        sec_gfx->layer_count--;
                        continue;
                    }
                    i++;
                }
                break;
            }
            sec_gfx = sec_gfx->next;
        }
        for (int i = 0; i < count; i++) {
            // Reassign layers
            assign_layer_to_section(layers_to_update[i]);
        }
    }
}

char *extract_gmd_key(const char *data, const char *key, const char *type) {
    char key_tag[32];
    snprintf(key_tag, sizeof(key_tag), "<k>%s</k>", key);
    
    char *key_pos = strstr(data, key_tag);
    if (!key_pos) {
        return NULL;
    }

    // Move past the key tag
    char *start = key_pos + strlen(key_tag);

    // Skip whitespace (spaces, tabs, newlines, etc.)
    while (*start && isspace((unsigned char)*start)) {
        start++;
    }

    char type_start_tag[16];
    snprintf(type_start_tag, sizeof(type_start_tag), "<%s>", type);

    // Confirm that the type start tag is here
    if (strncmp(start, type_start_tag, strlen(type_start_tag)) != 0) {
        printf("Expected start tag '%s' not found after key\n", type_start_tag);
        return NULL;
    }

    // Move past the type start tag
    start += strlen(type_start_tag);

    // Find the end tag
    char type_end_tag[16];
    snprintf(type_end_tag, sizeof(type_end_tag), "</%s>", type);
    char *end = strstr(start, type_end_tag);
    if (!end) {
        printf("Could not find end tag '%s'\n", type_end_tag);
        return NULL;
    }

    // Allocate and copy value
    int len = end - start;
    char *value = malloc(len + 1);
    if (!value) {
        printf("malloc for gmd key %s failed\n", key);
        return NULL;
    }
    strncpy(value, start, len);
    value[len] = '\0';
    return value;
}

int b64_char(char c) {
    if ('A' <= c && c <= 'Z') return c - 'A';
    if ('a' <= c && c <= 'z') return c - 'a' + 26;
    if ('0' <= c && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

void fix_base64_url(char *b64) {
    for (int i = 0; b64[i]; i++) {
        if (b64[i] == '-') b64[i] = '+';
        else if (b64[i] == '_') b64[i] = '/';
    }
}

int base64_decode(const char *in, unsigned char *out) {
    int len = 0;
    for (int i = 0; in[i] && in[i+1] && in[i+2] && in[i+3]; i += 4) {
        int a = b64_char(in[i]);
        int b = b64_char(in[i+1]);
        int c = in[i+2] == '=' ? 0 : b64_char(in[i+2]);
        int d = in[i+3] == '=' ? 0 : b64_char(in[i+3]);

        if (a == -1 || b == -1 || c == -1 || d == -1) {
            printf("Invalid base64 character at position %d\n", i);
            return -1;
        }

        out[len++] = (a << 2) | (b >> 4);
        if (in[i+2] != '=') out[len++] = (b << 4) | (c >> 2);
        if (in[i+3] != '=') out[len++] = (c << 6) | d;
    }
    printf("Decoded %d bytes from base64\n", len);
    return len;
}

uLongf get_uncompressed_size(unsigned char *data, int data_len) {
    z_stream strm;
    memset(&strm, 0, sizeof(strm));
    strm.next_in = data;
    strm.avail_in = data_len;

    if (inflateInit2(&strm, 15 | 32) != Z_OK) {  // auto-detect gzip/zlib
        return 0;
    }

    uLongf total_out = 0;
    unsigned char buf[4096];

    do {
        strm.next_out = buf;
        strm.avail_out = sizeof(buf);
        int ret = inflate(&strm, Z_NO_FLUSH);
        if (ret == Z_STREAM_ERROR || ret == Z_DATA_ERROR || ret == Z_MEM_ERROR) {
            inflateEnd(&strm);
            return 0;
        }
        total_out += sizeof(buf) - strm.avail_out;
        if (ret == Z_STREAM_END) break;
    } while (strm.avail_in > 0);

    inflateEnd(&strm);
    return total_out;
}


char *decompress_data(unsigned char *data, int data_len, uLongf *out_len) {
    uLongf final_size = get_uncompressed_size(data, data_len);
    printf("Decompressing to a final size of %lu bytes...\n", (unsigned long)final_size);

    z_stream strm = {0};
    strm.next_in = data;
    strm.avail_in = data_len;

    if (inflateInit2(&strm, 15 | 32) != Z_OK) {   // auto-detect gzip/zlib
        printf("Failed to initialize zlib stream for GZIP\n");
        return NULL;
    }

    // Allocate exactly enough memory (+1 for null terminator if needed)
    char *out = malloc(final_size + 1);
    if (!out) {
        printf("malloc failed for %lu bytes\n", (unsigned long)final_size);
        inflateEnd(&strm);
        return NULL;
    }

    strm.next_out = (Bytef *)out;
    strm.avail_out = final_size;

    int ret = inflate(&strm, Z_FINISH);
    if (ret != Z_STREAM_END) {
        printf("inflate failed with code %d\n", ret);
        free(out);
        inflateEnd(&strm);
        return NULL;
    }

    *out_len = strm.total_out;
    out[*out_len] = '\0'; // Null-terminate if treating as string

    inflateEnd(&strm);

    printf("Decompressed %lu bytes successfully\n", (unsigned long)*out_len);
    return out;
}

char *get_metadata_value(const char *levelString, const char *key) {
    if (!levelString || !key) return NULL;

    // Find the first semicolon, which separates metadata from objects
    const char *end = strchr(levelString, ';');
    if (!end) return NULL;

    // We'll scan only the metadata portion
    size_t metadataLen = end - levelString;
    char *metadata = malloc(metadataLen + 1);
    if (!metadata) return NULL;

    strncpy(metadata, levelString, metadataLen);
    metadata[metadataLen] = '\0';

    // Tokenize metadata by comma
    char *token = strtok(metadata, ",");
    while (token) {
        if (strcmp(token, key) == 0) {
            char *value = strtok(NULL, ",");  // get value after key
            if (!value) break;

            // Copy and return value
            char *result = strdup(value);
            free(metadata);
            return result;
        }
        token = strtok(NULL, ",");
    }
    
    free(metadata);
    return NULL;
}

char *decompress_level(char *data) {
    printf("Loading level data...\n");

    char *b64 = extract_gmd_key((const char *) data, "k4", "s");
    if (!b64) {
        // Empty level
        return data;
    }

    fix_base64_url(b64);

    unsigned char *decoded = malloc(strlen(b64));
    int decoded_len = base64_decode(b64, decoded);
    if (decoded_len <= 0) {
        printf("Failed to decode base64\n");
        free(b64);
        free(decoded);
        return NULL;
    }

    uLongf decompressed_len;
    char *decompressed = decompress_data(decoded, decoded_len, &decompressed_len);
    if (!decompressed) {
        printf("Decompression failed (check zlib error above)\n");
        free(decoded);
        free(b64);
        return NULL;
    }

    free(decoded);
    free(b64);
    
    return decompressed;
}

// I don't even know what this does
char **split_string(const char *str, char delimiter, int *outCount) {
    char **result = NULL;
    int count = 0;
    const char *start = str;
    const char *ptr = str;

    while (*ptr) {
        if (*ptr == delimiter) {
            int len = ptr - start;
            if (len > 0) {
                char *token = (char *)malloc(len + 1);
                strncpy(token, start, len);
                token[len] = '\0';

                result = (char **)realloc(result, sizeof(char*) * (count + 1));
                result[count++] = token;
            }
            start = ptr + 1;
        }
        ptr++;
    }
    if (ptr > start) {
        int len = ptr - start;
        char *token = (char *)malloc(len + 1);
        strncpy(token, start, len);
        token[len] = '\0';
        result = (char **)realloc(result, sizeof(char*) * (count + 1));
        result[count++] = token;
    }

    *outCount = count;
    return result;
}

void free_string_array(char **arr, int count) {
    for (int i = 0; i < count; i++) free(arr[i]);
    free(arr);
}

// Parse bool from string ("1" = true, else false)
bool parse_bool(const char *str) {
    return (str[0] == '1' && str[1] == '\0');
}

HSV parse_hsv_string(const char *string) {
    int count = 0;
    char **hsv_string = split_string(string, 'a', &count);
    HSV obtained = { 0 };    

    // Theres always 5 elements
    if (count != 5) {
        return obtained;
    }

    obtained.h = atoi(hsv_string[0]);
    obtained.s = atof(hsv_string[1]);
    obtained.v = atof(hsv_string[2]);
    obtained.sChecked = parse_bool(hsv_string[3]);
    obtained.sChecked = parse_bool(hsv_string[4]);
    return obtained;
}

void parse_ints(short *int_array, const char *string) {
    int count = 0;
    char **ints = split_string(string, '.', &count);
    
    for (int j = 0; j < MAX_GROUPS_PER_OBJECT; j += 1) {
        if (j < count) int_array[j] = (short) atoi(ints[j]);
        else int_array[j] = 0;
    }
}

void parse_color_channel(GDColorChannel *channels, int i, char *channel_string) {
    GDColorChannel channel = {0};  // Zero-initialize
    int kvCount = 0;
    char **kvs = split_string(channel_string, '_', &kvCount);

    for (int j = 0; j + 1 < kvCount; j += 2) {
        int key = atoi(kvs[j]);
        const char *valStr = kvs[j + 1];

        switch (key) {
            case 1:  channel.fromRed = atoi(valStr); break;
            case 2:  channel.fromGreen = atoi(valStr); break;
            case 3:  channel.fromBlue = atoi(valStr); break;
            case 4:  channel.playerColor = atoi(valStr); break;
            case 5:  channel.blending = atoi(valStr) != 0; break;
            case 6:  channel.channelID = atoi(valStr); break;
            case 7:  channel.fromOpacity = atof(valStr); break;
            case 8:  channel.toggleOpacity = atoi(valStr) != 0; break;
            case 9:  channel.inheritedChannelID = atoi(valStr); break;
            case 10: channel.hsv = parse_hsv_string(valStr); break;
            case 11: channel.toRed = atoi(valStr); break;
            case 12: channel.toGreen = atoi(valStr); break;
            case 13: channel.toBlue = atoi(valStr); break;
            case 14: channel.deltaTime = atof(valStr); break;
            case 15: channel.toOpacity = atof(valStr); break;
            case 16: channel.duration = atof(valStr); break;
            case 17: channel.copyOpacity = atoi(valStr) != 0; break;
        }
    }

    channels[i] = channel;
    free_string_array(kvs, kvCount);
}


int parse_color_channels(const char *colorString, GDColorChannel **outArray) {
    if (!colorString || !outArray) return 0;

    int count = 0;
    // Split string into each channel
    char **entries = split_string(colorString, '|', &count);
    if (!entries) return 0;

    GDColorChannel *channels = malloc(sizeof(GDColorChannel) * count);
    if (!channels) {
        printf("Couldn't alloc color channels\n");
        free_string_array(entries, count);
        return 0;
    }

    for (int i = 0; i < count; i++) {
        parse_color_channel(channels, i, entries[i]);
    }

    *outArray = channels;
    free_string_array(entries, count);
    return count;
}

GDValueType get_value_type_for_key(int key) {
    switch (key) {
        case 1:  return GD_VAL_INT;    // Object ID
        case 2:  return GD_VAL_FLOAT;  // X Position
        case 3:  return GD_VAL_FLOAT;  // Y Position
        case 4:  return GD_VAL_BOOL;   // Flipped Horizontally
        case 5:  return GD_VAL_BOOL;   // Flipped Vertically
        case 6:  return GD_VAL_FLOAT;  // Rotation
        case 7:  return GD_VAL_INT;    // (Color trigger) Red
        case 8:  return GD_VAL_INT;    // (Color trigger) Green
        case 9:  return GD_VAL_INT;    // (Color trigger) Blue
        case 10: return GD_VAL_FLOAT;  // (Color trigger) Duration
        case 11: return GD_VAL_BOOL;   // (Color trigger) Touch Triggered
        case 14: return GD_VAL_BOOL;   // (Color trigger) Tint ground
        case 15: return GD_VAL_BOOL;   // (Color trigger) Player 1 color
        case 16: return GD_VAL_BOOL;   // (Color trigger) Player 2 color
        case 17: return GD_VAL_BOOL;   // (Color trigger) Blending
        case 19: return GD_VAL_INT;    // 1.9 color channel
        case 21: return GD_VAL_INT;    // Main col channel
        case 22: return GD_VAL_INT;    // Detail col channel
        case 23: return GD_VAL_INT;    // (Color trigger) Target color ID
        case 24: return GD_VAL_INT;    // Zlayer
        case 25: return GD_VAL_INT;    // Zorder
        case 28: return GD_VAL_INT;    // (Move trigger) Offset X
        case 29: return GD_VAL_INT;    // (Move trigger) Offset Y
        case 30: return GD_VAL_INT;    // (Various) Easing
        case 32: return GD_VAL_FLOAT;  // Scale
        case 35: return GD_VAL_FLOAT;  // (Color trigger) Opacity
        case 41: return GD_VAL_BOOL;   // Main col HSV enabled
        case 42: return GD_VAL_BOOL;   // Detail col HSV enabled
        case 43: return GD_VAL_HSV;    // Main col HSV
        case 44: return GD_VAL_HSV;    // Detail col HSV
        case 49: return GD_VAL_HSV;    // (Color trigger) Copy color HSV
        case 50: return GD_VAL_INT;    // (Color trigger) Copy color id
        case 57: return GD_VAL_INT_ARRAY; // Groups
        case 58: return GD_VAL_BOOL;   // (Move trigger) Lock to player x
        case 59: return GD_VAL_BOOL;   // (Move trigger) Lock to player y
        case 128: return GD_VAL_FLOAT; // Scale x
        case 129: return GD_VAL_FLOAT; // Scale y
        default:
            return GD_VAL_INT; // Default fallback
    }
}

int parse_gd_object(const char *objStr, GDObject *obj) {
    int count = 0;
    // Split object into each key
    char **tokens = split_string(objStr, ',', &count);
    if (count < 1) {
        free_string_array(tokens, count);
        return 0;
    }

    obj->propCount = 0;

    // Iterate through all keys, up to 15
    for (int i = 0; i + 1 < count && obj->propCount < 15; i += 2) {
        int key = atoi(tokens[i]);
        const char *valStr = tokens[i + 1];
        GDValueType type = get_value_type_for_key(key);

        obj->keys[obj->propCount] = key;
        obj->types[obj->propCount] = type;

        switch (type) {
            case GD_VAL_INT:
                obj->values[obj->propCount].i = atoi(valStr);
                break;
            case GD_VAL_FLOAT:
                obj->values[obj->propCount].f = atof(valStr);
                break;
            case GD_VAL_BOOL:
                obj->values[obj->propCount].b = parse_bool(valStr);
                break;
            case GD_VAL_HSV:
                obj->values[obj->propCount].hsv = parse_hsv_string(valStr);
                break;
            case GD_VAL_INT_ARRAY:
                short array[MAX_GROUPS_PER_OBJECT];
                parse_ints(array, valStr);
                for (int i = 0; i < MAX_GROUPS_PER_OBJECT; i++) {
                    obj->values[obj->propCount].int_array[i] = array[i];
                }
                break;
            default:
                obj->values[obj->propCount].i = atoi(valStr);
                break;
        }

        obj->propCount++;
    }

    free_string_array(tokens, count);
    return 1;
}

int get_main_channel_id(int id) {
    ObjectDefinition obj = objects[id];
    for (int i = 0; i < obj.num_layers; i++) {
        int col_channel = obj.layers[i].col_channel;
        if (obj.layers[i].color_type == COLOR_MAIN) return col_channel;
    }
    return -1;
}

int get_detail_channel_id(int id) {
    ObjectDefinition obj = objects[id];
    for (int i = 0; i < obj.num_layers; i++) {
        if (obj.layers[i].color_type == COLOR_DETAIL) return obj.layers[i].col_channel;
    }
    return -1;
}

int convert_1p9_channel(int channel) {
    switch (channel) {
        case 1: return P1;
        case 2: return P2;
        case 3: return 1;
        case 4: return 2;
        case 5: return LBG;
        case 6: return 3;
        case 7: return 4;
        case 8: return THREEDL;
    }
    return 0;
}

// Convert some 2.0 objects into the 1.9 ones, blame robtop for making GD convert those to 2.0
int convert_object(int id) {
    switch (id) {
        // Saws
        case 1734:
            return DARKCOGWHEEL_BIG;
        case 1735:
            return DARKCOGWHEEL_MEDIUM;
        case 1736:
            return DARKCOGWHEEL_SMALL;
        case 1705:
            return SAW_BIG;
        case 1706:
            return SAW_MEDIUM;
        case 1707:
            return SAW_SMALL;
        case 1708:
            return DARKBLADE_BIG;
        case 1709:
            return DARKBLADE_MEDIUM;
        case 1710:
            return DARKBLADE_SMALL;

        // User coin
        case 1329:
            return SECRET_COIN;

        // Slopes
        case 1743:
            return CHECKER_SLOPE_45;
        case 1744:
            return CHECKER_SLOPE_22_66;

        case 1745:
            return B13_SLOPE_45;
        case 1746:
            return B13_SLOPE_22_66;

        case 1747:
            return BLACK_SLOPE_45;
        case 1748:
            return BLACK_SLOPE_22_66;

        case 1749:
            return B15_SLOPE_45;
        case 1750:
            return B15_SLOPE_22_66;
        
        case 1338:
            return OUTLINE_SLOPE_45;
        case 1339:
            return OUTLINE_SLOPE_45;

        // Ground spikes

        case 1715:
            return GROUND_SPIKE;

        case 1719:
            return WAVY_GROUND_SPIKE;

        case 1720:
            return WAVY_PIT_RIGHT;
        case 1721:
            return WAVY_PIT_LEFT;
        
        case 1716:
            return GROUND_SPIKE_NO_GRADIENT;
        case 1717:
            return GROUND_SPIKE_SLOPE_45;
        case 1718:
            return GROUND_SPIKE_SLOPE_22_66;

        case 1722:
            return WAVY_GROUND_SPIKE_NO_GRADIENT;
        case 1723:
            return WAVY_GROUND_SPIKE_SLOPE_45;
        case 1724:
            return WAVY_GROUND_SPIKE_SLOPE_22_66;

        case 1725:
            return SPIKE_PIT;
        case 1726:
            return SPIKE_PIT_END;
        
        case 1728:
            return SEMICIRCLE_PIT;
        case 1729:
            return SEMICIRCLE_PIT_END;
        
        case 1730:
            return SQUARE_PIT;
        case 1731:
            return SQUARE_PIT_EDGE;

        // Fake spikes
        case 1889:
            return FAKE_SPIKE;
        case 1890:
            return FAKE_HALF_SPIKE;
        case 1891:
            return FAKE_MEDIUM_SPIKE;
        case 1892:
            return MINI_FAKE_SPIKE;
        
    }
    return id;
}

ObjectType obtain_type_from_id(int id) {
    switch (id) {
        case BG_TRIGGER:
        case GROUND_TRIGGER:
        case LINE_TRIGGER:
        case OBJ_TRIGGER:
        case OBJ_2_TRIGGER:
        case COL2_TRIGGER:
        case COL3_TRIGGER:
        case COL4_TRIGGER:
        case THREEDL_TRIGGER:
        case 899: // 2.0 col trigger
        case 915: // 2.0 line trigger
            return COL_TRIGGER;
        case 901:
            return MOVE_TRIGGER;
        case 1007:
            return ALPHA_TRIGGER;

    }
    return NORMAL_OBJECT;
}

GameObject *convert_to_game_object(const GDObject *obj) {
    GameObject *object = malloc(sizeof(GameObject));
    if (!object) return NULL;

    // Initialize all fields to 0
    memset(object, 0, sizeof(GameObject));

    object->id = obj->values[0].i;
    object->type = obtain_type_from_id(object->id);
    
    if (object->type == NORMAL_OBJECT) {
        // Set to default colors
        object->object.main_col_channel = 0;
        object->object.detail_col_channel = 0;
        object->object.u1p9_col_channel = 0;

        object->object.scale_x = 1.f;
        object->object.scale_y = 1.f;
    }

    // Temporarily convert user coins (added in 2.0) into secret coins
    object->id = convert_object(object->id);
    object->opacity = 1.f;

    if (object->id >= OBJECT_COUNT && object->id != 901 && object->id != 899 && object->id != 915 && object->id != 1007) {
        object->id = 42;
    }

    // Get a random value for this object
    object->random = rand();

    for (int i = 0; i < obj->propCount; i++) {
        int key = obj->keys[i];
        GDValueType type = obj->types[i];
        GDValue val = obj->values[i];

        // Default members
        switch (key) {
            case 2:  // X
                if (type == GD_VAL_FLOAT) object->x = val.f;
                break;
            case 3:  // Y
                if (type == GD_VAL_FLOAT) object->y = val.f;
                break;
            case 4:  // FlippedH
                if (type == GD_VAL_BOOL) object->flippedH = val.b;
                break;
            case 5:  // FlippedV
                if (type == GD_VAL_BOOL) object->flippedV = val.b;
                break;
            case 6:  // Rotation
                if (type == GD_VAL_FLOAT) object->rotation = val.f;
                break;
            case 57: // Groups
                if (type == GD_VAL_INT_ARRAY) {
                    for (int i = 0; i < MAX_GROUPS_PER_OBJECT; i++) {
                        object->groups[i] = val.int_array[i];
                    }
                }
                break;
        }

        // Col trigger members
        if (object->type == NORMAL_OBJECT) {
            switch (key) {
                case 19: // 1.9 channel id
                    if (type == GD_VAL_INT) object->object.u1p9_col_channel = convert_1p9_channel(val.i);
                    break;
                case 21: // Main col channel
                    if (type == GD_VAL_INT) object->object.main_col_channel = val.i;
                    break;
                case 22: // Detail col channel
                    if (type == GD_VAL_INT) object->object.detail_col_channel = val.i;
                    break;
                case 24: // Z layer
                    if (type == GD_VAL_INT) object->object.zlayer = val.i;
                    break;
                case 25: // Z order
                    if (type == GD_VAL_INT) object->object.zorder = val.i;
                    break;
                case 32: // Scale
                    if (type == GD_VAL_FLOAT) object->object.scale_x = object->object.scale_y = val.f;
                    break;
                case 41: // Main col HSV enabled
                    if (type == GD_VAL_BOOL) object->object.main_col_HSV_enabled = val.b;
                    break;
                case 42: // Detail col HSV enabled
                    if (type == GD_VAL_BOOL) object->object.detail_col_HSV_enabled = val.b;
                    break;
                case 43: // Main col HSV
                    if (type == GD_VAL_HSV) object->object.main_col_HSV = val.hsv;
                    break;
                case 44: // Detail col HSV
                    if (type == GD_VAL_HSV) object->object.detail_col_HSV = val.hsv;
                    break;
                case 128: // Scale x
                    if (type == GD_VAL_FLOAT) object->object.scale_x = val.f;
                    break;
                case 129: // Scale y
                    if (type == GD_VAL_FLOAT) object->object.scale_y = val.f;
                    break;
            }
        } else {
            if (key == 10) { // Duration
                if (type == GD_VAL_FLOAT) object->trigger.trig_duration = val.f;
            } else if (key == 11) { // Touch triggered
                if (type == GD_VAL_BOOL) object->trigger.touchTriggered = val.b;
            }
            switch (object->type) {
                case COL_TRIGGER:
                    switch (key) {
                        case 7:  // Color R
                            if (type == GD_VAL_INT) object->trigger.col_trigger.trig_colorR = val.i;
                            break;
                        case 8:  // Color G
                            if (type == GD_VAL_INT) object->trigger.col_trigger.trig_colorG = val.i;
                            break;
                        case 9:  // Color B
                            if (type == GD_VAL_INT) object->trigger.col_trigger.trig_colorB = val.i;
                            break;
                        case 14: // Tint Ground
                            if (type == GD_VAL_BOOL) object->trigger.col_trigger.tintGround = val.b;
                            break;
                        case 15: // Player 1 color
                            if (type == GD_VAL_BOOL) object->trigger.col_trigger.p1_color = val.b;
                            break;
                        case 16: // Player 2 color
                            if (type == GD_VAL_BOOL) object->trigger.col_trigger.p2_color = val.b;
                            break;
                        case 17: // Blending
                            if (type == GD_VAL_BOOL) object->trigger.col_trigger.blending = val.b;
                            break;
                        case 23: // Target color ID
                            if (type == GD_VAL_INT) object->trigger.col_trigger.target_color_id = val.i;
                            break;
                        case 35: // Opacity
                            if (type == GD_VAL_FLOAT) object->trigger.col_trigger.opacity = val.f;
                            break;
                        case 49: // Copy color HSV
                            if (type == GD_VAL_HSV) object->trigger.col_trigger.copied_hsv = val.hsv;
                            break;
                        case 50: // Copy color ID
                            if (type == GD_VAL_INT) object->trigger.col_trigger.copied_color_id = val.i;
                            break;
                    }
                    break;
                case ALPHA_TRIGGER:
                    switch (key) {
                        case 35: // Opacity
                            if (type == GD_VAL_FLOAT) object->trigger.alpha_trigger.opacity = val.f;
                            break;
                        case 51: // Target group id
                            if (type == GD_VAL_INT) object->trigger.alpha_trigger.target_group = val.i;
                            break;
                    }
                    break;
                case MOVE_TRIGGER:
                    switch (key) {
                        case 28:  // Offset X
                            if (type == GD_VAL_INT) object->trigger.move_trigger.offsetX = val.i;
                            break;
                        case 29:  // Offset Y
                            if (type == GD_VAL_INT) object->trigger.move_trigger.offsetY = val.i;
                            break;
                        case 30:  // Easing
                            if (type == GD_VAL_INT) object->trigger.move_trigger.easing = val.i;
                            break;
                        case 51: // Target group id
                            if (type == GD_VAL_INT) object->trigger.move_trigger.target_group = val.i;
                            break;
                        case 58: // Lock to player x
                            if (type == GD_VAL_BOOL) object->trigger.move_trigger.lock_to_player_x = val.b;
                            break;
                        case 59: // Lock to player y
                            if (type == GD_VAL_BOOL) object->trigger.move_trigger.lock_to_player_y = val.b;
                            break;
                    }
                default:
                    break;
            }
        }
    }
    
    // Modify level ending pos
    if (object->x > level_info.last_obj_x) {
        level_info.last_obj_x = object->x;
    }

    if (object->type == NORMAL_OBJECT) {
        object->object.zsheetlayer = objects[object->id].spritesheet_layer;
        object->object.zlayer = objects[object->id].def_zlayer;
        object->object.zorder = objects[object->id].def_zorder;

        // Setup slope
        if (objects[object->id].is_slope) {
            int orientation = object->rotation / 90;
            if (object->flippedH && object->flippedV) orientation += 2;
            else if (object->flippedH) orientation += 1;
            else if (object->flippedV) orientation += 3;
            
            orientation = orientation % 4;
            if (orientation < 0) orientation += 4;

            object->object.orientation = orientation;
        }
    }
    
    ObjectHitbox hitbox = objects[object->id].hitbox;

    // Modify height and width depending on rotation
    if ((int) fabsf(object->rotation) % 180 != 0) {
        object->width = hitbox.height * object->object.scale_y;
        object->height = hitbox.width * object->object.scale_x;
    } else {
        object->width = hitbox.width * object->object.scale_x;
        object->height = hitbox.height * object->object.scale_y;
    }

    // Register its groups
    register_object(object);

    return object;
}

void free_game_object(GameObject *obj) {
    if (!obj) return;
    free(obj);
}

GDGameObjectList *convert_all_to_game_objects(GDObjectList *objList) {
    if (!objList) return NULL;

    GameObject **objectArray = malloc(sizeof(GameObject *) * objList->objectCount);
    if (!objectArray) return NULL;

    printf("Converting objects...\n");

    for (int i = 0; i < objList->objectCount; i++) {
        objectArray[i] = convert_to_game_object(&objList->objects[i]);
        assign_object_to_section(objectArray[i]);
        if (!objectArray[i]) {
            printf("Failed to convert object %d\n", i);
            for (int j = 0; j < i; j++) {
                free_game_object(objectArray[j]);
            }
            free(objectArray);
            return NULL;
        }
    }

    printf("Allocating list...\n");

    GDGameObjectList *gameObjectList = malloc(sizeof(GDGameObjectList));
    if (!gameObjectList) {
        printf("Failed to allocate the game object list");
        for (int i = 0; i < objList->objectCount; i++) {
            free_game_object(objectArray[i]);
        }
        free(objectArray);
        return NULL;
    }

    gameObjectList->count = objList->objectCount;
    gameObjectList->objects = objectArray;

    return gameObjectList;
}

void free_gd_object_list(GDObjectList *list) {
    free(list->objects);
    free(list);
}

GDObjectList *parse_string(const char *levelString) {
    int sectionCount = 0;

    // Split the string in object sections
    char **sections = split_string(levelString, ';', &sectionCount);

    if (sectionCount < 3) {
        printf("Level string missing sections!\n");
        free_string_array(sections, sectionCount);
        return NULL;
    }
    
    printf("Parsing string...\n");

    int objectCount = sectionCount - 1;
    printf("%d\n", objectCount);
    
    // Allocate GD objects
    GDObject *objects = (GDObject *)malloc(sizeof(GDObject) * objectCount);

    printf("Size of gdobjects %d bytes\n", sizeof(GDObject) * objectCount);
    if (objects == NULL) {
        free_string_array(sections, sectionCount);
        printf("Couldn't allocate GD Objects\n");
        return NULL;
    }

    for (int i = 1; i < sectionCount; i++) {
        if (!parse_gd_object(sections[i], &objects[i - 1])) {
            printf("Failed to parse object at section %d\n", i);
        }
    }

    free_string_array(sections, sectionCount);
    
    printf("Allocating object list...\n");

    GDObjectList *objectList = malloc(sizeof(GDObjectList));
    if (!objectList) {
        printf("Memory allocation failed for objectList\n");
        free(objects);
        return NULL;
    }

    objectList->objectCount = objectCount;
    objectList->objects = objects;

    return objectList;
}

void free_game_object_list(GDGameObjectList *list) {
    if (!list) return;
    if (list->objects) {
        for (int i = 0; i < list->count; i++) {
            free_game_object(list->objects[i]);
        }
        free(list->objects);
    }
    free(list);
}

int compare_sortable_layers(const void *a, const void *b) {
    GDLayerSortable *layerSortA = *(GDLayerSortable **)a;
    GDLayerSortable *layerSortB = *(GDLayerSortable **)b;
    
    GDObjectLayer *GDlayerA = layerSortA->layer;
    GDObjectLayer *GDlayerB = layerSortB->layer;

    struct ObjectLayer *layerA = GDlayerA->layer;
    struct ObjectLayer *layerB = GDlayerB->layer;

    GameObject *objA = GDlayerA->obj;
    GameObject *objB = GDlayerB->obj;

    int zlayerA = layerSortA->zlayer + layerA->zlayer_offset;
    int zlayerB = layerSortB->zlayer + layerB->zlayer_offset;
    
    int obj_idA = objA->id;
    int obj_idB = objB->id;

    int sheetA = objects[obj_idA].spritesheet_layer;
    int sheetB = objects[obj_idB].spritesheet_layer;

    // Get blending zlayer modifier, don't affect player and only affect normal objects
    if (obj_idA != PLAYER_OBJECT && sheetA == SHEET_BLOCKS) {
        int col_channelA = GDlayerA->col_channel;
        bool blendingA = channels[col_channelA].blending | GDlayerA->blending;
        if (blendingA ^ (zlayerA % 2 == 0)) {
            zlayerA--;
        }
    }

    if (obj_idB != PLAYER_OBJECT && sheetB == SHEET_BLOCKS) {
        int col_channelB = GDlayerB->col_channel;
        bool blendingB = channels[col_channelB].blending | GDlayerA->blending;
        if (blendingB ^ (zlayerB % 2 == 0)) {
            zlayerB--;
        }
    }
    
    if (zlayerA != zlayerB)
        return zlayerA - zlayerB; // Ascending

    if (sheetA != sheetB)
        return sheetB - sheetA; // Descending

    int zorderA = layerSortA->zorder;
    int zorderB = layerSortB->zorder;

    if (zorderA != zorderB)
        return zorderA - zorderB; // Ascending

    return layerSortA->originalIndex - layerSortB->originalIndex; // Stable fallback
}

// Me when this is like Java (using Comparable interface)
void sort_layers_by_layer(GDObjectLayerList *list) {
    if (!list || list->count <= 1) return;

    printf("Sorting layer list\n");
    
    // Wrap objects with indices
    sortable_list = malloc(sizeof(GDLayerSortable) * list->count);

    if (sortable_list == NULL) {
        printf("Couldn't allocate sortable layer\n");
        return;
    }
    
    for (int i = 0; i < list->count; i++) {
        sortable_list[i].layer = list->layers[i];
        sortable_list[i].originalIndex = i;
        sortable_list[i].layerNum = list->layers[i]->layerNum;
        
        GameObject *obj = sortable_list[i].layer->obj;
        sortable_list[i].zlayer = obj->object.zlayer;
        sortable_list[i].zorder = obj->object.zorder;

        assign_layer_to_section(&sortable_list[i]);
    }
}

void free_game_object_array(GameObject **array, int count) {
    if (!array) return;
    for (int i = 0; i < count; i++) {
        free_game_object(array[i]);
    }
    free(array); // Free the array of pointers itself
}

GDObjectLayerList *fill_layers_array(GDGameObjectList *objList) {
    // Count layers
    int layerCount = 0;
    for (int i = 0; i < objList->count; i++) {
        GameObject *obj = objList->objects[i];

        int obj_id = obj->id;

        if (obj_id < OBJECT_COUNT)
            layerCount += objects[obj->id].num_layers;
    }

    printf("Allocating %d bytes for %d layers\n", sizeof(GDObjectLayer) * layerCount, layerCount);
    GDObjectLayer *layers = malloc(sizeof(GDObjectLayer) * layerCount);

    if (layers == NULL) {
        printf("Couldn't allocate layers\n");
        return NULL;
    }

    // Add player for rendering, not used for gameplay
    GameObject *obj = malloc(sizeof(GameObject));
    obj->id = PLAYER_OBJECT;
    obj->type = NORMAL_OBJECT;
    obj->object.zlayer = LAYER_T1-1;
    obj->object.zorder = 0;
    obj->object.zsheetlayer = 0;
    GDObjectLayer *layer = malloc(sizeof(GDObjectLayer));
    layer->obj = obj;
    layer->layer = (struct ObjectLayer *) &objects[obj->id].layers[0];
    layer->layerNum = 0;
    layer->col_channel = WHITE;
    layer->blending = FALSE;
    GDLayerSortable sortable_layer;
    sortable_layer.layer = layer;
    sortable_layer.originalIndex = 0;
    sortable_layer.zlayer = obj->object.zlayer;
    sortable_layer.zorder = obj->object.zorder;

    gfx_player_layer = sortable_layer;
    player_game_object = obj;

    printf("Allocated %d layers\n", layerCount);

    // Fill array
    int count = 0;
    for (int i = 0; i < objList->count; i++) {
        GameObject *obj = objList->objects[i];

        int obj_id = obj->id;
        int obj_type = obj->type;

        if (obj_id < OBJECT_COUNT && obj_type == NORMAL_OBJECT)
            for (int j = 0; j < objects[obj->id].num_layers; j++) {
                struct ObjectLayer *layer = (struct ObjectLayer *) &objects[obj->id].layers[j];
                layers[count].layer = layer;
                layers[count].obj = obj;
                layers[count].layerNum = j;

                int col_channel = layer->col_channel;

                // Get layer's color channel
                if (is_modifiable(layer->col_channel)) {
                    if (obj->object.u1p9_col_channel > 0) {
                        // Get 1.9 color channel
                        if (layer->color_type == COLOR_DETAIL) col_channel = obj->object.u1p9_col_channel;
                    } else { 
                        // 2.0+ color channels
                        if (obj->object.main_col_channel > 0) {
                            if (layer->color_type == COLOR_MAIN) {
                                col_channel = obj->object.main_col_channel;  
                            } else {
                                if (get_main_channel_id(obj_id) <= 0)col_channel = obj->object.main_col_channel; 
                            }
                        }
                        if (obj->object.detail_col_channel > 0) {
                            if (layer->color_type == COLOR_DETAIL) {
                                if (get_main_channel_id(obj_id) >= 0) col_channel = obj->object.detail_col_channel;  
                            }    
                        }
                    }
                }

                layers[count].blending = FALSE;

                // Convert object glow color channel into main color channel
                if (obj->object.main_col_channel > 0 && col_channel == OBJ_BLENDING) {
                    col_channel = obj->object.main_col_channel;
                    layers[count].blending = TRUE;
                }
                layers[count].col_channel = col_channel;
                count++;
            }
    }
    
    printf("Finished filling %d layers\n", count);

    printf("Allocating layer list\n");
    GDObjectLayerList *layerList = malloc(sizeof(GDObjectLayerList));
    
    if (layerList == NULL) {
        printf("Couldn't allocate layer list\n");
        free(obj);
        free(layers);
        return NULL;
    }

    // Allocate array of pointers to GDObjectLayer
    layerList->layers = malloc(sizeof(GDObjectLayer *) * count);

    if (layerList->layers == NULL) {
        printf("Couldn't allocate layer pointers\n");
        free(layers);
        free(layerList);
        return NULL;
    }

    for (int i = 0; i < count; i++) {
        layerList->layers[i] = &layers[i];
    }

    layerList->count = count;

    return layerList;
}

void free_layer_list(GDObjectLayerList *list) {
    if (!list) return;
    if (list->layers) {
        // No need to free each layer as it has been allocated as the layer list
        free(list->layers);
    }
    free(list);
}

int parse_old_channels(char *level_string, GDColorChannel **outArray) {
    GDColorChannel *channels = malloc(sizeof(GDColorChannel) * 2);
    if (!channels) {
        printf("Couldn't alloc initial pre 2.0 color channels\n");
        return 0;
    }

    char *v19_bg = get_metadata_value(level_string, "kS29");

    int i = 0;

    if (v19_bg) { // 1.9 only
        parse_color_channel(channels, i, v19_bg);
        channels[i].channelID = BG;
        i++;

        parse_color_channel(channels, i, get_metadata_value(level_string, "kS30"));
        channels[i].channelID = GROUND;
        i++;

        char *line = get_metadata_value(level_string, "kS31");
        if (line) {
            channels = realloc(channels, sizeof(GDColorChannel) * (i + 1));
            parse_color_channel(channels, i, line);
            channels[i].channelID = LINE;
            i++;
        }
        
        char *obj = get_metadata_value(level_string, "kS32");
        if (obj) {
            channels = realloc(channels, sizeof(GDColorChannel) * (i + 1));
            parse_color_channel(channels, i, obj);
            channels[i].channelID = OBJ;
            i++;
        }

        char *col1 = get_metadata_value(level_string, "kS33");
        if (col1) {
            channels = realloc(channels, sizeof(GDColorChannel) * (i + 1));
            parse_color_channel(channels, i, col1);
            channels[i].channelID = 1;
            i++;
        }

        char *col2 = get_metadata_value(level_string, "kS34");
        if (col2) {
            channels = realloc(channels, sizeof(GDColorChannel) * (i + 1));
            parse_color_channel(channels, i, col2);
            channels[i].channelID = 2;
            i++;
        }

        char *col3 = get_metadata_value(level_string, "kS35");
        if (col3) {
            channels = realloc(channels, sizeof(GDColorChannel) * (i + 1));
            parse_color_channel(channels, i, col3);
            channels[i].channelID = 3;
            i++;
        }

        char *col4 = get_metadata_value(level_string, "kS36");
        if (col4) {
            channels = realloc(channels, sizeof(GDColorChannel) * (i + 1));
            parse_color_channel(channels, i, col4);
            channels[i].channelID = 4;
            i++;
        }

        char *dl3 = get_metadata_value(level_string, "kS37");
        if (dl3) {
            channels = realloc(channels, sizeof(GDColorChannel) * (i + 1));
            parse_color_channel(channels, i, dl3);
            channels[i].channelID = THREEDL;
            i++;
        }
        
        *outArray = channels;
        return i;
    }

    // Pre 1.9
    int bg_r = atoi(get_metadata_value(level_string, "kS1"));
    int bg_g = atoi(get_metadata_value(level_string, "kS2"));
    int bg_b = atoi(get_metadata_value(level_string, "kS3"));

    GDColorChannel bg_channel = {0};
    bg_channel.channelID = BG;
    bg_channel.fromRed = bg_r;
    bg_channel.fromGreen = bg_g;
    bg_channel.fromBlue = bg_b;

    char *bg_player_color = get_metadata_value(level_string, "kS16");
    if (bg_player_color) {
       bg_channel.playerColor = atoi(bg_player_color);
    }

    channels[i] = bg_channel;

    i++;

    int g_r = atoi(get_metadata_value(level_string, "kS4"));
    int g_g = atoi(get_metadata_value(level_string, "kS5"));
    int g_b = atoi(get_metadata_value(level_string, "kS6"));

    GDColorChannel g_channel = {0};
    g_channel.channelID = GROUND;
    g_channel.fromRed = g_r;
    g_channel.fromGreen = g_g;
    g_channel.fromBlue = g_b;

    char *g_player_color = get_metadata_value(level_string, "kS17");
    if (g_player_color) {
        g_channel.playerColor = atoi(g_player_color);
    }
    
    channels[i] = g_channel;
    i++;

    char *line_r = get_metadata_value(level_string, "kS7");
    char *line_g = get_metadata_value(level_string, "kS8");
    char *line_b = get_metadata_value(level_string, "kS9");

    if (line_r && line_g && line_b) {
        GDColorChannel line_channel = {0};
        line_channel.channelID = LINE;
        line_channel.fromRed = atoi(line_r);
        line_channel.fromGreen = atoi(line_g);
        line_channel.fromBlue = atoi(line_b);
        
        char *line_player_color = get_metadata_value(level_string, "kS18");
        if (line_player_color) {
            line_channel.playerColor = atoi(line_player_color);
        }

        channels = realloc(channels, sizeof(GDColorChannel) * (i + 1));
        channels[i] = line_channel;
        i++;
    }

    char *obj_r = get_metadata_value(level_string, "kS10");
    char *obj_g = get_metadata_value(level_string, "kS11");
    char *obj_b = get_metadata_value(level_string, "kS12");

    if (obj_r && obj_g && obj_b) {
        GDColorChannel obj_channel = {0};
        obj_channel.channelID = OBJ;
        obj_channel.fromRed = atoi(obj_r);
        obj_channel.fromGreen = atoi(obj_g);
        obj_channel.fromBlue = atoi(obj_b);

        char *obj_player_color = get_metadata_value(level_string, "kS19");
        if (obj_player_color) {
            obj_channel.playerColor = atoi(obj_player_color);
        }
        
        channels = realloc(channels, sizeof(GDColorChannel) * (i + 1));
        channels[i] = obj_channel;
        i++;
    }

    char *obj_2_r = get_metadata_value(level_string, "kS13");
    char *obj_2_g = get_metadata_value(level_string, "kS14");
    char *obj_2_b = get_metadata_value(level_string, "kS15");

    if (obj_2_r && obj_2_g && obj_2_b) {
        GDColorChannel obj_2_channel = {0};
        obj_2_channel.channelID = 1;
        obj_2_channel.fromRed = atoi(obj_2_r);
        obj_2_channel.fromGreen = atoi(obj_2_g);
        obj_2_channel.fromBlue = atoi(obj_2_b);
        
        char *obj_2_player_color = get_metadata_value(level_string, "kS20");
        if (obj_2_player_color) {
            obj_2_channel.playerColor = atoi(obj_2_player_color);
        }

        char *obj_2_blending = get_metadata_value(level_string, "kA5");
        if (obj_2_blending) {
            obj_2_channel.blending = atoi(obj_2_blending) != 0;
        }
        
        channels = realloc(channels, sizeof(GDColorChannel) * (i + 1));
        channels[i] = obj_2_channel;
        i++;
    }

    *outArray = channels;

    return i;
}

GDGameObjectList *objectsArrayList = NULL;

GDObjectLayerList *layersArrayList = NULL;
int channelCount = 0;
GDColorChannel *colorChannels = NULL;

void load_level_info(char *data, char *level_string) {
    char *gmd_song_id = extract_gmd_key((const char *) data, "k8", "i");
    if (!gmd_song_id) {
        level_info.song_id = 0; // Stereo Madness
    } else {
        level_info.song_id = atoi(gmd_song_id); // Official song id
    }

    char *gmd_custom_song_id = extract_gmd_key((const char *) data, "k45", "i");
    if (!gmd_custom_song_id) {
        level_info.custom_song_id = -1;
    } else {
        level_info.custom_song_id = atoi(gmd_custom_song_id); // Custom song id
    }
    
    char *gmd_song_offset = get_metadata_value(level_string, "kA13");
    if (gmd_song_offset) {
        level_info.song_offset = atof(gmd_song_offset);
    } else {
        level_info.song_offset = 0;
    }

    char *background_data = get_metadata_value(level_string, "kA6");
    if (background_data) {
        level_info.background_id = CLAMP(atoi(background_data) - 1, 0, BG_COUNT - 1);
    } else {
        level_info.background_id = 0;
    }

    char *ground_data = get_metadata_value(level_string, "kA7");
    if (ground_data) {
        level_info.ground_id = CLAMP(atoi(ground_data) - 1, 0, G_COUNT - 1);
    } else {
        level_info.ground_id = 0;
    }
    
    char *gamemode_data = get_metadata_value(level_string, "kA2");
    if (gamemode_data) {
        level_info.initial_gamemode = CLAMP(atoi(gamemode_data), 0, GAMEMODE_COUNT - 1);
    } else {
        level_info.initial_gamemode = GAMEMODE_CUBE;
    }

    char *mini_data = get_metadata_value(level_string, "kA3");
    if (gamemode_data) {
        level_info.initial_mini = atoi(mini_data) != 0;
    } else {
        level_info.initial_mini = 0; 
    }

    char *speed_data = get_metadata_value(level_string, "kA4");
    if (speed_data) {
        level_info.initial_speed = CLAMP(atoi(speed_data), 0, SPEED_COUNT - 1);
        if (level_info.initial_speed == 0) level_info.initial_speed = SPEED_NORMAL;
        else if (level_info.initial_speed == 1) level_info.initial_speed = SPEED_SLOW;
    } else {
        level_info.initial_speed = SPEED_NORMAL;
    }

    char *dual_data = get_metadata_value(level_string, "kA8");
    if (dual_data) {
        level_info.initial_dual = atoi(dual_data) != 0;
    } else {
        level_info.initial_dual = 0; 
    }

    char *upsidedown_data = get_metadata_value(level_string, "kA11");
    if (upsidedown_data) {
        level_info.initial_upsidedown = atoi(upsidedown_data) != 0;
    } else {
        level_info.initial_upsidedown = 0; 
    }
}

int load_level(char *data, bool is_custom) {
    level_info.level_is_custom = is_custom;

    char *level_string = decompress_level(data);

    if (level_string == NULL) {
        printf("Failed decompressing the level.\n");
        return 1;
    }

    level_info.last_obj_x = 570.f;

    // Ignore empty levels
    if (level_string != data) {

        // Get level starting colors
        char *metaStr = get_metadata_value(level_string, "kS38");
        channelCount = parse_color_channels(metaStr, &colorChannels);

        // Fallback to pre 2.0 color keys
        if (!channelCount) {
            channelCount = parse_old_channels(level_string, &colorChannels);
        }
        
        load_level_info(data, level_string);

        GDObjectList *objectsList = parse_string(level_string);

        free(level_string);

        if (objectsList == NULL) {
            printf("Failed parsing the objects.\n");
            return 2;
        }

        objectsArrayList = convert_all_to_game_objects(objectsList);
        free_gd_object_list(objectsList);

        if (objectsArrayList == NULL) {
            printf("Failed converting objects to game object structs.\n");
            return 3;
        }

        layersArrayList = fill_layers_array(objectsArrayList);

        if (layersArrayList == NULL) {
            printf("Couldn't sort layers\n");
            free_game_object_list(objectsArrayList);
            return 4;
        }

        sort_layers_by_layer(layersArrayList);
        level_info.level_is_empty = FALSE;
        level_info.object_count = objectsArrayList->count;
        level_info.layer_count = layersArrayList->count;
    } else {
        level_info.level_is_empty = TRUE;
        level_info.object_count = 0;
        level_info.layer_count = 1; // Player
        objectsArrayList = malloc(sizeof(GDGameObjectList));
        objectsArrayList->count = 0;
        objectsArrayList->objects = NULL;
        layersArrayList = fill_layers_array(objectsArrayList);
    }
    
    memset(col_trigger_buffer, 0, sizeof(col_trigger_buffer));
    memset(move_trigger_buffer, 0, sizeof(move_trigger_buffer));

    level_info.pulsing_type = random_int(0,2);

    // Load level's bg and ground texture
    bg = GRRLIB_LoadTexturePNG(backgrounds[level_info.background_id]);
    ground = GRRLIB_LoadTexturePNG(grounds[level_info.ground_id]);

    int rounded_last_obj_x = (int) (level_info.last_obj_x / 30) * 30 + 15;
    level_info.wall_x = (rounded_last_obj_x) + (11.f * 30.f) - 3;
    level_info.wall_y = 0;
    full_init_variables();

    reset_color_channels();
    set_color_channels();

    load_coin_texture();

    printf("Finished loading level\n");

    return 0;
}

void unload_level() {

    if (layersArrayList) {
        free_layer_list(layersArrayList);
        layersArrayList = NULL;
    }

    if (sortable_list) {
        free(sortable_list);
        sortable_list = NULL;
    }

    if (objectsArrayList) {
        free_game_object_list(objectsArrayList);
        objectsArrayList = NULL;
    }

    if (colorChannels) {
        free(colorChannels);
        colorChannels = NULL;
    }
    clear_groups();

    GRRLIB_FreeTexture(bg);
    GRRLIB_FreeTexture(ground);
    channelCount = 0;
    memset(&state.particles, 0, sizeof(state.particles));
    free_sections();
    full_init_variables();
    unload_coin_texture();
}

void reset_color_channels() {
    for (s32 i = 0; i < 999; i++) {
        channels[i].color.r = 255;
        channels[i].color.g = 255;
        channels[i].color.b = 255;
        channels[i].alpha = 1.f;
        channels[i].blending = FALSE;
    }

    channels[BG].color.r = 56;
    channels[BG].color.g = 121;
    channels[BG].color.b = 255;
    channels[BG].alpha = 1.f;
    channels[BG].blending = FALSE;
    
    channels[GROUND].color.r = 56;
    channels[GROUND].color.g = 121;
    channels[GROUND].color.b = 255;
    channels[GROUND].alpha = 1.f;
    channels[GROUND].blending = FALSE;

    channels[LINE].color.r = 255;
    channels[LINE].color.g = 255;
    channels[LINE].color.b = 255;
    channels[LINE].alpha = 1.f;
    channels[LINE].blending = TRUE;

    channels[OBJ].color.r = 255;
    channels[OBJ].color.g = 255;
    channels[OBJ].color.b = 255;
    channels[OBJ].alpha = 1.f;
    channels[OBJ].blending = FALSE;

    channels[OBJ_BLENDING].alpha = 1.f;
    channels[OBJ_BLENDING].copy_color_id = OBJ;
    channels[OBJ_BLENDING].blending = TRUE;

    channels[P1].color = p1;
    channels[P1].alpha = 1.f;
    channels[P1].blending = TRUE;
    channels[P2].color = p2;
    channels[P2].alpha = 1.f;
    channels[P2].blending = TRUE;
    
    channels[BLACK].color.r = 0;
    channels[BLACK].color.g = 0;
    channels[BLACK].color.b = 0;
    channels[BLACK].alpha = 1.f;
    channels[BLACK].blending = FALSE;

    channels[WHITE].color.r = 255;
    channels[WHITE].color.g = 255;
    channels[WHITE].color.b = 255;
    channels[WHITE].alpha = 1.f;
    channels[WHITE].blending = FALSE;
}

void set_color_channels() {
    for (int i = 0; i < channelCount; i++) {
        GDColorChannel colorChannel = colorChannels[i];
        int id = colorChannel.channelID;

        switch (id) {
            case P1:
            case P2:
            case BLACK:
            case WHITE:
                break;

            default:
                if (id < COL_CHANNEL_COUNT) {
                    Color color;
                    color.r = colorChannel.fromRed;
                    color.g = colorChannel.fromGreen;
                    color.b = colorChannel.fromBlue;

                    channels[id].alpha = colorChannel.fromOpacity;
                    channels[id].blending = colorChannel.blending;
                    channels[id].copy_color_id = colorChannel.inheritedChannelID;

                    if (colorChannel.inheritedChannelID > 0) {
                        channels[id].hsv = colorChannel.hsv;
                    }
                    
                    channels[id].color = color;

                    if (colorChannel.playerColor == 1) channels[id].color = p1;
                    if (colorChannel.playerColor == 2) channels[id].color = p2;
                }
        }
    }
}

char *get_level_name(char *data_ptr) {
    return extract_gmd_key((const char *) data_ptr, "k2", "s");
}

char *get_author_name(char *data_ptr) {
    return extract_gmd_key((const char *) data_ptr, "k5", "s");
}

void reload_level() {
    memset(col_trigger_buffer, 0, sizeof(col_trigger_buffer));
    memset(move_trigger_buffer, 0, sizeof(move_trigger_buffer));
    memset(&state.particles, 0, sizeof(state.particles));
    for (int i = 0; i < objectsArrayList->count; i++) {
        GameObject *obj = objectsArrayList->objects[i];
        obj->activated[0] = obj->activated[1] = FALSE;
        obj->toggled = FALSE;
        obj->collided[0] = obj->collided[1] = FALSE;
        obj->hitbox_counter[0] = obj->hitbox_counter[1] = 0;
        obj->transition_applied = FADE_NONE;
        obj->object.delta_x = 0;
        obj->object.delta_y = 0;
        obj->opacity = 1.f;
    }
    reset_color_channels();
    set_color_channels();
}

// https://github.com/gd-programming/gd.docs/issues/87
void calculate_lbg() {
    struct ColorChannel channel = channels[BG];
    float h,s,v;
    
    convertRGBtoHSV(channel.color.r, channel.color.g, channel.color.b, &h, &s, &v);

    s -= 0.20f;
    s = clampf(s, 0.f, 1.f);
    v += 0.20f;
    v = clampf(v, 0.f, 1.f);

    unsigned char r,g,b;

    convertHSVtoRGB(h, s, v, &r, &g, &b);

    // Set here no lerping LBG
    channels[LBG_NO_LERP].color.r = r;
    channels[LBG_NO_LERP].color.g = g;
    channels[LBG_NO_LERP].color.b = b;
    channels[LBG_NO_LERP].blending = TRUE;

    float factor = (channel.color.r + channel.color.g + channel.color.b) / 150.f;

    if (factor < 1.f) {
        r = r * factor + p1.r * (1 - factor);
        g = g * factor + p1.g * (1 - factor);
        b = b * factor + p1.b * (1 - factor);
    }

    // Set here lerped LBG
    channels[LBG].color.r = r;
    channels[LBG].color.g = g;
    channels[LBG].color.b = b;
    channels[LBG].blending = TRUE;
}


char *load_song(const char *file_name, size_t *out_size) {
    char full_path[273];
    snprintf(full_path, sizeof(full_path), "%s/%s/%s/%s", launch_dir, RESOURCES_FOLDER, SONGS_FOLDER, file_name);
    return read_file(full_path, out_size);
}

char *load_user_song(int id, size_t *out_size) {
    char full_path[273];
    snprintf(full_path, sizeof(full_path), "%s/%s/%d.mp3", launch_dir, USER_SONGS_FOLDER, id);
    return read_file(full_path, out_size);
}

bool check_song(int id) {
    char full_path[273];
    snprintf(full_path, sizeof(full_path), "%s/%s/%d.mp3", launch_dir, USER_SONGS_FOLDER, id);
    return access(full_path, F_OK) == 0;
}

void update_percentage() {
    float progress = (state.player.x / (level_info.last_obj_x + (11 * 30.f))) * 100;
    if (progress > 100) progress = 100;
    state.level_progress = progress;
}
