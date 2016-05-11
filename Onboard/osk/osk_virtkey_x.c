
/*
 * Copyright © 2006-2008 Chris Jones <tortoise@tortuga>
 * Copyright © 2010, 2013 marmuta <marmvta@gmail.com>
 * Copyright © 2013 Gerd Kohlberger <lowfi@chello.at>
 *
 * This file is part of Onboard.
 *
 * Onboard is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * Onboard is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <gdk/gdkx.h>
#include <gdk/gdkkeysyms.h>

#include <X11/keysym.h>
#include <X11/XKBlib.h>
#include <X11/extensions/XTest.h>
#include <X11/extensions/XKBrules.h>

#include "osk_virtkey_x.h"

#define N_MOD_INDICES (Mod5MapIndex + 1)

typedef struct VirtkeyX VirtkeyX;

struct VirtkeyX {
    VirtkeyBase base;

    Display   *xdisplay;
    KeySym    *keysyms;
    XkbDescPtr kbd;

    KeyCode    modifier_table[N_MOD_INDICES];
    int        n_keysyms_per_keycode;
    int        shift_mod_index;
    int        alt_mod_index;
    int        meta_mod_index;
};

static int
virtkey_x_get_current_group (VirtkeyBase *base)
{
    VirtkeyX* this = (VirtkeyX*) base;
    XkbStateRec state;

    if (XkbGetState (this->xdisplay, XkbUseCoreKbd, &state) != Success)
    {
        PyErr_SetString (OSK_EXCEPTION, "XkbGetState failed");
        return -1;
    }
    return state.locked_group;
}

static char*
virtkey_x_get_current_group_name (VirtkeyBase* base)
{
    VirtkeyX* this = (VirtkeyX*) base;
    char* result = NULL;
    int group;

    if (!this->kbd->names || !this->kbd->names->groups)
    {
        PyErr_SetString (OSK_EXCEPTION, "no group names available");
        return NULL;
    }

    group = virtkey_x_get_current_group(base);
    if (group < 0)
        return NULL;

    if (this->kbd->names->groups[group] != None)
    {
        char *name = XGetAtomName (this->xdisplay,
                                   this->kbd->names->groups[group]);
        if (name)
        {
            result = strdup(name);
            XFree (name);
        }
    }

    return result;
}

static int
virtkey_x_get_keycode_from_keysym (VirtkeyBase* base, int keysym, unsigned int *mod_mask)
{
    VirtkeyX* this = (VirtkeyX*) base;
    static int modified_key = 0;
    KeyCode code = 0;

    code = XKeysymToKeycode (this->xdisplay, keysym);
    if (code)
    {
        int group;

        group = virtkey_x_get_current_group(base);
        if (group < 0)
            return 0;

        if (XkbKeycodeToKeysym (this->xdisplay, code, group, 0) != keysym)
        {
            /* try shift modifier */
            if (XkbKeycodeToKeysym (this->xdisplay, code, group, 1) == keysym)
                *mod_mask |= 1;
            else
                code = 0;
        }
    }
    if (!code)
    {
        int index;
        /* Change one of the last 10 keysyms to our converted utf8,
         * remapping the x keyboard on the fly. This make assumption
         * the last 10 arn't already used.
         */
        modified_key = (modified_key + 1) % 10;

        /* Point at the end of keysyms, modifier 0 */
        index = (this->kbd->max_key_code - this->kbd->min_key_code - modified_key - 1) *
                 this->n_keysyms_per_keycode;

        this->keysyms[index] = keysym;

        XChangeKeyboardMapping (this->xdisplay,
                                this->kbd->min_key_code,
                                this->n_keysyms_per_keycode,
                                this->keysyms,
                                this->kbd->max_key_code - this->kbd->min_key_code);
        XSync (this->xdisplay, False);
        /* From dasher src:
         * There's no way whatsoever that this could ever possibly
         * be guaranteed to work (ever), but it does.
         *
         * The below is lightly safer:
         *
         * code = XKeysymToKeycode(fk->xdisplay, keysym);
         *
         * but this appears to break in that the new mapping is not immediatly
         * put to work. It would seem a MappingNotify event is needed so
         * Xlib can do some changes internally? (xlib is doing something
         * related to above?)
         */
        code = this->kbd->max_key_code - modified_key - 1;
    }
    return code;
}

/**
 * based on code from libgnomekbd
 * gkbd-keyboard-drawing.c, set_key_label_in_layout
 */
char*
virtkey_gdk_get_label_from_keysym (KeySym keyval)
{
    char* label;

    switch (keyval) {
        case GDK_KEY_Scroll_Lock:
            label = "Scroll\nLock";
            break;

        case GDK_KEY_space:
            label = " ";
            break;

        case GDK_KEY_Sys_Req:
            label = "Sys Rq";
            break;

        case GDK_KEY_Page_Up:
            label = "Page\nUp";
            break;

        case GDK_KEY_Page_Down:
            label = "Page\nDown";
            break;

        case GDK_KEY_Num_Lock:
            label = "Num\nLock";
            break;

        case GDK_KEY_KP_Page_Up:
            label = "Pg Up";
            break;

        case GDK_KEY_KP_Page_Down:
            label = "Pg Dn";
            break;

        case GDK_KEY_KP_Home:
            label = "Home";
            break;

        case GDK_KEY_KP_Left:
            label = "Left";
            break;

        case GDK_KEY_KP_End:
            label = "End";
            break;

        case GDK_KEY_KP_Up:
            label = "Up";
            break;

        case GDK_KEY_KP_Begin:
            label = "Begin";
            break;

        case GDK_KEY_KP_Right:
            label = "Right";
            break;

        case GDK_KEY_KP_Enter:
            label = "Enter";
            break;

        case GDK_KEY_KP_Down:
            label = "Down";
            break;

        case GDK_KEY_KP_Insert:
            label = "Ins";
            break;

        case GDK_KEY_KP_Delete:
            label = "Del";
            break;

        case GDK_KEY_dead_grave:
            label = "ˋ";
            break;

        case GDK_KEY_dead_acute:
            label = "ˊ";
            break;

        case GDK_KEY_dead_circumflex:
            label = "ˆ";
            break;

        case GDK_KEY_dead_tilde:
            label = "~";
            break;

        case GDK_KEY_dead_macron:
            label = "ˉ";
            break;

        case GDK_KEY_dead_breve:
            label = "˘";
            break;

        case GDK_KEY_dead_abovedot:
            label = "˙";
            break;

        case GDK_KEY_dead_diaeresis:
            label = "¨";
            break;

        case GDK_KEY_dead_abovering:
            label = "˚";
            break;

        case GDK_KEY_dead_doubleacute:
            label = "˝";
            break;

        case GDK_KEY_dead_caron:
            label = "ˇ";
            break;

        case GDK_KEY_dead_cedilla:
            label = "¸";
            break;

        case GDK_KEY_dead_ogonek:
            label = "˛";
            break;

        case GDK_KEY_dead_belowdot:
            label = ".";
            break;

        case GDK_KEY_Mode_switch:
            label = "AltGr";
            break;

        case GDK_KEY_Multi_key:
            label = "Compose";
            break;

        default:
        {
            static char buf[256];
            const gunichar uc = gdk_keyval_to_unicode (keyval);
            if (uc && g_unichar_isgraph (uc))
            {
                int l = MIN(g_unichar_to_utf8 (uc, buf), sizeof(buf)-1);
                buf[l] = '\0';
                label = buf;
            }
            else
            {
                const char *name = gdk_keyval_name (keyval);
                if (!name)
                {
                    label = "";
                }
                else
                {
                    size_t l = MIN(strlen(name), sizeof(buf)-1);
                    strncpy(buf, name, l);
                    buf[l] = '\0';
                    if (l > 2 && name[0] == '0' && name[1] == 'x') // hex number?
                    {
                        // Most likely an erroneous keysym and not Onboard's fault.
                        // Show the hex number fully as label for easier debugging.
                        // Happend with Belgian CapsLock+keycode 51,
                        // keysym 0x039c on Xenial.
                        buf[MIN(l, 10)] = '\0';
                    }
                    else
                    {
                        buf[MIN(l, 2)] = '\0';
                    }
                    label = buf;
                }
            }
        }
    }
    return label;
}

static void
virtkey_x_get_label_from_keycode(VirtkeyBase* base,
    int keycode, int modmask, int group,
    char* label, int max_label_size)
{
    VirtkeyX* this = (VirtkeyX*) base;
    unsigned int mods;
    int label_size;
    KeySym keysym;
    XKeyPressedEvent ev;

    memset(&ev, 0, sizeof(ev));
    ev.type = KeyPress;
    ev.display = this->xdisplay;

    mods = XkbBuildCoreState (modmask, group);

    ev.state = mods;
    ev.keycode = keycode;
    label_size = XLookupString(&ev, label, max_label_size, &keysym, NULL);
    label[label_size] = '\0';

    if (keysym)
    {
        strncpy(virtkey_gdk_get_label_from_keysym (keysym), 
                label, max_label_size);
        label[max_label_size] = '\0';
    }
}

static int
virtkey_x_get_keysym_from_keycode(VirtkeyBase* base,
                                  int keycode, int modmask, int group)
{
    KeySym keysym;
    unsigned int mods_rtn;

    VirtkeyX* this = (VirtkeyX*) base;

    XkbTranslateKeyCode (this->kbd,
                         keycode,
                         XkbBuildCoreState (modmask, group),
                         &mods_rtn,
                         &keysym);
    return keysym;
}

/**
 * Reads the contents of the root window property _XKB_RULES_NAMES.
 */
struct RulesNames
{
    char* rules_file;
    char* model;
    char* layout;
    char* variant;
    char* options;
};

static char**
virtkey_x_get_rules_names(VirtkeyBase* base, int* numentries)
{
    VirtkeyX* this = (VirtkeyX*) base;
    XkbRF_VarDefsRec vd;
    char *tmp = NULL;
    char** results;
    const int n = 5;

    if (!XkbRF_GetNamesProp (this->xdisplay, &tmp, &vd))
        return NULL;

    results = malloc(sizeof(char*) * n);
    if (!results)
        return NULL;

    *numentries = n;

    if (tmp)
    {
        results[0] = strdup(tmp);
        XFree (tmp);
    }
    else
        results[0] = strdup("");

    if (vd.model)
    {
        results[1] = strdup(vd.model);
        XFree (vd.model);
    }
    else
        results[1] = strdup("");

    if (vd.layout)
    {
        results[2] = strdup(vd.layout);
        XFree (vd.layout);
    }
    else
        results[2] = strdup("");

    if (vd.variant)
    {
        results[3] = strdup(vd.variant);
        XFree (vd.variant);
    }
    else
        results[3] = strdup("");
 
    if (vd.options)
    {
        results[4] = strdup(vd.options);
        XFree (vd.options);
    }
    else
        results[4] = strdup("");

    return results;
}

/**
 * returns a plus-sign separated string of all keyboard layouts
 */
static char*
virtkey_x_get_layout_symbols (VirtkeyBase* base)
{
    VirtkeyX* this = (VirtkeyX*) base;
    char* result = NULL;
    char* symbols;

    if (!this->kbd->names || !this->kbd->names->symbols)
    {
        PyErr_SetString (OSK_EXCEPTION, "no symbols names available");
        return NULL;
    }

    symbols = XGetAtomName (this->xdisplay, this->kbd->names->symbols);
    if (symbols)
    {
        result = strdup(symbols);
        XFree (symbols);
    }

    return result;
}

void
virtkey_x_set_modifiers (VirtkeyBase* base,
                         int mod_mask, bool lock, bool press)
{
    VirtkeyX* this = (VirtkeyX*) base;
    if (lock)
        XkbLockModifiers (this->xdisplay, XkbUseCoreKbd,
                            mod_mask, press ? mod_mask : 0);
    else
        XkbLatchModifiers (this->xdisplay, XkbUseCoreKbd,
                            mod_mask, press ? mod_mask : 0);

    XSync (this->xdisplay, False);
}

static int
virtkey_x_init_keyboard (VirtkeyX *self)
{
    self->kbd = XkbGetKeyboard (self->xdisplay,
                              XkbCompatMapMask | XkbNamesMask | XkbGeometryMask,
                              XkbUseCoreKbd);
#ifndef NDEBUG
    /* test missing keyboard (LP:#526791) keyboard on/off every 10 seconds */
    if (getenv ("VIRTKEY_DEBUG"))
    {
        if (self->kbd && time(NULL) % 20 < 10)
        {
            XkbFreeKeyboard (self->kbd, XkbAllComponentsMask, True);
            self->kbd = NULL;
        }
    }
#endif
    if (!self->kbd)
    {
        PyErr_SetString (OSK_EXCEPTION, "XkbGetKeyboard failed.");
        return -1;
    }
    if (XkbGetNames (self->xdisplay, XkbAllNamesMask, self->kbd) != Success)
    {
        PyErr_SetString (OSK_EXCEPTION, "XkbGetNames failed.");
        return -1;
    }

    return 0;
}

static int
virtkey_x_init (VirtkeyBase *base)
{
    VirtkeyX* this = (VirtkeyX*) base;
    XModifierKeymap *modifiers;
    int mod_index, mod_key;

    GdkDisplay* display = gdk_display_get_default ();
    if (!GDK_IS_X11_DISPLAY (display)) // Wayland, MIR?
    {
        PyErr_SetString (OSK_EXCEPTION, "not an X display");
        return -1;
    }
    this->xdisplay = GDK_DISPLAY_XDISPLAY (display);

    if (virtkey_x_init_keyboard (this) < 0)
        return -1;

    /* init modifiers */
    this->keysyms = XGetKeyboardMapping (this->xdisplay,
                                       this->kbd->min_key_code,
                                       this->kbd->max_key_code - this->kbd->min_key_code + 1,
                                       &this->n_keysyms_per_keycode);

    modifiers = XGetModifierMapping (this->xdisplay);
    for (mod_index = 0; mod_index < 8; mod_index++)
    {
        this->modifier_table[mod_index] = 0;

        for (mod_key = 0; mod_key < modifiers->max_keypermod; mod_key++)
        {
            int keycode = modifiers->modifiermap[mod_index *
                                                 modifiers->max_keypermod +
                                                 mod_key];
            if (keycode)
            {
                this->modifier_table[mod_index] = keycode;
                break;
            }
        }
    }

    XFreeModifiermap (modifiers);

    for (mod_index = Mod1MapIndex; mod_index <= Mod5MapIndex; mod_index++)
    {
        if (this->modifier_table[mod_index])
        {
            KeySym ks = XkbKeycodeToKeysym (this->xdisplay,
                                            this->modifier_table[mod_index],
                                            0, 0);

            /* Note: ControlMapIndex is already defined by xlib */
            switch (ks) {
                case XK_Meta_R:
                case XK_Meta_L:
                    this->meta_mod_index = mod_index;
                    break;

                case XK_Alt_R:
                case XK_Alt_L:
                    this->alt_mod_index = mod_index;
                    break;

                case XK_Shift_R:
                case XK_Shift_L:
                    this->shift_mod_index = mod_index;
                    break;
            }
        }
    }

    return 0;
}

static int
virtkey_x_reload (VirtkeyBase* base)
{
    VirtkeyX* this = (VirtkeyX*) base;

    if (this->kbd)
    {
        XkbFreeKeyboard (this->kbd, XkbAllComponentsMask, True);
        this->kbd = NULL;
    }

    if (virtkey_x_init (base) < 0)
        return -1;

    return 0;
}

static void
virtkey_x_destruct (VirtkeyBase *base)
{
    VirtkeyX* this = (VirtkeyX*) base;

    if (this->kbd)
        XkbFreeKeyboard (this->kbd, XkbAllComponentsMask, True);

    if (this->keysyms)
        XFree (this->keysyms);
}

VirtkeyBase*
virtkey_x_new(void)
{
   VirtkeyBase* this = (VirtkeyBase*) malloc(sizeof(VirtkeyX));
   this->init = virtkey_x_init;
   this->destruct = virtkey_x_destruct;
   this->reload = virtkey_x_reload;
   this->get_current_group = virtkey_x_get_current_group;
   this->get_current_group_name = virtkey_x_get_current_group_name;
   this->get_label_from_keycode = virtkey_x_get_label_from_keycode;
   this->get_keysym_from_keycode = virtkey_x_get_keysym_from_keycode;
   this->get_keycode_from_keysym = virtkey_x_get_keycode_from_keysym;
   this->get_rules_names = virtkey_x_get_rules_names;
   this->get_layout_symbols = virtkey_x_get_layout_symbols;
   this->set_modifiers = virtkey_x_set_modifiers;
   return this;
}

