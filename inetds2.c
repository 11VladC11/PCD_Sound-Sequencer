/*
 * Cojocaru Vlad
 * IR3 - grupa 4
 * Proiect curs - T32 sound sequencer - inetds2
 * implementeaza componenta clientilor ordinari pe fir separat si porneste demo-ul curent
 * afiseaza portul inet si trimite controlul catre logica de procesare pe straturi
 * trateaza erori simple de afisare si esecul demo-ului pornit din proto
 */

#include <stdio.h> /* printf si perror la afisare*/

#include "proto.h"

/* fir clienti ordinari separat*/
void *inet_main(void *args)
{
    const struct contextserver *context;

    context = (const struct contextserver *)args;
    if (printf("fir inet clienti ordinari: port %d pentru compozitia %s\n",
               context->setari.portclienti,
               context->setari.numecompozitie) < 0) {
        perror("printf");
        return (void *)1;
    }

    if (ruleazademo(context) != 0) {
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