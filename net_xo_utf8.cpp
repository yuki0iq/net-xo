// net_xo.cpp : Defines the entry point for the console application.
//

#include <vector>
#include <iostream>
#include <iomanip>
#include <string>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#ifdef WIN32
#include <process.h>
#include <Ws2tcpip.h>
#include <conio.h>
#pragma comment(lib,"ws2_32")
#define cls system("cls")
#else
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <netdb.h>
typedef int SOCKET;
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define cls system("clear")
#define closesocket close
#define Sleep(n) usleep(n)
#define HOSTENT hostent
#endif
#include "CritSect.h"

using namespace std;
#define MAX_IDS 19998
#define game(idX) fields[(idX-1)/2]
#define SYMX 'X'
#define SYMO 'O'
#define SYM0 'O'
#define SYM_EMPTY '.'

struct CLIENT
{
	SOCKET s;
	int id;
	char sym;
};
struct SERV_GAME_IN
{
	CLIENT c1;
	CLIENT c2;
};
enum game_state
{
	cont, //continue
	x_win, //xs
	o_win, //os
	draw  //draw
};
enum COLOR
{
	BLACK = 0,
	DARK_RED = 1,
	DARK_GREEN = 2,
	OLIVE = 3,
	DARK_BLUE = 4,
	DARK_PINK = 5,
	DARK_BLUEGREEN = 6,
	LIGHT_GRAY = 7,
	GRAY = 8,
	RED = 9,
	GREEN = 10,
	YELLOW = 11,
	BLUE = 12,
	PINK = 13,
	BLUEGREEN = 14,
	WHITE = 15
};

char fields[MAX_IDS / 2][3][3]; //vector< char[3][3] > fields (MAX_IDS / 2);
vector<bool> ids(MAX_IDS / 2);
bool bRU = false; //Localization

bool init();
void reEmpty(int idX);
void reEmpty(char arr[3][3]);
void serv_xo();
bool gameFull(int idX);
game_state isWin(int idX);
void out(char arr[3][3], const char *beg = "     ");
#ifdef WIN32
void __cdecl game_serv_xo(void *args);
#else
void *game_serv_xo(void *args);
#endif
void getPos(char game_f[3][3], char sym, char mov[2]);
void cli_xo();
int _getNewID();
#ifdef WIN32
COLOR COLOR_FROM_COLORWIN(WORD winColor);
WORD COLORWIN_FROM_COLOR(COLOR color);
#else
string COLORLINUX_FROM_COLOR(COLOR color, bool bBackColor);
#endif
void GetColors(COLOR &rColorText, COLOR &rColorBkgnd);
void SetColors(COLOR colorText, COLOR colorBkgnd);
char *getIP();
bool isIP(const char *ip);
COLOR stdBack = BLACK;
COLOR stdText = LIGHT_GRAY;
#define Reset() SetColors(stdText, stdBack)
#define Error() SetColors(RED, stdBack)
#define PlayerConnect() SetColors(YELLOW, stdBack)
#define Info() SetColors(GREEN, stdBack)
#define _send(s,what,size,f) {if(send(s,what,size,f) <= 0) { Error(); perror("Send failed"); Reset(); goto END; }}
#define _recv(s,what,size,f) {if(recv(s,what,size,f) <= 0) { Error(); perror("Recv failed"); Reset(); goto END; }}


//beg for dev-c++ only
#define NS_INADDRSZ  4
#define NS_IN6ADDRSZ 16
#define NS_INT16SZ   2
#define uint8_t unsigned char
#define uint32_t unsigned
int inet_pton4(const char *src, char *dst)
{
    uint8_t tmp[NS_INADDRSZ], *tp;

    int saw_digit = 0;
    int octets = 0;
    *(tp = tmp) = 0;

    int ch;
    while ((ch = *src++) != '\0')
    {
        if (ch >= '0' && ch <= '9')
        {
            uint32_t n = *tp * 10 + (ch - '0');

            if (saw_digit && *tp == 0)
                return 0;

            if (n > 255)
                return 0;

            *tp = n;
            if (!saw_digit)
            {
                if (++octets > 4)
                    return 0;
                saw_digit = 1;
            }
        }
        else if (ch == '.' && saw_digit)
        {
            if (octets == 4)
                return 0;
            *++tp = 0;
            saw_digit = 0;
        }
        else
            return 0;
    }
    if (octets < 4)
        return 0;

    memcpy(dst, tmp, NS_INADDRSZ);

    return 1;
}
int inet_pton6(const char *src, char *dst)
{
    static const char xdigits[] = "0123456789abcdef";
    uint8_t tmp[NS_IN6ADDRSZ];

    uint8_t *tp = (uint8_t*) memset(tmp, '\0', NS_IN6ADDRSZ);
    uint8_t *endp = tp + NS_IN6ADDRSZ;
    uint8_t *colonp = NULL;

    /* Leading :: requires some special handling. */
    if (*src == ':')
    {
        if (*++src != ':')
            return 0;
    }

    const char *curtok = src;
    int saw_xdigit = 0;
    uint32_t val = 0;
    int ch;
    while ((ch = tolower(*src++)) != '\0')
    {
        const char *pch = strchr(xdigits, ch);
        if (pch != NULL)
        {
            val <<= 4;
            val |= (pch - xdigits);
            if (val > 0xffff)
                return 0;
            saw_xdigit = 1;
            continue;
        }
        if (ch == ':')
        {
            curtok = src;
            if (!saw_xdigit)
            {
                if (colonp)
                    return 0;
                colonp = tp;
                continue;
            }
            else if (*src == '\0')
            {
                return 0;
            }
            if (tp + NS_INT16SZ > endp)
                return 0;
            *tp++ = (uint8_t) (val >> 8) & 0xff;
            *tp++ = (uint8_t) val & 0xff;
            saw_xdigit = 0;
            val = 0;
            continue;
        }
        if (ch == '.' && ((tp + NS_INADDRSZ) <= endp) &&
                inet_pton4(curtok, (char*) tp) > 0)
        {
            tp += NS_INADDRSZ;
            saw_xdigit = 0;
            break; /* '\0' was seen by inet_pton4(). */
        }
        return 0;
    }
    if (saw_xdigit)
    {
        if (tp + NS_INT16SZ > endp)
            return 0;
        *tp++ = (uint8_t) (val >> 8) & 0xff;
        *tp++ = (uint8_t) val & 0xff;
    }
    if (colonp != NULL)
    {
        /*
         * Since some memmove()'s erroneously fail to handle
         * overlapping regions, we'll do the shift by hand.
         */
        const int n = tp - colonp;

        if (tp == endp)
            return 0;

        for (int i = 1; i <= n; i++)
        {
            endp[-i] = colonp[n - i];
            colonp[n - i] = 0;
        }
        tp = endp;
    }
    if (tp != endp)
        return 0;

    memcpy(dst, tmp, NS_IN6ADDRSZ);

    return 1;
}
int inet_pton(int af, const char *src, char *dst)
{
    switch (af)
    {
    case AF_INET:
        return inet_pton4(src, dst);
    case AF_INET6:
        return inet_pton6(src, dst);
    default:
        return -1;
    }
}
//end for dev-c++ only


int main()
{
	if (!init())
	{
		Error();
		printf("INIT FAILED\n");
		Reset();
		return 0;
	}
	printf(
		"Tic-tac-toe over Internet! Крестики-нолики по интернету!\n"
		"Choose language and type of application:\n"
		"  [RU,server(1)/RU,client(2)/EN,server(3)/default:EN,client] "
	);
	int ch = getchar();
	if (ch == '1')
	{
		bRU = true;
		serv_xo();
	}
	else if(ch == '2')
	{
		bRU = true;
		cli_xo();
	}
	else if (ch == '3')
	{
		bRU = false;
		serv_xo();
	}
	else
	{
		bRU = false;
		cli_xo();
	}
	printf(bRU ? "Спасибо за игру! Для продолжения нажми ВВОД..." : "Thank you for playing! To continue ENTER...");
	getchar();

	return 0;
}

bool init()
{
	bool bRet = false;
#ifdef WIN32
	GetColors(stdText, stdBack);
#else
	bRet = true;
#endif
	system("chcp 1251");
	setlocale(LC_ALL, "");
	cls;
	Info();
	printf("NET_XO 1.0\n");
	Reset();
#ifdef WIN32
	// Initiate use of Ws2_32.dll (ver2.2) by the process.
	WORD wVersionRequested = MAKEWORD(2, 2);
	WSADATA wsaData;
	int iRes = ::WSAStartup(wVersionRequested, &wsaData);
	if (0 == iRes)
	{
		// Confirm that the WinSock DLL supports ver2.2.
		if (LOBYTE(wsaData.wVersion) == 2 &&
			HIBYTE(wsaData.wVersion) == 2)
		{
			bRet = true;
		}
	}
#endif
	bRet &= theCritSect.Init();
	return bRet;
}

void reEmpty(int idX)
{
	for (int i = 0; i < 3; ++i)
		for (int j = 0; j < 3; ++j)
			game(idX)[i][j] = SYM_EMPTY;
}
void reEmpty(char arr[3][3])
{
	for (int i = 0; i < 3; ++i)
		for (int j = 0; j < 3; ++j)
			arr[i][j] = SYM_EMPTY;
}

void serv_xo()
{
	SERV_GAME_IN *in = 0;
	sockaddr_in local, peer;
	SOCKET s = INVALID_SOCKET;
	int rc, size_peer = sizeof(sockaddr_in);
	local.sin_family = AF_INET;
	local.sin_port = htons(1212);
	local.sin_addr.s_addr = htonl(INADDR_ANY);
	for (int i = 0; i < MAX_IDS / 2; ++i)
	{
		reEmpty(i);
	}
	Info();
	cout << (bRU ? "СЕРВЕР\n" : "SERVER\n");
	Reset();
	s = socket(AF_INET, SOCK_STREAM, 0);
	if (INVALID_SOCKET == s)
	{
		Error();
		perror(bRU?"Невозможно создать сокет\n":"Can't create socket. Exiting...\n");
		Reset();
		return;
	}
#ifdef DEBUG
	cout << (bRU ? "Сокет создан\n" : "Socket created\n");
#endif
	rc = bind(s, (sockaddr*)&local, sizeof(local));
	if (rc == SOCKET_ERROR)
	{
		Error();
		perror(bRU ? "Невозможно пробиндить\n" : "Can't bind. Exiting...\n");
		Reset();
		return;
	}
#ifdef DEBUG
	cout << (bRU ? "Сокет пробинжен\n" : "Socket binded\n");
#endif
	rc = listen(s, 6);
	if (rc == SOCKET_ERROR)
	{
		Error();
		perror(bRU ? "Невозможно создать сокет\n" : "Can't listen. Exiting...\n");
		Reset();
		return;
	}
#ifdef DEBUG
	cout << (bRU ? "Открыт слушающий сокет\n" : "Started listening on socket\n");
#endif
	while (true)
	{
		in = new SERV_GAME_IN;

#ifdef DEBUG
		cout << (bRU ? "Жду клиента\n" : "Waiting for client\n");
#endif
#ifdef _WIN32
		in->c1.s = accept(s, (sockaddr*)&peer, &size_peer);
#else
		in->c1.s = accept(s, (sockaddr*)&peer, (socklen_t*)&size_peer);
#endif // def _WIN32
		if (in->c1.s == INVALID_SOCKET)
		{
			Error();
			perror(bRU ? "Невозможно принять клиента\n" : "Can't accept. Exiting...\n");
			Reset();
			goto END_serv_xo;
		}
		ENTER_GCRITSECT;
		PlayerConnect();
		cout << (bRU ? "Игрок подключен, IP: " : "Player connected with IP: ") << inet_ntoa(peer.sin_addr) << endl;;
		Reset();
		LEAVE_GCRITSECT;
#ifdef DEBUG
		cout << (bRU ? "Жду еще клиента\n" : "Waiting for second client\n");
#endif
#ifdef _WIN32
		in->c2.s = accept(s, (sockaddr*)&peer, &size_peer);
#else
		in->c2.s = accept(s, (sockaddr*)&peer, (socklen_t*)&size_peer);
#endif // def _WIN32
		if (in->c2.s == INVALID_SOCKET)
		{
			Error();
			perror(bRU ? "Невозможно принять клиента\n" : "Can't accept. Exiting...\n");
			Reset();
			goto END_serv_xo;
		}
		ENTER_GCRITSECT;
		PlayerConnect();
		cout << (bRU ? "Игрок подключен, IP: " : "Player connected with IP: ") << inet_ntoa(peer.sin_addr) << endl;
		Reset();
		LEAVE_GCRITSECT;
		in->c1.id = _getNewID();
		in->c2.id = in->c1.id + 1;
	#ifdef DEBUG
		cout << (bRU ? "Запускаю хост игры\n" : "Start host for game\n");
	#endif

	#ifndef WIN32
		pthread_t tid; /* идентификатор потока */
		pthread_attr_t attr; /* атрибуты потока */
							 /* получаем дефолтные значения атрибутов */
		pthread_attr_init(&attr);
		int iRes = pthread_create(&tid, &attr, game_serv_xo, in);
		if (iRes != 0)
		{
			delete in;
		}
		in = 0;
	#else
		unsigned uThr = _beginthread(game_serv_xo, 1024, in);
		if(-1L == uThr)
		{
			delete in;
		}
		in = 0;
	#endif
	}

END_serv_xo:
#ifdef DEBUG
	cout << (bRU ? "Завершаю сервер\n" : "End server\n");
#endif // DEBUG
	if(0 == in)
	{
		delete in;
		in = 0;
	}
	if(INVALID_SOCKET != s)
	{
		closesocket(s);
		s = INVALID_SOCKET;
	}
}

bool gameFull(int idX)
{
	for (int i = 0; i < 3; ++i)
		for (int j = 0; j < 3; ++j)
			if (game(idX)[i][j] == SYM_EMPTY)
				return false;
	return true;
}

game_state isWin(int idX)
{
	if ((game(idX)[0][0] == SYMX && game(idX)[0][1] == SYMX && game(idX)[0][2] == SYMX) ||
		(game(idX)[1][0] == SYMX && game(idX)[1][1] == SYMX && game(idX)[1][2] == SYMX) ||
		(game(idX)[2][0] == SYMX && game(idX)[2][1] == SYMX && game(idX)[2][2] == SYMX) ||
		(game(idX)[0][0] == SYMX && game(idX)[1][0] == SYMX && game(idX)[2][0] == SYMX) ||
		(game(idX)[0][1] == SYMX && game(idX)[1][1] == SYMX && game(idX)[2][1] == SYMX) ||
		(game(idX)[0][2] == SYMX && game(idX)[1][2] == SYMX && game(idX)[2][2] == SYMX) ||
		(game(idX)[0][0] == SYMX && game(idX)[1][1] == SYMX && game(idX)[2][2] == SYMX) ||
		(game(idX)[0][2] == SYMX && game(idX)[1][1] == SYMX && game(idX)[2][0] == SYMX)) //win 1st player (user, Xs)
	{
		return x_win;
	}
	else if ((game(idX)[0][0] == SYMO && game(idX)[0][1] == SYMO && game(idX)[0][2] == SYMO) ||
		     (game(idX)[1][0] == SYMO && game(idX)[1][1] == SYMO && game(idX)[1][2] == SYMO) ||
		     (game(idX)[2][0] == SYMO && game(idX)[2][1] == SYMO && game(idX)[2][2] == SYMO) ||
		     (game(idX)[0][0] == SYMO && game(idX)[1][0] == SYMO && game(idX)[2][0] == SYMO) ||
		     (game(idX)[0][1] == SYMO && game(idX)[1][1] == SYMO && game(idX)[2][1] == SYMO) ||
		     (game(idX)[0][2] == SYMO && game(idX)[1][2] == SYMO && game(idX)[2][2] == SYMO) ||
		     (game(idX)[0][0] == SYMO && game(idX)[1][1] == SYMO && game(idX)[2][2] == SYMO) ||
		     (game(idX)[0][2] == SYMO && game(idX)[1][1] == SYMO && game(idX)[2][0] == SYMO)) //win 2nd player (AI, Os)
	{
		return o_win;
	}
	else if (gameFull(idX)) //draw
	{
		return draw;
	}
	else
	{
		return cont;
	}
}

void out(char arr[3][3], const char *beg/* = "     "*/)
{
	Info();
	printf("%s%c %c %c\n%s%c %c %c\n%s%c %c %c\n", beg, arr[0][0], arr[0][1], arr[0][2], beg, arr[1][0], arr[1][1], arr[1][2], beg, arr[2][0], arr[2][1], arr[2][2]);
	Reset();
}

#ifdef WIN32
void __cdecl game_serv_xo(void *args)
#else
void *game_serv_xo(void *args)
#endif
{
	SERV_GAME_IN *arg = (SERV_GAME_IN*)args;
	int id = (arg->c1.id - 1) / 2 + 1;
	ids[id] = true;
	ENTER_GCRITSECT;
	cout << setw(4) << id << (bRU ? " Начало игрового цикла\n" : " Beginning Gameloop\n");
	LEAVE_GCRITSECT;
	game_state win;
	char mov[2];

//	bool bFirst = true, bWin = false, bXs = true;
	while (true)
	{
		// Decide who is who.
	#ifdef DEBUG
		cout << setw(4) << id << (bRU ? " Получение Х и О для игроков\n" : " Getting Xs and Os for clients\n");
	#endif
		arg->c1.sym = SYMX;
		arg->c2.sym = SYM0;
	#ifdef DEBUG
		cout << setw(4) << id << (bRU ? " Клиент 1: " : " Client 1: ") << arg->c1.sym << endl;
		cout << setw(4) << id << (bRU ? " Клиент 2: " : " Client 2: ") << arg->c2.sym << endl;

		// Send "who is who".
		cout << setw(4) << id << (bRU ? " Отправка Х и О игрокам\n" : " Sending Xs and Os to clients\n");
	#endif
		_send(arg->c1.s, &arg->c1.sym, 1, 0);
		_send(arg->c2.s, &arg->c2.sym, 1, 0);

		CLIENT *pGamer    = &arg->c1;
		CLIENT *pOpponent = &arg->c2;

		reEmpty(pGamer->id);

	#ifdef DEBUG
		cout << setw(4) << id << (bRU ? " Начало подцикла игры\n" : " Beginning 2nd gameloop\n");
	#endif
		while(true)
		{
			// Wait gamer.
		#ifdef DEBUG
			cout << setw(4) << id << (bRU ? " Жду игрока\n" : " Wait gamer\n");
		#endif
			ENTER_GCRITSECT;
			cout << setw(4) << id << (bRU ? " Поле:\n" : " Gamefield:\n");
			out(game(pGamer->id));
			LEAVE_GCRITSECT;
			_recv(pGamer->s, mov, 2, 0);

			// Handle gamer pos.
		#ifdef DEBUG
			cout << setw(4) << id << (bRU ? " Обрабатываю игрока\n" : " Handle gamer\n");
		#endif
			game(pGamer->id)[mov[0]][mov[1]] = pGamer->sym;
			win = isWin(pGamer->id);

			// Send status, gamer's pos to opponent.
		#ifdef DEBUG
			cout << setw(4) << id << (bRU ? " Отправляю статус и позицию игрока противнику\n" : " Send status, gamer's pos to opponent\n");
		#endif
			_send(pOpponent->s, (char*)&win, sizeof(win), 0);
			_send(pOpponent->s, mov, 2, 0);

			if(cont == win)
			{
			#ifdef DEBUG
				cout << setw(4) << id << (bRU ? " Меняю игрока и противника местами\n" : " Swap gamer and opponent\n");
			#endif
				swap(pGamer, pOpponent);
				continue;
			}

			// Send status, faked opponent's pos to gamer.
		#ifdef DEBUG
			cout << setw(4) << id << (bRU ? " Отправляю статус, 3, 3 игроку\n" : " Send status, 3, 3 to gamer\n");
		#endif
			ENTER_GCRITSECT;
			cout << setw(4) << id << (bRU ? " Игра закончена, " : " Game over, ");
			Info();
			cout << (x_win == win ? "Xs" : o_win == win ? "Os" : "Draw") << endl;
			Reset();
			LEAVE_GCRITSECT;
			_send(pGamer->s, (char*)&win, sizeof(win), 0);
			mov[0] = 3;
			mov[1] = 3;
			_send(pGamer->s, mov, 2, 0);
			swap(arg->c1, arg->c2);

			break;
		}
	#ifdef DEBUG
		cout << setw(4) << id << (bRU ? " Завершение подцикла игры\n" : " Ending 2nd gameloop\n");
	#endif
	}

END:
	cout << setw(4) << id << (bRU ? " Завершение игры и очистка ID для следующих игр\n" : " End game and clearing ID for future games\n");
	LEAVE_GCRITSECT;
	ids[id] = false;
	closesocket(arg->c1.s);
	closesocket(arg->c2.s);

	delete arg;
#ifndef WIN32
	pthread_exit(0);
#endif
}

void getPos(char game_f[3][3], char sym, char mov[2])
{
	while(true)
	{
		mov[0] = 0;
		mov[1] = 0;
		cout << (bRU ? "Введи РЯД и КОЛОНКУ для " : "Enter ROW and COL for ") << sym << (bRU ? " (к примеру 23 - ряд 2 колонка 3): " : " (e.g 23 - row 2 col 3): ");
		string s;
		Info();
		do getline(cin, s);
		while (s.size() < 2);
		Reset();
		mov[0]=s[0]-'1';
		mov[1]=s[1]-'1';
		if(mov[0] > 2 || mov[1] > 2 || mov[0] < 0 || mov[1] < 0)
		{
			cout << (bRU ? "Невозможно поставить: Вне границ\n" : "Can't place: Outside of range. Try again\n");
			continue;
		}
		if(SYM_EMPTY != game_f[mov[0]][mov[1]])
		{
			cout << (bRU ? "Невозможно постваить: Занято\n" : "Can't place: Resulting cell is full. Try again\n");
			continue;
		}
		break;
	}
	game_f[mov[0]][mov[1]] = sym;
}

void cli_xo()
{
	sockaddr_in peer;
	SOCKET s = INVALID_SOCKET;
	int rc;
	char game_f[3][3];
	char mov[2];
	char sym;

	Info();
	cout << (bRU ? "КЛИЕНТ\n" : "CLIENT\n");
	Reset();
	peer.sin_family = AF_INET;
	peer.sin_port = htons(1212);
	peer.sin_addr.s_addr = inet_addr(getIP());
	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s == INVALID_SOCKET)
	{
		Error();
		perror(bRU ? "Невозможно создать сокет\n" : "Can't create socket. Exiting...\n");
		Reset();
		goto END;
	}
#ifdef DEBUG
	cout << (bRU ? "Сокет создан\n" : "Socket created\n");
#endif
	rc = connect(s, (sockaddr*)&peer, sizeof(peer));
	if (rc != 0)
	{
		Error();
		perror(bRU ? "Невозможно присоединиться\n" : "Can't connect. Exiting...\n");
		Reset();
		goto END;
	}
#ifdef DEBUG
	cout << (bRU ? "Соединение установлено\n" : "Connected\n");

	cout << (bRU ? "Начало игрового клиента\n" : "Begin game client\n");
	Sleep(1000);
#endif

NewGame_cli_xo:
	_recv(s, &sym, 1, 0);

	reEmpty(game_f);
	cls;
	out(game_f, "");
	if(SYMX == sym)
	{
		getPos(game_f, sym, mov);
		_send(s, mov, 2, 0);
	}
	while (true)
	{
		game_state status;
		
		cls;
		out(game_f, "");
		cout << (bRU ? "Жду соперника...\n" : "Waiting for opponent...\n");
		_recv(s, (char*)&status, sizeof(status), 0);
		_recv(s, mov, 2, 0);

		if(mov[0] != 3 && mov[1] != 3)
			game_f[mov[0]][mov[1]] = (SYMX == sym ? SYM0 : SYMX);
		cls;
		out(game_f, "");

		if(cont == status)
		{
			getPos(game_f, sym, mov);
			_send(s, mov, 2, 0);
			continue;
		} // if(cont == status)

		Info();
		if(o_win == status || x_win == status)
		{
			cout << (bRU ? "Вы " : "You ")
				<< ((o_win == status && SYMO == sym) || (x_win == status && SYMX == sym)?(bRU ? "выиграли :)" : "win :)"):(bRU ? "проиграли :(" : "lose :("))
				<< "\n";
		}
		if(draw == status)
		{
			cout << (bRU ? "НИЧЬЯ\n" : "DRAW\n");
		}
		Reset();
		printf(bRU ? "Начать заново? [Y(да, по умолчанию)/N(нет)] " : "Continue? [default:Y/N] ");
		int ch = tolower(getchar());
		if (ch == 'n')
			break;

		goto NewGame_cli_xo;
	}

END:
#ifdef DEBUG
	cout << (bRU ? "Завершаю клиента\n" : "Ending client\n");
#endif
	if(INVALID_SOCKET != s)
	{
		closesocket(s);
		s = INVALID_SOCKET;
	}
}

int _getNewID()
{
	for (int r = 1; r <= (int)ids.size(); ++r)
		if (ids[r - 1] == false)
			return r * 2 - 1;
	return 0;
}

COLOR m_colorText;
COLOR m_colorBkgnd;
#ifdef WIN32
COLOR COLOR_FROM_COLORWIN(WORD winColor)
{
	COLOR color = BLACK;
	switch (winColor)
	{
		case FOREGROUND_RED:                                                               color = DARK_RED;       break;
		case FOREGROUND_GREEN:                                                             color = DARK_GREEN;     break;
		case (FOREGROUND_RED | FOREGROUND_GREEN):                                          color = OLIVE;          break;
		case FOREGROUND_BLUE:                                                              color = DARK_BLUE;      break;
		case (FOREGROUND_RED | FOREGROUND_BLUE):                                           color = DARK_PINK;      break;
		case (FOREGROUND_GREEN | FOREGROUND_BLUE):                                         color = DARK_BLUEGREEN; break;
		case (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE):                        color = LIGHT_GRAY;     break;
		case FOREGROUND_INTENSITY:                                                         color = GRAY;           break;
		case (FOREGROUND_RED | FOREGROUND_INTENSITY):                                      color = RED;            break;
		case (FOREGROUND_GREEN | FOREGROUND_INTENSITY):                                    color = GREEN;          break;
		case (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY):                   color = YELLOW;         break;
		case (FOREGROUND_BLUE | FOREGROUND_INTENSITY):                                     color = BLUE;           break;
		case (FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY):                    color = PINK;           break;
		case (FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY):                  color = BLUEGREEN;      break;
		case (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY): color = WHITE;          break;
		default: break;
	}
	return color;
}
WORD COLORWIN_FROM_COLOR(COLOR color)
{
	WORD winColor = 0;
	switch (color)
	{
		case DARK_RED:       winColor = FOREGROUND_RED;                                                             break;
		case DARK_GREEN:     winColor = FOREGROUND_GREEN;                                                           break;
		case OLIVE:          winColor = FOREGROUND_RED | FOREGROUND_GREEN;                                          break;
		case DARK_BLUE:      winColor = FOREGROUND_BLUE;                                                            break;
		case DARK_PINK:      winColor = FOREGROUND_RED | FOREGROUND_BLUE;                                           break;
		case DARK_BLUEGREEN: winColor = FOREGROUND_GREEN | FOREGROUND_BLUE;                                         break;
		case LIGHT_GRAY:     winColor = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;                        break;
		case GRAY:           winColor = FOREGROUND_INTENSITY;                                                       break;
		case RED:            winColor = FOREGROUND_RED | FOREGROUND_INTENSITY;                                      break;
		case GREEN:          winColor = FOREGROUND_GREEN | FOREGROUND_INTENSITY;                                    break;
		case YELLOW:         winColor = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;                   break;
		case BLUE:           winColor = FOREGROUND_BLUE | FOREGROUND_INTENSITY;                                     break;
		case PINK:           winColor = FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY;                    break;
		case BLUEGREEN:      winColor = FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;                  break;
		case WHITE:          winColor = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY; break;
		case BLACK:
		default:             winColor = 0;                                                                          break;
	}
	return winColor;
}
#else
string COLORLINUX_FROM_COLOR(COLOR color, bool bBackColor)
{
	char col[4];
	int linuxColor = 0;
	switch (color)
	{
		case DARK_RED:       linuxColor = 31; break;
		case DARK_GREEN:     linuxColor = 32; break;
		case OLIVE:          linuxColor = 33; break;
		case DARK_BLUE:      linuxColor = 34; break;
		case DARK_PINK:      linuxColor = 35; break;
		case DARK_BLUEGREEN: linuxColor = 36; break;
		case LIGHT_GRAY:     linuxColor = 37; break;
		case GRAY:           linuxColor = 90; break;
		case RED:            linuxColor = 91; break;
		case GREEN:          linuxColor = 92; break;
		case YELLOW:         linuxColor = 93; break;
		case BLUE:           linuxColor = 94; break;
		case PINK:           linuxColor = 95; break;
		case BLUEGREEN:      linuxColor = 96; break;
		case WHITE:          linuxColor = 97; break;
		case BLACK: default: linuxColor = 30;  break;
	}
	if (bBackColor) linuxColor += 10;
	sprintf(col, "%d", linuxColor);
	return string(col);
}
#endif
void GetColors(COLOR &rColorText, COLOR &rColorBkgnd)
{
#ifdef WIN32
	if (0 != GetStdHandle(STD_OUTPUT_HANDLE))
	{
		CONSOLE_SCREEN_BUFFER_INFO csbi = { 0 };
		BOOL bRes = ::GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
		if (bRes)
		{
			WORD winColorText = (0x0F & csbi.wAttributes);
			WORD winColorBkgnd = (0x0F & (csbi.wAttributes >> 4));

			rColorText = COLOR_FROM_COLORWIN(winColorText);
			rColorBkgnd = COLOR_FROM_COLORWIN(winColorBkgnd);
		}
	}
#else // ifdef WIN32
	rColorText = m_colorText;
	rColorBkgnd = m_colorBkgnd;
#endif // def WIN32
}
//=============================================================================
void SetColors(COLOR colorText, COLOR colorBkgnd)
{
#ifdef WIN32
	if (0 != GetStdHandle(STD_OUTPUT_HANDLE))
	{
		WORD winColorText = COLORWIN_FROM_COLOR(colorText);
		WORD winColorBkgnd = COLORWIN_FROM_COLOR(colorBkgnd);

		WORD wAttr = /*(WORD)*/(0xF0 & (winColorBkgnd << 4));
		wAttr |= /*(WORD)*/(0x0F & winColorText);
		BOOL bRes = ::SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), wAttr);
		int r = 0;
	}
#else // ifdef WIN32
	// Using ANSI escape sequence, where ESC[colT;colBm set colT, colB: "\x1b[37;43mtext".
	string str = "\x1b[";                           // "\x1b["
	str += COLORLINUX_FROM_COLOR(colorText, false); // "\x1b[colT"
	str += ";";                                     // "\x1b[colT;"
	str += COLORLINUX_FROM_COLOR(colorBkgnd, true); // "\x1b[colT;colB"
	str += "m";                                     // "\x1b[colT;colBm"
	cout << str.c_str();
	//
	m_colorText = colorText;
	m_colorBkgnd = colorBkgnd;
#endif // def WIN32
}

char ip[200];
char *getIP()
{
	memset(ip, 0, sizeof(ip));
	string s;
	getline(cin, s);
	printf(bRU ? "Введи адрес сервера: " : "Enter server address to connect to game: ");
	Info();
	getline(cin, s);
	Reset();
	if (!isIP(s.c_str()))
	{
		HOSTENT *h = gethostbyname(s.c_str());
		in_addr a = *(in_addr*)(h->h_addr);
		strcpy(ip, inet_ntoa(a));
	}
	else
	{
		for (int i = 0; i < (int)s.size(); ++i)
		{
			ip[i] = s[i];
		}
	}
	return ip;
}

bool isIP(const char *ip)
{
	//in_addr i;
	//int iRes = inet_pton(AF_INET, ip, &i);
	//dev c++
	char i[100];
	int iRes = inet_pton(AF_INET, ip, i);
	return iRes == 1;
}
