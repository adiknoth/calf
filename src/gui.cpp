/* Calf DSP Library
 * GUI functions for a plugin.
 * Copyright (C) 2007-2011 Krzysztof Foltman
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 * Boston, MA  02110-1301  USA
 */
 
#include <calf/gui_config.h>
#include <calf/gui_controls.h>
#include <calf/preset.h>
#include <calf/preset_gui.h>
#include <gdk/gdk.h>

#include <iostream>

using namespace calf_plugins;
using namespace std;

/******************************** GUI proper ********************************/

plugin_gui::plugin_gui(plugin_gui_window *_window)
: last_status_serial_no(0)
, window(_window)
{
    ignore_stack = 0;
    top_container = NULL;
    current_control = NULL;
    param_count = 0;
    container = NULL;
    effect_name = NULL;
    preset_access = new gui_preset_access(this);
}

param_control *plugin_gui::create_control_from_xml(const char *element, const char *attributes[])
{
    if (!strcmp(element, "knob"))
        return new knob_param_control;
    if (!strcmp(element, "hscale"))
        return new hscale_param_control;
    if (!strcmp(element, "vscale"))
        return new vscale_param_control;
    if (!strcmp(element, "combo"))
        return new combo_box_param_control;
    if (!strcmp(element, "check"))
        return new check_param_control;
    if (!strcmp(element, "radio"))
        return new radio_param_control;
    if (!strcmp(element, "toggle"))
        return new toggle_param_control;
    if (!strcmp(element, "tap"))
        return new tap_button_param_control;
    if (!strcmp(element, "spin"))
        return new spin_param_control;
    if (!strcmp(element, "button"))
        return new button_param_control;
    if (!strcmp(element, "label"))
        return new label_param_control;
    if (!strcmp(element, "value"))
        return new value_param_control;
    if (!strcmp(element, "vumeter"))
        return new vumeter_param_control;
    if (!strcmp(element, "line-graph"))
        return new line_graph_param_control;
    if (!strcmp(element, "phase-graph"))
        return new phase_graph_param_control;
    if (!strcmp(element, "keyboard"))
        return new keyboard_param_control;
    if (!strcmp(element, "curve"))
        return new curve_param_control;
    if (!strcmp(element, "led"))
        return new led_param_control;
    if (!strcmp(element, "tube"))
        return new tube_param_control;
    if (!strcmp(element, "entry"))
        return new entry_param_control;
    if (!strcmp(element, "filechooser"))
        return new filechooser_param_control;
    if (!strcmp(element, "listview"))
        return new listview_param_control;
    if (!strcmp(element, "notebook"))
        return new notebook_param_control;
    return NULL;
}

control_container *plugin_gui::create_container_from_xml(const char *element, const char *attributes[])
{
    if (!strcmp(element, "table"))
        return new table_container;
    if (!strcmp(element, "vbox"))
        return new vbox_container;
    if (!strcmp(element, "hbox"))
        return new hbox_container;
    if (!strcmp(element, "align"))
        return new alignment_container;
    if (!strcmp(element, "frame"))
        return new frame_container;
    if (!strcmp(element, "scrolled"))
        return new scrolled_container;
    return NULL;
}

void plugin_gui::xml_element_start(void *data, const char *element, const char *attributes[])
{
    plugin_gui *gui = (plugin_gui *)data;
    gui->xml_element_start(element, attributes);
}

int plugin_gui::get_param_no_by_name(string param_name)
{
    int param_no = -1;
    map<string, int>::iterator it = param_name_map.find(param_name);
    if (it == param_name_map.end())
        g_error("Unknown parameter %s", param_name.c_str());
    else
        param_no = it->second;

    return param_no;
}

void plugin_gui::xml_element_start(const char *element, const char *attributes[])
{
    if (ignore_stack) {
        ignore_stack++;
        return;
    }
    control_base::xml_attribute_map xam;
    while(*attributes)
    {
        xam[attributes[0]] = attributes[1];
        attributes += 2;
    }
    
    if (!strcmp(element, "if"))
    {
        if (!xam.count("cond") || xam["cond"].empty())
        {
            g_error("Incorrect <if cond=\"[!]symbol\"> element");
        }
        string cond = xam["cond"];
        bool state = true;
        if (cond.substr(0, 1) == "!") {
            state = false;
            cond.erase(0, 1);
        }
        if (window->environment->check_condition(cond.c_str()) == state)
            return;
        ignore_stack = 1;
        return;
    }
    control_container *cc = create_container_from_xml(element, attributes);
    if (cc != NULL)
    {
        cc->attribs = xam;
        cc->create(this, element, xam);
        cc->set_std_properties();
        gtk_container_set_border_width(cc->container, cc->get_int("border"));

        container_stack.push_back(cc);
        current_control = NULL;
        return;
    }
    if (!container_stack.empty())
    {
        current_control = create_control_from_xml(element, attributes);
        if (current_control)
        {
            current_control->control_name = element;
            current_control->attribs = xam;
            int param_no = -1;
            if (xam.count("param"))
            {
                param_no = get_param_no_by_name(xam["param"]);
            }
            if (param_no != -1)
                current_control->param_variable = plugin->get_metadata_iface()->get_param_props(param_no)->short_name;
            current_control->create(this, param_no);
            current_control->set_std_properties();
            current_control->init_xml(element);
            current_control->add_context_menu_handler();
            if (current_control->is_container()) {
                gtk_container_set_border_width(current_control->container, current_control->get_int("border"));
                container_stack.push_back(current_control);
                control_stack.push_back(current_control);
            }
            return;
        }
    }
    g_error("Unxpected element %s in GUI definition\n", element);
}

void plugin_gui::xml_element_end(void *data, const char *element)
{
    plugin_gui *gui = (plugin_gui *)data;
    unsigned int ss = gui->container_stack.size();
    if (gui->ignore_stack) {
        gui->ignore_stack--;
        return;
    }
    if (!strcmp(element, "if"))
    {
        return;
    }
    if (gui->current_control and !gui->current_control->is_container())
    {
        (*gui->container_stack.rbegin())->add(gui->current_control->widget, gui->current_control);
        gui->current_control->hook_params();
        gui->current_control->set();
        gui->current_control->created();
        gtk_widget_show_all(gui->current_control->widget);
        gui->current_control = NULL;
        return;
    }
    if (ss > 1) {
        gui->container_stack[ss - 2]->add(GTK_WIDGET(gui->container_stack[ss - 1]->container), gui->container_stack[ss - 1]);
        gtk_widget_show_all(GTK_WIDGET(gui->container_stack[ss - 1]->container));
    }
    else {
        gui->top_container = gui->container_stack[0];
        gtk_widget_show_all(GTK_WIDGET(gui->container_stack[0]->container));
    }
    int css = gui->control_stack.size();
    if (ss > 1 and gui->container_stack[ss - 1]->is_container() and css) {
        gui->control_stack[css - 1]->hook_params();
        gui->control_stack[css - 1]->set();
        gui->control_stack[css - 1]->created();
        
        gui->control_stack.pop_back();
    }
    gui->container_stack.pop_back();
    gui->current_control = NULL;
}


GtkWidget *plugin_gui::create_from_xml(plugin_ctl_iface *_plugin, const char *xml)
{
    current_control = NULL;
    top_container = NULL;
    parser = XML_ParserCreate("UTF-8");
    plugin = _plugin;
    container_stack.clear();
    control_stack.clear();
    ignore_stack = 0;
    
    param_name_map.clear();
    read_serials.clear();
    int size = plugin->get_metadata_iface()->get_param_count();
    read_serials.resize(size);
    for (int i = 0; i < size; i++)
        param_name_map[plugin->get_metadata_iface()->get_param_props(i)->short_name] = i;
    
    XML_SetUserData(parser, this);
    XML_SetElementHandler(parser, xml_element_start, xml_element_end);
    XML_Status status = XML_Parse(parser, xml, strlen(xml), 1);
    if (status == XML_STATUS_ERROR)
    {
        g_error("Parse error: %s in XML", XML_ErrorString(XML_GetErrorCode(parser)));
    }
    
    XML_ParserFree(parser);
    last_status_serial_no = plugin->send_status_updates(this, 0);
    GtkWidget *eventbox  = gtk_event_box_new();
    GtkWidget *decoTable = gtk_table_new(3, 1, FALSE);
    
    // decorations
    // overall background
    //GtkWidget *bgImg     = gtk_image_new_from_file(PKGLIBDIR "/background.png");
    //gtk_widget_set_size_request(GTK_WIDGET(bgImg), 1, 1);

    // images for left side
    GtkWidget *nwImg     = gtk_image_new_from_file(PKGLIBDIR "/side_nw.png");
    GtkWidget *swImg     = gtk_image_new_from_file(PKGLIBDIR "/side_sw.png");
    GtkWidget *wImg      = gtk_image_new_from_file(PKGLIBDIR "/side_w.png");
    gtk_widget_set_size_request(GTK_WIDGET(wImg), 56, 1);
    
    // images for right side
    GtkWidget *neImg     = gtk_image_new_from_file(PKGLIBDIR "/side_ne.png");
    GtkWidget *seImg     = gtk_image_new_from_file(PKGLIBDIR "/side_se.png");
    GtkWidget *eImg      = gtk_image_new_from_file(PKGLIBDIR "/side_e.png");
    GtkWidget *logoImg   = gtk_image_new_from_file(PKGLIBDIR "/side_e_logo.png");
    gtk_widget_set_size_request(GTK_WIDGET(eImg), 56, 1);
    
    // pack left box
    leftBox = gtk_vbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(leftBox), GTK_WIDGET(nwImg), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(leftBox), GTK_WIDGET(wImg), TRUE, TRUE, 0);
    gtk_box_pack_end(GTK_BOX(leftBox), GTK_WIDGET(swImg), FALSE, FALSE, 0);
    
     // pack right box
    rightBox = gtk_vbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(rightBox), GTK_WIDGET(neImg), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(rightBox), GTK_WIDGET(eImg), TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(rightBox), GTK_WIDGET(logoImg), FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(rightBox), GTK_WIDGET(seImg), FALSE, FALSE, 0);
    
    //gtk_table_attach(GTK_TABLE(decoTable), GTK_WIDGET(bgImg),     0, 2, 0, 2, (GtkAttachOptions)(GTK_EXPAND | GTK_SHRINK | GTK_FILL), (GtkAttachOptions)(GTK_EXPAND | GTK_SHRINK | GTK_FILL), 0, 0);
    gtk_table_attach(GTK_TABLE(decoTable), GTK_WIDGET(leftBox),   0, 1, 0, 1, (GtkAttachOptions)(0), (GtkAttachOptions)(GTK_EXPAND | GTK_FILL), 0, 0);
    gtk_table_attach(GTK_TABLE(decoTable), GTK_WIDGET(rightBox),  2, 3, 0, 1, (GtkAttachOptions)(0), (GtkAttachOptions)(GTK_EXPAND | GTK_FILL), 0, 0);
        
    gtk_table_attach(GTK_TABLE(decoTable), GTK_WIDGET(top_container->container), 1, 2, 0, 1, (GtkAttachOptions)(GTK_EXPAND | GTK_FILL), (GtkAttachOptions)(GTK_EXPAND | GTK_FILL), 15, 5);
    gtk_container_add( GTK_CONTAINER(eventbox), decoTable );
    gtk_widget_set_name( GTK_WIDGET(eventbox), "Calf-Plugin" );
    
    // create window with viewport
//    GtkWidget *sw = gtk_scrolled_window_new(NULL, NULL);
//    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_NEVER, GTK_POLICY_NEVER);
//    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw), GTK_SHADOW_NONE);
//    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(sw), GTK_WIDGET(decoTable));
    
    return GTK_WIDGET(eventbox);
}

void plugin_gui::send_configure(const char *key, const char *value)
{
    // XXXKF this should really be replaced by a separate list of SCI-capable param controls
    for (unsigned int i = 0; i < params.size(); i++)
    {
        assert(params[i] != NULL);
        send_configure_iface *sci = dynamic_cast<send_configure_iface *>(params[i]);
        if (sci)
            sci->send_configure(key, value);
    }
}

void plugin_gui::send_status(const char *key, const char *value)
{
    // XXXKF this should really be replaced by a separate list of SUI-capable param controls
    for (unsigned int i = 0; i < params.size(); i++)
    {
        assert(params[i] != NULL);
        send_updates_iface *sui = dynamic_cast<send_updates_iface *>(params[i]);
        if (sui)
            sui->send_status(key, value);
    }
}

void plugin_gui::on_idle()
{
    set<unsigned> changed;
    for (unsigned i = 0; i < read_serials.size(); i++)
    {
        int write_serial = plugin->get_write_serial(i);
        if (write_serial - read_serials[i] > 0)
        {
            read_serials[i] = write_serial;
            changed.insert(i);
        }
    }
    for (unsigned i = 0; i < params.size(); i++)
    {
        int param_no = params[i]->param_no;
        if (param_no != -1)
        {
            const parameter_properties &props = *plugin->get_metadata_iface()->get_param_props(param_no);
            bool is_output = (props.flags & PF_PROP_OUTPUT) != 0;
            if (is_output || (param_no != -1 && changed.count(param_no)))
                params[i]->set();
        }
        params[i]->on_idle();
    }    
    last_status_serial_no = plugin->send_status_updates(this, last_status_serial_no);
    // XXXKF iterate over par2ctl, too...
}

void plugin_gui::refresh()
{
    for (unsigned int i = 0; i < params.size(); i++)
        params[i]->set();
    plugin->send_configures(this);
    last_status_serial_no = plugin->send_status_updates(this, last_status_serial_no);
}

void plugin_gui::refresh(int param_no, param_control *originator)
{
    std::multimap<int, param_control *>::iterator it = par2ctl.find(param_no);
    while(it != par2ctl.end() && it->first == param_no)
    {
        if (it->second != originator)
            it->second->set();
        it++;
    }
}

void plugin_gui::set_param_value(int param_no, float value, param_control *originator)
{
    plugin->set_param_value(param_no, value);
    refresh(param_no);
}

/// Get a radio button group (if it exists) for a parameter
GSList *plugin_gui::get_radio_group(int param)
{
    map<int, GSList *>::const_iterator i = param_radio_groups.find(param);
    if (i == param_radio_groups.end())
        return NULL;
    else
        return i->second;
}

/// Set a radio button group for a parameter
void plugin_gui::set_radio_group(int param, GSList *group)
{
    param_radio_groups[param] = group;
}

void plugin_gui::show_rack_ears(bool show)
{
    // if hidden, add a no-show-all attribute so that LV2 host doesn't accidentally override
    // the setting by doing a show_all on the outermost container
    gtk_widget_set_no_show_all(leftBox, !show);
    gtk_widget_set_no_show_all(rightBox, !show);
    if (show)
    {
        gtk_widget_show(leftBox);
        gtk_widget_show(rightBox);
    }
    else
    {
        gtk_widget_hide(leftBox);
        gtk_widget_hide(rightBox);
    }
}

void plugin_gui::on_automation_add(GtkWidget *widget, void *user_data)
{
    plugin_gui *self = (plugin_gui *)user_data;
    self->plugin->add_automation(self->context_menu_last_designator, automation_range(0, 1, self->context_menu_param_no));
}

void plugin_gui::on_automation_delete(GtkWidget *widget, void *user_data)
{
    automation_menu_entry *ame = (automation_menu_entry *)user_data;
    ame->gui->plugin->delete_automation(ame->source, ame->gui->context_menu_param_no);
}

void plugin_gui::on_automation_set_lower_or_upper(automation_menu_entry *ame, bool is_upper)
{
    const parameter_properties *props = plugin->get_metadata_iface()->get_param_props(context_menu_param_no);
    float mapped = props->to_01(plugin->get_param_value(context_menu_param_no));
    
    automation_map mappings;
    plugin->get_automation(context_menu_param_no, mappings);
    automation_map::const_iterator i = mappings.find(ame->source);
    if (i != mappings.end())
    {
        if (is_upper)
            plugin->add_automation(context_menu_last_designator, automation_range(i->second.min_value, mapped, context_menu_param_no));
        else
            plugin->add_automation(context_menu_last_designator, automation_range(mapped, i->second.max_value, context_menu_param_no));
    }
}

void plugin_gui::on_automation_set_lower(GtkWidget *widget, void *user_data)
{
    automation_menu_entry *ame = (automation_menu_entry *)user_data;
    ame->gui->on_automation_set_lower_or_upper(ame, false);
}

void plugin_gui::on_automation_set_upper(GtkWidget *widget, void *user_data)
{
    automation_menu_entry *ame = (automation_menu_entry *)user_data;
    ame->gui->on_automation_set_lower_or_upper(ame, true);
}

void plugin_gui::cleanup_automation_entries()
{
    for (int i = 0; i < (int)automation_menu_callback_data.size(); i++)
        delete automation_menu_callback_data[i];
    automation_menu_callback_data.clear();
}

void plugin_gui::on_control_popup(param_control *ctl, int param_no)
{
    cleanup_automation_entries();
    if (param_no == -1)
        return;
    context_menu_param_no = param_no;
    GtkWidget *menu = gtk_menu_new();
    
    multimap<uint32_t, automation_range> mappings;
    plugin->get_automation(param_no, mappings);
    
    context_menu_last_designator = plugin->get_last_automation_source();
    
    GtkWidget *item;
    if (context_menu_last_designator != 0xFFFFFFFF)
    {
        stringstream ss;
        ss << "_Bind to: Ch" << (1 + (context_menu_last_designator >> 8)) << ", CC#" << (context_menu_last_designator & 127);
        item = gtk_menu_item_new_with_mnemonic(ss.str().c_str());
        g_signal_connect(item, "activate", (GCallback)on_automation_add, this);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    }
    else
    {
        item = gtk_menu_item_new_with_label("Send CC to automate");
        gtk_widget_set_sensitive(item, FALSE);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    }
    
    for(multimap<uint32_t, automation_range>::const_iterator i = mappings.begin(); i != mappings.end(); i++)
    {
        automation_menu_entry *ame = new automation_menu_entry(this, i->first);
        automation_menu_callback_data.push_back(ame);
        stringstream ss;
        ss << "Mapping: Ch" << (1 + (i->first >> 8)) << ", CC#" << (i->first & 127);
        item = gtk_menu_item_new_with_label(ss.str().c_str());
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
        
        GtkWidget *submenu = gtk_menu_new();        
        gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), submenu);
        
        item = gtk_menu_item_new_with_mnemonic("_Delete");
        g_signal_connect(item, "activate", (GCallback)on_automation_delete, ame);
        gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);

        item = gtk_menu_item_new_with_mnemonic("Set _lower limit");
        g_signal_connect(item, "activate", (GCallback)on_automation_set_lower, ame);
        gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);

        item = gtk_menu_item_new_with_mnemonic("Set _upper limit");
        g_signal_connect(item, "activate", (GCallback)on_automation_set_upper, ame);
        gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
        //g_signal_connect(item, "activate", (GCallback)on_automation_add, this);
        
    }
    
    gtk_widget_show_all(menu);
    gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, 3, gtk_get_current_event_time());
}

plugin_gui::~plugin_gui()
{
    cleanup_automation_entries();
    delete preset_access;
    for (std::vector<param_control *>::iterator i = params.begin(); i != params.end(); i++)
    {
        delete *i;
    }
}

/***************************** GUI environment ********************************************/

bool window_update_controller::check_redraw(GtkWidget *toplevel)
{
    GdkWindow *gdkwin = gtk_widget_get_window(toplevel);
    if (!gdkwin)
        return false;

    if (!gdk_window_is_viewable(gdkwin))
        return false;
    GdkWindowState state = gdk_window_get_state(gdkwin);
    if (state & GDK_WINDOW_STATE_ICONIFIED)
    {
        ++refresh_counter;
        if (refresh_counter & 15)
            return false;
    }
    return true;
}

/***************************** GUI environment ********************************************/

gui_environment::gui_environment()
{
    keyfile = g_key_file_new();
    
    gchar *fn = g_build_filename(getenv("HOME"), ".calfrc", NULL);
    string filename = fn;
    g_free(fn);
    g_key_file_load_from_file(keyfile, filename.c_str(), (GKeyFileFlags)(G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS), NULL);
    
    config_db = new calf_utils::gkeyfile_config_db(keyfile, filename.c_str(), "gui");
    gui_config.load(config_db);
}

gui_environment::~gui_environment()
{
    delete config_db;
    if (keyfile)
        g_key_file_free(keyfile);
}
