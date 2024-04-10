#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "morceaux_niveau.h"
#include "constantes.h"
#include "entite.h"
#include "entite_destructible.h"
#include "entite_obstacle.h"
#include "entite_pnj.h"
#include "listes.h"

#define LARGEUR_MUR 10

void (*fonctions_generation[N_MORCEAUX_NIVEAU])(void) = {
    generer_morceau_niveau_0,
};

void enfiler_fonction(int i) {
    int * i_fonction = malloc(sizeof(int));
    *i_fonction = i;
    printf("ajout à la file du morceau de niveau %i\n", i);
    ajout_droit(I_LISTE_MORCEAUX_NIVEAU, i_fonction);
}

void generer_morceau_niveau(int choix) {
    en_tete(I_LISTE_MORCEAUX_NIVEAU);
    if (hors_liste(I_LISTE_MORCEAUX_NIVEAU)) {
        int i_fonction;
        if (choix < 0 || choix >= N_MORCEAUX_NIVEAU) {
            i_fonction = rand() % N_MORCEAUX_NIVEAU;
            printf("génération morceau de niveau aléatoire %i\n", i_fonction);
        }
        else {
            i_fonction = choix;
            printf("génération morceau de niveau depuis paramètre %i\n", i_fonction);
        }
        fonctions_generation[i_fonction]();
    }
    else {
        int * i_fonction = oter_elt(I_LISTE_MORCEAUX_NIVEAU);
        printf("génération morceau de niveau depuis la file %i\n", *i_fonction);
        fonctions_generation[*i_fonction]();
        free(i_fonction);
    }
}

// génération par fonctions C plutôt que par parsage d’un fichier pour
// bénéficier de la Turing-complétion

void generer_ennemi(int x, int y){
    t_entite * nouv;
    nouv = creer_entite_pnj_depuis_spritesheet("squelette", x, y, 20, 20, VRAI);
    nouv->vitesse = 1./2;
    changer_hitbox(nouv, 20, 50, 50, 55);
    ajout_droit(I_LISTE_ENTITES, nouv);
}
void generer_morceau_niveau_0(void){
    en_queue(I_LISTE_ENTITES);

    // Demander un nombre aléatoire entre 0 et 10 à l'utilisateur
    int random;
    srand(time(NULL));

    int i, j, x = 10, y=110 ;
    int nb_pierres = 0;
    int ennemi;

    for (i=0; i<10; i++, y+=10){
        
        if (i%2 == 0) ;     // Une fois sur deux il n'y a pas de ligne de blocs
        else {
            for (j=0, x=10, nb_pierres = 0, ennemi=0; j<8; j++, x+= 10){
                random = rand() % 10 + 1;
                if (random < 4 && nb_pierres < 7){
                        ajout_droit(I_LISTE_ENTITES, creer_entite_obstacle("bloc_pierre", x, y, 10, 10, VRAI));
                        nb_pierres++;
                        if (i!=0 && i%5==0 && ennemi==0){
                            generer_ennemi(x,y);
                            ennemi=1;
                        }
                }
                else if (random == 4 || random == 5);
                else {
                    ajout_droit(I_LISTE_ENTITES, creer_entite_destructible("bloc_terre", x, y, 10, 10, VRAI));
                    if (i!=0 && i%5==0 && ennemi==0){
                            generer_ennemi(x,y);
                            ennemi=1;
                    }
                }
            }
        }
    }
}


void generer_murs(void) {
    int n_blocs_mur = 20; // nombre pour chaque mur

    en_queue(I_LISTE_ENTITES);
    for (int i = 0; i < n_blocs_mur; i++) {
        ajout_droit(I_LISTE_ENTITES, creer_entite_obstacle("bloc_pierre", 0, i*5, LARGEUR_MUR, 10, VRAI)); // côté gauche
        ajout_droit(I_LISTE_ENTITES, creer_entite_obstacle("bloc_pierre", 0, (n_blocs_mur+i)*5, LARGEUR_MUR, 10, VRAI)); // côté gauche écran du dessous
        ajout_droit(I_LISTE_ENTITES, creer_entite_obstacle("bloc_pierre", 90, i*5, LARGEUR_MUR, 10, VRAI)); // côté droit
        ajout_droit(I_LISTE_ENTITES, creer_entite_obstacle("bloc_pierre", 90, (n_blocs_mur+i)*5, LARGEUR_MUR, 10, VRAI)); // côté droit écran du dessous
    }
}