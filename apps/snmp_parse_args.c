/*
 * snmp_parse_args.c
 */

#include <config.h>

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#include <sys/types.h>
#include <stdio.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <ctype.h>
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#if TIME_WITH_SYS_TIME
# ifdef WIN32
#  include <sys/timeb.h>
# else
#  include <sys/time.h>
# endif
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif
#if HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#if HAVE_WINSOCK_H
#include <winsock.h>
#endif
#if HAVE_NETDB_H
#include <netdb.h>
#endif
#if HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#include "asn1.h"
#include "snmp_api.h"
#include "snmp_impl.h"
#include "snmp_client.h"
#include "mib.h"
#include "snmp.h"
#include "scapi.h"
#include "keytools.h"

#include "snmp_parse_args.h"
#include "version.h"
#include "system.h"
#include "parse.h"
#include "read_config.h"
#include "snmp_debug.h"

int random_access = 0;

void usage(void);

#define USM_AUTH_PROTO_MD5_LEN 10
static oid usmHMACMD5AuthProtocol[]  = { 1,3,6,1,6,3,10,1,1,2 };
#define USM_AUTH_PROTO_SHA_LEN 10
static oid usmHMACSHA1AuthProtocol[] = { 1,3,6,1,6,3,10,1,1,3 };
#define USM_PRIV_PROTO_DES_LEN 10
static oid usmDESPrivProtocol[]      = { 1,3,6,1,6,3,10,1,2,2 };

void
snmp_parse_args_usage(FILE *outf)
{
  fprintf(outf, "[-v 1|2c|2p|3] [-h] [-H] [-d] [-q] [-R] [-D] [-m <MIBS>] [-M <MIDDIRS>] [-p <P>] [-t <T>] [-r <R>] ");
  fprintf(outf, "[-T <B> <T>] [-e <E>] [-E <E>] [-n <N>] [-u <U>] [-l <L>] [-a <A>] [-A <P>] [-x <X>] [-X <P>] <hostname> {<community>|{<srcParty> <dstParty> <context>|}");
}

void
snmp_parse_args_descriptions(FILE *outf)
{
  fprintf(outf, "  -v 1|2c|2p|3\tspecifies snmp version to use.\n");
  fprintf(outf, "  -h\t\tthis help message.\n");
  fprintf(outf, "  -H\t\tDisplay configuration file directives understood.\n");
  fprintf(outf, "  -V\t\tdisplay version number.\n");
  fprintf(outf, "  -d\t\tdump input/output packets.\n");
  fprintf(outf, "  -q\t\tquick print output for easier parsing ability.\n");
  fprintf(outf, "  -f\t\tprint full object identifiers on output.\n");
  fprintf(outf, "  -s\t\tprint only last element of object identifiers.\n");
  fprintf(outf, "  -S\t\tmodule id plus last element of object identifiers.\n");
  fprintf(outf, "  -R\t\tuse \"random access\" to the mib tree.\n");
  fprintf(outf, "  -D[TOKEN,...]\t\tturn on debugging output, optionally by the list of TOKENs.\n");
  fprintf(outf, "  -m <MIBS>\tuse MIBS list instead of the default mib list.\n");
  fprintf(outf, "  -M <MIBDIRS>\tuse MIBDIRS as the location to look for mibs.\n");
  fprintf(outf, "  -p <P>\tuse port P instead of the default port.\n");
  fprintf(outf, "  -t <T>\tset the request timeout to T.\n");
  fprintf(outf, "  -r <R>\tset the number of retries to R.\n");
  fprintf(outf,
          "  -T <B> <T>\tset the destination engine boots/time for v3 requests.\n");
  fprintf(outf, "  -e <E>\tsecurity engine ID (e.g., 800000020109840301).\n");
  fprintf(outf, "  -E <E>\tcontext engine ID (e.g., 800000020109840301).\n");
  fprintf(outf, "  -n <N>\tcontext name (e.g., bridge1).\n");
  fprintf(outf, "  -u <U>\tsecurity name (e.g., bert).\n");
  fprintf(outf, "  -l <L>\tsecurity level (noAuthNoPriv|authNoPriv|authPriv).\n");
  fprintf(outf, "  -a <A>\tauthentication protocol (MD5|SHA)\n");
  fprintf(outf, "  -A <P>\tauthentication protocol pass phrase.\n");
  fprintf(outf, "  -x <X>\tprivacy protocol (DES).\n");
  fprintf(outf, "  -X <P>\tprivacy protocol pass phrase\n");
  fprintf(outf, "  -P <MIBOPTS>\tToggle various defaults controlling mib parsing:\n");
  snmp_mib_toggle_options_usage("\t\t  ", outf);
}
#define BUF_SIZE 512
int
snmp_parse_args(int argc, 
		char *argv[], 
		struct snmp_session *session)
{
  int arg;
  char *psz, *cp;
  char *Apsz = NULL;
  char *Xpsz = NULL;
  u_char buf[BUF_SIZE];
  size_t bsize;

  /* initialize session to default values */
  snmp_sess_init( session );

  /* get the options */
  for(arg = 1; (arg < argc) && (argv[arg][0] == '-'); arg++){
    switch(argv[arg][1]){
      case 'd':
        snmp_set_dump_packet(1);
        break;

      case 'R':
        random_access = 1;
        break;

      case 'q':
        snmp_set_quick_print(1);
        break;

      case 'D':
        debug_register_tokens(&argv[arg][2]);
        snmp_set_do_debugging(!snmp_get_do_debugging());
        break;

      case 'm':
        if (argv[arg][2] != 0)
          setenv("MIBS",&argv[arg][2], 1);
        else if (++arg < argc)
          setenv("MIBS",argv[arg], 1);
        else {
          fprintf(stderr,"Need MIBS after -m flag.\n");
          usage();
          exit(1);
        }
        break;

      case 'M':
        if (argv[arg][2] != 0)
          setenv("MIBDIRS",&argv[arg][2], 1);
        else if (++arg < argc)
          setenv("MIBDIRS",argv[arg], 1);
        else {
          fprintf(stderr,"Need MIBDIRS after -M flag.\n");
          usage();
          exit(1);
        }
        break;

      case 'f':
	snmp_set_full_objid(1);
	break;

      case 's':
	snmp_set_suffix_only(1);
	break;

      case 'S':
	snmp_set_suffix_only(2);
	break;

      case 'p':
        if (isdigit(argv[arg][2]))
          session->remote_port = atoi(&(argv[arg][2]));
        else if ((++arg<argc) && isdigit(argv[arg][0]))
          session->remote_port = atoi(argv[arg]);
        else {
          fprintf(stderr,"Need port number after -p flag.\n");
          usage();
          exit(1);
        }
        break;

      case 't':
        if (isdigit(argv[arg][2]))
          session->timeout = atoi(&(argv[arg][2])) * 1000000L;
        else if ((++arg<argc) && isdigit(argv[arg][0]))
          session->timeout = atoi(argv[arg]) * 1000000L;
        else {
          fprintf(stderr,"Need time in seconds after -t flag.\n");
          usage();
          exit(1);
        }
        break;

      case 'r':
        if (isdigit(argv[arg][2]))
          session->retries = atoi(&(argv[arg][2]));
        else if ((++arg<argc) && isdigit(argv[arg][0]))
          session->retries = atoi(argv[arg]);
        else {
          fprintf(stderr,"Need number of retries after -r flag.\n");
          usage();
          exit(1);
        }
        break;

      case 'T':
        if (isdigit(argv[arg][2]))
          session->engineBoots = (u_long)(atol(&(argv[arg][2])));
        else if ((++arg<argc) && isdigit(argv[arg][0]))
          session->engineBoots = (u_long)(atol(argv[arg]));
        else {
          fprintf(stderr,"Need engine boots value after -T flag.\n");
          usage();
          exit(1);
        }
        if ((++arg<argc) && isdigit(argv[arg][0]))
          session->engineTime = (u_long)(atol(argv[arg]));
        else {
          fprintf(stderr,"Need engine time value after -T flag.\n");
          usage();
          exit(1);
        }
        break;

      case 'V':
        fprintf(stderr,"UCD-snmp version: %s\n", VersionInfo);
        exit(0);

      case 'v':
        if (argv[arg][2] != 0)
          psz = &(argv[arg][2]);
        else
          psz = argv[++arg];
        if( psz == NULL) {
          fprintf(stderr,"Need version value after -v flag. \n");
          usage();
          exit(1);
        }
        if (!strcmp(psz,"1")) {
          session->version = SNMP_VERSION_1;
        } else if (!strcasecmp(psz,"2c")) {
          session->version = SNMP_VERSION_2c;
        } else if (!strcasecmp(psz,"2p")) {
          session->version = SNMP_VERSION_2p;
        } else if (!strcasecmp(psz,"3")) {
          session->version = SNMP_VERSION_3;
        } else {
          fprintf(stderr,"Invalid version specified after -v flag: %s\n", psz);
          usage();
          exit(1);
        }
        break;

      case 'e':
        if (argv[arg][2] != 0)
          psz = &(argv[arg][2]);
        else
          psz = argv[++arg];
        if( psz == NULL) {
          fprintf(stderr,"Need engine ID value after -e flag. \n");
          usage();
          exit(1);
        }
	if ((bsize = hex_to_binary(psz,buf)) <= 0) {
          fprintf(stderr,"Need engine ID value after -e flag. \n");
          usage();
          exit(1);
	}
	session->securityEngineID = (u_char *)malloc(bsize);
	memcpy(session->securityEngineID, buf, bsize);
	session->securityEngineIDLen = bsize;
        break;

      case 'E':
        if (argv[arg][2] != 0)
          psz = &(argv[arg][2]);
        else
          psz = argv[++arg];
        if( psz == NULL) {
          fprintf(stderr,"Need engine ID value after -E flag. \n");
          usage();
          exit(1);
        }
	if ((bsize = hex_to_binary(psz,buf)) <= 0) {
          fprintf(stderr,"Need engine ID value after -E flag. \n");
          usage();
          exit(1);
	}
	session->contextEngineID = (u_char *)malloc(bsize);
	memcpy(session->contextEngineID, buf, bsize);
	session->contextEngineIDLen = bsize;
        break;

      case 'n':
        if (argv[arg][2] != 0)
          psz = &(argv[arg][2]);
        else
          psz = argv[++arg];
        if( psz == NULL) {
          fprintf(stderr,"Need context name value after -n flag. \n");
          usage();
          exit(1);
        }
	session->contextName = strdup(psz);
	session->contextNameLen = strlen(psz);
        break;

      case 'u':
        if (argv[arg][2] != 0)
          psz = &(argv[arg][2]);
        else
          psz = argv[++arg];
        if( psz == NULL) {
          fprintf(stderr,"Need security user name value after -u flag. \n");
          usage();
          exit(1);
        }
	session->securityName = strdup(psz);
	session->securityNameLen = strlen(psz);
        break;

      case 'l':
        if (argv[arg][2] != 0)
          psz = &(argv[arg][2]);
        else
          psz = argv[++arg];
        if( psz == NULL) {
          fprintf(stderr,"Need security level value after -l flag. \n");
          usage();
          exit(1);
        }
        if (!strcmp(psz,"noAuthNoPriv") || !strcmp(psz,"1") ||
            !strcmp(psz,"nanp")) {
          session->securityLevel = SNMP_SEC_LEVEL_NOAUTH;
        } else if (!strcmp(psz,"authNoPriv") || !strcmp(psz,"2") ||
            !strcmp(psz,"anp")) {
          session->securityLevel = SNMP_SEC_LEVEL_AUTHNOPRIV;
        } else if (!strcmp(psz,"authPriv") || !strcmp(psz,"3") ||
            !strcmp(psz,"ap")) {
          session->securityLevel = SNMP_SEC_LEVEL_AUTHPRIV;
        } else {
          fprintf(stderr,"Invalid security level specified after -l flag: %s\n", psz);
          usage();
          exit(1);
        }

        break;

      case 'a':
        if (argv[arg][2] != 0)
          psz = &(argv[arg][2]);
        else
          psz = argv[++arg];
        if( psz == NULL) {
          fprintf(stderr,"Need authentication protocol value after -a flag. \n");
          usage();
          exit(1);
        }
        if (!strcmp(psz,"MD5")) {
          session->securityAuthProto = usmHMACMD5AuthProtocol;
          session->securityAuthProtoLen = USM_AUTH_PROTO_MD5_LEN;
        } else if (!strcmp(psz,"SHA")) {
          session->securityAuthProto = usmHMACSHA1AuthProtocol;
          session->securityAuthProtoLen = USM_AUTH_PROTO_SHA_LEN;
        } else {
          fprintf(stderr,"Invalid authentication protocol specified after -a flag: %s\n", psz);
          usage();
          exit(1);
        }
        break;

      case 'x':
        if (argv[arg][2] != 0)
          psz = &(argv[arg][2]);
        else
          psz = argv[++arg];
        if( psz == NULL) {
          fprintf(stderr,"Need privacy protocol value after -x flag. \n");
          usage();
          exit(1);
        }
        if (!strcmp(psz,"DES")) {
          session->securityPrivProto = usmDESPrivProtocol;
          session->securityPrivProtoLen = USM_PRIV_PROTO_DES_LEN;
        } else {
          fprintf(stderr,"Invalid privacy protocol specified after -x flag: %s\n", psz);
          usage();
          exit(1);
        }
        break;

      case 'A':
        if (argv[arg][2] != 0)
          Apsz = &(argv[arg][2]);
        else
          Apsz = argv[++arg];
        if( Apsz == NULL) {
          fprintf(stderr,"Need authentication pass phrase value after -A flag. \n");
          usage();
          exit(1);
        }
        break;

      case 'X':
        if (argv[arg][2] != 0)
          Xpsz = &(argv[arg][2]);
        else
          Xpsz = argv[++arg];
        if( Xpsz == NULL) {
          fprintf(stderr,"Need privacy pass phrase value after -X flag. \n");
          usage();
          exit(1);
        }

        break;

      case 'P':
        if (argv[arg][2] != 0)
          cp = &argv[arg][2];
        else if (++arg<argc)
          cp = &argv[arg][2];
        else {
          fprintf(stderr,"Need option arguments after -P flag.\n");
          usage();
          exit(1);
        }
        cp = snmp_mib_toggle_options(cp);
        if (cp != NULL) {
          fprintf(stderr,"Unknown parsing option passed to -P: %c.\n", *cp);
          usage();
          exit(1);
        }
        break;

      case 'h':
        usage();
        exit(0);
        break;

      case 'H':
        init_snmp("snmpapp");
        fprintf(stderr, "Configuration directives understood:\n");
        read_config_print_usage("  ");
        exit(0);

      default:
        /* This should be removed to support options in clients that
           have more parameters than the defaults above! */
        fprintf(stderr, "invalid option: -%c\n", argv[arg][1]);
        usage();
        exit(1);
        break;
    }
  }

  /* read in MIB database and initialize the snmp library*/
  init_snmp("snmpapp");

  /* make master key from pass phrases */
  if (Apsz) {
      session->securityAuthKeyLen = USM_AUTH_KU_LEN;
      if (generate_Ku(session->securityAuthProto,
                      session->securityAuthProtoLen,
                      (u_char *)Apsz, strlen(Apsz),
                      session->securityAuthKey,
                      &session->securityAuthKeyLen) != SNMPERR_SUCCESS) {
          fprintf(stderr,"Error generating Ku from authentication pass phrase. \n");
          usage();
          exit(1);
      }
  }
  if (Xpsz) {
      session->securityPrivKeyLen = USM_PRIV_KU_LEN;
      if (generate_Ku(session->securityAuthProto,
                      session->securityAuthProtoLen,
                      (u_char *)Xpsz, strlen(Xpsz),
                      session->securityPrivKey,
                      &session->securityPrivKeyLen) != SNMPERR_SUCCESS) {
          fprintf(stderr,"Error generating Ku from privacy pass phrase. \n");
          usage();
          exit(1);
      }
  }
  /* get the hostname */
  if (arg == argc) {
    fprintf(stderr,"No hostname specified.\n");
    usage();
    exit(1);
  }
  session->peername = argv[arg++];     /* hostname */

  /* get community or party */
  if ((session->version == SNMP_VERSION_1) ||
      (session->version == SNMP_VERSION_2c)) {
    /* v1 and v2c - so get community string */
    if (arg == argc) {
      fprintf(stderr,"No community name specified.\n");
      usage();
      exit(1);
    }
    session->community = (unsigned char *)argv[arg];
    session->community_len = strlen((char *)argv[arg]);
    arg++;
  }
  return arg;
}

oid
*snmp_parse_oid(char *argv,
		oid *root,
		size_t *rootlen)
{
  if (random_access) {
    if (get_node(argv,root,rootlen)) {
      return root;
    }
  } else {
    if (read_objid(argv,root,rootlen)) {
      return root;
    }
  }
  return NULL;
}

