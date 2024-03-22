#include <stdlib.h>                                                                                           
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include "tour.h"
#include "constantes.h"
#include "entite.h"
#include "spritesheets.h"



// Vérifie si deux entités se chevauchent
void verif_collision(t_entite * e1, t_entite * e2) {
    int a_collision = SDL_HasIntersection(&(e1->hitbox), &(e2->hitbox));
    e1->a_collision = e2->a_collision = a_collision;
}

void boucle_jeu(SDL_Renderer * rend) {
    SDL_Event event;
    int doit_boucler = VRAI;
    int repere_defilement = 0;
    long long compteur_frames = 0;
    int pas_defilement = 0;
    int parite_defilement = 0;
    const int DUREE_CREUSER = 8; // Nombre de frames de l'animation "creuser"

	//obstacle -> case pierre obstacle_terre -> case en terre
    // Textures pour les obstacles
    SDL_Texture * tex_obstacle, * tex_obstacle_terre;
    t_entite * fond, * fond_tour, * fond_tour_2, * perso;
    t_entite * obstacle, * obstacle2, * obstacle3 = NULL, * obstacle4 = NULL;
    t_entite * obstacle_terre, * obstacle_terre2, * obstacle_terre3 = NULL, *obstacle_terre4 = NULL;

    // Chargement de l'image de blocs de pierre pour les obstacles
    tex_obstacle = IMG_LoadTexture(rend, "ressources/Tour/Blocs/pierres_sombres/pierre.jpg");
    if (!tex_obstacle) {
        printf("Erreur lors du chargement de l'image des blocs de pierre : %s\n", IMG_GetError());
        // Générer une texture de secours en cas d'échec
        tex_obstacle = SDL_CreateTexture(rend, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, TAILLE_L, TAILLE_H);
        SDL_SetRenderTarget(rend, tex_obstacle);
        SDL_SetRenderDrawColor(rend, 180, 180, 80, 255);
        SDL_RenderFillRect(rend, NULL);
        SDL_SetRenderTarget(rend, NULL);
    }

    // Chargement de l'image de blocs de terre pour les obstacles
    tex_obstacle_terre = IMG_LoadTexture(rend, "ressources/Tour/Blocs/terre/terre.jpg");
    if (!tex_obstacle_terre) {
        printf("Erreur lors du chargement de l'image des blocs de terre : %s\n", IMG_GetError());
        // Générer une texture de secours en cas d'échec
        tex_obstacle_terre = SDL_CreateTexture(rend, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, TAILLE_L, TAILLE_H);
        SDL_SetRenderTarget(rend, tex_obstacle_terre);
        SDL_SetRenderDrawColor(rend, 150, 75, 0, 255); // Couleur marron pour terre
        SDL_RenderFillRect(rend, NULL);
        SDL_SetRenderTarget(rend, NULL);
    }

    // Création des entités obstacles avec les images chargées
    // Blocs de pierre
    obstacle = creer_entite_depuis_texture(tex_obstacle, 0, 110, 10, 10, VRAI); // Taille ajustée pour être carrée
    obstacle2 = creer_entite_depuis_texture(tex_obstacle, 50, 170, 10, 10, VRAI); // Taille ajustée pour être carrée

    // Blocs de terre
    obstacle_terre = creer_entite_depuis_texture(tex_obstacle_terre, 10, 110, 10, 10, VRAI); // Taille ajustée pour être carrée
    obstacle_terre2 = creer_entite_depuis_texture(tex_obstacle_terre, 60, 170, 10, 10, VRAI); // Taille ajustée pour être carrée

    // Initialisation des entités de fond et de personnage
    fond = creer_entite("fond_jeu", -1, -1, -1, -1, FAUX);
    fond_tour = creer_entite("fond_tour", 0, 0, 100, 100, VRAI);
    fond_tour_2 = creer_entite("fond_tour", 0, 100, 100, 100, VRAI);

    perso = creer_entite_depuis_spritesheet("matt", 40, 20, 18, 12, VRAI);
    
    changer_hitbox(perso, 26, 24, 51, 76);
    perso->doit_afficher_hitbox = VRAI;

    // Activation de l'affichage des hitbox pour les obstacles
    obstacle->doit_afficher_hitbox = VRAI;
    obstacle_terre->doit_afficher_hitbox = VRAI;

    // Boucle de jeu principale
    while (doit_boucler) {
        // Gestion des événements
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_KEYDOWN:
                    if (event.key.repeat) break;
                    // Gestion des touches du clavier
                    switch (event.key.keysym.scancode) {
                        case SDL_SCANCODE_ESCAPE:
                        case SDL_SCANCODE_Q:
                            doit_boucler = FAUX;
                            break;
                        case SDL_SCANCODE_H:
                            perso->doit_afficher_hitbox = !perso->doit_afficher_hitbox;
                            break;
                        case SDL_SCANCODE_A:
                            perso->deplacement = GAUCHE;
                            if (perso->a_collision)
                                changer_animation(perso, DEPL_G);
                            break;
                        case SDL_SCANCODE_D:
                            perso->deplacement = DROITE;
                            if (perso->a_collision)
                                changer_animation(perso, DEPL_D);
                            break;
                        case SDL_SCANCODE_S:
                            if (perso->a_collision){
                                changer_animation(perso, CREUSER); 
                                if (obstacle_terre != NULL && SDL_HasIntersection(&(perso->hitbox), &(obstacle_terre->hitbox))) 
                                {
                                detruire_entite(&obstacle_terre);
                                }
                                if (obstacle_terre2 != NULL && SDL_HasIntersection(&(perso->hitbox), &(obstacle_terre2->hitbox))) 
                                {
                                detruire_entite(&obstacle_terre2);
                                }
                                if (obstacle_terre3 != NULL && SDL_HasIntersection(&(perso->hitbox), &(obstacle_terre3->hitbox))) 
                                {
                                detruire_entite(&obstacle_terre3);
                                }
                                if (obstacle_terre4 != NULL && SDL_HasIntersection(&(perso->hitbox), &(obstacle_terre4->hitbox))) 
                                {
                                detruire_entite(&obstacle_terre4);
                                }
                                }
                            break;
                        case SDL_SCANCODE_W:
                            if (perso->a_collision) {
                                if (perso->sens_regard == GAUCHE)
                                    changer_animation(perso, ATTQ_G);
                                else if (perso->sens_regard == DROITE)
                                    changer_animation(perso, ATTQ_D);
                            }
                            break;
                        default:
                            break;
                    }
                    break;
                case SDL_KEYUP:
                    switch (event.key.keysym.scancode) {
                        case SDL_SCANCODE_A:
                        case SDL_SCANCODE_D:
                        case SDL_SCANCODE_S:
                        case SDL_SCANCODE_W:
                            if (perso->a_collision)
                                changer_animation(perso, REPOS);
                            perso->deplacement = REPOS_MVT;
                            break;
                        default:
                            break;
                    }
            }
        }

        // Affichage des entités
        SDL_RenderClear(rend);
        fond->afficher(rend, fond);
        fond_tour->afficher(rend, fond_tour);
        fond_tour_2->afficher(rend, fond_tour_2);
        perso->afficher(rend, perso);
        obstacle->afficher(rend, obstacle);
        obstacle2->afficher(rend, obstacle2);
        if (obstacle_terre)
	        obstacle_terre->afficher(rend, obstacle_terre);
	if (obstacle_terre2)
        	obstacle_terre2->afficher(rend, obstacle_terre2);

        // Gestion des collisions
        verif_collision(perso, obstacle);
        if (!perso->a_collision) {
            verif_collision(perso, obstacle2);
        }
        if (!perso->a_collision && obstacle_terre) {
            verif_collision(perso, obstacle_terre);
        }
        if (!perso->a_collision && obstacle_terre2) {
            verif_collision(perso, obstacle_terre2);
        }

        // Déplacement et animation du personnage
        if (! perso->a_collision)
            changer_animation(perso, perso->sens_regard == GAUCHE ? CHUTE_G : CHUTE_D);
        deplacer(perso);
        animer(perso, compteur_frames);

        // Logique de défilement des obstacles
        if (!perso->a_collision) {
            // Calcul du pas de défilement en fonction de la vitesse de chute et du compteur de frames
            if (VITESSE_CHUTE >= 1) {
                pas_defilement = (int)VITESSE_CHUTE;
            } else if (compteur_frames % (int)(1 / (VITESSE_CHUTE)) == 0) {
                pas_defilement = 1;
            } else {
                pas_defilement = 0;
            }

            // Déplacement relatif des obstacles pour simuler le défilement
            obstacle->changer_pos_rel(obstacle, 0, -pas_defilement);
            obstacle2->changer_pos_rel(obstacle2, 0, -pas_defilement);
            if (obstacle_terre)
	            obstacle_terre->changer_pos_rel(obstacle_terre, 0, -pas_defilement);
            if (obstacle_terre2)
            obstacle_terre2->changer_pos_rel(obstacle_terre2, 0, -pas_defilement);
            fond_tour->changer_pos_rel(fond_tour, 0, -pas_defilement);
            fond_tour_2->changer_pos_rel(fond_tour_2, 0, -pas_defilement);

            repere_defilement += pas_defilement;
        }

        // Recréation des obstacles pour simuler un nouveau set d'obstacles après un certain défilement
        if (repere_defilement > 100 && parite_defilement == 0) {
            parite_defilement = 1;
            detruire_entite(&obstacle3);
            detruire_entite(&obstacle4);
            detruire_entite(&fond_tour);
            obstacle3 = creer_entite_depuis_texture(tex_obstacle, 0, 110, 10, 10, VRAI);
            obstacle4 = creer_entite_depuis_texture(tex_obstacle, 50, 170, 10, 10, VRAI);

            detruire_entite(&obstacle_terre3);
            detruire_entite(&obstacle_terre4);
            obstacle_terre3 = creer_entite_depuis_texture(tex_obstacle_terre, 10, 110, 10, 10, VRAI);
            obstacle_terre4 = creer_entite_depuis_texture(tex_obstacle_terre, 60, 170, 10, 10, VRAI);
            fond_tour = creer_entite("fond_tour",
                             0, fond_tour_2->rect_dst->y + fond_tour_2->rect_dst->h,
                             100, 100, VRAI);
        }
        if (repere_defilement >= 200 && parite_defilement == 1) {
            parite_defilement = 0;
            repere_defilement = 0;
            detruire_entite(&obstacle);
            detruire_entite(&obstacle2);
            detruire_entite(&fond_tour_2);
            detruire_entite(&obstacle_terre);
            detruire_entite(&obstacle_terre2);

            obstacle = creer_entite_depuis_texture(tex_obstacle, 0, 110, 10, 10, VRAI);
            obstacle2 = creer_entite_depuis_texture(tex_obstacle, 50, 170, 10, 10, VRAI);
            obstacle_terre = creer_entite_depuis_texture(tex_obstacle_terre, 10, 110, 10, 10, VRAI);
            obstacle_terre2 = creer_entite_depuis_texture(tex_obstacle_terre, 60, 170, 10, 10, VRAI);
            fond_tour_2 = creer_entite("fond_tour",
                             0, fond_tour->rect_dst->y + fond_tour->rect_dst->h,
                             100, 100, VRAI);
        }

        SDL_RenderPresent(rend);
        SDL_Delay(1000 / FPS); // Contrôle du taux de rafraîchissement
        compteur_frames++;
    }

    // Libération des ressources
    detruire_entite(&fond);
    detruire_entite(&fond_tour);
    detruire_entite(&fond_tour_2);
    detruire_entite(&perso);
    detruire_entite(&obstacle);
    detruire_entite(&obstacle2);
    detruire_entite(&obstacle3);
    detruire_entite(&obstacle4);
    detruire_entite(&obstacle_terre);
    detruire_entite(&obstacle_terre2);
    detruire_entite(&obstacle_terre3);
    detruire_entite(&obstacle_terre4);
}
