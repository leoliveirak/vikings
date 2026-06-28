#include <stdlib.h>
#include "config.h"
#include "chieftain.h"
#include "valhalla.h"

void chieftain_init(chieftain_t *self, valhalla_t *valhalla)
{
    /* TODO: Adicionar código aqui se necessário! */
 
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
    
    god_t god = rand() % NUMBER_OF_GODS;
    
    if (!valhalla_is_super(god)){

        god_t rival = valhalla_get_rival(god);
        
        pthread_mutex_lock(&self->valhalla->prayers_mutex);
        if (self->valhalla->prayers[god] > 1.05 * self->valhalla->prayers[rival]){
            god = rival;
        }
        pthread_mutex_unlock(&self->valhalla->prayers_mutex);
    
    } else {
        
        unsigned int total = 0;
        
        for (size_t i = 0; i < 6; i++){
            pthread_mutex_lock(&self->valhalla->prayers_mutex);
            total += self->valhalla->prayers[i];
            pthread_mutex_unlock(&self->valhalla->prayers_mutex); 
        }
        pthread_mutex_lock(&self->valhalla->prayers_mutex);
        
        if(self->valhalla->prayers[god] > 1.1 * total){
        
            pthread_mutex_unlock(&self->valhalla->prayers_mutex);
            return chieftain_get_god(self);
        
        }
        pthread_mutex_unlock(&self->valhalla->prayers_mutex);

    }

    return god;
}

void chieftain_finalize(chieftain_t *self)
{
    /* TODO: Adicionar código aqui se necessário! */

    plog("[chieftain] Finalized\n");
}
