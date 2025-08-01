#pragma once

typedef enum {
    GD_VAL_INT,
    GD_VAL_FLOAT,
    GD_VAL_BOOL,
    GD_VAL_UNKNOWN
} GDValueType;

typedef union {
    int i;
    float f;
    bool b;
} GDValue;

typedef struct {
    int id;              // key 1
    float x;             // key 2
    float y;             // key 3
    bool flippedH;       // key 4
    bool flippedV;       // key 5
    float rotation;      // key 6
    u8 trig_colorR;      // key 7
    u8 trig_colorG;      // key 8
    u8 trig_colorB;      // key 9
    float trig_duration; // key 10
    u8 touchTriggered:1; // key 11
    u8 tintGround:1;     // key 14
    u8 p1_color:1;       // key 15
    u8 p2_color:1;       // key 16
    u8 blending:1;       // key 17
    int main_col_channel;   // key 21
    int detail_col_channel; // key 22
    int target_color_id; // key 23
    int zsheetlayer;     // no key has this, but used internally
    int zlayer;          // key 24
    int zorder;          // key 25


    int random;                     // random number assigned to this object
    bool activated[2];              // if it has been activated
    unsigned int hitbox_counter[2]; // number of times the player has entered the hitbox
    bool collided[2];               // if the object just started being touched
    float ampl_scaling;             // the amplitude scaling for pulsing objects
    u8 transition_applied;          // the transition applied to the object
    bool toggled;                   // if its enabled or disabled
    int width;
    int height;

    // Slope
    unsigned char orientation;
} GameObject;

typedef struct {
    unsigned char keys[15];
    unsigned char propCount;
    unsigned char types[15];
    GDValue values[15];
} GDObject;

typedef struct {
    GDObject *objects;
    int objectCount;
} GDObjectList;

typedef struct {
    int count;
    GameObject **objects;
} GDTypedObjectList;

typedef struct {
    GameObject *obj;
    int originalIndex;
} GDObjectSortable;

typedef struct {
    GameObject *obj;
    struct ObjectLayer *layer;
    int layerNum;
} GDObjectLayer;

typedef struct {
    GDObjectLayer *layer;
    int zlayer;          
    int zorder;          
    int originalIndex;
    int layerNum;
} GDLayerSortable;

typedef struct {
    int count;
    GDObjectLayer **layers;
} GDObjectLayerList;

typedef struct {
    int fromRed;
    int fromGreen;
    int fromBlue;
    int playerColor;
    bool blending;
    int channelID;
    float fromOpacity;
    bool toggleOpacity;
    int inheritedChannelID;
    int hsv; // convert to struct
    int toRed;
    int toGreen;
    int toBlue;
    float deltaTime;
    float toOpacity;
    float duration;
    bool copyOpacity;
} GDColorChannel;

struct LoadedLevelInfo {
    float last_obj_x;
    float wall_x;
    int pulsing_type;
    int song_id;
    bool completing;
    int background_id;
    int ground_id;
    int initial_gamemode;
    bool initial_mini;
    unsigned char initial_speed;
    bool initial_dual;
    bool initial_upsidedown;
};

#define BG_COUNT 4
#define G_COUNT 4

#define SECTION_HASH_SIZE 600

#define SECTION_SIZE 128
#define GFX_SECTION_SIZE 128

typedef struct Section {
    GameObject **objects;
    int object_count;
    int object_capacity;

    int x, y; // Section coordinates
    struct Section *next; // For chaining in hash map
} Section;

typedef struct GFXSection {
    GDLayerSortable **layers;
    int layer_count;
    int layer_capacity;

    int x, y; // Section coordinates
    struct GFXSection *next; // For chaining in hash map
} GFXSection;

extern Section *section_hash[SECTION_HASH_SIZE];
extern GFXSection *section_gfx_hash[SECTION_HASH_SIZE];
unsigned int section_hash_func(int x, int y);
Section *get_or_create_section(int x, int y);
GFXSection *get_or_create_gfx_section(int x, int y);
void free_sections(void);

char *get_level_name(char *data_ptr);
int compare_sortable_layers(const void *a, const void *b);

extern struct LoadedLevelInfo level_info;

extern GDTypedObjectList *objectsArrayList;
extern GDObjectLayerList *layersArrayList;

extern GDLayerSortable gfx_player_layer;
extern GameObject *player_game_object;

void free_typed_object_list(GDTypedObjectList *list);
void free_typed_object_array(GameObject **array, int count);
void load_level(char *data);
void unload_level();
void reload_level();
void reset_color_channels();
void set_color_channels();
void calculate_lbg();
char *load_song(const char *file_name, size_t *out_size);