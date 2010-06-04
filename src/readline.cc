#include <narwhal.h>

#ifdef USE_READLINE
#include <readline/history.h>
#include <readline/readline.h>
#endif

#ifdef USE_EDITLINE
#include <editline/readline.h>
#endif

FUNCTION(READLINE_readline)
{
#if defined(USE_READLINE) || defined(USE_EDITLINE)
	char *str;
    if (ARGC == 0) {
		str = readline(NULL);
	}
	else {
        ARGN_UTF8(prompt, 0);
		str = readline(prompt);
    }

    if (str && *str)
        add_history(str);

    if (!str)
        return JS_null;

    return JS_str_utf8(str, strlen(str));
#else
    THROW("Not compiled with readline or editline support.")
#endif
}
END

FUNCTION(READLINE_addHistory, ARG_UTF8(str))
{
#if defined(USE_READLINE) || defined(USE_EDITLINE)
	add_history(str);
	return JS_null;
#else
    THROW("Not compiled with readline or editline support.")
#endif
}
END

NARWHAL_MODULE(readline)
{
	EXPORTS("readline", JS_fn(READLINE_readline));
    EXPORTS("addHistory", JS_fn(READLINE_addHistory));
}
END_NARWHAL_MODULE
