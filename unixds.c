/*
 * Cojocaru Vlad
 * IR3 - grupa 4
 * Proiect curs - T32 sound sequencer - unixds
 * implementeaza componenta admin locala pe fir separat pentru socketul unix planificat
 * afiseaza sumarul de administrare cu socketul si timeoutul citite din configuratie
 * trateaza erori simple de afisare pentru componenta admin
 */

#include <stdio.h> /* printf si perror la afisare */

#include "proto.h"

/* fir admin separat*/
void *unix_main(void *args)
{
    const struct contextserver *context;

    context = (const struct contextserver *)args;
    if (printf("fir unix admin: socket %s, timeout %d secunde\n",
               context->setari.socketsadmin,
               context->setari.timeoutadmin) < 0) {
        perror("printf");
        return (void *)1;
    }
  
    return NULL;
}

/*

exemple build:
./build.sh


exemple rulare cu eroare:
./server -c config/lipsa.cfg -d


exemple rulare cu succes:
./server -c config/sound_sequencer.cfg -d -e


*/