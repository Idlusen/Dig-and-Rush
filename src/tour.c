#include <stdlib.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <time.h>

#include "ressources.h"
#include "selection_personnages.h"
#include "tour.h"
#include "constantes.h"
#include "entite.h"
#include "entite_destructible.h"
#include "entite_pnj.h"
#include "entite_perso.h"
#include "morceaux_niveau.h"
#include "listes.h"
#include "texte.h"

#define MAX_RAYON_OMBRE (TAILLE_H)
#define FACTEUR_MIN_RAYON_OMBRE 4 // multipli par la largeur du personnage
#define FACTEUR_OBSCURCISSEMENT 6.
#define PAS_RAYON_OMBRE 18

#define PAS_ALPHA_FOND 7

#define DELAI_CREUSAGE 18 // nombre de frames



SDL_bool PointInFRect(const SDL_FPoint* p, const SDL_FRect* r) {
    if (p->x >= r->x && p->x < r->x + r->w &&
        p->y >= r->y && p->y < r->y + r->h) {
        return SDL_TRUE;
    } else {
        return SDL_FALSE;
    }
}

/**
 * @brief vérifie les collisions d’une entité avec les obstacles
 * @param e1 entité possiblement en collision
 * @param correction_defilement valeur à retourner si `e1` est le personnage joueur
 */
void verif_collision(t_entite * e1, float * correction_defilement) {
    // points haut gauche, haut droit etc., décalés sur l’axe vertical pour ne pas déclencher
    // une collision à la fois sur la gauche, la droite et le bas
    SDL_FPoint e1_hg = {e1->hitbox.x, e1->hitbox.y + 0.05 * e1->hitbox.h};
    SDL_FPoint e1_hd = {e1->hitbox.x + e1->hitbox.w, e1->hitbox.y + 0.05 * e1->hitbox.h};
    SDL_FPoint e1_bg = {e1->hitbox.x, e1->hitbox.y + 0.8 * e1->hitbox.h};
    SDL_FPoint e1_bd = {e1->hitbox.x + e1->hitbox.w, e1->hitbox.y + 0.8 * e1->hitbox.h};
    // point au milieu du bas de la hitbox
    SDL_FPoint e1_b = {e1->hitbox.x + 0.5 * e1->hitbox.w, e1->hitbox.y + e1->hitbox.h};

    // if (strcmp(e1->type, "matt") == 0) return;
    if ((e1->perso && e1->perso->est_mort) || (e1->pnj && e1->pnj->est_mort))
        return;

    e1->collisions.g = NULL;
    e1->collisions.d = NULL;
    e1->collisions.h = NULL;
    e1->collisions.b = NULL;

    en_tete(I_LISTE_ENTITES);
    while (!hors_liste(I_LISTE_ENTITES)) {
        t_entite * e2 = valeur_elt(I_LISTE_ENTITES);
        
        if (e1 == e2 || (!e2->est_obstacle && !(e2->pnj && e2->pnj->est_ecrasable)) 
            || (e2->pnj && e2->pnj->est_mort) || (e2->perso && e2->perso->est_mort)) {
            suivant(I_LISTE_ENTITES);
            continue;
        }

        // il y a collision à gauche, droite ou en haut si les deux points correspondants sont
        // dans la hitbox de l’entité candidate
        int collision_g = PointInFRect(&e1_hg, &(e2->hitbox)) || PointInFRect(&e1_bg, &(e2->hitbox));
        int collision_d = PointInFRect(&e1_hd, &(e2->hitbox)) || PointInFRect(&e1_bd, &(e2->hitbox));
        int collision_h = PointInFRect(&e1_hg, &(e2->hitbox)) || PointInFRect(&e1_hg, &(e2->hitbox));
        // il y a collision en bas si le point au milieu du bas de la hitbox et dans la hitbox candidate
        int collision_b = PointInFRect(&e1_b, &(e2->hitbox));
        // l’entité a une collision à gauche si elle est en collision à gauche avec au moins une entité de la liste
        // idem pour droite, haut, bas
        if (collision_g)
            e1->collisions.g = e2;
        if (collision_d)
            e1->collisions.d = e2;
        if (collision_h)
            e1->collisions.h = e2;
        if (collision_b)
            e1->collisions.b = e2;

        // replacement de l’entité si le chevauchement avec l’entité en collision est non négligeable
        if (collision_g && !collision_d) {
            float depassement = e2->hitbox.x + e2->hitbox.w - e1->hitbox.x;
            if (depassement >= 2)
                changer_pos_rel(e1, depassement, 0);
        }
        if (collision_d && !collision_g) {
            float depassement = e2->hitbox.x - (e1->hitbox.x + e1->hitbox.w);
            if (depassement <= -2)
                changer_pos_rel(e1, depassement, 0);
        }
        // replacement pour le chevauchement par le bas, utilisation du défilement car on suppose ici
        // que l’entité est le personnage joueur (à adapter quand nécessaire)
        if (correction_defilement != NULL && collision_b) {
            float depassement = e2->hitbox.y - (e1->hitbox.y + e1->hitbox.h);
            if (depassement < 0)
                *correction_defilement = depassement;
        }

        suivant(I_LISTE_ENTITES);
    }
}

void creuser(t_entite * a_creuser) {
    en_queue(I_LISTE_ENTITES);
    while(!hors_liste(I_LISTE_ENTITES)) {
        t_entite * elem = valeur_elt(I_LISTE_ENTITES);
        if (a_creuser == elem && elem->destructible) {
            jouer_audio(1, elem->destructible->id_son, 0);
            oter_elt(I_LISTE_ENTITES);
            detruire_entite(&elem);
            break;
        }
        else
            precedent(I_LISTE_ENTITES);
    }
}

void calculer_ombre(SDL_Texture * tex_ombre, int rayon,
                    t_entite * perso,
                    SDL_FRect rect_zone) {
    SDL_PixelFormat * format = SDL_AllocFormat(SDL_PIXELFORMAT_RGBA8888);
    Uint32 * pixels;
    int pitch;
    // permettre d’écrire les pixels de la texture
    SDL_LockTexture(tex_ombre, NULL, (void**)&pixels, &pitch);

    SDL_FRect rect_perso_abs;

    // gestion manuelle du décalage pour les animations d’attaque, pas idéal
    float w, h;
    if (perso->animation_courante->id == ATTQ_G || perso->animation_courante->id == ATTQ_D) {
        SDL_FRect rect_perso_decale = {perso->rect_dst->x + perso->animation_courante->decalage_dest_x, perso->rect_dst->y + perso->animation_courante->decalage_dest_y, perso->rect_dst->w, perso->rect_dst->h};
        rect_perso_abs = convertir_vers_absolu(&rect_perso_decale, rect_zone);
        w = rect_perso_abs.w / 3;
        h = rect_perso_abs.h / 3;
    }
    else {
        rect_perso_abs = convertir_vers_absolu(perso->rect_dst, rect_zone);
        w = rect_perso_abs.w;
        h = rect_perso_abs.h;
    }

    for (int i = rect_zone.x; i < rect_zone.x + rect_zone.w; i++) {
        for (int j = rect_zone.y; j < rect_zone.y + rect_zone.h; j++) {
            int dx = (i-(rect_perso_abs.x+w/2));
            int dy = (j-(rect_perso_abs.y+h/2));
            int carre_distance = dx*dx + dy*dy;
            // si la distance euclidienne entre le point (i,j) de la zone
            // de jeu et le centre du personnage est plus grande que `rayon`
            // écrire un pixel d’autant moins transparent qu’il est loin
            if (carre_distance > rayon*rayon) {
                float alpha = ((float)(carre_distance - rayon*rayon)) / (TAILLE_L/2*TAILLE_L/2 + TAILLE_H/2*TAILLE_H/2) * 255. * FACTEUR_OBSCURCISSEMENT;
                pixels[j * TAILLE_L + i] = SDL_MapRGBA(format, 0, 0, 0, alpha < 255 ? alpha : 255);
            }
            // sinon écrire un pixel transparent
            else
                pixels[j * TAILLE_L + i] = SDL_MapRGBA(format, 0, 0, 0, 0);
        }
    }

    // fin de l’écriture des pixels
    SDL_UnlockTexture(tex_ombre);
    SDL_FreeFormat(format);
}


int boucle_jeu(SDL_Renderer * rend) {
    SDL_Event event;
    int doit_boucler = VRAI;
    int est_en_pause = FAUX;
    long long compteur_frames = 0;
    float pas_defilement = 0;
    float repere_defilement = 100;
    int score = 0;
    int creusage_en_cours = FAUX; // Indicateur si l'animation de creusage est en cours
    int compteur_creusage = 0;


    srand(time(NULL));

    init_liste(I_LISTE_ENTITES);
    init_liste(I_LISTE_MORCEAUX_NIVEAU);

    int doit_quitter = FAUX;

    t_entite * fond, * fond_tour, * fond_tour_2, * perso;
    t_entite * fond_nuit;

    // Initialisation des entités de fond et de personnage
    fond = creer_entite("fond_jeu", -1, -1, -1, -1, FAUX);
    fond_nuit = creer_entite("fond_jeu_nuit", -1, -1, -1, -1, FAUX);
    fond_tour = creer_entite("fond_tour", 0, 0, 100, 100, VRAI);
    fond_tour_2 = creer_entite("fond_tour", 0, 100, 100, 100, VRAI);

    perso = creer_entite_perso((char*)personnage_selectionne, 40, 20, 15, 12, VRAI);
    
    generer_murs();

    int lumiere_est_allumee = VRAI;
    int lumiere_est_allumee_prec = FAUX;

    // texture pour afficher une ombre quand la lumière est éteinte
    SDL_FRect zone_jeu = {TAILLE_L/4, 0, TAILLE_L/2, TAILLE_H};
    SDL_Texture * tex_ombre = SDL_CreateTexture(rend, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, TAILLE_L, TAILLE_H);
    SDL_SetTextureBlendMode(tex_ombre, SDL_BLENDMODE_BLEND);
    int rayon_ombre = FACTEUR_MIN_RAYON_OMBRE * perso->rect_dst->w;
    SDL_SetTextureBlendMode(fond->texture, SDL_BLENDMODE_BLEND);
    SDL_SetTextureBlendMode(fond_nuit->texture, SDL_BLENDMODE_BLEND);
    int alpha_fond = 0;

    // compteur de FPS
    t_texte * texte_fps = creer_texte("police_defaut", 0, 0, 0, 255, 20, TAILLE_H-40, 100, 30);

    t_texte * texte_score = creer_texte("police_defaut", 0, 0, 0, 255, 20, 20, 160, 50);
    changer_texte(texte_score, "POINTS : %i", score);

    // chronométrage du temps de chaque frame
    clock_t chrono_deb, chrono_fin;
    int microsec_par_frame;

    // Boucle de jeu principale
    while (doit_boucler) {
        chrono_deb = clock();
        // Gestion des événements
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    doit_boucler = FAUX;
                    doit_quitter = VRAI ;
                    break;
                case SDL_KEYDOWN:
                    if (event.key.repeat) break;
                    // arrêter le creusage si l’on appuie sur une touche
                    // différente de celle de creusage même si l’on a maintenu
                    // cette dernière enfoncée
                    if (event.key.keysym.scancode != SDL_SCANCODE_S)
                        creusage_en_cours = FAUX;
                    switch (event.key.keysym.scancode) {
                        case SDL_SCANCODE_ESCAPE:
                        case SDL_SCANCODE_Q:
                            doit_boucler = FAUX;
                            break;
                        case SDL_SCANCODE_SPACE:
                            est_en_pause = !est_en_pause;
                            break;
                        case SDL_SCANCODE_H:
                            perso->doit_afficher_hitbox = !perso->doit_afficher_hitbox;
                            break;
                        case SDL_SCANCODE_L:
                            lumiere_est_allumee = !lumiere_est_allumee;
                            lumiere_est_allumee_prec = !lumiere_est_allumee;
                            break;
                        case SDL_SCANCODE_A:
                            perso->deplacement_prec = perso->deplacement;
                            perso->deplacement = GAUCHE;
                            if (perso->collisions.b)
                                changer_animation(perso, DEPL_G);
                            break;
                        case SDL_SCANCODE_D:
                            perso->deplacement_prec = perso->deplacement;
                            perso->deplacement = DROITE;
                            if (perso->collisions.b)
                                changer_animation(perso, DEPL_D);
                            break;
                        case SDL_SCANCODE_S:
                            if (perso->collisions.b){
                                changer_animation(perso, CREUSER); 
                                perso->deplacement_prec = perso->deplacement;
                                perso->deplacement = REPOS_MVT;
                                compteur_creusage = 0; // Réinitialiser le compteur à chaque fois que la touche est enfoncée
                                creusage_en_cours = VRAI; // Marquer que l'animation de creusage est en cours
                            }
                            break;
                        case SDL_SCANCODE_W:
                            if (perso->collisions.b) {
                                perso->deplacement_prec = perso->deplacement;
                                perso->deplacement = REPOS_MVT;
                                if (perso->sens_regard == GAUCHE) {
                                    changer_animation(perso, ATTQ_G);
                                    creuser(perso->collisions.g);
                                }
                                else if (perso->sens_regard == DROITE) {
                                    changer_animation(perso, ATTQ_D);
                                    creuser(perso->collisions.d);
                                }
                                perso_porter_coup(perso, &score, texte_score);
                            }
                            break;
                        default:
                            break;
                    }
                    break;
                case SDL_KEYUP:
                    switch (event.key.keysym.scancode) {
                        case SDL_SCANCODE_A:
                            if (perso->animation_courante->id == DEPL_G) {
                                if (perso->collisions.b)
                                    changer_animation(perso, REPOS);
                            }
                            if (perso->deplacement == perso->deplacement_prec)
                                perso->deplacement = REPOS_MVT;
                            else if (perso->deplacement == GAUCHE)
                                perso->deplacement = perso->deplacement_prec;
                            if (perso->deplacement_prec == GAUCHE)
                                perso->deplacement_prec = REPOS_MVT;
                            break;
                        case SDL_SCANCODE_D:
                            if (perso->animation_courante->id == DEPL_D) {
                                if (perso->collisions.b)
                                    changer_animation(perso, REPOS);
                            }
                            if (perso->deplacement == perso->deplacement_prec)
                                perso->deplacement = REPOS_MVT;
                            else if (perso->deplacement == DROITE)
                                perso->deplacement = perso->deplacement_prec;
                            if (perso->deplacement_prec == DROITE)
                                perso->deplacement_prec = REPOS_MVT;
                            break;
                        case SDL_SCANCODE_S:
                            if (perso->animation_courante->id == CREUSER) {
                                perso->deplacement = perso->deplacement_prec;
                                changer_animation(perso, REPOS);
                            }
                            creusage_en_cours = FAUX;
                            break;
                        case SDL_SCANCODE_W:
                            if (perso->animation_courante->id ==  ATTQ_G || perso->animation_courante->id == ATTQ_D) {
                                perso->deplacement = perso->deplacement_prec;
                                changer_animation(perso, REPOS);
                            }
                            break;
                        default:
                            break;
                    }
            }
           
        }
        if (est_en_pause) {
            SDL_Delay(1000/FPS);
            continue;
        }

        if (creusage_en_cours) {
            compteur_creusage++;
            if (compteur_creusage >= DELAI_CREUSAGE) {
                creuser(perso->collisions.b);
                creusage_en_cours = FAUX;
            } 
        }
        
        SDL_RenderClear(rend);

        if (lumiere_est_allumee && !lumiere_est_allumee_prec) {
            alpha_fond += PAS_ALPHA_FOND;
            if (alpha_fond >= 255)
                alpha_fond = 255;

            rayon_ombre += PAS_RAYON_OMBRE;
            if (rayon_ombre >= MAX_RAYON_OMBRE)
                rayon_ombre = MAX_RAYON_OMBRE;
            calculer_ombre(tex_ombre, rayon_ombre, perso, zone_jeu);

            if (alpha_fond >= 255 && rayon_ombre >= MAX_RAYON_OMBRE)
                lumiere_est_allumee_prec = VRAI;
        }
        else if (!lumiere_est_allumee && lumiere_est_allumee_prec) {
            alpha_fond -= PAS_ALPHA_FOND;
            if (alpha_fond <= 0)
                alpha_fond = 0;

            rayon_ombre -= PAS_RAYON_OMBRE;
            if (rayon_ombre <= FACTEUR_MIN_RAYON_OMBRE*perso->rect_dst->w)
                rayon_ombre = FACTEUR_MIN_RAYON_OMBRE * perso->rect_dst->w;
            calculer_ombre(tex_ombre, rayon_ombre, perso, zone_jeu);

            if (alpha_fond <= 0 && rayon_ombre <= FACTEUR_MIN_RAYON_OMBRE*perso->rect_dst->w)
                lumiere_est_allumee_prec = FAUX;
        }
        else if (!lumiere_est_allumee) {
            calculer_ombre(tex_ombre, rayon_ombre, perso, zone_jeu);
        }

        // Affichage des entités
        SDL_SetTextureAlphaMod(fond->texture, alpha_fond);
        SDL_SetTextureAlphaMod(fond_nuit->texture, 255-alpha_fond);
        afficher_entite(rend, fond);
        afficher_entite(rend, fond_nuit);

        afficher_entite(rend, fond_tour);
        afficher_entite(rend, fond_tour_2);

        if (perso->perso->temps_invu > 0) {
            perso->perso->temps_invu--;
            // faire clignoter le perso une fois toutes les 4 frames
            if (perso->perso->temps_invu % 4 > 0)
                afficher_entite(rend, perso);
        }
        else
            afficher_entite(rend, perso);

        en_tete(I_LISTE_ENTITES);
        while (!hors_liste(I_LISTE_ENTITES)) {
            t_entite * elem =  valeur_elt(I_LISTE_ENTITES);
            afficher_entite(rend, elem);
            suivant(I_LISTE_ENTITES);
        }

        SDL_RenderCopy(rend, tex_ombre, NULL, NULL);

        afficher_texte(rend, texte_fps);
        afficher_texte(rend, texte_score);

        SDL_RenderPresent(rend);

        // Gestion des collisions
        float correction_defilement = 0;
        verif_collision(perso, &correction_defilement);

        // Déplacement et animation du personnage
        if (! perso->collisions.b)
            changer_animation(perso, perso->sens_regard == GAUCHE ? CHUTE_G : CHUTE_D);
        else if (perso->animation_courante->id == CHUTE_G || perso->animation_courante->id == CHUTE_D)
            changer_animation(perso, REPOS);
        else if (perso->animation_courante->id == REPOS) {
            if (perso->deplacement == GAUCHE)
                changer_animation(perso, DEPL_G);
            else if (perso->deplacement == DROITE)
                changer_animation(perso, DEPL_D);
        }
        deplacer(perso, compteur_frames);
        animer(perso, compteur_frames);

        // évolution du comportement des pnj
        t_entite * pnjs[100]; // tableau temporaire pour stocker les pnjs trouvés car la vérification de collision interfère avec le parcours de la liste
        int n_pnjs = 0;
        en_tete(I_LISTE_ENTITES);
        while (!hors_liste(I_LISTE_ENTITES)) {
            t_entite * elem = valeur_elt(I_LISTE_ENTITES);
            if (elem->pnj)
                pnjs[n_pnjs++] = elem;
            suivant(I_LISTE_ENTITES);
        }
        for (int i = 0; i < n_pnjs; i++) {
            // tue un ennemi qui peut être écrasé si le personnage lui tombe dessus
            if (pnjs[i]->pnj->est_ecrasable && perso->collisions.b == pnjs[i]) {
                pnjs[i]->id_animation_suivante = ANIM_MORT_STATIQUE;
                changer_animation(pnjs[i], ANIM_MORT);
                pnjs[i]->deplacement = REPOS_MVT;
                pnjs[i]->pnj->est_mort = VRAI;
                pnjs[i]->pnj->est_ecrasable = FAUX;
                score += pnjs[i]->pnj->valeur_vaincu;
                changer_texte(texte_score, "POINTS : %i", score);

                continue;
            }
            // évolution du comportement du pnj i
            verif_collision(pnjs[i], NULL);
            pnjs[i]->pnj->comportement(pnjs[i], perso);
            deplacer(pnjs[i], compteur_frames);
            animer(pnjs[i], compteur_frames);
        }

        // Logique de défilement des obstacles
        if (!perso->collisions.b) {
            // Calcul du pas de défilement en fonction de la vitesse de chute et du compteur de frames
            if (VITESSE_CHUTE >= 1) {
                pas_defilement = VITESSE_CHUTE;
            } else if (compteur_frames % (int)(1 / (VITESSE_CHUTE)) == 0) {
                pas_defilement = 1;
            } else {
                pas_defilement = 0;
            }
        }
        else {
            pas_defilement = correction_defilement;
        }
        repere_defilement += pas_defilement;

        // Déplacement relatif des obstacles pour simuler le défilement
        en_tete(I_LISTE_ENTITES);
        while (!hors_liste(I_LISTE_ENTITES)) {
            t_entite * elem = valeur_elt(I_LISTE_ENTITES);
            changer_pos_rel(elem, 0, -pas_defilement);
            suivant(I_LISTE_ENTITES);
        }
        changer_pos_rel(fond_tour, 0, -pas_defilement);
        changer_pos_rel(fond_tour_2, 0, -pas_defilement);

        // Génération de nouvelles entités et des fonds de tour alternés une fois qu’une hauteur de tour a défilé
        float bas_fond_tour = fond_tour->rect_dst->y + fond_tour->rect_dst->h;
        float bas_fond_tour_2 = fond_tour_2->rect_dst->y + fond_tour_2->rect_dst->h;
        if (bas_fond_tour < 0) {
            detruire_entite(&fond_tour);
            fond_tour = creer_entite("fond_tour", 0, bas_fond_tour_2, 100, 100, VRAI);
        }
        if (bas_fond_tour_2 < 0) {
            detruire_entite(&fond_tour_2);
            fond_tour_2 = creer_entite("fond_tour", 0, bas_fond_tour, 100, 100, VRAI);
        }
        if (bas_fond_tour < 0 || bas_fond_tour_2 < 0) {
            // suppression des entités ayant défilé au-dessus de la zone de jeu
            en_queue(I_LISTE_ENTITES);
            while (!hors_liste(I_LISTE_ENTITES)) {
                t_entite * elem = valeur_elt(I_LISTE_ENTITES);
                if (elem->rect_dst->y + elem->rect_dst->h < 0) {
                    oter_elt(I_LISTE_ENTITES);
                    detruire_entite(&elem);
                }
                else
                    precedent(I_LISTE_ENTITES);
            }

            generer_murs();
        }
        if (repere_defilement > 100) {
            generer_morceau_niveau(repere_defilement);
            repere_defilement = 0;
        }

        compteur_frames++;

        chrono_fin = clock();
        microsec_par_frame = (chrono_fin - chrono_deb) * 1000000 / CLOCKS_PER_SEC;
        // printf("%i μs\n", microsec_par_frame);
        if (compteur_frames % 10 == 0)
            changer_texte(texte_fps, "%.2f FPS", 1000000./microsec_par_frame);

        int attente = 1000 / FPS - microsec_par_frame/1000;

        SDL_Delay(attente > 0 ? attente : 0); // Contrôle du taux de rafraîchissement
    }

    // Libération des ressources
    en_queue(I_LISTE_ENTITES);
    while (!hors_liste(I_LISTE_ENTITES)) {
        t_entite * elem = valeur_elt(I_LISTE_ENTITES);
        detruire_entite(&elem);
        oter_elt(I_LISTE_ENTITES);
    }
    detruire_entite(&fond);
    detruire_entite(&fond_tour);
    detruire_entite(&fond_tour_2);
    detruire_entite(&perso);
    detruire_texte(&texte_fps);
    detruire_texte(&texte_score);

    return doit_quitter ;
}
