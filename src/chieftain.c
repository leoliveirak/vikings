#include <stdlib.h>
#include "config.h"
#include "chieftain.h"
#include "valhalla.h"

/* Cadeiras */
#define CHAIR_EMPTY     0
#define CHAIR_WARRIOR   1
#define CHAIR_BERSERKER 2

/* Pratos */
#define PLATE_FREE      0
#define PLATE_TAKEN     1

/* Indices da matriz de pratos */
#define PLATE_SLOT_A    0
#define PLATE_SLOT_B    1
#define PLATE_SLOT_NONE -1

/* Criação de funções utilitárias */
static inline int left_of(int i, int N) {return (i - 1 + N) % N;}
static inline int right_of(int i, int N) {return (i + 1) % N;}

void chieftain_init(chieftain_t *self, valhalla_t *valhalla)
{
    pthread_mutex_init(&self->table_mutex, NULL);
    pthread_cond_init(&self->table_cond, NULL);
    self->chairs = (int *)calloc(config.table_size, sizeof(int));
    self->plates = (int *)calloc(config.table_size, sizeof(int));
    self->assigned_plates = (int **)malloc(sizeof(int *) * config.table_size);
    for (int i = 0; i < config.table_size; i++)
    {
        self->assigned_plates[i] = (int *)calloc(2, sizeof(int));
    }
    self->valhalla = valhalla;
    plog("[chieftain] Initialized\n");
}

int chieftain_acquire_seat_plates(chieftain_t *self, int berserker)
{
    /* TODO: Implementar! */
    return 1;
}

void chieftain_release_seat_plates(chieftain_t *self, int pos)
{
    /* TODO: Implementar! */
}

god_t chieftain_get_god(chieftain_t *self)
{
    /* TODO: Implementar! O código abaixo deve ser modificado! */
    god_t god = THOR;
    
    return god;
}

void chieftain_finalize(chieftain_t *self)
{
    pthread_mutex_destroy(&self->table_mutex);
    pthread_cond_destroy(&self->table_cond);
    free(self->chairs);
    free(self->plates);
    for (int i = 0; i < config.table_size; i++)
    {
        free(self->assigned_plates[i]);
    }
    free(self->assigned_plates);
    plog("[chieftain] Finalized\n");
}
