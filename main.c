#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define INCREMENTO_ATTIVITA 1048576 //2^20
#define SOGLIA_DECADIMENTO 68719476736 //2^36
#define FATTORE_DECADIMENTO 65536 //2^16

typedef struct Nd {
    char* pietanza;
    int valore;
    bool negato;
    long attivita;
    struct Nd * next;
    struct Nd * nextOrdine;
    struct Nd * menu;
    struct NdDp * dipendente;
} Nodo;
typedef Nodo * ptrNodo;
ptrNodo *piattiInAttesa;

typedef struct HNodo {
    char *keyPietanza;
    ptrNodo pietanza;
    bool occupato;
} HNodo;

typedef struct {
    bool tautologiaSoddisfatto;
    bool soddisfatto;
    ptrNodo listaOrdini;
    int numOrdiniDp;
    int numOrdiniLiberi;
} Dipendente;
typedef struct NdDp{
    Dipendente dipendente;
    struct NdDp *next;
} NodoDipendente;
typedef NodoDipendente *ptrNodoDp;

typedef struct {
    ptrNodo pietanza;
    int numPietanze;
} Menu;
typedef struct {
    Menu menu;
    ptrNodoDp dipendenti;
    int numDipendenti;
} ListaMenuDipendenti;

int algoritmoHash (char *n, int m){
    unsigned int hash=0;
    for(int i=0; n[i]!='\0'; i++){
        hash=(hash*31)+n[i];
    }
    return hash%m;//sondaggio lineare
}

ptrNodo inserimentoMultiset(HNodo M[], int m, char *n, ptrNodo pietanza){
    int hash=algoritmoHash(n, m);
    for(int i=0; i<m; i++){
        int j=(hash+i)%m;
        if (M[j].occupato == true && strcmp(M[j].keyPietanza, n) == 0)
            return pietanza;
        if (M[j].occupato == false){
            M[j].keyPietanza = malloc(strlen(n)+1);
            strcpy(M[j].keyPietanza, n);
            M[j].pietanza = pietanza;
            M[j].occupato = true;
            return pietanza;
        }
    }
    //fprintf ("Tabella di Hash in overflow\n");
    exit(1);
}

ptrNodo ricercaMultiset(HNodo M[], int m, char *n){
    int hash=algoritmoHash(n, m);
    for (int i=0; i<m; i++) {
        int j=(hash+i)%m;
        if (M[j].occupato==true && strcmp(M[j].keyPietanza,n)==0){
            return M[j].pietanza;
        }
        if (M[j].occupato==false){
            return NULL;
        }
    }
    return NULL;
}

typedef enum{
    MODIFICA_STATO_PIATTO,
    DIPENDENTE_SODDISFATTO,
    ORDINI_LIBERI_RIDOTTO,
} Stato;

typedef struct{
    Stato stato;
    ptrNodo listaPiatto;
    ptrNodoDp dipendente;
}ElementoCronologia;
ElementoCronologia *cronologia;

int numCibiNelMenu=0;
int numDipendenti=0;
int contatorePiattiCoda=0;
int nextPosizione=0;
int liberaPosizione=0;
int numPiattiOrdinati = 0;
long totaleLetterali=0;
bool menuRistorante = true;
ptrNodo *piattiOrdinati = NULL;

ptrNodo insInTestaOrdini(HNodo hash[], int m, ptrNodo testa, char* pietanza, bool negato) {
    ptrNodo temp = malloc(sizeof(Nodo));
    if(temp!=NULL){
        temp->negato=negato;
        temp->next=testa;
        temp->menu=ricercaMultiset(hash,m,pietanza);
        temp->nextOrdine=temp->menu->nextOrdine;
        temp->menu->nextOrdine=temp;
        testa=temp;
    }
    return testa;
}
ptrNodo insInFondoOrdini(HNodo hash[], int m, ptrNodo lista, char* pietanza, bool negato) {
    if (lista==NULL)
        return insInTestaOrdini(hash,m,lista, pietanza, negato);
    lista->next = insInFondoOrdini(hash, m, lista->next, pietanza, negato);
    return lista;
}

HNodo* creaHash(int m){
    HNodo *menuHash= calloc(m,sizeof(HNodo));
    return menuHash;
}

ptrNodo insInTestaMenu(ptrNodo menu, char* pietanza, int dimensione, int valore, bool negato) {
    ptrNodo temp = malloc(sizeof(Nodo));
    if(temp!=NULL) {
        temp->pietanza = malloc(dimensione + 1);
        strcpy(temp->pietanza, pietanza);
        temp->negato=negato;
        temp->valore=valore;
        temp->next = menu;
        temp->attivita=0;
        temp->nextOrdine=NULL;
        temp->dipendente=NULL;
        menu=temp;
    }
    return menu;
}

ptrNodo creaMenu(ptrNodo testaMenu, char* pietanza, int dimensione, int valore, bool negato){
    testaMenu=insInTestaMenu(testaMenu,pietanza, dimensione, valore, negato);
    return testaMenu;
}

ptrNodoDp insInFondoDp(ptrNodoDp lista, ptrNodoDp nuovo) {
    if (lista==NULL) return nuovo;
    lista->next = insInFondoDp(lista->next, nuovo);
    return lista;
}


bool elaboraOrdini(ptrNodoDp lista, ptrNodo menu, ptrNodo corrente);

void insInCoda(ptrNodo lista){
    piattiInAttesa[liberaPosizione]=lista;
    liberaPosizione++;
}

bool propagaAssegnazioni(ptrNodoDp lista, ptrNodo menu){
    while(nextPosizione < liberaPosizione){
        if(elaboraOrdini(lista, menu, piattiInAttesa[nextPosizione]->nextOrdine)==false){
            nextPosizione=0;
            liberaPosizione=0;
            return false;
        }
        nextPosizione++;
    }
    nextPosizione=0;
    liberaPosizione=0;
    return true;
}

ptrNodoDp creoListaOrdiniDp(HNodo hash[], int m, ptrNodoDp testaLista, char* pietanza, bool negato) {
    if(testaLista==NULL) {
        testaLista= malloc(sizeof(NodoDipendente));
        testaLista->dipendente.listaOrdini=NULL;
        testaLista->dipendente.numOrdiniDp=0;
        testaLista->next=NULL;
    }
    ptrNodo temp=testaLista->dipendente.listaOrdini;
    ptrNodo trovato=ricercaMultiset(hash,m,pietanza);
    while(temp!=NULL){
        if(temp->menu==trovato && ((negato==true && temp->negato==false) || (negato==false && temp->negato==true))){
            testaLista->dipendente.tautologiaSoddisfatto=true;
            return testaLista;
        }
        if(temp->menu==trovato && ((negato==true && temp->negato==true) || (negato==false && temp->negato==false))){
            return testaLista;
        }
        temp=temp->next;
    }
    testaLista->dipendente.listaOrdini=insInTestaOrdini(hash, m, testaLista->dipendente.listaOrdini, pietanza, negato);
    testaLista->dipendente.listaOrdini->dipendente=testaLista;
    testaLista->dipendente.numOrdiniDp++;
    totaleLetterali++;
    return testaLista;
}

ptrNodo ricercaPiattoLibero(ptrNodoDp lista){
    int numPiattiLiberi=0;
    ptrNodo temp=lista->dipendente.listaOrdini;
    ptrNodo riferimento;
    while(temp!=NULL){
        if(temp->menu->valore==-1){
            numPiattiLiberi++;
            riferimento=temp;
        }
        temp=temp->next;
    }
    if(numPiattiLiberi==1) return riferimento;
    return NULL;
}

ptrNodo ricercaPiattoMenuLibero(ptrNodo lista);

void salvaStatoPiatto(ptrNodo piatto){
    cronologia[contatorePiattiCoda].stato=MODIFICA_STATO_PIATTO;
    cronologia[contatorePiattiCoda].listaPiatto = piatto;
    contatorePiattiCoda++;
}

void salvaStatoDipendente(ptrNodoDp dp, Stato tipo){
    cronologia[contatorePiattiCoda].stato=tipo;
    cronologia[contatorePiattiCoda].dipendente=dp;
    contatorePiattiCoda++;
}

void ripristinaStato(int puntoDiRipristino){
    for(int i=contatorePiattiCoda-1; i>=puntoDiRipristino; i--){
        if(cronologia[i].stato==MODIFICA_STATO_PIATTO){
            cronologia[i].listaPiatto->valore=-1;
        }else if(cronologia[i].stato==DIPENDENTE_SODDISFATTO){
            cronologia[i].dipendente->dipendente.soddisfatto=false;
        }else if(cronologia[i].stato==ORDINI_LIBERI_RIDOTTO){
            cronologia[i].dipendente->dipendente.numOrdiniLiberi++;
        }
    }
    contatorePiattiCoda=puntoDiRipristino;
}

bool esploraScelte(ptrNodoDp lista, ptrNodo menu){
    ptrNodo liberoMenu=ricercaPiattoMenuLibero(menu);
    int puntoDiRipristino=contatorePiattiCoda;
    if(liberoMenu==NULL) return true;
    liberoMenu->valore=1;
    salvaStatoPiatto(liberoMenu);
    insInCoda(liberoMenu);
    if(propagaAssegnazioni(lista,menu)==true && esploraScelte(lista,menu)==true) return true;
    ripristinaStato(puntoDiRipristino);
    liberoMenu->valore=0;
    salvaStatoPiatto(liberoMenu);
    insInCoda(liberoMenu);
    if(propagaAssegnazioni(lista,menu)==true && esploraScelte(lista,menu)==true) return true;
    ripristinaStato(puntoDiRipristino);
    return false;
}

bool valutaStatoCorrente(ptrNodoDp lista, ptrNodo menu, ptrNodoDp corrente){
    ptrNodo libero;
    while(corrente!=NULL){
        if(corrente->dipendente.soddisfatto==false && corrente->dipendente.tautologiaSoddisfatto==false){
            if(corrente->dipendente.numOrdiniLiberi==0) return false;
            if(corrente->dipendente.numOrdiniLiberi==1){
                libero=ricercaPiattoLibero(corrente);
                if(libero!=NULL){
                    libero->menu->valore=!(libero->negato);
                    salvaStatoPiatto(libero->menu);
                    insInCoda(libero->menu);
                    if(propagaAssegnazioni(lista,menu)==false) return false;
                }
            }
        }
        corrente=corrente->next;
    }
    return esploraScelte(lista, menu);
}

bool elaboraOrdini(ptrNodoDp lista, ptrNodo menu, ptrNodo corrente){
    ptrNodo libero;
    while(corrente!=NULL){
        if(corrente->dipendente->dipendente.tautologiaSoddisfatto==true || corrente->dipendente->dipendente.soddisfatto==true){
            corrente=corrente->nextOrdine;
            continue;
        }
        if((corrente->negato==true && corrente->menu->valore==0) || (corrente->negato==false && corrente->menu->valore==1)){
            corrente->dipendente->dipendente.soddisfatto=true;
            salvaStatoDipendente(corrente->dipendente,DIPENDENTE_SODDISFATTO);
        } else{
            corrente->dipendente->dipendente.numOrdiniLiberi--;
            salvaStatoDipendente(corrente->dipendente,ORDINI_LIBERI_RIDOTTO);
            if(corrente->dipendente->dipendente.numOrdiniLiberi==0){
                ptrNodo temp=corrente->dipendente->dipendente.listaOrdini;
                while(temp!=NULL){
                    temp->menu->attivita += INCREMENTO_ATTIVITA;
                    temp=temp->next;
                }
                bool decadimento=false;
                int controllo=1;
                for(int i=0; i<numPiattiOrdinati; i++){
                   if(controllo==1 && piattiOrdinati[i]->attivita>SOGLIA_DECADIMENTO){
                       decadimento=true;
                       controllo=0;
                   }
                }
                for(int i=0; i<numPiattiOrdinati; i++){
                    if(decadimento==true){
                        piattiOrdinati[i]->attivita=piattiOrdinati[i]->attivita/FATTORE_DECADIMENTO;
                    }
                }
                return false;
            }
            if(corrente->dipendente->dipendente.numOrdiniLiberi==1){
                libero=ricercaPiattoLibero(corrente->dipendente);
                if(libero!=NULL){
                    libero->menu->valore=!(libero->negato);
                    salvaStatoPiatto(libero->menu);
                    insInCoda(libero->menu);
                }
            }
        }
        corrente=corrente->nextOrdine;
    }
    return true;
}


ptrNodo ricercaPiattoMenuLibero(ptrNodo lista){
    ptrNodo temp=lista;
    ptrNodo maxAttivita=NULL;
    while(temp!=NULL){
        if(temp->valore==-1){
            if(maxAttivita==NULL || temp->attivita>maxAttivita->attivita) maxAttivita=temp;
        }
        temp=temp->next;
    }
    return maxAttivita;
}

bool assegnaLetteraliPuri(ptrNodoDp lista, ptrNodo menu){
    ptrNodo temp1=menu;
    ptrNodo temp2;
    bool vistoPositivo;
    bool vistoNegativo;
    while(temp1!=NULL){
        vistoPositivo=false;
        vistoNegativo=false;
        temp2=temp1->nextOrdine;
        while(temp2!=NULL){
            if(temp2->dipendente->dipendente.tautologiaSoddisfatto==false){
                if(temp2->negato==false){
                    vistoPositivo=true;
                } else{
                    vistoNegativo=true;
                }
            }
            temp2=temp2->nextOrdine;
        }
        if(temp1->valore==-1){
            if(vistoPositivo==true && vistoNegativo==false){
                temp1->valore=1;
                salvaStatoPiatto(temp1);
                insInCoda(temp1);
            } else if(vistoNegativo==true && vistoPositivo==false){
                temp1->valore=0;
                salvaStatoPiatto(temp1);
                insInCoda(temp1);
            }
        }
        temp1=temp1->next;
    }
    return propagaAssegnazioni(lista,menu);
}


ptrNodo resettaPiattiMenu(ptrNodo menu){
    ptrNodo temp=menu;
    while(temp!=NULL){
        temp->valore=-1;
        temp=temp->next;
    }
    return menu;
}

ptrNodoDp resettaOrdiniLiberiDp(ptrNodoDp lista){
    ptrNodoDp temp=lista;
    while(temp!=NULL){
        temp->dipendente.numOrdiniLiberi=temp->dipendente.numOrdiniDp;
        temp->dipendente.soddisfatto=false;
        temp=temp->next;
    }
    return lista;
}

void menuValido(ptrNodoDp* lista, int numOrdiniLiberi, ptrNodo menu){
    int contatore=0;
    int stampaIniziale=1;
    menu=resettaPiattiMenu(menu);
    resettaOrdiniLiberiDp(lista[0]);
    contatorePiattiCoda=0;
    while(assegnaLetteraliPuri(lista[0],menu)==false  || valutaStatoCorrente(lista[0],menu,lista[0])==false){
        if (stampaIniziale==1){
            printf("KO\n");
            stampaIniziale=0;
        }
        numOrdiniLiberi--;
        lista[numOrdiniLiberi]->dipendente.tautologiaSoddisfatto=true;
        contatore--;
        printf("%d\n", contatore);
        menu=resettaPiattiMenu(menu);
        resettaOrdiniLiberiDp(lista[0]);
        contatorePiattiCoda=0;
    }
    printf("OK\n");
}

int main(void){
    ListaMenuDipendenti lista;
    lista.menu.pietanza = NULL;
    lista.menu.numPietanze = 0;
    lista.dipendenti = NULL;
    lista.numDipendenti = 0;
    char pietanza[100];
    char spazio;
    HNodo* menuHash= NULL;
    ptrNodo menuTesta = NULL;
    ptrNodoDp dipendenteCorrente = NULL;
    while (scanf("%99s%c", pietanza, &spazio) == 2) {
        if (menuRistorante) {
            menuTesta=creaMenu(menuTesta,pietanza,strlen(pietanza),-1, false);
            numCibiNelMenu++;
            if (spazio=='\n'){
                menuHash=creaHash(numCibiNelMenu);
                ptrNodo temp=menuTesta;
                while(temp!=NULL){
                    inserimentoMultiset(menuHash,numCibiNelMenu,temp->pietanza,temp);
                    temp=temp->next;
                }
                menuRistorante = false;
            }
        } else {
            if(pietanza[0]=='-')dipendenteCorrente=creoListaOrdiniDp(menuHash,numCibiNelMenu,dipendenteCorrente,pietanza+1,true);
            else dipendenteCorrente=creoListaOrdiniDp(menuHash,numCibiNelMenu,dipendenteCorrente,pietanza,false);
            if(spazio=='\n'){
                lista.dipendenti=insInFondoDp(lista.dipendenti, dipendenteCorrente);
                lista.numDipendenti++;
                dipendenteCorrente=NULL;
            }
        }
    }
    ptrNodo temp=menuTesta;
    piattiOrdinati=malloc((numCibiNelMenu==0? 1:numCibiNelMenu)*sizeof(ptrNodo));
    while(temp!=NULL){
        piattiOrdinati[numPiattiOrdinati++]=temp;
        temp=temp->next;
    }
    piattiInAttesa=malloc(numCibiNelMenu*sizeof(ptrNodo));
    cronologia=malloc((numCibiNelMenu+lista.numDipendenti+totaleLetterali) *sizeof(ElementoCronologia));
    ptrNodoDp *listaTempDp = malloc((lista.numDipendenti? lista.numDipendenti:1)*sizeof(ptrNodoDp));
    ptrNodoDp temp1=lista.dipendenti;
    int i=0;
    while(temp1!=NULL){
        listaTempDp[i++]=temp1;
        temp1=temp1->next;
    }
    menuValido(listaTempDp,lista.numDipendenti, menuTesta);
    return 0;
}