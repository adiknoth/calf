#include <calf/gui_config.h>

using namespace std;

gui_config::gui_config()
{
    rows = 0;
    cols = 1;
    rack_ears = true;
}

gui_config::~gui_config()
{
}

void gui_config::load(gui_config_db_iface *db)
{
    rows = db->get_int("rack-rows", gui_config().rows);
    cols = db->get_int("rack-cols", gui_config().cols);
    rack_ears = db->get_bool("show-rack-ears", gui_config().rack_ears);
}

void gui_config::save(gui_config_db_iface *db)
{
    db->set_int("rack-rows", rows);
    db->set_int("rack-cols", cols);
    db->set_bool("show-rack-ears", rack_ears);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

gconf_config_db::gconf_config_db(const char *parent_key)
{
    prefix = string(parent_key) + "/";
    client = gconf_client_get_default();
    if (client)
    {
        GError *err = NULL;
        gconf_client_add_dir(client, parent_key, GCONF_CLIENT_PRELOAD_ONELEVEL, &err);
        if (err)
        {
            client = NULL;
            handle_error(err);
        }
    }
}

void gconf_config_db::handle_error(GError *error)
{
    if (error)
    {
        string msg = error->message;
        g_error_free(error);
        throw config_exception(msg.c_str());
    }
}

bool gconf_config_db::has_dir(const char *key)
{
    GError *err = NULL;
    bool result = gconf_client_dir_exists(client, (prefix + key).c_str(), &err);
    if (err)
        handle_error(err);
    return result;
}

bool gconf_config_db::get_bool(const char *key, bool def_value)
{
    GError *err = NULL;
    bool value = (bool)gconf_client_get_bool(client, (prefix + key).c_str(), &err);
    if (err)
        handle_error(err);
    return value;
}

int gconf_config_db::get_int(const char *key, int def_value)
{
    GError *err = NULL;
    int value = gconf_client_get_int(client, (prefix + key).c_str(), &err);
    if (err)
        handle_error(err);
    return value;
}

std::string gconf_config_db::get_string(const char *key, const std::string &def_value)
{
    GError *err = NULL;
    gchar *value = (gchar *)gconf_client_get_string(client, (prefix + key).c_str(), &err);
    if (err)
        handle_error(err);
    string svalue = value;
    g_free(value);
    return svalue;
}

void gconf_config_db::set_bool(const char *key, bool value)
{
    GError *err = NULL;
    gconf_client_set_bool(client, (prefix + key).c_str(), (gboolean)value, &err);
    if (err)
        handle_error(err);
}

void gconf_config_db::set_int(const char *key, int value)
{
    GError *err = NULL;
    gconf_client_set_int(client, (prefix + key).c_str(), value, &err);
    if (err)
        handle_error(err);
}

void gconf_config_db::set_string(const char *key, const std::string &value)
{
    GError *err = NULL;
    gconf_client_set_string(client, (prefix + key).c_str(), value.c_str(), &err);
    if (err)
        handle_error(err);
}

gconf_config_db::~gconf_config_db()
{
    g_object_unref(G_OBJECT(client));
}
