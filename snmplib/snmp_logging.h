#ifndef SNMP_LOGGING_H
#define SNMP_LOGGING_H

#ifdef __cplusplus
extern "C" {
#endif

#if HAVE_SYSLOG_H
#include <syslog.h>
#endif
#if HAVE_STDARG_H
#include <stdarg.h>
#else
#include <varargs.h>
#endif

#ifndef LOG_ERR
#define LOG_EMERG       0       /* system is unusable */
#define LOG_ALERT       1       /* action must be taken immediately */
#define LOG_CRIT        2       /* critical conditions */
#define LOG_ERR         3       /* error conditions */
#define LOG_WARNING     4       /* warning conditions */
#define LOG_NOTICE      5       /* normal but significant condition */
#define LOG_INFO        6       /* informational */
#define LOG_DEBUG       7       /* debug-level messages */

#endif

void init_snmp_logging(void);
void snmp_disable_syslog(void);
void snmp_disable_filelog(void);
void snmp_disable_stderrlog(void);
void snmp_disable_log(void);
void snmp_enable_syslog(void);
void snmp_enable_filelog(const char *logfilename, int dont_zero_log);
void snmp_enable_stderrlog(void);

int snmp_log(int priority, const char *format, ...);
int snmp_vlog(int priority, const char *format, va_list ap);
   /*  0 - successful message formatting */
   /* -1 - Could not format log-string */
   /* -2 - Could not allocate memory for log-message */
   /* -3 - Log-message too long! */

void snmp_log_perror(const char *s);

#ifdef __cplusplus
}
#endif

#endif /* SNMP_LOGGING_H */
