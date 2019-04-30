#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <inttypes.h>
#include <errno.h>
#include <string.h>

#ifndef HAVE_GETSERVBYPORT_R

extern "C" int getservbyport_r(int port, const char *prots,
	struct servent *se, char *buf, size_t buflen, struct servent **res)
{
	int i;
	struct sockaddr_in sin = {
		.sin_family = AF_INET,
		.sin_port = (in_port_t) port,
	};

	if (!prots) {
		int r = getservbyport_r(port, "tcp", se, buf, buflen, res);
		if (r) r = getservbyport_r(port, "udp", se, buf, buflen, res);
		return r;
	}

	/* Align buffer */
	i = (uintptr_t)buf & sizeof(char *)-1;
	if (!i) i = sizeof(char *);
	if (buflen < 3*sizeof(char *)-i)
		return ERANGE;
	buf += sizeof(char *)-i;
	buflen -= sizeof(char *)-i;

	if (strcmp(prots, "tcp") && strcmp(prots, "udp")) return EINVAL;

	se->s_port = port;
	se->s_proto = (char *)prots;
	se->s_aliases = (char **)buf;
	buf += 2*sizeof(char *);
	buflen -= 2*sizeof(char *);
	se->s_aliases[1] = 0;
	se->s_aliases[0] = se->s_name = buf;

	switch (getnameinfo((const struct sockaddr *) &sin, sizeof sin, 0, 0, buf, buflen,
		strcmp(prots, "udp") ? 0 : NI_DGRAM)) {
	case EAI_MEMORY:
	case EAI_SYSTEM:
		return ENOMEM;
	default:
		return ENOENT;
	case 0:
		break;
	}

	*res = se;
	return 0;
}
#endif