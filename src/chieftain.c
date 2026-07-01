#include <stdlib.h>
#include "config.h"
#include "chieftain.h"
#include "valhalla.h"
#include <math.h>

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
static inline int is_table_gap (int a, int b, int N) {return (a == N - 1 && b == 0) || (a == 0 && b == N - 1);}
static inline int neighbot_is_safe(chieftain_t *self, int me, int viz, int viking_type, int N)
{
    if (is_table_gap(me, viz, N))
    {
        return 1;
    }
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

    pthread_cond_init(&self->pray_cond, NULL);
    self->vikings_finish_eating = 0;
    for (int i = 0; i < config.table_size; i++)
    {
        self->assigned_plates[i] = (int *)calloc(2, sizeof(int));
    }

    for (int i = 0; i < NUMBER_OF_GODS; i++)
        self->livres[i] = 0;

    pthread_mutex_init(&self->livres_mutex, NULL);
    
    self->valhalla = valhalla;
    plog("[chieftain] Initialized\n");
}

int chieftain_acquire_seat_plates(chieftain_t *self, int berserker)
{
    int viking_type = berserker ? CHAIR_BERSERKER : CHAIR_WARRIOR;
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
            if (!neighbot_is_safe(self, i, left, viking_type, N) ||
                !neighbot_is_safe(self, i,  right, viking_type, N))
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

    /* Verificação de Barreira Comer/Rezar */
    if (++self->vikings_finish_eating == config.horde_size) pthread_cond_broadcast(&self->pray_cond);
   
    pthread_cond_broadcast(&self->table_cond);
    pthread_mutex_unlock(&self->table_mutex);
  
}

god_t chieftain_get_god(chieftain_t *self)
{
    pthread_mutex_lock(&self->table_mutex);
    while (self->vikings_finish_eating < config.horde_size)
        pthread_cond_wait(&self->pray_cond, &self->table_mutex);
    pthread_mutex_unlock(&self->table_mutex);
    
    god_t candidatos[NUMBER_OF_GODS];
    int count = 0;  

    pthread_mutex_lock(&self->livres_mutex);

    for (god_t i = 0; i < NUMBER_OF_GODS; i++) {
        
        if (!valhalla_is_super(i)) {
            god_t rival = valhalla_get_rival(i);
            int permitido = ceil((1.0 + RIVAL_TOLERANCE_RATE) * self->livres[rival]);
            
            if (permitido < 1) permitido = 1;
            
            if (self->livres[i] < permitido)
                candidatos[count++] = i;
        
        } else {
            int total = 0;
            
            for (int j = 0; j < NUMBER_OF_GODS; j++) 
                
            if (!valhalla_is_super((god_t) j))
                    total += self->livres[j];

            int permitido = ceil((1.0 + SUPER_GOD_TOLERANCE_RATE) * total);
            
            if (permitido < 1) permitido = 1;
            
            if (self->livres[i] < permitido)
                candidatos[count++] = i;
        }
    }

    god_t god = candidatos[rand() % count];
    self->livres[god]++;  // registra a concessão

    pthread_mutex_unlock(&self->livres_mutex);  // libera antes de retornar

    return god;
}

void chieftain_finalize(chieftain_t *self)
{
    pthread_mutex_destroy(&self->table_mutex);
    pthread_mutex_destroy(&self->livres_mutex);

    pthread_cond_destroy(&self->table_cond);
    pthread_cond_destroy(&self->pray_cond);

    free(self->chairs);
    free(self->plates);
    for (int i = 0; i < config.table_size; i++)
    {
        free(self->assigned_plates[i]);
    }
    free(self->assigned_plates);
    plog("[chieftain] Finalized\n");
}
