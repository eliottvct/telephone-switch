/*
 * commutateur.h
 *
 *  Created on: 20 janv. 2017
 *     Authors: evincent, tmaherault
 */

/* Type booléen */
typedef int bool;
#define true 1
#define false 0

#define MAX 512;
#define MAX_ABONNES 20;

/* Enumération */
typedef enum Etats {DISPONIBLE, ENTRAINDESONNER, ENTRAINDAPPELER, ENDERANGEMENT} Etats;

/* Structures */
typedef struct Abonne {
	int m_id;
	char *m_prenom;
    char *m_numero;
    enum Etats m_etat;
    SEM_ID m_sid;
} Abonne;

typedef struct Liaison {
	int m_id;
	MSG_Q_ID m_id_file_messages_appelant_vers_appele;
	MSG_Q_ID m_id_file_messages_appele_vers_appelant;
	int *m_id_appelant;
	int *m_id_appele;
	int m_duree;
} Liaison;

/* Tableaux d'objets */
struct Abonne Abonnes[];
struct Liaison Liaisons[];

/* Tâches */
int ID_TACHE_CLAVIER;
int ID_TACHE_SONNERIE;
int ID_TACHE_APPEL;

/* Sémaphores */
SEM_ID ID_SEMAPHORE_SONNERIE;
SEM_ID ID_SEMAPHORE_APPEL;

/* Watchdog */
WDOG_ID ID_WATCHDOG;

/* Variables globales */
int ID_ABONNE = 0;
int ID_ABONNE_COURANT = -1;
int ID_APPELANT = -1;
int ID_APPELE = -1;
int ID_LIAISON = 0;
int ID_LIAISON_COURANTE = -1;
int MAX_MESSAGES = 100;
int MAX_MESSAGE_LENGTH = 50;

/* fonctions */
void faireScenario();
void creerAbonne(char *prenom, char *numero);
int trouverAbonne(char *numero);
const char *etatAbonne(int *id_abonne);
void etablirLiaison(int *id_appelant, int *id_appele);
MSG_Q_ID creerFileMessages();
void tacheAppelant(struct Liaison li);
void tacheAppele(struct Liaison li);
void envoyerMessage(MSG_Q_ID file_messages_id, char *message);
void faireSonner();
void tropTard();
void faireAppel();
void terminerAppel(struct Liaison *li);
void terminerCommutateur();
void afficherMenu();
void lireClavier();
