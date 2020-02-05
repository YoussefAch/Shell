/** mini Projet -- (Mini Shell)
 *  Implémentation de pipe entre n commandes
 *  Auteur : Youssef Achenchabe
 */

#include <stdio.h>
#include <stdlib.h>
#include "readcmd.h"
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "processus.h"
#include <signal.h>
#include <fcntl.h>



/** Fonction qui permet d'executer une jème commande et gérer l'entrée 
 *  et la sortie des données 
 */
int creerProcessus(int entree, int sortie, struct cmdline* ligne, int j, int desc[2]) {
	
	pid_t pid;
	
	/* Création du processus fils qui executera la commande j */
	pid = fork();
	
	
	if (pid == 0) {
	  
		  /** Si l'sortie n'est pas la sortie standard 
		   * redirection de la sortie vers l'entrée du nouveau tube
		   *  mais avant on ferme la sortie de ce tube 
		   */
		  if (sortie != 1) {
			  
			  if (close(desc[0]) == -1) {
				  perror("erreur de fermeture");
				  exit(0);
			  }
			  
			  
			  if( dup2 (sortie, 1) == -1) {
				  perror("Erreur dup2");
				  exit(1);
			  }
			  
		  }
		  
		  
		  /** Si l'entrée n'est pas l'entrée standard 
		   * redirection de l'entrée vers la sortie du tube précédant
		   * dont on a gardé le descripteur de sa sortie  dans la variable
		   * entree  
		   */
		  if (entree != 0) {
			  
			  
			  if (dup2 (entree, 0) == -1) {
					perror("erreur dup2");
					exit(0);
			  }
			  
			  
			  if (close(entree) == -1) {
				    perror("Erreur fermeture");  
				    exit(0);
			  }
		  }



		  execvp(ligne->seq[j][0],ligne->seq[j]);
		  perror("Erreur execution ");
		  exit(1);
    }

  return 0;
	
}	



/**
   Transforme un tableau de chaines de caractère en une chaine de caractères.
 */
char* getCommande(struct cmdline* ligne){
	int i = 1;
	
	char* commande = (char*)malloc(15*sizeof(char));;
	strcpy(commande, ligne->seq[0][0]);
	
	while (ligne->seq[0][i] != NULL) {
		strcat(commande, " ");
		strcat(commande, ligne->seq[0][i]);
		++i;
	}
	if (ligne->backgrounded != NULL) {
		strcat(commande, " &");
	}
	return commande;
}



/** Déclaration de la liste des processus dans job */
List_proc liste_proc;



/** Identifiant du job dans la liste des processus */
int idshell = 1;



/** Traitant du signal SIGCHLD 
 *  dans lequel On repère les changements d'états de tous les fils
 *  et je mets à jour la liste des processus */

void trait_sigchld (int sig) {
	
	int etatFils;
	pid_t pidFils;
	
	/** Boucle sur les fils qui ont changé d'état */
    while ((pidFils = waitpid(-1, &etatFils, WNOHANG | WUNTRACED | WCONTINUED)) > 0 ) {
			
		
	    /** un fils a changé d'état */
			
			if (WIFSTOPPED(etatFils)) {
				
				printf("\nprocessus de pid [%d] est suspendu\n", pidFils);
				modifier_etat(getID(pidFils, liste_proc), SUSPENDU, &liste_proc);
				
			} else if (WIFCONTINUED(etatFils)) {
				
				printf("\nprocessus de pid [%d] est repris\n", pidFils);
				modifier_etat(getID(pidFils, liste_proc), ACTIF, &liste_proc);
				
			} else if (WIFEXITED(etatFils) ) {
				
				/** afficher si le processus est lancé en tache de fond */
				if (procBG(pidFils,liste_proc)) {
					printf("\nprocessus de pid [%d] est terminé\n",pidFils);
				}
				supprimer(pidFils, &liste_proc);
				
			} else if (WIFSIGNALED(etatFils)) {
				printf("\nprocessus de pid [%d] est tué par le signal %d\n",pidFils,WTERMSIG(etatFils));
				supprimer(pidFils, &liste_proc);
			}
			
	}
		
	
	
}



/** Traitant du signal SIGINT
 *  redirection du signal au processus en avant plan au lieu 
 *  de tuer le pere et tout ses fils (l'application shell) 
 */
void trait_sigint(int sig) {
	printf("\n");
	kill(getPID(idshell,liste_proc),SIGINT);
	return;
}



/** Traitant du signal SIGTSTP
 *  suspendre les processus en avant plan
 */
void trait_sigtstp(int sig) {
	printf("\n");
	kill(getPID(idshell,liste_proc),SIGTSTP);
	return;
}





/**
 * Programme Principal  - Mini SHELL
 */
int main(int argc, char *argv[]) {
	
	
	/** Ignorer les signaux SIGINT (ctrl+c) et SIGTSTP (ctrl+z)
	 *  au début du programme pour ne pas causer la fermeture de 
	 *  l'application shell */
	
			/* ctrl-c */
			signal(SIGINT, SIG_IGN);
		
			/* ctrl-z */
			signal(SIGTSTP, SIG_IGN);


		
	/** Associer au signal SIGCHLD recu par le père lors de 
	 *  la terminaison du fils à son traitant 
	 */
	signal(SIGCHLD, trait_sigchld);
	


	/** Déclaration de la ligne de commande */
	struct cmdline* ligne;
	
	
	
	/** Initialisation de la liste des processus de jobs */
	liste_proc = nouvelle_liste();
	
	
	
	
	/** Déclaration du pid du processus fils qui execute les commandes */
	pid_t pid;
	
	
	/** Déclaration du Descripteur de fichier  E/S */
	int DescFichier;
	
	
	/** Déclaration du tableau des descripteurs */
	int desc[2];

	
	/** Boucle infinie du shell */
	while (1) {
			
			
		/**Afficher à l'utilisateur shell>> tant qu'il tape entrer 
		 * sur le clavier 
		 */
		do {
			printf("shell>> ");
			ligne = readcmd();
		}while(ligne->seq[0] == NULL);
			
			
			
		/** Implémentation des commandes internes "exit", "cd", "jobs", 
		 * "stop", "cont"*/
		
		
		/* Commande : exit */	
		if (strcmp(ligne->seq[0][0] , "exit") == 0) {
			exit(0);
			
		/* Commande cd */
		} else if (strcmp(ligne->seq[0][0] , "cd") == 0 && ligne->seq[0][1] != NULL) {
				if (chdir(ligne->seq[0][1]) == -1) {;
					perror("erreur du chemin");
				}
				continue; 
				/* Passer directement au tour de boucle suivant 
				 * sans executer le proc fils */
		} else if (strcmp(ligne->seq[0][0] , "cd") == 0 && ligne->seq[0][1] == NULL) {
				chdir(getenv("HOME")); 
				continue;
				/* Passer directement au tour de boucle suivant 
				 * sans executer le proc fils */   
				 
		/* Commande jobs */
		} else if (strcmp(ligne->seq[0][0] , "jobs") == 0) {
				if (liste_proc != NULL) {
					afficher(liste_proc);
				}
				continue;
		
		/* Commande stop */		
		} else if (strcmp(ligne->seq[0][0] , "stop") == 0) {
				int id = atoi(ligne->seq[0][1]);
				kill(getPID(id, liste_proc),SIGSTOP);
				continue;
		
		/* Commande cont */		
		} else if (strcmp(ligne->seq[0][0] , "cont") == 0) {
				int id = atoi(ligne->seq[0][1]);
				kill(getPID(id, liste_proc),SIGCONT);
				continue;
		}
		
		
		
		
		/* Création du proccessus fils qui executera la commande*/
		pid = fork();
			
			
		if ( pid < 0 ) {
			/** fork n'a pas marché*/
			perror("processus fils a échoué");
			exit(1);
			
		} else if (pid == 0) {
		
			/** Si la commande est en avant-plan
			 *  On associe au signaux de  ctrlC et ctrlZ les nouveaux
			 *  traitant pour que le processus en avant plan subit ces 
			 *  signaux */
			if (ligne->backgrounded == NULL) {
				
				/* ctrl-c */
				signal(SIGINT, trait_sigint);
				
				/* ctrl-z */
				signal(SIGTSTP, trait_sigtstp);


				/** ---------------------------*/
				/** GESTION DES REDIRECTIONS   */
				
					/* si c'est juste la sortie qui est redirigée */
					if (ligne->out != NULL && ligne->in == NULL) {
						
						/** Ouvrir l'accès au fichier pour ecrire la sortie
						 *  de la commande 
						 */
						DescFichier = open(ligne->out, O_CREAT | O_TRUNC | O_WRONLY, 0600);
						
						if (DescFichier == -1) {
							perror("Erreur d'ouverture du fichier ");
							exit(1);
						}
						/** Création d'un nouveau point d'accès au fichier 
						 *  (la sortie standard)
						 */
						if (dup2(DescFichier,1) == -1 ) {
							perror("Erreur dup2 ");
							exit(2);
						}
						
						/** Fermeture du fichier */
						if (close(DescFichier) == -1) {
							perror("Erreur close");
							exit(3);
						}
						
					/* si l'entrée et la sortie sont redirigées vers des fichiers */
					} else if (ligne->out != NULL && ligne->in != NULL) {
						
						
						/**-------------------------------------------------*/
						/** GESION de l'entrée -----------------------------*/
						
						/** Ouvrir l'accès au fichier pour ecrire la sortie
						 *  de la commande 
						 */					
						DescFichier = open(ligne->in, O_RDONLY, 0600);
						
						if (DescFichier == -1) {
							perror("Erreur d'ouverture du fichier ");
							exit(1);
						}
						
						/** Création d'un nouveau point d'accès au fichier 
						 *  (la sortie standard)
						 */
						if (dup2(DescFichier,0) == -1 ) {
							perror("Erreur dup2 ");
							exit(2);
						}
						
						/** Fermeture du fichier */
						if (close(DescFichier) == -1) {
							perror("Erreur close");
							exit(3);
						}


						/**-------------------------------------------------*/
						/** GESION de la sortie ----------------------------*/					
						DescFichier = open(ligne->out, O_CREAT | O_TRUNC | O_WRONLY, 0600);
						
						if (DescFichier == -1) {
							perror("Erreur d'ouverture du fichier ");
							exit(1);
						}
						
						if (dup2(DescFichier,1) == -1 ) {
							perror("Erreur dup2 ");
							exit(2);
						}
						
						/** Fermeture du fichier */
						if (close(DescFichier) == -1) {
							perror("Erreur close");
							exit(3);
						}
						
					/* si c'est juste l'entrée qui est redirigée */
					} else if (ligne->out == NULL && ligne->in != NULL) {
						
						/** Ouvrir l'accès au fichier pour lire l'entrée
						 *  de la commande 
						 */					
						DescFichier = open(ligne->in, O_RDONLY, 0600);
						
						if (DescFichier == -1) {
							perror("Erreur d'ouverture du fichier ");
							exit(1);
						}
						
						/** Création d'un nouveau point d'accès au fichier 
						 *  (la sortie standard)
						 */
						if (dup2(DescFichier,0) == -1 ) {
							perror("Erreur dup2 ");
							exit(2);
						}
						
						/** Fermeture du fichier */
						if (close(DescFichier) == -1) {
							perror("Erreur close");
							exit(3);
						}
						
						
					}
					
				/** FIN DE LA GESTION DES REDIRECTIONS   */
				/** -------------------------------------*/



				/** DEBUT GESTION des PIPES */
				/** ----------------------------------------------------*/
			
				/* Pas de pipe : commande simple */
				if (ligne->seq[1] == NULL) {		

					execvp(ligne->seq[0][0],ligne->seq[0]);
					perror("Erreur execution");
					exit(1);
					
				
					
				} else {
					
					/** calul du nombre de commandes */
					int nbCommandes = 0;
					int i = 0;
					while (ligne->seq[i] != NULL) {
						nbCommandes++;
						i = i + 1;
					}
					
					
					/** indice de boucle */
					int j;
					
					
					/** l'entrée de la premiere commande est l'entrée standard */
					int entree = 0; 
					
					
					/* on itère sur le nombre de pipes dans la ligne de commande */
					for (j = 0; j < nbCommandes -1; ++j) {
						
						/** création du tube */
						if ( pipe(desc) == -1 ) {
							perror("pipe error");
							exit(0);
						}

						/** execution de la commande et gestion d'entrée et sortie
						 * de la j eme commande */
						creerProcessus(entree, desc[1], ligne, j, desc);
						
						/** mettre à jour l'entrée de la commande suivante
						 * qui est biensur la sortie du tube */
						entree = desc[0];
						
						
						/** fermer l'entrée du tube */
						if (close(desc[1]) == -1 ) {
							perror("Erreur de fermeture");
							exit(1);
						}
					

					}
					
					
					
					/* la sortie de la dernière commande est par défaut 
					 * la sortie standard, mais il reste à vérifier l'entrée
					 * de cette dernière commande si elle est bien la sortie du
					 * dernier tube crée 
					 */
					if (entree != 0) {
						
						if (dup2(entree, 0) == -1) {
							perror("Erreur dup2");
							exit(0);
						}
						
						if (close(entree) == -1) {
							perror("Erreur de fermeture");
							exit(1);
						}
					}
					
					
					/* execution de la dernière commande qui n'était pas
					 * incluse dans la boucle*/
					execvp(ligne->seq[j][0],ligne->seq[j]);
					perror("Erreur d'execution");
					exit(0);	
				}
					
							
				
				/** FIN GESTION du PIPE SIMPLE entre deux commandes     */
				/** ----------------------------------------------------*/					
				
			} 
			/** Si la commande est en arrière-plan 	
			 *  les signaux SIGINT et SIGTSTP sont déja ignorés
			 */
			
					
				
			
		} else {


			/** Processus père*/
			
			/** On attend la fin de l'execution du processus 
			 *  fils seulement s'il n'est pas executé en tache
			 *  de fond 
			 */

			 
			/** on réinitialise le compteur si la liste est devenue 
			 *  vide 
			 */
			if (liste_proc == NULL) {
				idshell = 1;
			}
			
			

			/** Si la commande est en avant-plan */
			if (ligne->backgrounded == NULL) {
				
				/** Insérer le processus dans la liste */
				inserer_en_tete(idshell++, pid, ACTIF, getCommande(ligne),FG, &liste_proc);
				
				
				/** attendre le signal SIGCHLD (changement de l'état du fils) */
				while(procActif(pid,liste_proc)) {
					pause();
				}
				
					
			/* Si la commande est en arriere-plan */
			} else {

				/** Insérer le processus dans la liste */
				inserer_en_tete(idshell, pid, ACTIF, getCommande(ligne),BG, &liste_proc);
				
				/** Affichage de l'id et pid du processus crée */
				printf("[%d]\t%d\n",idshell,pid);
				
				/** Incrémentation du compteur du shell */
				idshell++;

			}

				
		}
	}
	/* Il ne doit pas arriver ici */
	return  -1;
}
