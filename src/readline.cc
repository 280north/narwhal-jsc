#include <narwhal.h>

#include <readline/history.h>
#include <readline/readline.h>

FUNCTION(READLINE_readline)
{
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
}
END

FUNCTION(READLINE_addHistory, ARG_UTF8(str))
{
	add_history(str);
}
END

NARWHAL_MODULE(readline)
{
	EXPORTS("readline", JS_fn(READLINE_readline));
    EXPORTS("addHistory", JS_fn(READLINE_addHistory));
}
END_NARWHAL_MODULE
