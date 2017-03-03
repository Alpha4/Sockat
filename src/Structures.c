/*
*
* Structure.c
* Author: Aurélien Brisseau & Sullivan Pineau
* Date: 03/03/2017 
*
*/

#include "Structures.h"

#include <string.h>

/***********************************************/
// Implémentation de la fonction sup_caractere_nouvelle_ligne définie dans .h
/***********************************************/
void sup_caractere_nouvelle_ligne(char *text)
{
	int len = strlen(text) - 1;
	if (text[len] == '\n')
	{
		text[len] = '\0';
	}
}
