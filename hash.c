#include "hash.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define CAPACIDAD_INICIAL 101 // Número primo
#define CONSTANTE_REDIMENSION 2

typedef enum estados {
    VACIO, OCUPADO, BORRADO // VACIO = 0, OCUPADO = 1, BORRADO = 2
}estados_t;

typedef struct campo {
    char* clave;
    void* valor;
    estados_t estado;
}campo_t;

struct hash {
    unsigned long capacidad;
    size_t cantidad;
    campo_t** tabla;
    hash_destruir_dato_t destruir;
};

struct hash_iter {
    hash_t* hash;
    unsigned long posicion;
};

// Función de Hash de "The C Programming Language (Second Edition)", Brian Kernighan & Dennis Ritchie, Capítulo 6, Pág. 144.
unsigned long funcion_hash(const hash_t* hash, const char* s) {
    unsigned long hashval;
    for (hashval=0; *s!='\0'; s++) 
        hashval = (unsigned long)*s + 31 * hashval;
    return hashval % hash->capacidad;
}

hash_t* hash_crear(hash_destruir_dato_t destruir_dato) {
    hash_t* hash = malloc(sizeof(hash_t));
    if (hash == NULL) {
        return NULL;
    }
    hash->tabla = malloc(sizeof(campo_t) * CAPACIDAD_INICIAL);
    if (hash->tabla == NULL) {
        free(hash);
        return NULL;
    }
    for (int i = 0; i < CAPACIDAD_INICIAL; i++) {
        hash->tabla[i]->estado = VACIO;
    }
    hash->capacidad = CAPACIDAD_INICIAL;
    hash->cantidad = 0;
    hash->destruir = destruir_dato;
    return hash;
}

size_t hash_cantidad(const hash_t* hash) {
    return hash->cantidad;
}

void hash_destruir(hash_t* hash) {
    for (int i = 0; i < hash->capacidad; i++) {
        if (hash->tabla[i]->estado == OCUPADO) {
            if (hash->destruir != NULL) { // hash->destruir vale la función hash_destruir_dato
                hash->destruir(hash->tabla[i]->valor);
                hash->destruir(hash->tabla[i]->clave);
            } else {
                free(hash->tabla[i]->valor);
                free(hash->tabla[i]->clave);
            }
            
        }
    }
    free(hash->tabla);
    free(hash);
}

bool hash_pertenece(const hash_t* hash, const char* clave) {
    if (hash->cantidad == 0) {
        return false;
    }
    unsigned long pos_en_hash = funcion_hash(hash, clave);
    return (hash->tabla[pos_en_hash]->estado == OCUPADO && (strcmp(clave, hash->tabla[pos_en_hash]->clave) == 0)); // Compara caracter a caracter, otra clave podría
}                                                                                                                  // haber colisionado en esa posición

void* hash_obtener(const hash_t* hash, const char* clave) {
    if (!hash_pertenece(hash, clave)) {
        return NULL;
    }
    unsigned long pos_en_hash = funcion_hash(hash, clave);
    return hash->tabla[pos_en_hash]->valor;
}

/* Iterador del hash */

hash_iter_t* hash_iter_crear(const hash_t *hash) {
    hash_iter_t* iter = malloc(sizeof(hash_iter_t));
    if (iter == NULL) {
        return NULL;
    }
    iter->posicion = 0;
    iter->hash = hash;
    return iter;
}

const char* hash_iter_ver_actual(const hash_iter_t *iter) {
    if (hash_iter_al_final(iter)) {
        return NULL;
    }
    return iter->hash->tabla[iter->posicion]->clave;
}

bool hash_iter_al_final(const hash_iter_t *iter) {
    return iter->posicion == iter->hash->capacidad;
}

void hash_iter_destruir(hash_iter_t* iter) {
    free(iter);
}

bool hash_iter_avanzar(hash_iter_t *iter) { // Sólo avanza por los campos ocupados
    if (hash_iter_al_final(iter)) {
        return false;
    }
    while (!hash_iter_al_final(iter)) {
        iter->posicion++;
        if (iter->hash->tabla[iter->posicion]->estado == OCUPADO) {
            return true;
        }
    }
}
