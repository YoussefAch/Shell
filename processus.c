#include <stdio.h>
#include <malloc.h>

#include "processus.h"

/* définition du type qui servira de chaînon à la liste chaïnée */
struct fiche_proc {
  int id_shell;
  int pid;
  Etat e;
  char* commande;
  Niveau niveau;
  List_proc suivant;
};

/* liste vide */

List_proc nouvelle_liste() {
	return NULL;
}

/* affichage */
void afficher(List_proc l) {
	List_proc aux;
	aux = l;
	if (aux == NULL) {printf("Liste Vide\n");}
	while (aux != NULL) {
		printf("[%d]\t%d\t",aux->id_shell,aux->pid);
		if (aux->e == ACTIF) { printf("ACTIF\t");}
		else { printf("SUSPENDU\t"); }
		printf("%s",aux->commande);
		printf("\n");
		aux = aux->suivant;
	}	
}

/* inserer en tete */
int inserer_en_tete(int id, int pid, Etat e, char* comm, Niveau niv, List_proc *l) {
	int res;
	List_proc nouveau = (List_proc)malloc(sizeof(struct fiche_proc));
	if (nouveau != NULL) {
		nouveau->id_shell = id;
		nouveau->pid = pid;
		nouveau->e = e;
		nouveau->commande = comm;
		nouveau->niveau = niv;
		nouveau->suivant = *l; 
		*l = nouveau;
		res = 0;
	} else {
		res =1;
	}
	return res;
}

int modifier_etat(int id, Etat nouvEtat, List_proc* l) {
	
	List_proc aux = *l;
	while(aux != NULL) {
		if (aux->id_shell == id) {
			aux->e = nouvEtat;
			return 0;
			
		} else {
			aux = aux->suivant;
		}
	}
	return -1;
}


int procActif(int pidd, List_proc l) {
	List_proc aux;
	aux = l;
	if (aux == NULL) {return 0;}
	while (aux != NULL) {
		if (aux->pid == pidd && aux->e == ACTIF) { return 1;}
		else { aux = aux->suivant;}
		
	}
	return 0;
}
int getID(int pidd, List_proc l) {
	List_proc aux = l;
	while(aux != NULL) {
		if(aux->pid == pidd) {
			return aux->id_shell;
		}else {
			aux = aux->suivant;
		}
	}
	return -1;
}

int procBG(int pidd, List_proc l) {
	List_proc aux;
	aux = l;
	if (aux == NULL) {return 0;}
	while (aux != NULL) {
		if (aux->pid == pidd && aux->niveau == BG) { return 1;}
		else { aux = aux->suivant;}
		
	}
	return 0;	
	
}

int getPID(int id, List_proc l) {
	List_proc aux = l;
	while(aux != NULL) {
		if(aux->id_shell == id) {
			return aux->pid;
		}else {
			aux = aux->suivant;
		}
	}
	return -1;
}

/* supprimer */

int supprimer(int pidd, List_proc *l) {
	int res;
	List_proc parcours;
	List_proc precedant;
	if (*l == NULL) { res = 1; }
	else {
		 /* si le noeud à supprimer est à la tete */
		 if ( (*l)->pid == pidd ) {
			parcours = *l;
			*l = (*l)->suivant;
			free(parcours);
			res = 0;
		 }
		 /* Si la liste contient un seul element*/
		 else if ((*l)->suivant == NULL) {
			if ( (*l)->pid != pidd ) {
					res = 1;
			}
		 }
		 /* si le noeud est au milieu et la liste contient au mois deux elements*/
		 else  {
			precedant = *l;
			parcours = precedant->suivant;

			while ( parcours != NULL && parcours->pid != pidd ) {
					precedant = parcours;
					parcours = parcours->suivant;
			}
			if (parcours == NULL) { res = 1; }
			else {
					precedant->suivant = parcours->suivant;
					free(parcours);
					res = 0;
			}	
		}

	}
	return res;	
}

void appliquer_partout_fe(List_proc l, Etat (*fe)(Etat)) {
	
		List_proc aux;
		aux = l;
		while(aux!=NULL) {
				aux->e = fe(aux->e);
				aux = aux->suivant;
		}
}







