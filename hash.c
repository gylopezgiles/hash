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
    campo_t* tabla;
    hash_destruir_dato_t destruir;
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
    for (int i = 0; i < CAPACIDAD_INICIAL; ++i) {
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
    for (int i = 0; i < hash->cap; ++i) {
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