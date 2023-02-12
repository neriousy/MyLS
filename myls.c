#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <limits.h>
#include <pwd.h>
#include <grp.h>


typedef struct {
    char * name;
    char * temp_name; //miejsce na nazwe bez "."
} Plik;


int maxUserWidth = 0; // zmienne do wyswietlania kolumn
int maxGrpWidth = 0;
int maxSizeWidth = 0;
int maxLinkWidth = 0;

void mysyserr(char *mymsg); //funkcja do wyswietlania bledow
void permissions(struct stat st, int * toRead); // funkcja wyswietlajaca prawa do plku
void umcTime(struct stat st); // funkcja wyswietlajaca date o uzyciu pliku, modyfikacjach pliku i zmianie stanu pliku
char * month(int d); // funkcja zamieniajaca liczbe na miesiac
void read2Lines(char * filename); // funkcja zczytujaca dwie pierwsze linie z pliku
void printLink(char * filename); // funkcja wyswietlajaca sciezke do dowiazania (jesli istnieje)
void sizeByte(struct stat st); // funkcja wyswietlajaca rozmiar pliku
int fileType(struct stat st); // funkcja wyswietlajaca typ pliku
char * path(); // funkcja wyswietlaja sciezke do pliku
int nameSort(const void * a, const void * b); // funkcja sortujaca pliki wg. nazwy bez kropek;
int fileTypeShort(struct stat st); // funkcja zwracajaca typ pliki w krotkim formacie (d, l, -)
char *owner(struct stat st); // funkcja zwracajaca wlasciciela pliku
char *group(struct stat st); // funkcja zwracajaca grupe pliku
void lastUsed(struct stat st); // funkcja wywietlajaca date ostatniego dostepu
void showSym(struct stat st); // funkcja wyswietlajaca link do pliku
void columnWidth(Plik file); // funkcja zbierajaca dane o szerokosciach kolumn
void nicePrintingWords(int maxWidth, char *word); // funkcja wyswietlajaca slowa
void nicePrintingNumbers(int maxWidth, int number); // funkcja wyswietlajaca liczby

int main(int argc, char * argv[]) {

    int j = 0, i = 0;
    int filesCount = 0;
    int * toRead; //zmienna pomocnicza czy zczytywac z pliku dwa pierwsze wersje
    int symbolic = 0; //zmienna pomocnicza czy dany plik jest plikiem symbolicznym
    struct stat st; // struktura stat
    char * aPath;
    DIR * dire;
    struct dirent * file;
    toRead = (int * ) malloc(sizeof(int));
    toRead[0] = 1;

    if (argc == 1) { // praca programu bez podania argumentu
        dire = opendir(".");
        if (dire == NULL) {
            mysyserr("Blad z odczytem folderu");
            exit(1);
        }

        for (file = readdir(dire); file != NULL; file = readdir(dire)) {
            filesCount++; // zliczanie ilosci plikow w folderze
        }
        closedir(dire);

        Plik files[i];
        dire = opendir(".");
        if (dire == NULL) {
            mysyserr("Blad z odczytem folderu");
            exit(1);
        }

        for (file = readdir(dire); file != NULL; file = readdir(dire)) {
            files[j].name = file -> d_name;
            files[j].temp_name = file -> d_name;
            columnWidth(files[j]);
            if (files[j].temp_name[0] == '.') { // jesli plik zaczyna sie kropka wtedy temp_name jest bez kropki na starcie
                files[j].temp_name++;           // do prostszego sortowania
            }
            j++;
        }

        qsort(files, filesCount, sizeof(Plik), nameSort); // sortowanie struktur wg. temp_name
        for (j = 0; j < filesCount; j++) {
            lstat(files[j].name, & st);
            symbolic = fileTypeShort(st);
            permissions(st, toRead);
	        nicePrintingNumbers(maxLinkWidth, st.st_nlink);
	        nicePrintingWords(maxUserWidth, owner(st));
	        nicePrintingWords(maxGrpWidth, group(st));
            nicePrintingNumbers(maxSizeWidth, st.st_size);
            lastUsed(st);
            printf("%-s", files[j].name);
            if (symbolic == 1) { // jesli plik jest linkkiem wyswietla jego dowiazanie
                char buff[PATH_MAX];
                readlink(files[j].name, buff, PATH_MAX);
                printf(" -> %s", buff);
            }
            printf("\n");
        }
    } else if (argc == 2) { // kod wykonywany po podaniu argumentu
        if(lstat(argv[1], & st) == -1){
		mysyserr("Blad z odczytem pliku ");
		exit(1);
	}; // pobieranie informacji o pliku
        printf("Informacje o: %s\n", argv[1]);
        symbolic = fileType(st);
        aPath = path();
        printf("Sciezka: \t%s/%s\n", aPath, argv[1]);
        if (symbolic == 1) {
            printLink(argv[1]);
        }
        sizeByte(st);
        printf("Uprawnienia: \t");
        permissions(st, toRead);
        printf("\n");
        umcTime(st);
        if ((toRead[0] == 1) && (st.st_size != 0)) {
            read2Lines(argv[1]);
        } else if((toRead[0] == 1) && (st.st_size == 0)){
		printf("Plik jest pusty\n");
	    }
        free(toRead);
    } else{
	printf("Podaj maksymalnie jeden argument\n");
    }
    return 0;
}

void mysyserr(char *mymsg){ // funkcja do wyswietlkania bledow
	printf("ERROR: %s (errno: %d, %s)\n", mymsg, errno, strerror(errno));
}

void permissions(struct stat st, int * toRead) { // wyswietlenie praw w formacie rwx-rw-r

    st.st_mode & S_IRUSR ? printf("r") : printf("-");
    st.st_mode & S_IWUSR ? printf("w") : printf("-");
    if (st.st_mode & S_ISUID) {
        st.st_mode & S_IXUSR ? (printf("s"), toRead[0] = 0) : printf("S");
    } else {
        st.st_mode & S_IXUSR ? (printf("x"), toRead[0] = 0) : printf("-");
    }
    st.st_mode & S_IRGRP ? printf("r") : printf("-");
    st.st_mode & S_IWGRP ? printf("w") : printf("-");
    if (st.st_mode & S_ISGID) {
        st.st_mode & S_IXGRP ? (printf("s"), toRead[0] = 0) : printf("S");
    } else {
        st.st_mode & S_IXGRP ? (printf("x"), toRead[0] = 0) : printf("-");
    }

    st.st_mode & S_IROTH ? printf("r") : printf("-");
    st.st_mode & S_IWOTH ? printf("w") : printf("-");
    if (st.st_mode & S_ISVTX) {
        st.st_mode & S_IXOTH ? (printf("t"), toRead[0] = 0) : printf("T");
    } else {
        st.st_mode & S_IXOTH ? (printf("x"), toRead[0] = 0) : printf("-");
    }
}

char * month(int d) { // zamiana cyfry na miesiac po polsku
    
    char * m;
    switch (d) {
        case 0:
            m = "Styczen";
            break;
        case 1:
            m = "Luty";
            break;
        case 2:
            m = "Marzec";
            break;
        case 3:
            m = "Kwiecien";
            break;
        case 4:
            m = "Maj";
            break;
        case 5:
            m = "Czerwiec";
            break;
        case 6:
            m = "Lipiec";
            break;
        case 7:
            m = "Sierpien";
            break;
        case 8:
            m = "Wrzesien";
            break;
        case 9:
            m = "Pazdziernik";
            break;
        case 10:
            m = "Listopad";
            break;
        case 11:
            m = "Grudzien";
            break;
        default:
            m = "Blad";
            break;
        }
    return m;
}

int fileType(struct stat st) {

    int symbolic = 0;
    printf("Typ pliku: \t");
    switch (st.st_mode & S_IFMT) { // sprawdzanie typu pliku
        case S_IFBLK:
            printf("Block device\n");
            break;
        case S_IFCHR:
            printf("Character device\n");
            break;
        case S_IFDIR:
            printf("Katalog\n");
            break;
        case S_IFIFO:
            printf("FIFO/pipe\n");
            break;
        case S_IFLNK:
            printf("Dowiazanie symboliczne\n");
            symbolic = 1;
            break;
        case S_IFREG:
            printf("Zwykly plik\n");
            break;
        case S_IFSOCK:
            printf("Socket\n");
            break;
        default:
            printf("Nieznany\n");
            break;
        }
    return symbolic; // zwraca czy plik jest linkiem 0 lub 1
}


char * path() { // zwraca aktualna sciezke 
    char * path;
    char bufor[PATH_MAX + 1];
    path = getcwd(bufor, PATH_MAX + 1); //getcwd = getCurrentWorkingDirectory
    return path;
}

void umcTime(struct stat st) { // wyswietlenie dat kiedy plik byl ostatnio uzywany/modyfikowany/mial zmieniany stan wykorzystywane w 2 trybie
    
    struct tm datetime;
    char * miesiac;
    datetime = * (localtime(&st.st_atime)); // wypelnienie struktury tm datetime danymi z struktury st
    miesiac = month(datetime.tm_mon); // zmiana miesiaca z cyfry na nazwe
    printf("Ostatnio uzywany: \t\t ");

    if(datetime.tm_mday < 10){ // jesli dzien miesiaca 1, 2,..,9 wyswietla jako 01,...,09
	    printf("0%d ",datetime.tm_mday);
    } else{
	    printf("%d ", datetime.tm_mday);
    }
    printf("%s %d roku o ", miesiac, datetime.tm_year + 1900);
    if (datetime.tm_hour < 10) { // jesli godzina jest mniejsza od 10 to zaczyna sie od 0 np. 06
        printf("0%d:", datetime.tm_hour);
    } else {
        printf("%d:", datetime.tm_hour);
    }

    if (datetime.tm_min < 10) {
        printf("0%d:", datetime.tm_min);
    } else {
        printf("%d:", datetime.tm_min);
    }

    if (datetime.tm_sec < 10) {
        printf("0%d\n", datetime.tm_sec);
    } else {
        printf("%d\n", datetime.tm_sec);
    }


    datetime = * (localtime( & st.st_mtime));
    miesiac = month(datetime.tm_mon);
    printf("Ostatnio modyfikowany: \t\t ");
    if(datetime.tm_mday < 10){
	    printf("0%d ",datetime.tm_mday);
    } else{
	    printf("%d ", datetime.tm_mday);
    }

    printf("%s %d roku o ", miesiac, datetime.tm_year + 1900);

    if (datetime.tm_hour < 10) {
        printf("0%d:", datetime.tm_hour);
    } else {
        printf("%d:", datetime.tm_hour);
    }

    if (datetime.tm_min < 10) {
        printf("0%d:", datetime.tm_min);
    } else {
        printf("%d:", datetime.tm_min);
    }

    if (datetime.tm_sec < 10) {
        printf("0%d\n", datetime.tm_sec);
    } else {
        printf("%d\n", datetime.tm_sec);
    }

    datetime = * (localtime( & st.st_ctime));
    miesiac = month(datetime.tm_mon);
    printf("Ostatnio zmieniony stan: \t ");
    if(datetime.tm_mday < 10){
	    printf("0%d ",datetime.tm_mday);
    } else{
	    printf("%d ", datetime.tm_mday);
    }
    printf("%s %d roku o ", miesiac, datetime.tm_year + 1900);

    if (datetime.tm_hour < 10) {
        printf("0%d:", datetime.tm_hour);
    } else {
        printf("%d:", datetime.tm_hour);
    }

    if (datetime.tm_min < 10) {
        printf("0%d:", datetime.tm_min);
    } else {
        printf("%d:", datetime.tm_min);
    }

    if (datetime.tm_sec < 10) {
        printf("0%d\n", datetime.tm_sec);
    } else {
        printf("%d\n", datetime.tm_sec);
    }
}


void read2Lines(char * filename) { // wyswietlenie do dwoch pierwszy lin pliku

    int fd, i;
    char buffor[2];
    fd = open(filename, O_RDWR);
    if (fd == -1) {
        mysyserr("Czytanie z pliku\n");
    }

    printf("Poczatek zawartosci:\n");
    for (i = 0; i <= 1; i++) {
        do {
            read(fd, buffor, 1);
            printf("%c", buffor[0]);
        } while (buffor[0] != '\n');
    }
    close(fd);
}


void printLink(char * filename) { // pelna sciezka do pliku na ktory wskazuje link
    char buff[PATH_MAX];
    realpath(filename, buff);
    printf("Wskazuje na: \t%s\n", buff);
}

void sizeByte(struct stat st) { // wyswietlenie wielkosci pliku z dobra odmiana
    if (st.st_size == 1) {
        printf("Rozmiar: \t%ld bajt\n", st.st_size);
    } else if (st.st_size > 1 && st.st_size < 5) {
        printf("Rozmiar: \t%ld bajty\n", st.st_size);
    } else {
        printf("Rozmiar: \t%ld bajtow\n", st.st_size);
    }
}


int nameSort(const void * a, const void * b) { // sortowanie plikow wg. nazwy wyłączając znak kropki dla plikow typu dotfile
    const Plik * _a = a;
    const Plik * _b = b;
    return strcasecmp(_a -> temp_name, _b -> temp_name);

}


int fileTypeShort(struct stat st) { // wyswietlenie typow pliku w krotkim formacie
   
    int symbolic = 0;
    switch (st.st_mode & S_IFMT) {
        case S_IFDIR:
            printf("d");
            break;
        case S_IFLNK:
            printf("l");
            symbolic = 1;
            break;
        case S_IFREG:
            printf("-");
            break;
        case S_IFIFO:
            printf("f");
            break;
        case S_IFSOCK:
            printf("s");
            break;
        case S_IFBLK:
            printf("b");
            break;
        case S_IFCHR:
            printf("c");
            break;
        default:
            printf("-");
            break;
    }
    return symbolic;
}


char* group(struct stat st) { // zwrocenie nazwy grupy
    struct group * grp;
    grp = getgrgid(st.st_gid);
	return grp -> gr_name;
}


char* owner(struct stat st){ // zwrocenie nazwy wlasciciela pliku
	struct passwd *pwd;
	pwd = getpwuid(st.st_uid);
	return pwd -> pw_name;
}



void lastUsed(struct stat st) { // wyswietlenie daty kiedy plik byl ostatnio uzywany, funkcja uzywana w 1 trybie
    time_t time_ms;
    time( & time_ms);
    struct tm * data = localtime( & time_ms);
    struct tm datetime;
    char * miesiac;
    int year = data -> tm_year + 1900;
    datetime = * (localtime( & st.st_mtime));
    switch (datetime.tm_mon) { // zmiana cyfry miesiaca na format krotki po angielsku
        case 0:
            miesiac = "Jan";
            break;
        case 1:
            miesiac = "Feb";
            break;
        case 2:
            miesiac = "Mar";
            break;
        case 3:
            miesiac = "Apr";
            break;
        case 4:
            miesiac = "May";
            break;
        case 5:
            miesiac = "Jun";
            break;
        case 6:
            miesiac = "Jul";
            break;
        case 7:
            miesiac = "Aug";
            break;
        case 8:
            miesiac = "Sep";
            break;
        case 9:
            miesiac = "Oct";
            break;
        case 10:
            miesiac = "Nov";
            break;
        case 11:
            miesiac = "Dec";
            break;
        default:
            miesiac = "Unk";
            break;
    }

    printf(" %s ", miesiac);
    if(datetime.tm_mday >= 10){
	    printf("%d ", datetime.tm_mday);
    } else{
	    printf("0%d ", datetime.tm_mday);
    }
    if (year != (datetime.tm_year + 1900)) { // jesli data nie jest z tego roku wyswietli rok, jak z tego samego roku wyswietl godzine i minute
        printf(" %d ", datetime.tm_year + 1900);
    } else {
        if (datetime.tm_hour < 10) {
            printf("0%d:", datetime.tm_hour);
        } else {
            printf("%d:", datetime.tm_hour);
        }
        if (datetime.tm_min < 10) {
            printf("0%d ", datetime.tm_min);
        } else {
            printf("%d ", datetime.tm_min);
        }
    }
}



void columnWidth(Plik file){ // szukanie najszerszych ciagow w kolumnach: iloscl linkow, nazwa uzytkownika, nazwa grupy, rozmiar pliku

    int curUserWidth = 0;
    int curGrpWidth = 0;
    struct stat st;
    struct passwd * pwd;
    struct group * grp;
    char temp[30];
    int tempSize = 0;

    lstat(file.name, &st);
    pwd = getpwuid(st.st_uid);
    grp = getgrgid(st.st_gid);

    curUserWidth = strlen(pwd -> pw_name);
    if(curUserWidth > maxUserWidth) maxUserWidth = curUserWidth;

    curGrpWidth = strlen(grp -> gr_name);
    if(curGrpWidth > maxGrpWidth) maxGrpWidth = curGrpWidth;

    tempSize = st.st_size;
    sprintf(temp, "%d", tempSize); // rzutowanie inta na char*
    if(strlen(temp) > maxSizeWidth) maxSizeWidth = strlen(temp);

    tempSize = st.st_nlink;
    sprintf(temp, "%d", tempSize);
    if(strlen(temp) > maxLinkWidth) maxLinkWidth = strlen(temp);
}


void nicePrintingWords(int maxWidth, char* word){ // wyswietlenie wyrazow i dopelnienie do kolumny 

	int wordWidth;
	wordWidth = strlen(word);
	printf(" %s ", word);
	while(maxWidth > wordWidth){
		printf(" ");
		wordWidth++;
	}
}


void nicePrintingNumbers(int maxWidth, int number){ // dopelnienie do kolumny i wyswietlenie liczb
	int wordWidth;
	char temp[30];
	sprintf(temp, "%d", number);
	wordWidth = strlen(temp);
	while(maxWidth > wordWidth){
		printf(" ");
		wordWidth++;
	}
	printf(" %d", number);
}