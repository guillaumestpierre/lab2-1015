//[
// Pour enlever les solutions des .cpp: remplacer "//\[[^\n]*\n.*?//\][^\r\n]*[\r]*\n[\t]*" par rien, en mode regex avec ". matches new line" (dans Notepad++), dans tous les .cpp.
//]
#pragma region "Includes"//{
#define _CRT_SECURE_NO_WARNINGS // On permet d'utiliser les fonctions de copies de chaînes qui sont considérées non sécuritaires.

#include "structures.hpp"      // Structures de données pour la collection de films en mémoire.

#include "bibliotheque_cours.hpp"
#include "verification_allocation.hpp" // Nos fonctions pour le rapport de fuites de mémoire.

#include <iostream>
#include <fstream>
#include <string>
#include <limits>
#include <algorithm>
#include "cppitertools/range.hpp"
#include "gsl/span"
#include "debogage_memoire.hpp"        // Ajout des numéros de ligne des "new" dans le rapport de fuites.  Doit être après les include du système, qui peuvent utiliser des "placement new" (non supporté par notre ajout de numéros de lignes).
using namespace std;
using namespace iter;
using namespace gsl;

#pragma endregion//}

typedef uint8_t UInt8;
typedef uint16_t UInt16;

#pragma region "Fonctions de base pour lire le fichier binaire"//{

UInt8 lireUint8(istream& fichier)
{
	UInt8 valeur = 0;
	fichier.read((char*)&valeur, sizeof(valeur));
	return valeur;
}
UInt16 lireUint16(istream& fichier)
{
	UInt16 valeur = 0;
	fichier.read((char*)&valeur, sizeof(valeur));
	return valeur;
}
string lireString(istream& fichier)
{
	string texte;
	texte.resize(lireUint16(fichier));
	fichier.read((char*)&texte[0], streamsize(sizeof(texte[0])) * texte.length());
	return texte;
}

#pragma endregion//}

//TODO: Une fonction pour ajouter un Film à une ListeFilms, le film existant déjà; on veut uniquement ajouter le pointeur vers le film existant.  Cette fonction doit doubler la taille du tableau alloué, avec au minimum un élément, dans le cas où la capacité est insuffisante pour ajouter l'élément.  Il faut alors allouer un nouveau tableau plus grand, copier ce qu'il y avait dans l'ancien, et éliminer l'ancien trop petit.  Cette fonction ne doit copier aucun Film ni Acteur, elle doit copier uniquement des pointeurs.
void ajoutFilm(Film* film, Film*& ListeFilms []) {

	//Vérification que ListeFilms n'est pas pleine
	int CompteurFilms = 0;
	for (int i = 0; i < ListeFilms.length(); i++) {
		if (ListeFilms[i] != nullptr) {
			CompteurFilms++;
		}
	}

	//Création d'une nouvelle liste si ListeFilms est pleine et ajout du film
	// Comment retourner un liste plus grande si c++ ne permet pas de retourner de liste ?
	if (CompteurFilms == ListeFilms.lenght()) {
		Films* NouvelleListeFilms[];
		int k = 0;
		for (Film* filmADeplacer : ListeFilms) {
			NouvelleListeFilms[k] = filmADeplacer;
		}

		//Est-ce suffisant pour "supprimer" l'ancienne liste ?
		ListeFilms.clear();

		NouvelleListeFilms.resize(CompteurFilms + 1, film);
		NouvelleListeFilms.resize(CompteurFilms * 2);
		
	}
	else
	{
		ListeFilms.push_back(film);
	}
}

//TODO: Une fonction pour enlever un Film d'une ListeFilms (enlever le pointeur) sans effacer le film; la fonction prenant en paramètre un pointeur vers le film à enlever.  L'ordre des films dans la liste n'a pas à être conservé.
//Comment est-ce possible de modifier une liste si elle n'est pas passée en paramètre ?
void retraitFilm(Film* filmRecherché, Film*& ListeFilms[]) {
	for (Film* film : ListeFilms) {
		if (filmRecherché == film) {
			ListeFIlms.remove(film);
		}
	}
}

//TODO: Une fonction pour trouver un Acteur par son nom dans une ListeFilms, qui retourne un pointeur vers l'acteur, ou nullptr si l'acteur n'est pas trouvé.  Devrait utiliser span.

//TODO: Compléter les fonctions pour lire le fichier et créer/allouer une ListeFilms.  La ListeFilms devra être passée entre les fonctions, pour vérifier l'existence d'un Acteur avant de l'allouer à nouveau (cherché par nom en utilisant la fonction ci-dessus).
Acteur* lireActeur(istream& fichier)
{
	Acteur* retour = nullptr;

	Acteur acteur = {};
	acteur.nom = lireString(fichier);
	acteur.anneeNaissance = lireUint16(fichier);
	acteur.sexe = lireUint8(fichier);

	Acteur* acteurExistant = trouverActeur(listeFilms, acteur.nom);

	if (acteurExistant != nullptr)
		retour = acteurExistant;
	else 
	{
		retour = new Acteur(acteur);
	}
	return retour;
}

Film* lireFilm(istream& fichier, ListeFilms& listeFilms)
{
	Film film = {};
	film.titre = lireString(fichier);
	film.realisateur = lireString(fichier);
	film.anneeSortie = lireUint16(fichier);
	film.recette = lireUint16(fichier);
	film.acteurs.nElements = lireUint8(fichier);

	Film* ptrFilm = new Film(film); 
	ptrFilm->acteurs.elements = new Acteur * [ptrFilm->acteurs.nElements];
	
	//Pourrait etre remplace par un span
	for (int i = 0; i < film.acteurs.nElements; i++) 
	{
		ptrFilm->acteurs.elements[i] = lireActeur(fichier, listeFilms);
		ajouterFilm(ptrFilm->acteurs.elements[i]->joueDans, ptrFilm);
	}

	return ptrFilm;
}

ListeFilms creerListe(string nomFichier)
{
	ifstream fichier(nomFichier, ios::binary);
	fichier.exceptions(ios::failbit);

	int nElements = lireUint16(fichier);
	ListeFilms listeFilms = {0,0};

	for (int i = 0; i < nElements; i++)
	{
		listeFilms.nElements = i;
		ajouterFilm(listeFilms, lireFilm(fichier, listeFilms));
	}

	listeFilms.nElements = nElements;
	return listeFilms;
}

void detruireFilm(Film* film)
{
	for (int i = 0; i < film->acteurs.nElements; i++)
	{
		enleverFilm(film->acteurs.elements[i]->joueDans, film);
		if (film->acteurs.elements[i]->joueDans.nElements == 0)
		{
			delete[] film->acteurs.elements[i]->joueDans.elements;
			delete film->acteurs.elements[i];
		}
	}

	delete[] film->acteurs.elements;
	delete film;
}

//TODO: Une fonction pour détruire un film (relâcher toute la mémoire associée à ce film, et les acteurs qui ne jouent plus dans aucun films de la collection).  Noter qu'il faut enleve le film détruit des films dans lesquels jouent les acteurs.  Pour fins de débogage, affichez les noms des acteurs lors de leur destruction.

//TODO: Une fonction pour détruire une ListeFilms et tous les films qu'elle contient.

void afficherActeur(const Acteur& acteur)
{
	cout << "  " << acteur.nom << ", " << acteur.anneeNaissance << " " << acteur.sexe << endl;
}

void afficherFilm(const Film& film)
{
	cout << "Titre : " << film.titre << "\n";
	cout << "Acteurs : " << "\n";
	for (const Acteur* acteur : spanListeActeurs(film.acteurs))
	{
		cout << "*";
		afficherActeur(*acteur);
	}
}

void afficherListeFilms(const ListeFilms& listeFilms)
{
	static const string ligneDeSeparation = "\n---------------------------\n";
	cout << ligneDeSeparation;

	for (const Film* film : spanListeFilms(listeFilms)) 
	{
		afficherFilm(*film);
		cout << ligneDeSeparation;
	}
}

void afficherFilmographieActeur(const ListeFilms& listeFilms, const string& nomActeur)
{
	Acteur* acteur = trouverActeur(listeFilms, nomActeur);
	//Commenter ces lignes puisqu'elles ne sont pas execute dans
	//le code final afin de laisser que des lignes executes
	/*if (acteur == nullptr)
		cout << "Aucun acteur de ce nom" << endl;
	else*/
	afficherListeFilms(acteur->joueDans);
}

int main()
{
	bibliotheque_cours::activerCouleursAnsi();
	static const string ligneDeSeparation = "\n\033[35m════════════════════════════════════════\033[0m\n";

	ListeFilms listeFilms = creerListe("films.bin");
	cout << ligneDeSeparation << "Le premier film de la liste est:" << endl;

	afficherFilm(*listeFilms.elements[0]);
	cout << ligneDeSeparation << "Les films sont:" << endl;

	afficherListeFilms(listeFilms);

	trouverActeur(listeFilms, "Benedict Cumberbatch")->anneeNaissance = 1976;
	cout << ligneDeSeparation << "Liste des films où Benedict Cumberbatch joue sont:" << endl;

	afficherFilmographieActeur(listeFilms, "Benedict Cumberbatch");

	detruireFilm(listeFilms.elements[0]);
	enleverFilm(listeFilms, listeFilms.elements[0]);

	cout << ligneDeSeparation << "Les films sont maintenant:" << endl;

	afficherListeFilms(listeFilms);

	detruireListe(listeFilms);
}
