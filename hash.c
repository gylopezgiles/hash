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
    const hash_t* hash;
    unsigned long posicion;
    size_t recorridos;
};

unsigned long funcion_hash(unsigned long capacidad, const char* s) {
    unsigned long hashval;
    for (hashval=0; *s != '\0' ; s++) 
        hashval = ((unsigned long)*s + 31) + hashval;
    return hashval % capacidad;
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
    for(int i=0; i< hash->capacidad; i++){
        hash->tabla[i] = malloc(sizeof(campo_t));
        if(hash->tabla[i] == NULL){
            free(hash->tabla);
            free(hash);
            return NULL;
        }
        hash->tabla[i]->estado = VACIO;
        hash->tabla[i]->clave = NULL;
    }
    hash->destruir = destruir_dato;
    return hash;
}

bool hash_redimensionar(hash_t* hash, size_t tam_nuevo){
    campo_t** tabla_nueva = malloc(sizeof(campo_t*)*tam_nuevo);
    if(tam_nuevo > 0 && tabla_nueva == NULL){
        return false;
    }
    for(int i=0; i< tam_nuevo; i++){
        tabla_nueva[i] = malloc(sizeof(campo_t));
        if(tabla_nueva[i] == NULL){
            free(tabla_nueva);
            return false;
        }
        tabla_nueva[i]->estado = VACIO;
        tabla_nueva[i]->clave = NULL;
    }
    for (int i = 0; i < hash->capacidad; i++) {
		if (hash->tabla[i]->estado == OCUPADO) {		
			size_t pos = funcion_hash(tam_nuevo, hash->tabla[i]->clave);
			tabla_nueva[pos] = hash->tabla[i];
		}else{
            free(hash->tabla[i]->clave);
            free(hash->tabla[i]);
        }
	}
	campo_t** aux = hash->tabla;
	hash->tabla = tabla_nueva;
	hash->capacidad = tam_nuevo;
	free(aux);
    return true;
}

bool hash_guardar(hash_t* hash, const char* clave, void* dato){
    unsigned long posicion = funcion_hash(hash->capacidad, clave);
    if(hash_pertenece(hash, clave)){
        if(hash->destruir != NULL) {
			hash->destruir(hash->tabla[posicion]->valor);
        }
        hash->tabla[posicion]->valor = dato;
        hash->tabla[posicion]->estado = OCUPADO;
        return true;
    }
    if(hash->capacidad == hash->cantidad){
        if(!hash_redimensionar(hash, hash->capacidad*2)){
            return false;
        }
    }
    hash->tabla[posicion]->valor = dato;
    char* copia_clave = strdup(clave);
    if(copia_clave == NULL){
        return NULL;
    } 
    hash->tabla[posicion]->clave = copia_clave;
    hash->tabla[posicion]->estado = OCUPADO;
    hash->cantidad++;
    return true;
}

void* hash_borrar(hash_t* hash, const char* clave){
    if(hash_cantidad(hash) == 0){
        return NULL;
    }
    void* dato = NULL;
    if(hash_pertenece(hash, clave)){
        unsigned long posicion = funcion_hash(hash->capacidad, clave);
        dato = hash->tabla[posicion]->valor;
        hash->tabla[posicion]->estado = BORRADO;
        hash->cantidad--;
    }
    if(hash->cantidad < hash->capacidad/4 && hash->capacidad/2 > CAPACIDAD_INICIAL){
        hash_redimensionar(hash, hash->capacidad/2);
    }
    return dato;
}

size_t hash_cantidad(const hash_t* hash) {
    return hash->cantidad;
}

void hash_destruir(hash_t* hash) {
    for (int i = 0; i < hash->capacidad; i++) {
        if(hash->tabla[i]->clave !=NULL){
            if (hash->destruir != NULL) { 
                hash->destruir(hash->tabla[i]->valor);
                hash->destruir(hash->tabla[i]->clave);
            } else {
                free(hash->tabla[i]->clave);
            }
        }
        free(hash->tabla[i]);
    }
    free(hash->tabla);
    free(hash);
}

bool hash_pertenece(const hash_t* hash, const char* clave) {
    if (hash_cantidad(hash) == 0) {
        return false;
    }
    unsigned long pos_en_hash = funcion_hash(hash->capacidad, clave);
    campo_t* campo = hash->tabla[pos_en_hash];
    return campo->estado == OCUPADO && strcmp(campo->clave, clave) == 0 ; 
}

void* hash_obtener(const hash_t* hash, const char* clave) {
    if (!hash_pertenece(hash, clave)) {
        return NULL;
    }
    unsigned long pos_en_hash = funcion_hash(hash->capacidad, clave);
    return hash->tabla[pos_en_hash]->valor;
}

/* Iterador del hash */

hash_iter_t* hash_iter_crear(const hash_t* hash) {
    hash_iter_t* iter = malloc(sizeof(hash_iter_t));
    if (iter == NULL) {
        return NULL;
    }
    iter->posicion = 0;
    iter->recorridos = 0;
    iter->hash = hash;
    return iter;
}

const char* hash_iter_ver_actual(const hash_iter_t* iter) {
    if (hash_iter_al_final(iter)) {
        return NULL;
    }
    char* clave = NULL;
    campo_t* campo = iter->hash->tabla[iter->posicion];
    if(campo != NULL){
        clave = campo->clave;
    }
    return clave;
}

bool hash_iter_al_final(const hash_iter_t* iter) {
    return iter->recorridos == hash_cantidad(iter->hash);
}

void hash_iter_destruir(hash_iter_t* iter) {
    free(iter);
}

bool hash_iter_avanzar(hash_iter_t* iter) { 
    while (!hash_iter_al_final(iter)) {
        iter->posicion++;
        if (iter->hash->tabla[iter->posicion]->estado == OCUPADO) {
            iter->recorridos++;
            return true;
        }
    }
    return false;
}
