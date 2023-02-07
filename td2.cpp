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

#pragma region "Spans pour le tp (il faut des const sinon les fcts les plus en bas ne fonctionnent pas)"//{
span<Film*> spanListeFilms(const ListeFilms& listeFilms)
{ 
	return span(listeFilms.elements, listeFilms.nElements);
}

span<Acteur*> spanListeActeurs(const ListeActeurs& listeActeurs)
{ 
	return span(listeActeurs.elements, listeActeurs.nElements);
}
#pragma endregion//}

void ajouterFilm(ListeFilms& listeFilms, Film* ptrFilm)
{
	//Augmentation de la taille du tableau pour que la moitie des places soient nulles apres la fonction.
	if (listeFilms.nElements == listeFilms.capacite)
	{
		int nouvelleCapacite = (listeFilms.nElements + 1 * 2);
		Film** ptrListeFilms = new Film* [nouvelleCapacite];

		for (int i = 0; i < listeFilms.nElements; i++)
		{
			ptrListeFilms[i] = listeFilms.elements[i];
		}

		delete[] listeFilms.elements;


		listeFilms.elements = ptrListeFilms;
		listeFilms.capacite = nouvelleCapacite;
	}
	listeFilms.elements[listeFilms.nElements] = ptrFilm;
	listeFilms.nElements++;
}

void enleverFilm(ListeFilms& listeFilms, Film* ptrFilm)
{
	for (Film*& film : spanListeFilms(listeFilms))
	{ 
		if (film == ptrFilm)
		{
			if (listeFilms.nElements > 1)
				film = listeFilms.elements[listeFilms.nElements - 1];
			listeFilms.nElements--;
		}
	}
	return;
}

Acteur* trouverActeur(const ListeFilms& listeFilms, const string& nom)
{
	Acteur* retour = nullptr;
	for (Film* film : spanListeFilms(listeFilms))
	{
		for (Acteur* acteur : spanListeActeurs(film->acteurs))
		{
			if (acteur->nom == nom)
				retour = acteur;
		}
	}
	return retour;
}

Acteur* lireActeur(istream& fichier, ListeFilms& listeFilms)
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

void detruireListe(ListeFilms& listeFilms)
{
	for (Film* film : spanListeFilms(listeFilms))
		detruireFilm(film);
	delete[] listeFilms.elements;
}


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
