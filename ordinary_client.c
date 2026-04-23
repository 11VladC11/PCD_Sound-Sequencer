/*
 * Cojocaru Vlad
 * IR3 - grupa 4
 * Proiect curs - T32 sound sequencer - client ordinar C
 * valideaza configuratia clientului ordinar si afiseaza protocolul de demo curent
 * citeste portul inet, numele compozitiei si verifica fisierul folosit pentru demo
 * trateaza erori de config, argumente, stat, utilizator curent si afisare
 */

#include <libconfig.h> /* libconfig pentru citirea configuratiei */
#include <pwd.h> /* getpwuid_r pentru user in mod thread-safe */
#include <stdio.h> /* printf si perror la afisare */
#include <stdlib.h> /* constante standard */
#include <string.h> /* strlen si strcmp pentru texte */
#include <sys/stat.h> /* stat pentru validarea fisierelor mari */
#include <unistd.h> /* write */

enum {
    marimetext = 128,
    baza10 = 10,
    marimecale = 512
};

struct optiuniclient {
    char caleconfig[marimecale];
    char caleinput[marimecale];
    char numecompozitie[marimetext];
    int arataprotocol;
    int afiseazamediu;
    int areinput;
};

/* scrie tot textul in descriptor; util la ajutor si erori*/
static int scrietot(int descriptor, const char *text, size_t lungime)
{
    size_t scris;

    scris = 0;
    while (scris < lungime) {
        ssize_t rezultat;

        rezultat = write(descriptor, text + scris, lungime - scris);
        if (rezultat < 0) {
            perror("write");
            return -1;
        }

        scris += (size_t)rezultat;
    }

    return 0;
}

/* scrie un text complet in descriptor */
static int scrietext(int descriptor, const char *text)
{
    return scrietot(descriptor, text, strlen(text));
}

/* copiaza un text cu verificare de marime, ca sa evit depasirea bufferului */
static int copiatext(char *destinatie, size_t marime, const char *sursa)
{
    size_t indice;

    indice = 0;
    while (sursa[indice] != '\0') {
        if (indice + 1 >= marime) {
            return -1;
        }

        destinatie[indice] = sursa[indice];
        indice++;
    }

    destinatie[indice] = '\0';
    return 0;
}

/* transforma un numar in text*/
static int numarlatext(int valoare, char *text, size_t marime)
{
    char invers[marimetext];
    size_t index;
    size_t pozitie;
    unsigned int numar;

    if (marime == 0U) {
        return -1;
    }

    numar = (unsigned int)valoare;
    index = 0;
    do {
        invers[index] = (char)('0' + (numar % baza10));
        numar /= baza10;
        index++;
    } while (numar != 0U && index < sizeof(invers));

    pozitie = 0;
    while (index > 0U) {
        if (pozitie + 1 >= marime) {
            return -1;
        }

        index--;
        text[pozitie] = invers[index];
        pozitie++;
    }

    text[pozitie] = '\0';
    return 0;
}

/* obtine utilizatorul curent fara getenv, ca sa ramana thread-safe */
static int obtineutilizator(char *user, size_t marime)
{
    char buffer[marimecale];
    struct passwd intrare;
    struct passwd *rezultat;

    if (getpwuid_r(getuid(), &intrare, buffer, sizeof(buffer), &rezultat) == 0 &&
        rezultat != NULL &&
        copiatext(user, marime, rezultat->pw_name) == 0) {
        return 0;
    }

    return copiatext(user, marime, "nedefinit");
}

/* afiseaza eroarea de configurare fara fprintf, cu fisier si linie*/
static int afiseazaeroareconfig(const config_t *config)
{
    char linie[marimetext];
    const char *fisier;
    const char *text;

    fisier = config_error_file(config);
    text = config_error_text(config);
    if (numarlatext(config_error_line(config), linie, sizeof(linie)) != 0) {
        return -1;
    }

    if (scrietext(STDERR_FILENO, "config: ") != 0 ||
        scrietext(STDERR_FILENO, fisier == NULL ? "necunoscut" : fisier) != 0 ||
        scrietext(STDERR_FILENO, ":") != 0 ||
        scrietext(STDERR_FILENO, linie) != 0 ||
        scrietext(STDERR_FILENO, " - ") != 0 ||
        scrietext(STDERR_FILENO, text == NULL ? "eroare necunoscuta" : text) != 0 ||
        scrietext(STDERR_FILENO, "\n") != 0) {
        return -1;
    }

    return 0;
}

/* initializeaza valorile implicite ale clientului, ca sa poata porni si fara argumente */
static int initializeazaoptiuni(struct optiuniclient *optiuni)
{
    if (copiatext(optiuni->caleconfig, sizeof(optiuni->caleconfig), "config/sound_sequencer.cfg") != 0 ||
        copiatext(optiuni->numecompozitie, sizeof(optiuni->numecompozitie), "demo_string_layers") != 0) {
        return -1;
    }

    optiuni->caleinput[0] = '\0';
    optiuni->arataprotocol = 0;
    optiuni->afiseazamediu = 0;
    optiuni->areinput = 0;
    return 0;
}

/* parseaza argumentele si opreste la prima forma invalida */
static int parseazaoptiuni(int argc, char **argv, struct optiuniclient *optiuni)
{
    for (int index = 1; index < argc; index++) {
        if (strcmp(argv[index], "-c") == 0) {
            index++;
            if (index >= argc || copiatext(optiuni->caleconfig, sizeof(optiuni->caleconfig), argv[index]) != 0) {
                return -1;
            }
            continue;
        }

        if (strcmp(argv[index], "-i") == 0) {
            index++;
            if (index >= argc || copiatext(optiuni->caleinput, sizeof(optiuni->caleinput), argv[index]) != 0) {
                return -1;
            }
            optiuni->areinput = 1;
            continue;
        }

        if (strcmp(argv[index], "-n") == 0) {
            index++;
            if (index >= argc || copiatext(optiuni->numecompozitie, sizeof(optiuni->numecompozitie), argv[index]) != 0) {
                return -1;
            }
            continue;
        }

        if (strcmp(argv[index], "-s") == 0) {
            optiuni->arataprotocol = 1;
            continue;
        }

        if (strcmp(argv[index], "-e") == 0) {
            optiuni->afiseazamediu = 1;
            continue;
        }

        if (strcmp(argv[index], "-h") == 0) {
            return 1;
        }

        return -1;
    }

    return 0;
}

/* afiseaza sumarul clientului cu valorile minime folosite acum */
static int afiseazasumar(const char *numecompozitie, const char *numeconfig, int portclienti)
{
    if (printf("client ordinar\n") < 0 ||
        printf("port INET planificat: %d\n", portclienti) < 0 ||
        printf("compozitie curenta: %s\n", numecompozitie) < 0 ||
        printf("compozitie demo din config: %s\n", numeconfig) < 0) {
        perror("printf");
        return -1;
    }

    return 0;
}

/* afiseaza mediul relevant pentru client*/
static int afiseazamediu(void)
{
    char user[marimetext];

    if (obtineutilizator(user, sizeof(user)) != 0) {
        return -1;
    }

    if (printf("mediu: USER=%s\n", user) < 0) {
        perror("printf");
        return -1;
    }

    return 0;
}

/* afiseaza fisierul trimis si marimea lui, cu stat*/
static int afiseazafisierdemo(const char *caleinput)
{
    struct stat stare;

    if (stat(caleinput, &stare) < 0) {
        perror("stat");
        return -1;
    }

    if (printf("fisier demo: %s\n", caleinput) < 0 ||
        printf("dimensiune demo pentru chunking: %ld bytes\n", (long)stare.st_size) < 0) {
        perror("printf");
        return -1;
    }

    return 0;
}

/* afiseaza protocolul minim pentru upload si interogare*/
static int afiseazaprotocol(void)
{
    /* aici descriu doar schimbul de mesaje folosit acum*/
    if (printf("protocol sincron:\n") < 0 ||
        printf("UPLOAD <composition_name> <total_bytes>\n") < 0 ||
        printf("CHUNK <index> <chunk_bytes>\n") < 0 ||
        printf("STATUS <composition_name>\n") < 0 ||
        printf("RESULT <composition_name>\n") < 0) {
        perror("printf");
        return -1;
    }

    return 0;
}

/* afiseaza mesajul de ajutor*/
static int afiseazaajutor(void)
{
    if (scrietext(STDOUT_FILENO, "folosire: ./ordinary_client [-c config] [-i fisier] [-n nume] [-s] [-e] [-h]\n") != 0 ||
        scrietext(STDOUT_FILENO, "  -c  fisierul de configurare libconfig\n") != 0 ||
        scrietext(STDOUT_FILENO, "  -i  fisierul de intrare pentru demo\n") != 0 ||
        scrietext(STDOUT_FILENO, "  -n  numele compozitiei\n") != 0 ||
        scrietext(STDOUT_FILENO, "  -s  afiseaza protocolul curent\n") != 0 ||
        scrietext(STDOUT_FILENO, "  -e  afiseaza si informatii de mediu\n") != 0 ||
        scrietext(STDOUT_FILENO, "  -h  afiseaza acest mesaj\n") != 0) {
        return -1;
    }

    return 0;
}

int main(int argc, char **argv)
{
    config_t config;
    struct optiuniclient optiuni;
    char numeconfig[marimetext];
    const char *text;
    int portclienti;

    portclienti = initializeazaoptiuni(&optiuni);
    if (portclienti != 0) {
        return EXIT_FAILURE;
    }

    portclienti = parseazaoptiuni(argc, argv, &optiuni);
    if (portclienti < 0) {
        return EXIT_FAILURE;
    }

    if (portclienti > 0) {
        return afiseazaajutor() == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
    }

    /* citesc doar campurile din config de care clientul are nevoie acum */
    config_init(&config);
    if (config_read_file(&config, optiuni.caleconfig) == CONFIG_FALSE) {
        (void)afiseazaeroareconfig(&config);
        config_destroy(&config);
        return EXIT_FAILURE;
    }

    if (config_lookup_string(&config, "audio.composition_name", &text) == CONFIG_FALSE ||
        copiatext(numeconfig, sizeof(numeconfig), text) != 0 ||
        config_lookup_int(&config, "server.ordinary_port", &portclienti) == CONFIG_FALSE) {
        config_destroy(&config);
        (void)scrietext(STDERR_FILENO, "configuratie client invalida\n");
        return EXIT_FAILURE;
    }

    config_destroy(&config);

    /* dupa validare, afisez sumarul local */
    if (afiseazasumar(optiuni.numecompozitie, numeconfig, portclienti) != 0) {
        return EXIT_FAILURE;
    }

    if (optiuni.afiseazamediu != 0 && afiseazamediu() != 0) {
        return EXIT_FAILURE;
    }

    if (optiuni.areinput != 0 && afiseazafisierdemo(optiuni.caleinput) != 0) {
        return EXIT_FAILURE;
    }

    /* pot afisa protocolul fara trafic real */
    if (optiuni.arataprotocol != 0) {
        return afiseazaprotocol() == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

/*

exemple build:
gcc -Wall -Wextra -Wpedantic -Werror -g -std=c11 -D_POSIX_C_SOURCE=200809L ordinary_client.c $(pkg-config --cflags --libs libconfig) -o ordinary_client


exemple rulare cu eroare:
./ordinary_client -c config/lipsa.cfg -s




exemple rulare cu succes:
./ordinary_client -c config/sound_sequencer.cfg -s -i Sounds/violin-wav/violin_A3_025_forte_arco-normal.wav -n demo_string_layers -e


*/
