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
static inline int neighbot_is_safe(chieftain_t *self, int viz, int viking_type)
{
    return (self->chairs[viz] == CHAIR_EMPTY) || self->chairs[viz] == viking_type;
}

static int find_free_plate_pair(chieftain_t *self, int i, int esq, int dir,
                                int *p1, int *p2) 
{
    int candidates[3] = { i, esq, dir };

    for (int a = 0; a < 3; a++) {
        for (int b = a + 1; b < 3; b++) {
            int pa = candidates[a];
            int pb = candidates[b];
            if (self->plates[pa] == PLATE_FREE && 
                self->plates[pb] == PLATE_FREE)
            {
                *p1 = pa;
                *p2 = pb;
                return 1;
            }
        }
    }
    return 0;
}

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
    int viking_type = berserker;
    int N = config.table_size;
    int seat = -1;

    pthread_mutex_lock(&self->table_mutex);

    while(1)
    {
        for (int i = 0; i < N; i++)
        {
            /* Regra 1: cadeira livre */
            if (self->chairs[i] != CHAIR_EMPTY)
            {
                continue;
            }

            int left = left_of(i, N);
            int right = right_of(i, N);

            /* Regra 2: vizinhos seguros */
            if (!neighbot_is_safe(self, left, viking_type) ||
                !neighbot_is_safe(self, right, viking_type))
            {
                continue;
            }

            /* Regra 3: encontrar par de pratos livres */
            int p1, p2;
            if (!find_free_plate_pair(self, i, left, right, &p1, &p2))
            {
                continue;
            }

            self->chairs[i] = viking_type;
            self->plates[p1] = PLATE_TAKEN;
            self->plates[p2] = PLATE_TAKEN;
            self->assigned_plates[i][PLATE_SLOT_A] = p1;
            self->assigned_plates[i][PLATE_SLOT_B] = p2;
            seat = i;
            break;
        }
        if (seat != -1)
        {
            break;
        }
        pthread_cond_wait(&self->table_cond, &self->table_mutex);
    }
    pthread_mutex_unlock(&self->table_mutex);
    return seat;
}

void chieftain_release_seat_plates(chieftain_t *self, int pos)
{
    pthread_mutex_lock(&self->table_mutex);

    int p1 = self->assigned_plates[pos][PLATE_SLOT_A];
    int p2 = self->assigned_plates[pos][PLATE_SLOT_B];

    self->chairs[pos] = CHAIR_EMPTY;
    self->plates[p1] = PLATE_FREE;
    self->plates[p2] = PLATE_FREE;
    self->assigned_plates[pos][PLATE_SLOT_A] = PLATE_SLOT_NONE;
    self->assigned_plates[pos][PLATE_SLOT_B] = PLATE_SLOT_NONE;

    pthread_cond_broadcast(&self->table_cond);
    pthread_mutex_unlock(&self->table_mutex);
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
