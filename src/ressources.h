#include "uthash.h"

#define TAILLE_MAX_CHEMIN 200
#define TAILLE_MAX_ID 100

typedef struct {
    SDL_Texture * texture;
    char id[TAILLE_MAX_ID];
    UT_hash_handle hh;
} t_texture; 

extern t_texture * textures;

typedef enum { FIN_TAB_ANIMS, REPOS, CHUTE_G, CHUTE_D, DEPL_G, DEPL_D, CREUSER, ATTQ_G, ATTQ_D } t_id_anim;

typedef struct {
    t_id_anim id;
    int x_sprite_ini;
    int y_sprite;
    int w_sprite;
    int h_sprite;
    int decalage_dest_x;
    int decalage_dest_y;
    int longueur;
    float vitesse_anim;
} t_animation;


typedef struct {
    SDL_Texture * texture;
    t_animation ** animations;
    int n_animations;
    char id[TAILLE_MAX_ID];
    UT_hash_handle hh;
} t_spritesheet; 

extern t_spritesheet * spritesheets;

typedef struct {
    SDL_AudioSpec spec;
    uint32_t length;
    uint8_t * buffer;
    char id[TAILLE_MAX_ID];
    UT_hash_handle hh;
} t_son; 

extern t_son * sons;

void init_ressources(SDL_Renderer *);

SDL_Texture * recuperer_texture(const char*);
t_spritesheet * recuperer_spritesheet(const char*);
t_son * recuperer_son(const char*);

t_animation * recuperer_animation(t_animation **, int, t_id_anim);

void detruire_ressources(void);
