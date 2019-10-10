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
    campo_t** tabla;
    size_t cantidad;
    unsigned long posicion;
};

// Función de Hash de "The C Programming Language (Second Edition)", Brian Kernighan & Dennis Ritchie, Capítulo 6, Pág. 144.
unsigned long funcion_hash(const hash_t* hash, const char* s) {
    unsigned long hashval;
    for (hashval=0; *s != '\0' ; s++) 
        hashval = (unsigned long)*s + 31 * hashval;
    return hashval % hash->capacidad;
}

hash_t* hash_crear(hash_destruir_dato_t destruir_dato) {
    hash_t* hash = malloc(sizeof(hash_t));
    if (hash == NULL) {
        return NULL;
    }
    campo_t** tabla = malloc(sizeof(campo_t*)*CAPACIDAD_INICIAL);
    if (tabla == NULL) {
        free(hash);
        return NULL;
    }
    hash->tabla = tabla;
    hash->cantidad = 0;
    hash->capacidad = CAPACIDAD_INICIAL;
    hash->destruir = destruir_dato;
    return hash;
}

bool hash_redimensionar(hash_t* hash, size_t tam_nuevo){
    campo_t** tabla_nueva = realloc(hash->tabla, tam_nuevo * sizeof(campo_t*));
    if(tam_nuevo > 0 && tabla_nueva == NULL){
        return false;
    }
    hash->tabla = tabla_nueva;
    hash->capacidad = tam_nuevo;
    return true;
}

bool hash_guardar(hash_t* hash, const char* clave, void* dato){
    campo_t* campo = malloc(sizeof(campo_t));
    if (campo == NULL) {
        return NULL;
    }
    if(hash->capacidad == hash->cantidad){
        if(!hash_redimensionar(hash, hash->capacidad*2)){
            free(campo);
            return false;
        }
    }
    campo->valor = dato;
    campo->clave = malloc(sizeof(char));
    if(campo->clave == NULL){
        free(campo);
        return false;
    }
    strcpy(campo->clave, clave);
    campo->estado = OCUPADO;
    unsigned long posicion = funcion_hash(hash, clave);
    hash->tabla[posicion] = campo;
    hash->cantidad += 1u;
    return true;
}

void* hash_borrar(hash_t* hash, const char* clave){
    void* dato = NULL;
    unsigned long posicion = funcion_hash(hash, clave);
    campo_t* campo = hash->tabla[posicion];
    if(campo != NULL && campo->estado == OCUPADO){
        dato = campo->valor;
        campo->estado = BORRADO;
        hash->cantidad-=1u;
    }
    return dato;
}

size_t hash_cantidad(const hash_t* hash) {
    return hash->cantidad;
}

void hash_destruir(hash_t* hash) {
    if(hash_cantidad(hash) > 0){
        for (int i = 0; i < hash->capacidad && hash->cantidad > 0; i++) {
            campo_t* campo = hash->tabla[i];
            if (campo != NULL) {
                if (hash->destruir != NULL) { // hash->destruir vale la función hash_destruir_dato
                    hash->destruir(campo->valor);
                    hash->destruir(campo->clave);
                }
            }
            hash->cantidad-=1u;
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

hash_iter_t* hash_iter_crear(const hash_t* hash) {
    hash_iter_t* iter = malloc(sizeof(hash_iter_t));
    if (iter == NULL) {
        return NULL;
    }
    iter->posicion = 0;
    iter->tabla = hash->tabla;
    iter->cantidad = hash->cantidad;
    return iter;
}

const char* hash_iter_ver_actual(const hash_iter_t* iter) {
    if (hash_iter_al_final(iter)) {
        return NULL;
    }
    char* clave = NULL;
    campo_t* campo = iter->tabla[iter->posicion];
    if(campo != NULL){
        clave = campo->clave;
    }
    return clave;
}

bool hash_iter_al_final(const hash_iter_t* iter) {
    return iter->cantidad == 0;
}

void hash_iter_destruir(hash_iter_t* iter) {
    free(iter);
}

bool hash_iter_avanzar(hash_iter_t* iter) { // Sólo avanza por los campos ocupados
    if (hash_iter_al_final(iter)) {
        return false;
    }
    while (!hash_iter_al_final(iter)) {
        iter->posicion+=1u;
        campo_t* campo = iter->tabla[iter->posicion];
        if (campo != NULL && campo->estado == OCUPADO) {
            iter->cantidad-=1u;
            return true;
        }
    }
    return true;
}
