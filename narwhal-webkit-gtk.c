#include <narwhal.h>

#include <webkit/webkitwebview.h>

int main(int argc, char *argv[], char *envp[])
{
    gtk_init(0, NULL);

    WebKitWebView* view = (WebKitWebView*)webkit_web_view_new();
    WebKitWebFrame* frame = webkit_web_view_get_main_frame(view);
    JSGlobalContextRef _context = webkit_web_frame_get_global_context(frame);

    JSValueRef exception = NULL;
    JSValueRef *_exception = &exception;

    CALL(narwhal, argc, argv, envp, 1);

    int code = !!(*_exception);

    return code;
}
