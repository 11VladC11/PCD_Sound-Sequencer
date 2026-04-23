/*
 * Cojocaru Vlad
 * IR3 - grupa 4
 * Proiect curs - T32 sound sequencer - client administrare
 * valideaza configuratia minima a clientului admin si afiseaza comenzile planificate
 * citeste socketul unix, timeoutul de administrare si poate afisa utilizatorul curent
 * trateaza erori de config, argumente, conversie in text si afisare
 */

#include <libconfig.h> /* libconfig pentru citirea configuratiei */
#include <pwd.h> /* getpwuid_r pentru user in mod thread-safe */
#include <stdio.h> /* printf si perror la afisare */
#include <stdlib.h> /* constante standard */
#include <string.h> /* strlen si strcmp pentru texte */
#include <unistd.h> /* write */

enum {
    marimetext = 128,
    baza10 = 10,
    marimecale = 512
};

struct optiuniadmin {
    char caleconfig[marimecale];
    int listare;
    int afiseazamediu;
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

/* scrie un text complet in descriptor*/
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

/* initializeaza optiunile implicite ale clientului admin*/
static int initializeazaoptiuni(struct optiuniadmin *optiuni)
{
    if (copiatext(optiuni->caleconfig, sizeof(optiuni->caleconfig), "config/sound_sequencer.cfg") != 0) {
        return -1;
    }

    optiuni->listare = 0;
    optiuni->afiseazamediu = 0;
    return 0;
}

/* parseaza argumentele si opreste la prima forma invalida   */
static int parseazaoptiuni(int argc, char **argv, struct optiuniadmin *optiuni)
{
    for (int index = 1; index < argc; index++) {
        if (strcmp(argv[index], "-c") == 0) {
            index++;
            if (index >= argc || copiatext(optiuni->caleconfig, sizeof(optiuni->caleconfig), argv[index]) != 0) {
                return -1;
            }
            continue;
        }

        if (strcmp(argv[index], "-l") == 0) {
            optiuni->listare = 1;
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

/* afiseaza eroarea de configurare fara fprintf, cu fisier si linie*/
static int afiseazaeroareconfig(const config_t *config)
{
    char linie[marimetext];
    const char *fisier;
    int rezultat;
    const char *text;

    fisier = config_error_file(config);
    text = config_error_text(config);
    rezultat = numarlatext(config_error_line(config), linie, sizeof(linie));
    if (rezultat != 0) {
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

/* afiseaza comenzile admin de baza planificate pentru milestone */
static int afiseazacategorii(void)
{
    /* aici arat doar categoriile de comenzi utile pentru milestone */
    if (printf("categorii de administrare:\n") < 0 ||
        printf("1. lista clienti conectati\n") < 0 ||
        printf("2. comenzi in curs\n") < 0 ||
        printf("3. durata medie de executie\n") < 0 ||
        printf("4. istoric procesari\n") < 0 ||
        printf("5. lungimea cozii FIFO\n") < 0 ||
        printf("6. starea firului API/WS\n") < 0) {
        perror("printf");
        return -1;
    }

    return 0;
}

/* afiseaza mesajul de ajutor*/
static int afiseazaajutor(void)
{
    if (scrietext(STDOUT_FILENO, "folosire: ./admin_client [-c config] [-l] [-e] [-h]\n") != 0 ||
        scrietext(STDOUT_FILENO, "  -c  fisierul de configurare libconfig\n") != 0 ||
        scrietext(STDOUT_FILENO, "  -l  afiseaza categoriile de comenzi admin\n") != 0 ||
        scrietext(STDOUT_FILENO, "  -e  afiseaza si informatii de mediu\n") != 0 ||
        scrietext(STDOUT_FILENO, "  -h  afiseaza acest mesaj\n") != 0) {
        return -1;
    }

    return 0;
}

int main(int argc, char **argv)
{
    config_t config;
    struct optiuniadmin optiuni;
    char socketadmin[marimecale];
    const char *text;
    int timeoutadmin;

    if (initializeazaoptiuni(&optiuni) != 0) {
        return EXIT_FAILURE;
    }

    /* valoarea pozitiva inseamna afisarea mesajului de ajutor */
    timeoutadmin = parseazaoptiuni(argc, argv, &optiuni);
    if (timeoutadmin < 0) {
        return EXIT_FAILURE;
    }

    if (timeoutadmin > 0) {
        return afiseazaajutor() == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
    }

    /* citesc doar socketul si timeoutul, adica datele minime pentru clientul admin */
    config_init(&config);
    if (config_read_file(&config, optiuni.caleconfig) == CONFIG_FALSE) {
        (void)afiseazaeroareconfig(&config);
        config_destroy(&config);
        return EXIT_FAILURE;
    }

    /*  unix si timeoutul sunt valorile minime folosite de clientul admin in demo */
    if (config_lookup_string(&config, "server.admin_socket", &text) == CONFIG_FALSE ||
        copiatext(socketadmin, sizeof(socketadmin), text) != 0 ||
        config_lookup_int(&config, "server.admin_timeout", &timeoutadmin) == CONFIG_FALSE) {
        config_destroy(&config);
        (void)scrietext(STDERR_FILENO, "configuratie admin invalida\n");
        return EXIT_FAILURE;
    }

    config_destroy(&config);
    if (printf("client admin\n") < 0 ||
        printf("socket UNIX planificat: %s\n", socketadmin) < 0 ||
        printf("timeout administrare: %d secunde\n", timeoutadmin) < 0) {
        perror("printf");
        return EXIT_FAILURE;
    }

    if (optiuni.afiseazamediu != 0) {
        char user[marimetext];

        if (obtineutilizator(user, sizeof(user)) != 0) {
            return EXIT_FAILURE;
        }

        if (printf("mediu: USER=%s\n", user) < 0) {
            perror("printf");
            return EXIT_FAILURE;
        }
    }

    /* -l afiseaza lista de categorii admin peste sumarul deja validat */
    if (optiuni.listare != 0) {
        return afiseazacategorii() == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

/*

exemple build:
gcc -Wall -Wextra -Wpedantic -Werror -g -std=c11 -D_POSIX_C_SOURCE=200809L admin_client.c $(pkg-config --cflags --libs libconfig) -o admin_client


exemple rulare cu eroare:
./admin_client -c config/lipsa.cfg -l


exemple rulare cu succes:
./admin_client -c config/sound_sequencer.cfg -l -e


*/
