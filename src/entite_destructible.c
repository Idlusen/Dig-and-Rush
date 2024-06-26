/** @file
 *  @brief implémentation du caractère destructible d’une `t_entite` (source)
 */

#include <string.h>

#include "entite_destructible.h"
#include "entite.h"
#include "constantes.h"

/**
 * @brief Alloue la mémoire pour un `t_destructible` et initialise ses champs.
 * 
 * @param id Identifiant de ressource utilisé pour définir les valeurs par défaut.
 * @return pointeur vers le `t_destructible` alloué et initialisé
 */

t_destructible * creer_destructible(char * id) {
    t_destructible * nouv = malloc(sizeof(t_destructible));
    int alea;
    if (strcmp(id, "bloc_terre") == 0) {
        alea = rand() % 3;
        // Sélection aléatoire du son de destruction en fonction du nombre aléatoire
        if (alea == 0)
            strcpy(nouv->id_son, "destruction_bloc_1");
        else if (alea == 1)
            strcpy(nouv->id_son, "destruction_bloc_2");
        else if (alea == 2)
            strcpy(nouv->id_son, "destruction_bloc_3");
    }
    else
        strcpy(nouv->id_son, "destruction_bloc_1");
    return nouv;
}

/**
 * @brief Crée une nouvelle structure d'entité destructible avec les attributs spécifiés.
 * 
 * @param id Identifiant de l'entité destructible.
 * @param x Position en x de l'entité destructible.
 * @param y Position en y de l'entité destructible.
 * @param w Largeur de l'entité destructible.
 * @param h Hauteur de l'entité destructible.
 * @param est_relatif 1 si les coordonnées sont relatives, 0 si elles sont absolues.
 * @return Pointeur vers la nouvelle structure d'entité créée.
 */

t_entite * creer_entite_destructible(char * id, float x, float y, float w, float h, int est_relatif) {
    t_entite * nouv = creer_entite(id, x, y, w, h, est_relatif);
    nouv->destructible = creer_destructible(id);
    /** une entité destructible est aussi une entité obstacle */
    nouv->est_obstacle = VRAI;
    return nouv;
}

/**
 * @brief Détruit une structure d'entité destructible et libère la mémoire associée.
 * 
 * @param d Pointeur vers le pointeur de la structure d'entité destructible à détruire.
 */

void detruire_destructible(t_destructible ** d) {
    if (*d) {
        free(*d);
    }
    *d = NULL;
}
