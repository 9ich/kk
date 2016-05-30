/*
 * TCP SSL/TLS for authentication with the account server. This is completely
 * separate from the UDP NET_* functions. This code simply uses the networking
 * functions provided by PolarSSL.
 */
// FIXME: enable certificate verification after sorting out certs
#ifdef _WIN32
#	include <winsock2.h>
#else
#	include <sys/types.h>
#	include <sys/socket.h>
#endif

#include "q_shared.h"
#include "qcommon.h"

//extern const char 	test_ca_crt[];

cvar_t	*net_accountServer;
cvar_t	*net_accountPort;

static char*
errstr(int err)
{
	static char buf[1024];

	buf[0] = '\0';
	error_strerror(err, buf, sizeof buf);
	return buf;
}

static void
debugfn(void *ctx, int level, const char *s)
{
	if(level < 0){
		Com_Printf("%s\n", s);
	}
}

/*
 * Opens a secure socket to srvname, or returns an error message.
 */
const char*
NET_SSLOpen(qssl_t *ssl, const char *srvname, int srvport)
{
	int r;

	// init RNG and session data
	memset(&ssl->ssl, 0, sizeof ssl->ssl);
	memset(&ssl->cacert, 0, sizeof ssl->cacert);
	Com_DPrintf("seeding SSL RNG... ");
	entropy_init(&ssl->entropy);
	r = ctr_drbg_init(&ssl->ctr, entropy_func, &ssl->entropy, nil, 0);
	if(r != 0){
		Com_Printf(S_COLOR_RED "FAIL: %s\n", errstr(r));
		NET_SSLFree(ssl);
		return errstr(r);
	}
	Com_DPrintf("ok\n");

/*
	// init certificates
	Com_DPrintf("loading CA root certificate... ");
	r = x509parse_crt(&a->cacert, (uchar*)test_ca_crt, strlen(test_ca_crt));
	if(r < 0){
		Com_Printf(S_COLOR_RED "FAIL: %s\n", errstr(r));
		goto Out;
	}
	Com_DPrintf("ok (%d skipped)\n", r);
*/

	Com_DPrintf("connecting to %s:%4d... ", srvname, srvport);
	r = net_connect(&ssl->serverfd, srvname, srvport);
	if(r != 0){
		Com_Printf(S_COLOR_RED "FAIL: %s\n", errstr(r));
		NET_SSLFree(ssl);
		return errstr(r);
	}
	Com_DPrintf("ok\n");

	// setup
	Com_DPrintf("setting up SSL/TLS structure... ");
	r = ssl_init(&ssl->ssl);
	if(r != 0){
		Com_Printf(S_COLOR_RED "FAIL: %s\n", errstr(r));
		NET_SSLFree(ssl);
		return errstr(r);
	}
	Com_DPrintf("ok\n");

	ssl_set_endpoint(&ssl->ssl, SSL_IS_CLIENT);
	ssl_set_authmode(&ssl->ssl, SSL_VERIFY_OPTIONAL);
	ssl_set_ca_chain(&ssl->ssl, &ssl->cacert, nil, "Server 1");
	ssl_set_rng(&ssl->ssl, ctr_drbg_random, &ssl->ctr);
	ssl_set_dbg(&ssl->ssl, debugfn, stdout);
	ssl_set_bio(&ssl->ssl, net_recv, &ssl->serverfd, net_send, &ssl->serverfd);

	#define msec 1000
	ssl->timeout.tv_sec = msec/1000;
	ssl->timeout.tv_usec = (msec%1000)*1000;
	setsockopt(ssl->serverfd, SOL_SOCKET, SO_RCVTIMEO, (void*)&ssl->timeout, sizeof ssl->timeout);
	setsockopt(ssl->serverfd, SOL_SOCKET, SO_SNDTIMEO, (void*)&ssl->timeout, sizeof ssl->timeout);

	// handshake
	Com_DPrintf("doing SSL/TLS handshake... ");
	while((r = ssl_handshake(&ssl->ssl)) != 0){
		if(r != POLARSSL_ERR_NET_WANT_READ &&
		   r != POLARSSL_ERR_NET_WANT_WRITE){
			Com_Printf(S_COLOR_RED "FAIL: %s\n", errstr(r));
			NET_SSLFree(ssl);
			return errstr(r);
		}
	}
	Com_DPrintf("ok\n");

/*
	// verify server certs
	Com_DPrintf("verifying peer X.509 cert... ");
	r = ssl_get_verify_result(&a->ssl);
	if(r != 0){
		Com_Printf(S_COLOR_RED "FAIL: ");
		if(r & BADCERT_EXPIRED)
			Com_Printf(S_COLOR_RED "server cert has expired, ");
		if(r & BADCERT_REVOKED)
			Com_Printf(S_COLOR_RED "server cert was revoked, ");
		if(r & BADCERT_CN_MISMATCH)
			Com_Printf(S_COLOR_RED "server CN mismatch, expected %s, \n",
			   "PolarSSL Server 1");
		if(r & BADCERT_NOT_TRUSTED)
			Com_Printf(S_COLOR_RED "self-signed or not signed by trusted CA\n");
		Com_Printf("\n");
		goto Out;
	}
	Com_DPrintf("ok\n");
*/
	return nil;
}

void
NET_SSLClose(qssl_t *ssl)
{
	ssl_close_notify(&ssl->ssl);
}

void
NET_SSLFree(qssl_t *ssl)
{
	x509_free(&ssl->cacert);
	net_close(ssl->serverfd);
	ssl_free(&ssl->ssl);
	memset(&ssl->ssl, 0, sizeof ssl->ssl);
}

const char*
NET_SSLWrite(qssl_t *ssl, uchar *buf, int len)
{
	int r;

	Com_DPrintf("writing to auth server...");
	while((r = ssl_write(&ssl->ssl, buf, len)) <= 0){
		if(r != POLARSSL_ERR_NET_WANT_READ &&
		   r != POLARSSL_ERR_NET_WANT_WRITE){
			Com_Printf(S_COLOR_RED "FAIL: %s\n", errstr(r));
			return errstr(r);
		}
	}
	Com_DPrintf("ok (%d bytes written)\n", r);
	return nil;
}

const char*
NET_SSLRead(qssl_t *ssl, uchar *buf, int len, int *nread)
{
	int r;
	uchar scratch[1024];

	*nread = 0;
	memset(buf, 0, len);
	Com_DPrintf("reading from auth server...");
	do{
		memset(scratch, 0, sizeof scratch);
		r = ssl_read(&ssl->ssl, scratch, (sizeof scratch)-1);

		if(r == POLARSSL_ERR_NET_WANT_READ ||
		   r == POLARSSL_ERR_NET_WANT_WRITE)
			goto Continue;

		if(r == POLARSSL_ERR_SSL_PEER_CLOSE_NOTIFY)
			break;
	
		if(r < 0){
			Com_Printf(S_COLOR_RED "FAIL: %s\n", errstr(r));
			break;
		}

		if(r == 0){
			Com_DPrintf("<EOF>\n");
			break;
		}
	Continue:
		Com_DPrintf(S_COLOR_CYAN "    got '%s' buf=%s nread=%d r=%d len=%d\n", scratch, buf, *nread, r, len);
		if(*nread + r > len){
			return "buffer too small";
		}
		memcpy(buf + *nread, scratch, r);
		*nread += r;
	}while(1);
	Com_DPrintf("buf=%s\n", buf);
	return nil;
}
