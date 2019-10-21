#include "hash.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define CAPACIDAD_INICIAL 101 // Número primo
#define CONSTANTE_REDIMENSION 2
#define CONSTANTE_CARGA 0.7
#define CONSTANTE_CARGA_ABAJO 0.1

typedef enum estados {
    VACIO, OCUPADO, BORRADO // VACIO = 0, OCUPADO = 1, BORRADO = 2
} estados_t;

typedef struct campo {
    char* clave;
    void* valor;
    estados_t estado;
} campo_t;

struct hash {
    unsigned long capacidad;
    size_t cantidad;
    size_t borrados;
    campo_t* tabla;
    hash_destruir_dato_t destruir;
};

struct hash_iter {
    const hash_t* hash;
    unsigned long posicion;
    size_t recorridos;
};

char *strdup(const char *s1);

// Función de Hash de "The C Programming Language (Second Edition)", Brian Kernighan & Dennis Ritchie, Capítulo 6, Pág. 144.
unsigned long funcion_hash(size_t capacidad, const char* s) {
    size_t hash = 14695981039346656037U;
    while (*s) {
        hash ^= (unsigned char) *s++;
        hash *= 1099511628211U;
    }
    return hash % capacidad;
}

unsigned long obtener_posicion_insertar(campo_t* tabla, size_t capacidad, unsigned long posicion_original){
    unsigned long pos = posicion_original;
    while (pos < capacidad){
        if(tabla[pos].estado != OCUPADO){
            return pos;
        }
        pos++;
    }
    pos = 0;
    while (pos < posicion_original) {
        if(tabla[pos].estado != OCUPADO){
            return pos;
        }
        pos++;
    }
    return pos;
}

unsigned long obtener_posicion_insertado(const hash_t* hash, unsigned long posicion_original, const char* clave){
    unsigned long pos = posicion_original;
    while (pos < hash->capacidad){
        if(hash->tabla[pos].estado == OCUPADO && strcmp(hash->tabla[pos].clave, clave) == 0){
            return pos;
        }
        pos++;
    }
    pos = 0;
    while (pos < posicion_original) {
        if(hash->tabla[pos].estado == OCUPADO && strcmp(hash->tabla[pos].clave, clave) == 0){
            return pos;
        }
        pos++;
    }
    return pos;
}

campo_t* crear_tabla(size_t capacidad){
    campo_t* tabla = malloc(capacidad*sizeof(campo_t));
    if(tabla == NULL){
        return NULL;
    }
    for(int i = 0; i < capacidad; i++){
        tabla[i].estado = VACIO;
    }
    return tabla;
}

hash_t* hash_crear(hash_destruir_dato_t destruir_dato){
    hash_t* hash = malloc(sizeof(hash_t));
    if(hash == NULL){
        return NULL;
    }
    hash->tabla = crear_tabla(CAPACIDAD_INICIAL);
    if(hash->tabla == NULL){
        free(hash);
        return NULL;
    }
    hash->capacidad = CAPACIDAD_INICIAL;
    hash->cantidad = 0;
    hash->borrados = 0;
    hash->destruir = destruir_dato;
    return hash;
}

double calcular_factor_carga(hash_t* hash){
    return ((double)(hash->cantidad) + (double)(hash->borrados))/((double)(hash->capacidad));
}

bool hash_redimensionar(hash_t* hash, size_t capacidad){
    campo_t* nueva_tabla = crear_tabla(capacidad);
    if(capacidad > 0 && nueva_tabla == NULL){
        return false;
    }
    for(int i = 0; i < hash->capacidad; i++){
        if(hash->tabla[i].estado == OCUPADO){
            unsigned long pos = obtener_posicion_insertar(nueva_tabla, capacidad, funcion_hash(capacidad, hash->tabla[i].clave));
            nueva_tabla[pos] = hash->tabla[i];
        } 
        if(hash->tabla[i].estado == BORRADO){
            free(hash->tabla[i].clave);
        }
    }
    free(hash->tabla);
	hash->tabla = nueva_tabla;
	hash->capacidad = capacidad;
    return true;

}

bool hash_guardar(hash_t* hash, const char* clave, void* dato){
    unsigned long pos;
    if(hash_pertenece(hash, clave)){
        pos = obtener_posicion_insertado(hash, funcion_hash(hash->capacidad, clave), clave);
        if(hash->destruir != NULL){
            hash->destruir(hash->tabla[pos].valor);
        }
        hash->tabla[pos].valor = dato;
        return true;
    }
    if(calcular_factor_carga(hash) > CONSTANTE_CARGA){
        if(!hash_redimensionar(hash, hash->capacidad*CONSTANTE_REDIMENSION)){
            return false;
        }
    }
    pos = obtener_posicion_insertar(hash->tabla, hash->capacidad, funcion_hash(hash->capacidad, clave));
    char* copia_clave = strdup(clave);
    if (copia_clave == NULL) {
        return false;
    }
    hash->tabla[pos].clave = copia_clave;
    hash->tabla[pos].valor = dato;
    hash->tabla[pos].estado = OCUPADO;
    hash->cantidad++;
    return true;
}

void* hash_borrar(hash_t* hash, const char* clave){
    if(!hash_pertenece(hash, clave)){
        return NULL;
    }
    unsigned long pos = obtener_posicion_insertado(hash,funcion_hash(hash->capacidad, clave), clave);
    void* valor = hash->tabla[pos].valor;
    hash->tabla[pos].estado = BORRADO;
    hash->cantidad--;
    hash->borrados++;
    if(calcular_factor_carga(hash) < CONSTANTE_CARGA_ABAJO){
        hash_redimensionar(hash, hash->capacidad/CONSTANTE_REDIMENSION);
    }
    return valor;
}

void* hash_obtener(const hash_t* hash, const char* clave){
    if(!hash_pertenece(hash, clave)){
        return NULL;
    }
    unsigned long pos = obtener_posicion_insertado(hash, funcion_hash(hash->capacidad, clave), clave);
    return hash->tabla[pos].valor;
}

bool hash_pertenece(const hash_t* hash, const char* clave){
    unsigned long pos = obtener_posicion_insertado(hash, funcion_hash(hash->capacidad, clave), clave);
    return hash->tabla[pos].estado == OCUPADO && strcmp(hash->tabla[pos].clave, clave) == 0;
}

size_t hash_cantidad(const hash_t* hash){
    return hash->cantidad;
}

void hash_destruir(hash_t* hash){
    for(int i = 0; i < hash->capacidad; i++){
        if(hash->tabla[i].estado != VACIO){
            if(hash->destruir != NULL){
                hash->destruir(hash->tabla[i].valor);
            }
            free(hash->tabla[i].clave);
        }   
    }
    free(hash->tabla);
    free(hash);
}

/* Iterador del hash */

hash_iter_t* hash_iter_crear(const hash_t* hash){
    hash_iter_t* hash_iter = malloc(sizeof(hash_iter_t));
    if(hash_iter == NULL){
        return NULL;
    }
    hash_iter->posicion = 0;
    hash_iter->recorridos = 0;
    hash_iter->hash = hash;
    hash_iter_avanzar(hash_iter);
    hash_iter->recorridos = 0;
    return hash_iter;
}

bool hash_iter_avanzar(hash_iter_t* iter){
    while(!hash_iter_al_final(iter)){
        iter->posicion++;
        if(iter->posicion < iter->hash->capacidad && iter->hash->tabla[iter->posicion].estado == OCUPADO){
            iter->recorridos++;
            return true;
        }
    }
    return false;
}

const char* hash_iter_ver_actual(const hash_iter_t* iter){
    if (hash_iter_al_final(iter) || iter->hash->tabla[iter->posicion].estado != OCUPADO) {
        return NULL;
    }
    return iter->hash->tabla[iter->posicion].clave;
}
    

bool hash_iter_al_final(const hash_iter_t* iter){
    return iter->recorridos == iter->hash->cantidad || iter->posicion == iter->hash->capacidad;
}

void hash_iter_destruir(hash_iter_t* iter){
    free(iter);
}

