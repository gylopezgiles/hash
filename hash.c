#define _POSIX_C_SOURCE 200809L
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

unsigned long funcion_hash(size_t capacidad, const char* str){
    unsigned long hash = 5381; /* init value */
    int i = 0;
    while (str[i] != '\0')
    {
        hash = ((hash << 5) + hash) + (unsigned long)str[i];
        i++;
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
        if(pos < hash->capacidad && hash->tabla[pos].estado == VACIO){
            return pos;
        }
    }
    pos = 0;
    while (pos < posicion_original) {
        if(hash->tabla[pos].estado == OCUPADO && strcmp(hash->tabla[pos].clave, clave) == 0){
            return pos;
        }
        pos++;
        if(pos < posicion_original && hash->tabla[pos].estado == VACIO){
            return pos;
        }
    }
    return pos;
}

unsigned long obtener_posicion_pertenece(const hash_t* hash, const char* clave){
    unsigned long pos = obtener_posicion_insertado(hash, funcion_hash(hash->capacidad, clave), clave);
    if(hash->tabla[pos].estado == OCUPADO && strcmp(hash->tabla[pos].clave, clave) == 0){
        return pos;
    }
    return hash->capacidad + 1;
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
    size_t total = hash->cantidad + hash->borrados;
    size_t no_vacios = 0;
    for(int i = 0; no_vacios < total; i++){
        if(hash->tabla[i].estado == OCUPADO){
            unsigned long pos = obtener_posicion_insertar(nueva_tabla, capacidad, funcion_hash(capacidad, hash->tabla[i].clave));
            nueva_tabla[pos] = hash->tabla[i];
            no_vacios++;
        } 
        if(hash->tabla[i].estado == BORRADO){
            free(hash->tabla[i].clave);
            no_vacios++;
        }
    }
    free(hash->tabla);
    hash->borrados = 0;
	hash->tabla = nueva_tabla;
	hash->capacidad = capacidad;
    return true;

}

bool hash_guardar(hash_t* hash, const char* clave, void* dato){
    unsigned long pos = obtener_posicion_pertenece(hash, clave);
    if(pos < hash->capacidad){
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
    unsigned long pos = obtener_posicion_pertenece(hash, clave);
    if(pos < hash->capacidad){
        void* valor = hash->tabla[pos].valor;
        hash->tabla[pos].estado = BORRADO;
        hash->cantidad--;
        hash->borrados++;
        if(calcular_factor_carga(hash) < CONSTANTE_CARGA_ABAJO){
            hash_redimensionar(hash, hash->capacidad/CONSTANTE_REDIMENSION);
        }
        return valor;
    }
    return NULL;
}

void* hash_obtener(const hash_t* hash, const char* clave){
    unsigned long pos = obtener_posicion_pertenece(hash, clave);
    if(pos < hash->capacidad){
        return hash->tabla[pos].valor;
    }
    return NULL;
}

bool hash_pertenece(const hash_t* hash, const char* clave){
    unsigned long pos = obtener_posicion_pertenece(hash, clave);
    return pos < hash->capacidad;
}

size_t hash_cantidad(const hash_t* hash){
    return hash->cantidad;
}

void hash_destruir(hash_t* hash){
    int no_vacios = 0;
    size_t total = hash->cantidad + hash->borrados;
    for(int i = 0; no_vacios < total; i++){
        if(hash->tabla[i].estado != VACIO){
            if(hash->destruir != NULL){
                hash->destruir(hash->tabla[i].valor);
            }
            free(hash->tabla[i].clave);
            no_vacios++;
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
        if(iter->recorridos == iter->hash->cantidad-1 && iter->hash->tabla[iter->posicion].estado == OCUPADO ){
            iter->recorridos++;
            return true;
        }
        iter->posicion++;
        if(iter->hash->tabla[iter->posicion].estado == OCUPADO){
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
