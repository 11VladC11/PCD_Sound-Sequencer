#ifndef PROTO_H
#define PROTO_H

enum {
    marimetext = 128,
    marimenote = 256,
    marimecale = 512,
    marimestraturi = 8
};

struct setariserver {
    char titlu[marimetext];
    char numecompozitie[marimetext];
    char socketsadmin[marimecale];
    char directoroutput[marimecale];
    int portclienti;
    int portapi;
    int timeoutadmin;
    int samplerate;
};

struct strat {
    char instrument[marimetext];
    char note[marimenote];
    char efect[marimetext];
    char caleinput[marimecale];
    int workloadms;
};

struct optiuniserver {
    char caleconfig[marimecale];
    int moddemo;
    int afiseazamediu;
};

struct contextserver {
    struct setariserver setari;
    struct strat straturi[marimestraturi];
    int numarstraturi;
};

int initializeazaoptiuni(struct optiuniserver *optiuni);
int parseazaoptiuni(int argc, char **argv, struct optiuniserver *optiuni);
int afiseazaajutor(void);
int createdirector(const char *cale);
int incarca_context(const char *caleconfig, struct contextserver *context);
int afiseazasumar(const struct contextserver *context);
int afiseazamediu(void);
int ruleazademo(const struct contextserver *context);

void *unix_main(void *args);
void *inet_main(void *args);
void *soap_main(void *args);

#endif