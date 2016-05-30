/*
 * Authentication with the account server, which tracks stats, elo, etc.
 * 
 * Each operation blocks until response or timeout.
 * There are 4 operations offered by the account server:
 * 	auth <username> <password>
 * 		Authenticate and receive a token for stat tracking.
 * 	register <username> <password>
 * 		Register an account.
 * 	delete <username> <password>
 * 		Delete an account.
 * 	putstats\n<token>\n<infostring>[...]\n
 * 		Send stats.
 */
#include "server.h"

typedef struct
{
	char	token[STATTOKEN_LEN];	// stat-tracking token sent by client
	int	stat[QSTAT_MAX];
} stat_t;

static stat_t stattab[4*MAX_CLIENTS];	// allow some room for disconnected and newly connected clients

void
SV_StatInit(void)
{
	memset(stattab, 0, sizeof stattab);
}

void
SV_StatAdd(int clientnum, int stat, int incr)
{
	int i;
	stat_t *p;
	client_t *cl;

	Com_DPrintf("SV_StatAdd(%d, %d, %d)\n", clientnum, stat, incr);

	if(clientnum < 0 || clientnum >= sv_maxclients->integer){
		Com_Printf("SV_StatAdd: invalid clientnum %d\n", clientnum);
		return;
	}
	cl = &svs.clients[clientnum];

	if(stat < 0 || stat >= QSTAT_MAX){
		Com_Printf("SV_StatAdd: invalid stat index %d\n", stat);
		return;
	}

	if(cl->stattoken[0] == '\0')
		return;	// client has not authenticated

	p = nil;
	for(i = 0; i < ARRAY_LEN(stattab); i++){
		if(stattab[i].token[0] == '\0')
			break;
		if(strcmp(cl->stattoken, stattab[i].token) == 0){
			p = &stattab[i];
			break;
		}
	}
	if(i == ARRAY_LEN(stattab))
		return;	// no room left to track another player
	if(p == nil){
		p = &stattab[i];
		Q_strncpyz(p->token, cl->stattoken, sizeof p->token);
	}

	p->stat[stat] += incr;
}

static void
putstats(const char *stats)
{
	qssl_t ssl;
	uchar buf[1024];
	const char *err;
	char *srvname;
	int srvport;
	int len, nread;

	srvname = net_accountServer->string;
	if(srvname == nil || srvname[0] == '\0')
		srvname = ACCOUNT_SERVER_NAME;

	srvport = net_accountPort->integer;
	if(srvport == 0)
		srvport = PORT_ACCOUNT;

	err = NET_SSLOpen(&ssl, srvname, srvport);
	if(err != nil){
		Com_Printf("putstats: %s\n", err);
		return;
	}

	*buf = 0;
	len = Com_sprintf((char*)buf, sizeof buf, "putstats\n%s\n", stats);
	err = NET_SSLWrite(&ssl, buf, len);
	if(err != nil){
		Com_Printf("putstats: %s\n", err);
		NET_SSLClose(&ssl);
		NET_SSLFree(&ssl);
		return;
	}

	nread = 0;
	err = NET_SSLRead(&ssl, buf, sizeof buf, &nread);
	if(err != nil){
		Com_Printf("putstats: %s\n", err);
		NET_SSLClose(&ssl);
		NET_SSLFree(&ssl);
		return;
	}

	Com_Printf("putstats ok\n");
	NET_SSLClose(&ssl);
	NET_SSLFree(&ssl);
	return;
}

/*
 * putstats\n123ABCDE\n1\\3\\2\\5\\10\\2\nABC123EE\\0\\3\n...
 */
void
SV_Putstats_f(void)
{
	int i, j;
	char buf[2048];
	char info[MAX_INFO_STRING];
	stat_t *p;

	buf[0] = '\0';
	for(i = 0; i < ARRAY_LEN(stattab); i++){
		if(stattab[i].token[0] == '\0')
			break;
	}
	if(i == 0)
		return;

	for(i = 0; i < ARRAY_LEN(stattab); i++){
		if(stattab[i].token[0] == '\0')
			break;

		p = &stattab[i];

		Q_strcat(buf, sizeof buf, va("%s\n", p->token));
		
		info[0] = '\0';
		for(j = 0; j < QSTAT_MAX; j++){
			if(p->stat[j] == 0)
				continue;
			Info_SetValueForKey(info, va("%i", j), va("%i", p->stat[j]));
		}
		Q_strcat(info, sizeof info, "\n");
		Q_strcat(buf, sizeof buf, info);
	}

	if(buf[0] != '\0')
		putstats(buf);
	SV_StatInit();
}

static char *stressstatsdata[] = {
	"",
	"putstats",
	"\\",
	"\\\\",
	"\\\\\\",
	"\\\\\\\\",
	"a\\b",
	"aa\\bb",
	"\\a\\b",
	"\\a",
	"\n\\aaa\\bbb",
	"\\0\\1\\2\\3\\4\\5",
	"\\a\\b",
	"\\;\\!",
	";",
	"\\\\;",
	"a\\b\\c",
	"aaa\\bbb\\ccc",
	"\\aaa\\bbb\\ccc"
};

void
SV_Stressstats_f(void)
{
	int i;

	for(i = 0; i < ARRAY_LEN(stressstatsdata); i++){
		putstats(stressstatsdata[i]);
		putstats(va("%s\n", stressstatsdata[i]));
		putstats(va("abcdef12\n%s", stressstatsdata[i]));
		putstats(va("abcdef12 %s", stressstatsdata[i]));
		putstats(va("abcdef12\n%s\n", stressstatsdata[i]));
		putstats(va("abcdef12 %s\n", stressstatsdata[i]));
	}
}
