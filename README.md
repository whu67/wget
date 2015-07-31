				   Calina Cristian 323CA
			Tema 4 Protocoale de comunicatie
			
			
			
In rezolvarea temei am procedat astfel:
	
1. Am facut descarcarea simpla a paginii. 
2. Am creat in functie de link structura de directoare
pentru acesta, mutandu-ma in directorul site-ului si de acolo
apeland mkdir -p pentru a le face recursiv.
3. Am inceput sa lucrez la recursivitate . M-am gandit sa fac
o functie recursiva ce primeste toate fisierele valide obtinute 
din linkul initial , numarul acestora, nivelul de recursivitate pe
care sunt , adresa , numele serverului , pathul catre director, 
optiunea -o daca este true sau false alaturi de logfile si optiunea
-e. Daca am level == 6 ies de pe nivel si ma intorc . Descarc toate
linkurile si dupa aceea obtin vecinii acestora pentru fiecare in
parte si apelez recursiv functia.
4. Pentru <-o logfile> am adaugat toate erorile ce le puteam intampina
in fisier in caz ca optiunea era selectata ,iar altfel la stderr.
5. Pentru -e am modificat conditiile de intoarcere a functiei ce intoarce
toate referintele dintr-un fisier sa tina cont doar de dimensiunea 
extensiei (sa fie egala cu 3 sau cu 4) si sa nu contina "http".

Se gasesc comentarii si in cod ce explica la fata locului.

	Folosire:
	
	Am facut si un makefile care sterge folderul creat cu "make clean" daca
	acesta este sub forma *.*.* si fisierul de log daca acesta are *.out .
	"make" va rula programul cu site-ul din test si toate optiunile selectate,
	fisierul de log fiind log.out
