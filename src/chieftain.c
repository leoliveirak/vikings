#include <stdlib.h>
#include "config.h"
#include "chieftain.h"
#include "valhalla.h"
#include <math.h>

void chieftain_init(chieftain_t *self, valhalla_t *valhalla)
{
    for (int i = 0; i < NUMBER_OF_GODS; i++)
        self->livres[i] = 0;

    pthread_mutex_init(&self->livres_mutex, NULL);
    
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
    /* TODO: Adicionar código aqui se necessário! */

    pthread_mutex_destroy(&self->livres_mutex);

    plog("[chieftain] Finalized\n");
}
