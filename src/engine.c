#include "engine.h"

typedef struct _IBusBugtestEngine IBusBugtestEngine;
typedef struct _IBusBugtestEngineClass IBusBugtestEngineClass;

#define BUFFER_LENGTH 10
struct _IBusBugtestEngine {
	IBusEngine parent;

    /* members */
    gchar *context;
    gboolean is_enabled;
};

struct _IBusBugtestEngineClass {
	IBusEngineClass parent;
};

/* functions prototype */
static void	ibus_bugtest_engine_class_init	(IBusBugtestEngineClass	*klass);
static void	ibus_bugtest_engine_init		(IBusBugtestEngine		*engine);
static void	ibus_bugtest_engine_destroy		(IBusBugtestEngine		*engine);
static gboolean
			ibus_bugtest_engine_process_key_event
                                            (IBusEngine             *engine,
                                             guint               	 keyval,
                                             guint               	 keycode,
                                             guint               	 modifiers);
static void ibus_bugtest_engine_focus_in    (IBusEngine             *engine);
static void ibus_bugtest_engine_focus_out   (IBusEngine             *engine);
static void ibus_bugtest_engine_reset       (IBusEngine             *engine);
static void ibus_bugtest_engine_enable      (IBusEngine             *engine);
static void ibus_bugtest_engine_disable     (IBusEngine             *engine);
static void ibus_engine_set_cursor_location (IBusEngine             *engine,
                                             gint                    x,
                                             gint                    y,
                                             gint                    w,
                                             gint                    h);
static void ibus_bugtest_engine_set_capabilities
                                            (IBusEngine             *engine,
                                             guint                   caps);
static void ibus_bugtest_engine_page_up     (IBusEngine             *engine);
static void ibus_bugtest_engine_page_down   (IBusEngine             *engine);
static void ibus_bugtest_engine_cursor_up   (IBusEngine             *engine);
static void ibus_bugtest_engine_cursor_down (IBusEngine             *engine);
static void ibus_bugtest_property_activate  (IBusEngine             *engine,
                                             const gchar            *prop_name,
                                             gint                    prop_state);
static void ibus_bugtest_engine_property_show
											(IBusEngine             *engine,
                                             const gchar            *prop_name);
static void ibus_bugtest_engine_property_hide
											(IBusEngine             *engine,
                                             const gchar            *prop_name);

static void ibus_bugtest_engine_commit_string
                                            (IBusBugtestEngine      *bugtest,
                                             const gchar            *string);
static void ibus_bugtest_engine_set_surrounding_text
                                            (IBusEngine *engine,
                                             IBusText *text,
                                             guint cursor_pos,
                                             guint anchor_pos);

static IBusEngineClass *parent_class = NULL;

G_DEFINE_TYPE (IBusBugtestEngine, ibus_bugtest_engine, IBUS_TYPE_ENGINE)

static void
ibus_bugtest_engine_class_init (IBusBugtestEngineClass *klass)
{
	IBusObjectClass *ibus_object_class = IBUS_OBJECT_CLASS (klass);
	IBusEngineClass *engine_class = IBUS_ENGINE_CLASS (klass);
    parent_class = (IBusEngineClass *)g_type_class_peek_parent(klass);

    ibus_object_class->destroy = (IBusObjectDestroyFunc) ibus_bugtest_engine_destroy;

    engine_class->process_key_event = ibus_bugtest_engine_process_key_event;

    engine_class->enable = ibus_bugtest_engine_enable;
    engine_class->disable = ibus_bugtest_engine_disable;

    engine_class->set_surrounding_text = ibus_bugtest_engine_set_surrounding_text;
    engine_class->focus_in = ibus_bugtest_engine_focus_in;
    engine_class->focus_out = ibus_bugtest_engine_focus_out;
}

static void
ibus_bugtest_engine_init (IBusBugtestEngine *bugtest)
{
    bugtest->context = NULL;
    bugtest->is_enabled = FALSE;
}

static void
ibus_bugtest_engine_destroy (IBusBugtestEngine *bugtest)
{
    g_free(bugtest->context);
    ((IBusObjectClass *)ibus_bugtest_engine_parent_class)->destroy((IBusObject *)bugtest);
}

static void
ibus_bugtest_engine_enable(IBusEngine *engine)
{
    IBusBugtestEngine *bugtest = (IBusBugtestEngine *)engine;
    parent_class->enable(engine);
    bugtest->is_enabled = TRUE;
    g_message("%s: Enabling surrounding text", __FUNCTION__);
    ibus_engine_get_surrounding_text(engine, NULL, NULL, NULL);
}

static void
ibus_bugtest_engine_disable(IBusEngine *engine)
{
    IBusBugtestEngine *bugtest = (IBusBugtestEngine *)engine;
    bugtest->is_enabled = FALSE;
    parent_class->disable(engine);
}

static gboolean
client_supports_surrounding_text(IBusEngine *engine)
{
    return (engine->client_capabilities & IBUS_CAP_SURROUNDING_TEXT) != 0;
}

static void
reset_context(IBusEngine *engine)
{
    IBusBugtestEngine *bugtest = (IBusBugtestEngine *)engine;

    g_message("%s: client_supports_surrounding_text=%d", __FUNCTION__, client_supports_surrounding_text(engine));

    if (bugtest->context) {
        g_free(bugtest->context);
        bugtest->context = NULL;
    }
    if (!bugtest->is_enabled)
        return;

    if (client_supports_surrounding_text(engine)) {
        IBusText *text;
        guint cursor_pos, anchor_pos;
        ibus_engine_get_surrounding_text(engine, &text, &cursor_pos, &anchor_pos);
        const gchar *buffer = ibus_text_get_text(text);
        g_message("%s: text=%s, cursor_pos=%d, anchor_pos=%d", __FUNCTION__, text->text, cursor_pos, anchor_pos);
        bugtest->context = g_utf8_substring(buffer, 0, cursor_pos);
    }
}

static void
ibus_bugtest_engine_commit_string(IBusBugtestEngine *bugtest,
                                    const gchar *string)
{
    g_message("%s: %s", __FUNCTION__, string);
    IBusText *text;
    text = ibus_text_new_from_static_string (string);
    g_object_ref_sink(text);
    ibus_engine_commit_text ((IBusEngine *)bugtest, text);
    g_object_unref(text);
}

static gboolean
ibus_bugtest_engine_process_key_event (IBusEngine *engine,
                                       guint       keyval,
                                       guint       keycode,
                                       guint       modifiers)
{
    IBusText *text;
    IBusBugtestEngine *bugtest = (IBusBugtestEngine *)engine;

    if (modifiers & IBUS_RELEASE_MASK)
        return FALSE;

    g_message("%s: Processing key event: keyval=%0x, keycode=%0x, modifiers=%0x", __FUNCTION__, keyval, keycode, modifiers);

    if (bugtest->context == NULL) {
        reset_context(engine);
    }

    if (keyval == IBUS_x ) {
        // delete previous character and replace it with uppercase version
        if (client_supports_surrounding_text(engine)) {
            g_message("%s: deleting surrounding text", __FUNCTION__);
            ibus_engine_delete_surrounding_text(engine, -1, 1);
        } else {
            g_message("%s: sending backspace", __FUNCTION__);
            ibus_engine_forward_key_event(engine, IBUS_KEY_BackSpace, 14, 0);
        }

        glong len = g_utf8_strlen(bugtest->context, -1);
        if (len > 0) {
            gchar c = bugtest->context[len - 1];
            gchar buffer[2];
            buffer[0] = c;
            buffer[1] = 0;
            ibus_bugtest_engine_commit_string(bugtest, g_utf8_strup(buffer, -1));
            return TRUE;
        }
    }

    gchar buffer[2];
    buffer[0] = (gchar)keyval;
    buffer[1] = 0;
    ibus_bugtest_engine_commit_string(bugtest, buffer);

    return TRUE;
}

static void
ibus_bugtest_engine_set_surrounding_text(IBusEngine *engine,
    IBusText *text, guint cursor_pos, guint anchor_pos)
{
    IBusBugtestEngine *bugtest = (IBusBugtestEngine *)engine;
    g_message("%s: text=%s, cursor_pos=%d, anchor_pos=%d", __FUNCTION__, text->text, cursor_pos, anchor_pos);
    if (bugtest->context) {
        g_free(bugtest->context);
        bugtest->context = NULL;
    }
    const gchar *buffer = ibus_text_get_text(text);
    bugtest->context = g_utf8_substring(buffer, 0, cursor_pos);
    parent_class->set_surrounding_text(engine, text, cursor_pos, anchor_pos);
}

static void
ibus_bugtest_engine_focus_in(IBusEngine *engine)
{
    g_message("------------------------------------------------------");
    parent_class->focus_in(engine);
    g_message("%s: Enabling surrounding text", __FUNCTION__);
    ibus_engine_get_surrounding_text(engine, NULL, NULL, NULL);
    reset_context(engine);
}

static void
ibus_bugtest_engine_focus_out(IBusEngine *engine)
{
    IBusBugtestEngine *bugtest = (IBusBugtestEngine *)engine;
    if (bugtest->context) {
        g_free(bugtest->context);
        bugtest->context = NULL;
    }
    parent_class->focus_out(engine);
}
