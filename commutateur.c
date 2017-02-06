/*
 * commutateur.c
 *
 *   Created on: 20 janv. 2017
 *      Authors: evincent, tmaherault
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <semLib.h>
#include "vxWorks.h"
#include "msgQLib.h"

#include "commutateur.h"


/*
 * main()
 * ----------------------------
 *   Fonction principale du programme
 *   
 */
int main(void) {
	faireScenario();
	afficherMenu();
	
	/* Initialisation des sémaphores */
	ID_SEMAPHORE_SONNERIE = semBCreate(SEM_Q_FIFO, 0);
	ID_SEMAPHORE_APPEL = semBCreate(SEM_Q_FIFO, 0);
	
	/* Initialisation des tâches */
	ID_TACHE_CLAVIER = taskSpawn("Lire Clavier", 90, 0x100, 2000, lireClavier);	//création et activation de la tache lireClavier()
	ID_TACHE_APPEL = taskSpawn("Commencer Appel", 90, 0x100, 2000, faireAppel);	//création et activation de la tache faireAppel()
	ID_TACHE_SONNERIE = taskSpawn("Faire Sonner", 90, 0x100, 2000, faireSonner);	//création et activation de la tache faireSonner()
}


/*
 * faireScenario()
 * ----------------------------
 *   Scénario du programme, lance la création des différents abonnés
 *
 */
void faireScenario() {
	printf("---------------------- Lancement du scénario ---------------------\n");
	creerAbonne("Proprietaire", "0202020202");
	creerAbonne("Eliott", "0553525847");
	creerAbonne("Thomas", "0296542875");
	int indexEliott = trouverAbonne("0553525847");
	int indexThomas = trouverAbonne("0296542875");
	ID_ABONNE_COURANT = trouverAbonne("0202020202");	//on initialise la variable globale ID_APPELANT
	
	Abonnes[indexThomas].m_etat = ENDERANGEMENT;	//on passe cet abonné en dérangement
	
	printf("------------------------- Fin du scénario ------------------------\n");
}

/*
 * creerAbonne()
 * ----------------------------
 *   Responsable de la création d'un abonné
 *
 *   E: prenom* -> le prénom de l'abonné
 *   E: numero* -> le numéro de téléphone de l'abonné
 *
 */
void creerAbonne(char *prenom, char *numero) {
	Abonne ab;
	ID_ABONNE++;
	ab.m_id = ID_ABONNE;
	ab.m_prenom = prenom;
	ab.m_numero = numero;
	ab.m_etat = DISPONIBLE;
	ab.m_sid = semBCreate(SEM_EMPTY, SEM_Q_FIFO);

	Abonnes[ID_ABONNE] = ab;	//on rajoute l'abonné dans le tableau d'abonnés
	printf(" - %d : %s, %s, %s\n", ab.m_id, ab.m_prenom, ab.m_numero, etatAbonne(&ID_ABONNE));
}

/*
 * trouverAbonne()
 * ----------------------------
 *   Retrouve un abonné en fonction de son numéro de téléphone
 *
 *   E: *numero -> le numéro de téléphone de l'abonné recherché
 *   S: index -> index de l'abonné si celui-ci a été trouvé, -1 sinon
 * 
 */
int trouverAbonne(char *numero) {
	int index = -1;
    int i;
    for (i=1; i <= ID_ABONNE; i++) {	//pour chaque abonné présent dans Abonnés
        if ((strcmp(Abonnes[i].m_numero, numero) == 0)) {	//si le numéro recherché est le même que celui de l'abonné à l'index courant
        	index = i;	//on sauvegarde l'index
        	break;
        }
    }
    //printf("Index retourné par trouverAbonné() :  %d\n", index);
    return index;
}

/*
 * etatAbonne()
 * ----------------------------
 *   Retourne la chaine de caractere correspondant à l'état de l'abonné
 *
 *   E: id_abonne -> l'abonné concerné
 *   S: l'état de l'abonné
 * 
 */
const char *etatAbonne(int *id_abonne) {
	const char *etat[100];
	switch(Abonnes[*id_abonne].m_etat) {
	case(DISPONIBLE):
		*etat = "disponible";
		break;
	case(ENTRAINDESONNER):
		*etat = "en train de sonner";
		break;
	case(ENTRAINDAPPELER):
		*etat = "en train d'appeler";
		break;
	case(ENDERANGEMENT):
		*etat = "en derangement";
		break;
	default:
		*etat = "";
		break;
	}
	return *etat;
}

/*
 * etablirLiaison()
 * ----------------------------
 *   Initialise la liaison entre un appelé et un appelant
 *
 *   E: id_appelant -> identifiant de l'appelant
 *   E: id_appele -> identifiant de l'appelé
 * 
 */
void etablirLiaison(int *id_appelant, int *id_appele) {
	/* Création de l'objet Liaison */
	Liaison li;
	ID_LIAISON++;
	li.m_id = ID_LIAISON;
	ID_LIAISON_COURANTE = ID_LIAISON;
	li.m_id_appelant = id_appelant;
	li.m_id_appele = id_appele;
	
	/* Création des files de messages */
	li.m_id_file_messages_appelant_vers_appele = creerFileMessages();
	li.m_id_file_messages_appele_vers_appelant = creerFileMessages();
	Liaisons[ID_LIAISON] = li;
	
	/* Initialisation des tâches respectives de l'appelant et l'appelé */
	int tacheAppelantId = taskSpawn("tacheAppelant", 90, 0x100, 2000, (FUNCPTR) tacheAppelant, li);
	if (tacheAppelantId == ERROR)
		printf("taskSpawn tacheAppelantId failed\n");
	int tacheAppeleId = taskSpawn("tacheAppele", 90, 0x100, 2000, (FUNCPTR) tacheAppele, li);
	if (tacheAppeleId == ERROR)
		printf("taskSpawn tacheAppeleId failed\n");
}

/*
 * creerFileMessages()
 * ----------------------------
 *   Responsable de la création d'une file de messages
 *
 *   S: msg_q_id -> l'identifiant de la file de messages
 *
 */
MSG_Q_ID creerFileMessages() {
	MSG_Q_ID msg_q_id = msgQCreate(MAX_MESSAGES, MAX_MESSAGE_LENGTH, MSG_Q_FIFO);
	if (msg_q_id == NULL)
		printf("msgQCreate in failed\n");
	return msg_q_id;
}

/*
 * tacheAppelant()
 * ----------------------------
 *   Tâche de l'appelant, gère l'appelant (état, vérifications) et sa communication avec l'appelé
 *
 *   E: li -> la liaison qui concerne l'appelant
 *
 */
void tacheAppelant(struct Liaison li) {
	char msgBuf[200];
	
	Abonnes[*li.m_id_appelant].m_etat = ENTRAINDAPPELER;
	
	envoyerMessage(li.m_id_file_messages_appelant_vers_appele, "call");	//on envoie un message à l'appelé pour qu'il sache qu'on l'appelle
	
	if(msgQReceive(li.m_id_file_messages_appele_vers_appelant, msgBuf, MAX_MESSAGE_LENGTH, WAIT_FOREVER) == ERROR)
		printf("msgQReceive in tacheAppelant failed");
	else {
		if (strcmp(msgBuf, "pret") == 0) {	//si l'appelant nous signale qu'il est prêt
			semGive(ID_SEMAPHORE_APPEL);	//on libère le sémaphore
			
			bool characterReceived = false;
			char input = 'a';
			while (!characterReceived) { //tant que l'appelant n'a pas appuyé sur 'q' et n'a pas rentré un input conforme
				input = getchar();
				switch(input) {
					case 'r':
						terminerAppel(&li);
						characterReceived = true;
						break;
						
					case 'q':
						terminerCommutateur();
						characterReceived = true;
						break;
												
					default:
						printf("Commande invalide.\n");
						break;
				}
			}
		}
		if (strcmp(msgBuf, "pas_pret") == 0) {	//si l'appelant nous signale qu'il n'est pas prêt
			//un signal previent l'appelant (tut tut tut)
			printf("L'appelé %s est %s, veuillez réessayer.\n", Abonnes[*li.m_id_appele].m_prenom, etatAbonne(li.m_id_appele));
			terminerAppel(&li);
		}
		else if (strcmp(msgBuf, "hangup") == 0) {
			printf("hangup recu\n");
		}
	}
}

/*
 * tacheAppele()
 * ----------------------------
 *   Tâche de l'appelé, gère l'appelé (état, vérifications) et sa communication avec l'appelant
 *
 *   E: li -> la liaison qui concerne l'appelé
 *
 */
void tacheAppele(struct Liaison li) {
	char msgBuf[200];
	
	if(msgQReceive(li.m_id_file_messages_appelant_vers_appele, msgBuf, MAX_MESSAGE_LENGTH, WAIT_FOREVER) == ERROR)
		printf("msgQReceive in tacheAppele failed");
	else {
		if (strcmp(msgBuf, "call") == 0) {	//si l'appelé reçoit un appel
			if (Abonnes[*li.m_id_appele].m_etat == DISPONIBLE) {	//si il est disponible
				printf("Le téléphone de l'appelé %s sonne, il peut décrocher en appuyant sur 'd' :\n\n", Abonnes[*li.m_id_appele].m_prenom);
				Abonnes[*li.m_id_appele].m_etat = ENTRAINDESONNER;
				semGive(ID_SEMAPHORE_SONNERIE);	//on libère le sémaphore
				
				bool characterReceived = false;
				char input = 'a';
				while (!characterReceived) { //tant que l'appelé n'a pas appuyé sur 'q' et n'a pas rentré un input conforme
					input = getchar();
					switch(input) {
						case 'd':
							printf("\nL'appelé %s a décroché ! Il peut raccrocher en appuyant  sur 'r'\n", Abonnes[*li.m_id_appele].m_prenom);
							printf("Démarrage de l'appel : \n\n");
							//semDelete(ID_TACHE_SONNERIE);	//on stoppe la sonnerie
							Abonnes[*li.m_id_appele].m_etat = ENTRAINDAPPELER;
							envoyerMessage(li.m_id_file_messages_appele_vers_appelant, "pret");	//on répond positivement à l'appelant
							characterReceived = true;
							break;
							
						case 'q':
							terminerCommutateur();
							characterReceived = true;
							break;
							
						default:
							printf("Commande invalide.\n");
							break;
					}
					if (input == 'd') {
						
					}
				}
			}
			else if (Abonnes[*li.m_id_appele].m_etat != DISPONIBLE || Abonnes[*li.m_id_appele].m_etat == ENDERANGEMENT)	//si l'appelé n'est pas disponible
				envoyerMessage(li.m_id_file_messages_appele_vers_appelant, "pas_pret");	//on répond négativement à l'appelant
		}
		else if (strcmp(msgBuf, "hangup") == 0) {	//si l'appelant a racroché
			//quit appel
			printf("hangup recu\n");
		}
	}
	//msgQDelete(msg_q_id);
}

/*
 * envoyerMessage()
 * ----------------------------
 *   Envoi un message dans une file de message
 *
 *   E: file_messages_id -> la file de messages concernée
 *   E: *message -> le message à envoyer
 *
 */
void envoyerMessage(MSG_Q_ID file_messages_id, char *message) {
	if ((msgQSend(file_messages_id, message, MAX_MESSAGE_LENGTH, WAIT_FOREVER, MSG_PRI_NORMAL)) == ERROR)
		printf("msgQSend in envoyerMessage() failed\n");
}

/*
 * faireSonner()
 * ----------------------------
 *   Sonnerie lancée par la tâche ID_TACHE_SONNERIE
 *
 */
void faireSonner() {
	while (true)  {
		semTake(ID_SEMAPHORE_SONNERIE, WAIT_FOREVER);	//on prend le sémaphore
		if ((ID_WATCHDOG = wdCreate()) == NULL)
			printf("wdCreate in faireSonner() failed\n");
		else {
		    if (wdStart(ID_WATCHDOG, 10 * sysClkRateGet(), (FUNCPTR) tropTard) == ERROR) {
		        printf("wdStart in faireSonner() failed\n");
		    }
		}
		while (Abonnes[ID_APPELE].m_etat == ENTRAINDESONNER) {
			printf("Dring !\n");
			sleep(1);
		}
	}
}

void tropTard() {
	printf("trop tard !");
}

/*
 * faireAppel()
 * ----------------------------
 *   Appel lancé par la tâche ID_TACHE_APPEL
 *
 */
void faireAppel() {
	while (true)  {
		semTake(ID_SEMAPHORE_APPEL, WAIT_FOREVER);		//on prend le sémaphore
		while (Abonnes[ID_APPELE].m_etat == ENTRAINDAPPELER && Abonnes[ID_APPELANT].m_etat == ENTRAINDAPPELER ) {
			printf("Blabla blabla blabla\n");
			Liaisons[ID_LIAISON_COURANTE].m_duree++;
			sleep(1);
		}
	}
}

/*
 * terminerAppel()
 * ----------------------------
 *   Appel lancé par la tâche ID_TACHE_APPEL
 *
 *   E: *li-> la liaison concernée par l'appel
 */
void terminerAppel(struct Liaison *li) {
	Abonnes[ID_APPELANT].m_etat = DISPONIBLE;
	Abonnes[ID_APPELE].m_etat = DISPONIBLE;
	
	msgQDelete(li->m_id_file_messages_appelant_vers_appele);
	msgQDelete(li->m_id_file_messages_appele_vers_appelant);
		
	printf("\nAu revoir ! L'appel a durée %d secondes\n\n", Liaisons[ID_LIAISON_COURANTE].m_duree);
	afficherMenu();
	taskResume(ID_TACHE_CLAVIER);
}

void terminerCommutateur() {
	taskDelete(ID_TACHE_SONNERIE);
	taskDelete(ID_TACHE_APPEL);
	semDelete(ID_SEMAPHORE_SONNERIE);
	semDelete(ID_SEMAPHORE_APPEL);
	
	printf("\nAu revoir\n");
	printf("------------------------- Fin du programme ------------------------\n");
	taskDelete(ID_TACHE_CLAVIER);
	exit(EXIT_SUCCESS);
}
/*
 * afficherMenu()
 * ----------------------------
 *   Responsable de l'affichage du menu
 *
 */
void afficherMenu() {
	printf("\n-------------------------------------------\n");
	printf("|                    MENU                 |\n");
	printf("-------------------------------------------\n");
	printf("Bienvenue. Voici les commandes disponibles : \n");
	printf(" - Réaliser un appel depuis ce terminal : d\n");
	printf(" - Simuler un appel vers ce terminal : s\n");
	printf(" - Quitter : q\n");
	printf("-------------------------------------------\n");
}

/*
 * lireClavier()
 * ----------------------------
 *   Gère les actions à réaliser en fonction des entrées clavier
 *
 */
void lireClavier() {
	bool saisie_numero = false;
	char input = 'a';
	char numeroAAppeler[11];
	int index = NULL;
	int i = 0;

	while (input) {
		input = getchar();
		
		if (!saisie_numero) {
			switch (input) {
			case 'd':
				if (ID_ABONNE_COURANT != -1) {
					if (Abonnes[ID_ABONNE_COURANT].m_etat == DISPONIBLE) {
						printf("\nVous décrochez le combiné. Vous êtes disponible.\n");
						printf("Veuillez saisir le numéro à appeler, puis appuyez sur 'Entrée' : \n");
						saisie_numero = true;
					}
					else {
						printf("\nVous ne pouvez pas décrocher le combiné car vous êtes %s.\n", etatAbonne(&ID_ABONNE_COURANT));
					}
				}
				else
					printf("L'utilisateur courant n'a pas été initialisé. Veuillez relancer le programme.\n");
				break;
				
			case 's':
				index = trouverAbonne("0553525847");	//on récupère un abonné qui va passer l'appel
				ID_APPELANT = Abonnes[index].m_id;
				ID_APPELE = ID_ABONNE_COURANT;
				printf("\nSimulation d'un appel vers ce terminal. %s va vous appeler dans quelques secondes...\n", Abonnes[index].m_prenom);
				etablirLiaison(&ID_APPELANT, &ID_APPELE);
				taskSuspend(ID_TACHE_CLAVIER);	//on désactive la saisie du menu pour récupérer les characteres plus loin dans le code
				break;
				
			case 'q':
				terminerCommutateur();
				break;
				
			default:
				printf("Commande invalide.\n");
			}
		}
		else {
			if (i < 10) {
				numeroAAppeler[i] = input;
				i++;
			}
			else {
				saisie_numero = false;
				printf("Vérification du numéro...\n");
				int size = sizeof(numeroAAppeler) / sizeof(char*);
				//printf("%d\n", size);
				
				index = trouverAbonne(numeroAAppeler);
				if (index != -1) {
					if (index != ID_ABONNE_COURANT) {
						printf("Le numéro est attribué ! \nDémarrage de la liaison avec %s ...\n", Abonnes[index].m_prenom);
						ID_APPELE = Abonnes[index].m_id;
						ID_APPELANT = ID_ABONNE_COURANT;
						etablirLiaison(&ID_APPELANT, &ID_APPELE);
						taskSuspend(ID_TACHE_CLAVIER);	//on désactive la saisie du menu pour récupérer les characteres dans la suite du code
					}
					else {
						printf("Vous ne pouvez pas vous appeler vous même. Veuillez réessayer.\n");
						i = 0;
						saisie_numero = true;
					}
				}
				else {
					printf("Le numéro n'est pas attribué. Veuillez réessayer.\n");
					i = 0;
					saisie_numero = true;
				}
			}
		}
	}
}
