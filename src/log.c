#include <stdarg.h>
#include <syslog.h>
#include <stdio.h>
#include <time.h>

#include "liblog.h"

static FILE *_logfile = NULL;

static enum log_level max = LIBLOG_ERROR;

static int _flags;
#define FLAG_SET(x) ((_flags & x) == x)

static void _print_time(FILE *stream)
{
	time_t rawtime;
	time(&rawtime);
	struct tm *current_time = localtime (&rawtime);
	char ts[16];
	strftime(ts, 50, "%b %d %H:%M:%S", current_time);
	fprintf(stream, "%s ", ts);
}

__attribute__((__format__ (__printf__, 2, 0)))
static void _stream_log(FILE *stream, const char *format, va_list *pargs, bool copy)
{
	int ret;

	va_list wargs;

	if (FLAG_SET(LIBLOG_FLAG_TIMESTAMP))
		_print_time(stream);

	if (copy) {
		va_copy(wargs, *pargs);
		ret = vfprintf(stream, format, wargs);
	} else {
		ret = vfprintf(stream, format, *pargs);
	}

	if (ret < 0)
		goto cleanup;
	putc('\n', stream);

cleanup:
	if (copy)
		va_end(wargs);
}

__attribute__((__format__ (__printf__, 2, 0)))
static void _logger(enum log_level level, const char *format, va_list *pargs)
{
	if (level > max)
		return;

	int need = FLAG_SET(LIBLOG_FLAG_SYSLOG) + FLAG_SET(LIBLOG_FLAG_CONSOLE) + (FLAG_SET(LIBLOG_FLAG_FILE) && _logfile != NULL);

	if (FLAG_SET(LIBLOG_FLAG_CONSOLE)) {
		_stream_log(stdout, format, pargs, need > 1);
	}

	if (FLAG_SET(LIBLOG_FLAG_FILE) && _logfile != NULL)
	        _stream_log(_logfile, format, pargs, need > 1);

	if (FLAG_SET(LIBLOG_FLAG_SYSLOG)) {
		vsyslog(LOG_NOTICE, format, *pargs);
	}
}

void log_open(const char *name, const char *file, int flags)
{
	_flags = flags;

	if (FLAG_SET(LIBLOG_FLAG_SYSLOG))
		openlog(name, LOG_CONS | LOG_PID | LOG_NDELAY, LOG_DAEMON);

	if (file != NULL) {
		_logfile = fopen(file, "a");
		if (_logfile != NULL)
			setvbuf(_logfile, NULL, _IOLBF, 0);
	}
}

void log_level(enum log_level level)
{
	max = level;
}

void log_reopen(const char *name, const char *file, int flags)
{
	log_close();
	log_open(name, file, flags);
}

void log_close(void)
{
	if (FLAG_SET(LIBLOG_FLAG_SYSLOG))
		closelog();

	if (_logfile != NULL)
		fclose(_logfile);
}

/*__attribute__((__format__ (__printf__, 2, 3)))
void logger(enum log_level level, const char *format, ...)
{
	va_list ap;
	va_start(ap, format);

	_logger(level, format, ap);

	va_end(ap);
}*/

__attribute__ ((format (printf, 1, 2)))
void log_debug(const char *format, ...)
{
	va_list ap;
	va_start(ap, format);

	_logger(LIBLOG_DEBUG, format, &ap);

	va_end(ap);
}

__attribute__ ((format (printf, 1, 2)))
void log_info(const char *format, ...)
{
	va_list ap;
	va_start(ap, format);

	_logger(LIBLOG_INFO, format, &ap);

	va_end(ap);
}

__attribute__ ((format (printf, 1, 2)))
void log_warn(const char *format, ...)
{
	va_list ap;
	va_start(ap, format);

	_logger(LIBLOG_WARN, format, &ap);

	va_end(ap);
}

__attribute__ ((format (printf, 1, 2)))
void log_error(const char *format, ...)
{
	va_list ap;
	va_start(ap, format);

	_logger(LIBLOG_ERROR, format, &ap);

	va_end(ap);
}
