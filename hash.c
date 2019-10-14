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

// Función de Hash de "The C Programming Language (Second Edition)", Brian Kernighan & Dennis Ritchie, Capítulo 6, Pág. 144.
unsigned long funcion_hash(size_t capacidad, const char* s) {
    size_t hash = 14695981039346656037U;
    while (*s) {
        hash ^= (unsigned char) *s++;
        hash *= 1099511628211U;
    }
    return hash % capacidad;
}

campo_t* crear_tabla(size_t capacidad){
    campo_t* tabla = malloc(sizeof(campo_t)*capacidad);
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
    size_t cantidad_total = hash->cantidad + hash->borrados;
    return (double)cantidad_total/hash->capacidad;
}

bool hash_redimensionar(hash_t* hash, size_t capacidad){
    campo_t* nueva_tabla = crear_tabla(capacidad);
    if(capacidad > 0 && nueva_tabla == NULL){
        return false;
    }
    for(int i = 0; i < hash->capacidad; i++){
        if(hash->tabla[i].estado == OCUPADO){
            unsigned long pos = funcion_hash(capacidad, hash->tabla[i].clave);
            nueva_tabla[pos] = hash->tabla[i];
        } 
    }
    campo_t* aux = hash->tabla;
	hash->tabla = nueva_tabla;
	hash->capacidad = capacidad;
	free(aux);
    return true;

}

bool hash_guardar(hash_t* hash, const char* clave, void* dato){
    unsigned long pos;
    if(hash_pertenece(hash, clave)){
        pos = funcion_hash(hash->capacidad, clave);
        hash->tabla[pos].valor = dato;
        return true;
    }
    if(calcular_factor_carga(hash) > CONSTANTE_CARGA){
        if(!hash_redimensionar(hash, hash->capacidad*CONSTANTE_REDIMENSION)){
            return false;
        }
    }
    pos = funcion_hash(hash->capacidad, clave);
    hash->tabla[pos].clave = malloc(sizeof(char)*sizeof(clave));
    if(hash->tabla[pos].clave == NULL){
        return false;
    }
    strcpy(hash->tabla[pos].clave, clave);
    hash->tabla[pos].valor = dato;
    hash->tabla[pos].estado = OCUPADO;
    hash->cantidad++;
    return true;
}

void* hash_borrar(hash_t* hash, const char* clave){
    if(!hash_pertenece(hash, clave)){
        return NULL;
    }
    unsigned long pos = funcion_hash(hash->capacidad, clave);
    void* valor = hash->tabla[pos].valor;
    hash->tabla[pos].estado = BORRADO;
    hash->cantidad--;
    hash->borrados++;
    free(hash->tabla[pos].clave);
    if(calcular_factor_carga(hash) < CONSTANTE_CARGA_ABAJO){
        hash_redimensionar(hash, hash->capacidad/CONSTANTE_REDIMENSION);
    }
    return valor;
}

void* hash_obtener(const hash_t* hash, const char* clave){
    if(!hash_pertenece(hash, clave)){
        return NULL;
    }
    unsigned long pos = funcion_hash(hash->capacidad, clave);
    return hash->tabla[pos].valor;
}

bool hash_pertenece(const hash_t* hash, const char* clave){
    unsigned long pos = funcion_hash(hash->capacidad, clave);
    return hash->tabla[pos].estado == OCUPADO;
}

size_t hash_cantidad(const hash_t* hash){
    return hash->cantidad;
}

void hash_destruir(hash_t* hash){
    for(int i = 0; i < hash->capacidad; i++){
        if(hash->tabla[i].estado == OCUPADO){
            if(hash->destruir != NULL){
                hash->destruir(hash->tabla[i].valor);
            }
            free(hash->tabla[i].clave);
        }
    }
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
    return hash_iter;
}

bool hash_iter_avanzar(hash_iter_t* iter){
    while(!hash_iter_al_final(iter)){
        if(iter->hash->tabla[iter->posicion].estado == OCUPADO){
            iter->recorridos++;
            return true;
        }
        iter->posicion++;
    }
    return false;
}

const char* hash_iter_ver_actual(const hash_iter_t* iter){
    if (hash_iter_al_final(iter) && iter->hash->tabla[iter->posicion].estado != OCUPADO) {
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

