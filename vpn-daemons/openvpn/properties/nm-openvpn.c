/* nm-openvpn.c : GNOME UI dialogs for configuring OpenVPN connections
 *
 * Copyright (C) 2005 Tim Niemueller <tim@niemueller.de>
 * Based on work by David Zeuthen, <davidz@redhat.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * $Id$
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gi18n-lib.h>
#include <string.h>
#include <glade/glade.h>

#define NM_VPN_API_SUBJECT_TO_CHANGE

#include <NetworkManager/nm-vpn-ui-interface.h>

#include "../src/nm-openvpn-service.h"

typedef struct _NetworkManagerVpnUIImpl NetworkManagerVpnUIImpl;

struct _NetworkManagerVpnUIImpl {
  NetworkManagerVpnUI parent;

  NetworkManagerVpnUIDialogValidityCallback callback;
  gpointer callback_user_data;

  gchar    *last_fc_dir;

  GladeXML *xml;

  GtkWidget *widget;

  GtkEntry       *w_connection_name;
  GtkEntry       *w_remote;
  GtkEntry       *w_ca;
  GtkEntry       *w_cert;
  GtkEntry       *w_key;
  GtkCheckButton *w_use_routes;
  GtkEntry       *w_routes;
  GtkCheckButton *w_use_lzo;
  GtkCheckButton *w_use_tap;
  GtkCheckButton *w_use_tcp;
  GtkExpander    *w_opt_info_expander;
  GtkButton      *w_import_button;
  GtkButton      *w_button_ca;
  GtkButton      *w_button_cert;
  GtkButton      *w_button_key;
  GtkComboBox    *w_connection_type;
  GtkNotebook    *w_settings_notebook;
  GtkButton      *w_button_shared_key;
  GtkEntry       *w_shared_key;
  GtkEntry       *w_local_ip;
  GtkEntry       *w_remote_ip;
  GtkEntry       *w_username;
  GtkEntry       *w_password_ca;
  GtkButton      *w_button_password_ca;
};

static void connection_type_changed(GtkComboBox *box, gpointer user_data);


static void 
openvpn_clear_widget (NetworkManagerVpnUIImpl *impl)
{
  gtk_entry_set_text (impl->w_connection_name, "");
  gtk_entry_set_text (impl->w_remote,   "");
  gtk_entry_set_text (impl->w_ca,   "");
  gtk_entry_set_text (impl->w_cert, "");
  gtk_entry_set_text (impl->w_key,  "");
  gtk_entry_set_text (impl->w_shared_key,  "");
  gtk_entry_set_text (impl->w_local_ip,  "");
  gtk_entry_set_text (impl->w_remote_ip,  "");
  gtk_entry_set_text (impl->w_username,  "");
  gtk_entry_set_text (impl->w_password_ca,  "");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (impl->w_use_routes), FALSE);
  gtk_entry_set_text (impl->w_routes, "");
  gtk_widget_set_sensitive (GTK_WIDGET (impl->w_routes), FALSE);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (impl->w_use_lzo), FALSE);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (impl->w_use_tap), FALSE);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (impl->w_use_tcp), FALSE);
  gtk_expander_set_expanded (impl->w_opt_info_expander, FALSE);
  gtk_combo_box_set_active (GTK_COMBO_BOX (impl->w_connection_type), 0);
  connection_type_changed (GTK_COMBO_BOX (impl->w_connection_type), impl);
}

static const char *
impl_get_display_name (NetworkManagerVpnUI *self)
{
  return _("OpenVPN Client");
}

static const char *
impl_get_service_name (NetworkManagerVpnUI *self)
{
  return "org.freedesktop.NetworkManager.openvpn";
}

static GtkWidget *
impl_get_widget (NetworkManagerVpnUI *self, GSList *properties, GSList *routes, const char *connection_name)
{
  GSList *i;
  NetworkManagerVpnUIImpl *impl = (NetworkManagerVpnUIImpl *) self->data;
  gboolean should_expand;

  openvpn_clear_widget (impl);

  should_expand = FALSE;

  if (connection_name != NULL)
    gtk_entry_set_text (impl->w_connection_name, connection_name);

  for (i = properties; i != NULL && g_slist_next (i) != NULL; i = g_slist_next (g_slist_next (i))) {
    const char *key;
    const char *value;

    key = i->data;
    value = (g_slist_next (i))->data;

    if (strcmp (key, "remote") == 0) {
      gtk_entry_set_text (impl->w_remote, value);		
    } else if (strcmp (key, "ca") == 0) {
      gtk_entry_set_text (impl->w_ca, value);
    } else if (strcmp (key, "cert") == 0) {
      gtk_entry_set_text (impl->w_cert, value);
    } else if (strcmp (key, "key") == 0) {
      gtk_entry_set_text (impl->w_key, value);
    } else if ( (strcmp (key, "comp-lzo") == 0) &&
		(strcmp (value, "yes") == 0) ) {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (impl->w_use_lzo), TRUE);
      should_expand = TRUE;
    } else if ( strcmp (key, "connection-type") == 0) {
      gint type_cbox_sel = 0;

      if ( strcmp (value, "x509") == 0 ) {
	type_cbox_sel = NM_OPENVPN_CONTYPE_X509;
      } else if ( strcmp (value, "shared-key") == 0 ) {
	type_cbox_sel = NM_OPENVPN_CONTYPE_SHAREDKEY;
      } else if ( strcmp (value, "password") == 0 ) {
	type_cbox_sel = NM_OPENVPN_CONTYPE_PASSWORD;
      } else if ( strcmp (value, "x509userpass") == 0 ) {
	type_cbox_sel = NM_OPENVPN_CONTYPE_X509USERPASS;
      }

      gtk_combo_box_set_active (GTK_COMBO_BOX (impl->w_connection_type), type_cbox_sel);
      connection_type_changed (GTK_COMBO_BOX (impl->w_connection_type), impl);
    } else if ( strcmp (key, "local-ip") == 0 ) {
      gtk_entry_set_text (impl->w_local_ip, value);
    } else if ( strcmp (key, "remote-ip") == 0 ) {
      gtk_entry_set_text (impl->w_remote_ip, value);
    } else if ( strcmp (key, "shared-key") == 0 ) {
      gtk_entry_set_text (impl->w_shared_key, value);
    } else if ( strcmp (key, "username") == 0 ) {
      gtk_entry_set_text (impl->w_username, value);
    } else if ( (strcmp (key,   "dev") == 0) &&
		(strcmp (value, "tap") == 0) ) {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (impl->w_use_tap), TRUE);
      should_expand = TRUE;
    } else if ( (strcmp (key,   "proto") == 0) &&
		(strcmp (value, "tcp") == 0) ) {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (impl->w_use_tcp), TRUE);
      should_expand = TRUE;
    }
  }

  if (routes != NULL) {
    GString *route_str;
    char *str;

    route_str = g_string_new ("");
    for (i = routes; i != NULL; i = g_slist_next (i)) {
      const char *route;
			
      if (i != routes)
	g_string_append_c(route_str, ' ');
			
      route = (const char *) i->data;
      g_string_append(route_str, route);
    }

    str = g_string_free (route_str, FALSE);
    gtk_entry_set_text (impl->w_routes, str);
    g_free (str);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (impl->w_use_routes), TRUE);
    gtk_widget_set_sensitive (GTK_WIDGET (impl->w_routes), TRUE);
    
    should_expand = TRUE;
  }

  gtk_expander_set_expanded (impl->w_opt_info_expander, should_expand);
  gtk_container_resize_children (GTK_CONTAINER (impl->widget));

  return impl->widget;
}

static GSList *
impl_get_properties (NetworkManagerVpnUI *self)
{
  GSList *data;
  NetworkManagerVpnUIImpl *impl = (NetworkManagerVpnUIImpl *) self->data;
  const char *connectionname;
  const char *remote;
  const char *ca;
  const char *cert;
  const char *key;
  const char *shared_key;
  const char *local_ip;
  const char *remote_ip;
  const char *username;
  gboolean    use_lzo;
  gboolean    use_tap;
  gboolean    use_tcp;

  connectionname         = gtk_entry_get_text (impl->w_connection_name);
  remote                 = gtk_entry_get_text (impl->w_remote);
  ca                     = gtk_entry_get_text (impl->w_ca);
  cert                   = gtk_entry_get_text (impl->w_cert);
  key                    = gtk_entry_get_text (impl->w_key);
  use_lzo                = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (impl->w_use_lzo));
  use_tap                = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (impl->w_use_tap));
  use_tcp                = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (impl->w_use_tcp));
  shared_key             = gtk_entry_get_text (impl->w_shared_key);
  local_ip               = gtk_entry_get_text (impl->w_local_ip);
  remote_ip              = gtk_entry_get_text (impl->w_remote_ip);
  username               = gtk_entry_get_text (impl->w_username);

  data = NULL;
  data = g_slist_append (data, g_strdup ("connection-type"));
  switch ( gtk_combo_box_get_active (GTK_COMBO_BOX (impl->w_connection_type)) ) {
  case NM_OPENVPN_CONTYPE_SHAREDKEY:
    data = g_slist_append (data, g_strdup ("shared-key"));
    break;
  case NM_OPENVPN_CONTYPE_PASSWORD:
    data = g_slist_append (data, g_strdup ("password"));
    break;
  case NM_OPENVPN_CONTYPE_X509USERPASS:
    data = g_slist_append (data, g_strdup ("x509userpass"));
    break;
  default: // NM_OPENVPN_CONTYPE_X509
    data = g_slist_append (data, g_strdup ("x509"));
    break;
  }
  data = g_slist_append (data, g_strdup ("dev"));
  data = g_slist_append (data, use_tap ? g_strdup ("tap") : g_strdup("tun"));
  data = g_slist_append (data, g_strdup ("remote"));
  data = g_slist_append (data, g_strdup (remote));
  data = g_slist_append (data, g_strdup ("proto"));
  data = g_slist_append (data, use_tcp ? g_strdup ("tcp") : g_strdup("udp"));
  data = g_slist_append (data, g_strdup ("ca"));
  data = g_slist_append (data, g_strdup (ca));
  data = g_slist_append (data, g_strdup ("cert"));
  data = g_slist_append (data, g_strdup (cert));
  data = g_slist_append (data, g_strdup ("key"));
  data = g_slist_append (data, g_strdup (key));
  data = g_slist_append (data, g_strdup ("comp-lzo"));
  data = g_slist_append (data, use_lzo ? g_strdup ("yes") : g_strdup("no"));
  data = g_slist_append (data, g_strdup ("shared-key"));
  data = g_slist_append (data, g_strdup (shared_key));
  data = g_slist_append (data, g_strdup ("local-ip"));
  data = g_slist_append (data, g_strdup (local_ip));
  data = g_slist_append (data, g_strdup ("remote-ip"));
  data = g_slist_append (data, g_strdup (remote_ip));
  data = g_slist_append (data, g_strdup ("username"));
  data = g_slist_append (data, g_strdup (username));

  return data;
}

static GSList *
get_routes (NetworkManagerVpnUIImpl *impl)
{
  GSList *routes;
  const char *routes_entry;
  gboolean use_routes;
  char **substrs;
  unsigned int i;

  routes = NULL;

  routes_entry = gtk_entry_get_text (impl->w_routes);
  use_routes = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (impl->w_use_routes));

  if (!use_routes)
    goto out;

  substrs = g_strsplit (routes_entry, " ", 0);
  for (i = 0; substrs[i] != NULL; i++) {
    char *route;

    route = substrs[i];
    if (strlen (route) > 0)
      routes = g_slist_append (routes, g_strdup (route));
  }

  g_strfreev (substrs);

 out:
  return routes;
}

static GSList *
impl_get_routes (NetworkManagerVpnUI *self)
{
  NetworkManagerVpnUIImpl *impl = (NetworkManagerVpnUIImpl *) self->data;

  return get_routes (impl);
}


/** Checks if ip is in notation
 * a.b.c.d where a,b,c,d in {0..255}
 */
static gboolean
check_ip (const char *ip)
{
  int d1, d2, d3, d4;
	
  if (sscanf (ip, "%d.%d.%d.%d", &d1, &d2, &d3, &d4) != 4) {
    return FALSE;
  }

  /* TODO: this can be improved a bit */
  if (d1 < 0 || d1 > 255 ||
      d2 < 0 || d2 > 255 ||
      d3 < 0 || d3 > 255 ||
      d4 < 0 || d4 > 255 ) {

    return FALSE;
  }

  return TRUE;
}

/** Checks if net cidr is in notation
 * a.b.c.d/n where a,b,c,d in {0..255} and
 * n in {0..32}
 */
static gboolean
check_net_cidr (const char *net)
{
  int d1, d2, d3, d4, mask;
	
  if (sscanf (net, "%d.%d.%d.%d/%d", &d1, &d2, &d3, &d4, &mask) != 5) {
    return FALSE;
  }

  /* TODO: this can be improved a bit */
  if (d1 < 0 || d1 > 255 ||
      d2 < 0 || d2 > 255 ||
      d3 < 0 || d3 > 255 ||
      d4 < 0 || d4 > 255 ||
      mask < 0 || mask > 32) {
    return FALSE;
  }

  return TRUE;
}


static char *
impl_get_connection_name (NetworkManagerVpnUI *self)
{
  const char *name;
  NetworkManagerVpnUIImpl *impl = (NetworkManagerVpnUIImpl *) self->data;

  name = gtk_entry_get_text (impl->w_connection_name);
  if (name != NULL)
    return g_strdup (name);
  else
    return NULL;
}


static gboolean
impl_is_valid (NetworkManagerVpnUI *self)
{
  NetworkManagerVpnUIImpl *impl = (NetworkManagerVpnUIImpl *) self->data;
  gboolean is_valid;
  gboolean use_routes;
  const char *routes_entry;
  const char *connectionname;
  const char *remote;
  gint connection_type =   gtk_combo_box_get_active (GTK_COMBO_BOX (impl->w_connection_type));

  connectionname         = gtk_entry_get_text (impl->w_connection_name);
  remote                 = gtk_entry_get_text (impl->w_remote);
  use_routes             = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (impl->w_use_routes));
  routes_entry           = gtk_entry_get_text (impl->w_routes);

  is_valid = FALSE;

  if ( (strlen (connectionname) == 0) ||
       (strlen (remote) == 0) ||
       (strstr (remote, " ") != NULL)  ||
       (strstr (remote, "\t") != NULL) ) {

    is_valid = FALSE;

  } else if ( connection_type == NM_OPENVPN_CONTYPE_SHAREDKEY ) {
    const char *shared_key;
    const char *local_ip;
    const char *remote_ip;

    shared_key             = gtk_entry_get_text (impl->w_shared_key);
    local_ip               = gtk_entry_get_text (impl->w_local_ip);
    remote_ip              = gtk_entry_get_text (impl->w_remote_ip);

    if ( (strlen (shared_key) > 0) &&
	 (strlen (local_ip) > 0) &&
	 (strlen (remote_ip) > 0) &&
	 check_ip (local_ip) &&
	 check_ip (remote_ip) &&
         g_file_test( shared_key, G_FILE_TEST_IS_REGULAR) ) {

      is_valid = TRUE;
    }

  } else if ( connection_type == NM_OPENVPN_CONTYPE_PASSWORD ) {

    const char *username;
    const char *ca;

    username    = gtk_entry_get_text (impl->w_username);
    ca          = gtk_entry_get_text (impl->w_password_ca);


    if (strlen (username) > 0 &&
	strlen (ca) > 0 &&
	g_file_test( ca, G_FILE_TEST_IS_REGULAR) ) {

      is_valid = TRUE;
    }
 
  } else if ( connection_type == NM_OPENVPN_CONTYPE_X509USERPASS ) {

    const char *username;
    const char *ca;
    const char *cert;
    const char *key;

    username    = gtk_entry_get_text (impl->w_username);
    ca          = gtk_entry_get_text (impl->w_password_ca);
    cert                   = gtk_entry_get_text (impl->w_cert);
    key                    = gtk_entry_get_text (impl->w_key);


    if (strlen (username) > 0 &&
	strlen (ca) > 0 &&
	strlen (cert) > 0 &&
	strlen (key) > 0 &&
	((!use_routes) || (use_routes && strlen (routes_entry) > 0)) &&
	/* validate ca/cert/key files */
	g_file_test( ca, G_FILE_TEST_IS_REGULAR) &&
	g_file_test( cert, G_FILE_TEST_IS_REGULAR) &&
	g_file_test( key, G_FILE_TEST_IS_REGULAR) ) {

      is_valid = TRUE;
    }
 
  } else {
    // default to NM_OPENVPN_CONTYPE_X509
    const char *ca;
    const char *cert;
    const char *key;

    ca                     = gtk_entry_get_text (impl->w_ca);
    cert                   = gtk_entry_get_text (impl->w_cert);
    key                    = gtk_entry_get_text (impl->w_key);

    /* initial sanity checking */
    if (strlen (ca) > 0 &&
	strlen (cert) > 0 &&
	strlen (key) > 0 &&
	((!use_routes) || (use_routes && strlen (routes_entry) > 0)) &&
	/* validate ca/cert/key files */
	g_file_test( ca, G_FILE_TEST_IS_REGULAR) &&
	g_file_test( cert, G_FILE_TEST_IS_REGULAR) &&
	g_file_test( key, G_FILE_TEST_IS_REGULAR) ) {

      is_valid = TRUE;
    }
    
  }

  /* validate routes: each entry must be of the form 'a.b.c.d/mask' */
  if (is_valid) {
    GSList *i;
    GSList *routes;
    
    routes = get_routes (impl);
      
    for (i = routes; is_valid && (i != NULL); i = g_slist_next (i)) {
      is_valid = (is_valid && check_net_cidr ( i->data ));
    }
      
    if (routes != NULL) {
      g_slist_foreach (routes, (GFunc)g_free, NULL);
      g_slist_free (routes);
    }
  }

  return is_valid;
}


static void 
use_routes_toggled (GtkToggleButton *togglebutton, gpointer user_data)
{
  NetworkManagerVpnUIImpl *impl = (NetworkManagerVpnUIImpl *) user_data;

  gtk_widget_set_sensitive (GTK_WIDGET (impl->w_routes), 
			    gtk_toggle_button_get_active (togglebutton));

  if (impl->callback != NULL) {
    gboolean is_valid;

    is_valid = impl_is_valid (&(impl->parent));
    impl->callback (&(impl->parent), is_valid, impl->callback_user_data);
  }
}


static void 
editable_changed (GtkEditable *editable, gpointer user_data)
{
  NetworkManagerVpnUIImpl *impl = (NetworkManagerVpnUIImpl *) user_data;

  if (impl->callback != NULL) {
    gboolean is_valid;

    is_valid = impl_is_valid (&(impl->parent));
    impl->callback (&(impl->parent), is_valid, impl->callback_user_data);
  }

  // Sync X.509 and password CA, we save the same for both. Since this is ONE
  // connection we do not expect the value to change
  if ( GTK_ENTRY (editable) == impl->w_ca ) {
    gtk_entry_set_text ( impl->w_password_ca, gtk_entry_get_text (GTK_ENTRY (impl->w_ca)));
  } else if ( GTK_ENTRY (editable) == impl->w_password_ca ) {
    gtk_entry_set_text ( impl->w_ca, gtk_entry_get_text (GTK_ENTRY (impl->w_password_ca)));    
  }
}


static void 
impl_set_validity_changed_callback (NetworkManagerVpnUI *self, 
				    NetworkManagerVpnUIDialogValidityCallback callback,
				    gpointer user_data)
{
  NetworkManagerVpnUIImpl *impl = (NetworkManagerVpnUIImpl *) self->data;

  impl->callback = callback;
  impl->callback_user_data = user_data;
}

static void
impl_get_confirmation_details (NetworkManagerVpnUI *self, gchar **retval)
{
  GString *buf;
  NetworkManagerVpnUIImpl *impl = (NetworkManagerVpnUIImpl *) self->data;
  const char *connectionname;
  const char *remote;
  const char *ca;
  const char *cert;
  const char *key;
  const char *shared_key;
  const char *local_ip;
  const char *remote_ip;
  const char *username;
  gboolean use_routes;
  const char *routes;
  gboolean use_lzo;
  gboolean use_tap;
  gboolean use_tcp;
  gint connection_type;

  connectionname         = gtk_entry_get_text (impl->w_connection_name);
  connection_type        = gtk_combo_box_get_active (impl->w_connection_type);
  remote                 = gtk_entry_get_text (impl->w_remote);
  cert                   = gtk_entry_get_text (impl->w_cert);
  key                    = gtk_entry_get_text (impl->w_key);
  shared_key             = gtk_entry_get_text (impl->w_shared_key);
  local_ip               = gtk_entry_get_text (impl->w_local_ip);
  remote_ip              = gtk_entry_get_text (impl->w_remote_ip);
  username               = gtk_entry_get_text (impl->w_username);
  use_routes             = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (impl->w_use_routes));
  routes                 = gtk_entry_get_text (impl->w_routes);
  use_lzo                = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (impl->w_use_lzo));
  use_tap                = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (impl->w_use_tap));
  use_tcp                = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (impl->w_use_tcp));

  
  // This is risky, should be variable length depending on actual data!
  buf = g_string_sized_new (512);

  g_string_append (buf, _("The following OpenVPN connection will be created:"));
  g_string_append (buf, "\n\n\t");
  g_string_append_printf (buf, _("Name:  %s"), connectionname);
  g_string_append (buf, "\n\n\t");

  switch ( connection_type ) {

  case NM_OPENVPN_CONTYPE_X509:
    ca = gtk_entry_get_text (impl->w_ca);

    g_string_append (buf, _("Connection Type: X.509 Certificates"));

    g_string_append (buf, "\n\t");
    g_string_append_printf (buf, _("CA:  %s"), ca);

    g_string_append (buf, "\n\t");
    g_string_append_printf (buf, _("Cert:  %s"), cert);

    g_string_append (buf, "\n\t");
    g_string_append_printf (buf, _("Key:  %s"), key);
    break;

  case NM_OPENVPN_CONTYPE_SHAREDKEY:
    g_string_append (buf, _("Connection Type: Shared Key"));

    g_string_append (buf, "\n\t");
    g_string_append_printf (buf, _("Shared Key:  %s"), shared_key);

    g_string_append (buf, "\n\t");
    g_string_append_printf (buf, _("Local IP:  %s"), local_ip);

    g_string_append (buf, "\n\t");
    g_string_append_printf (buf, _("Remote IP:  %s"), remote_ip);
    break;

  case NM_OPENVPN_CONTYPE_PASSWORD:
    ca = gtk_entry_get_text (impl->w_password_ca);
    g_string_append (buf, _("Connection Type: Password"));

    g_string_append (buf, "\n\t");
    g_string_append_printf (buf, _("CA:  %s"), ca);

    g_string_append (buf, "\n\t");
    g_string_append_printf (buf, _("Username:  %s"), username);
    break;

  case NM_OPENVPN_CONTYPE_X509USERPASS:
    ca = gtk_entry_get_text (impl->w_ca);

    g_string_append (buf, _("Connection Type: X.509 with Password Authentication"));

    g_string_append (buf, "\n\t");
    g_string_append_printf (buf, _("CA:  %s"), ca);

    g_string_append (buf, "\n\t");
    g_string_append_printf (buf, _("Cert:  %s"), cert);

    g_string_append (buf, "\n\t");
    g_string_append_printf (buf, _("Key:  %s"), key);

    g_string_append (buf, "\n\t");
    g_string_append_printf (buf, _("Username:  %s"), username);
    break;

  }

  g_string_append (buf, "\n\t");
  g_string_append_printf (buf, _("Remote:  %s"), remote);

  g_string_append (buf, "\n\t");
  g_string_append_printf( buf, _("Device: %s"), ((use_tap) ? _("TAP") : _("TUN")));

  g_string_append (buf, "\n\t");
  g_string_append_printf( buf, _("Protocol: %s"), ((use_tcp) ? _("TCP") : _("UDP")));

  if (use_routes) {
    g_string_append (buf, "\n\t");
    g_string_append_printf (buf, _("Routes:  %s"), routes);
  }

  g_string_append (buf, "\n\t");
  g_string_append_printf( buf, _("Use LZO Compression: %s"), ((use_lzo) ? _("Yes") : _("No")));

  g_string_append (buf, "\n\n");
  g_string_append (buf, _("The connection details can be changed using the \"Edit\" button."));
  g_string_append (buf, "\n");

  *retval = g_string_free (buf, FALSE);
}

static gboolean
import_from_file (NetworkManagerVpnUIImpl *impl, const char *path)
{
  char *basename;
  GKeyFile *keyfile;
  gboolean file_is_good;

  file_is_good = TRUE;
  basename = g_path_get_basename (path);

  keyfile = g_key_file_new ();
  if (g_key_file_load_from_file (keyfile, path, 0, NULL)) {
    char *connectionname = NULL;
    char *remote = NULL;
    char *ca = NULL;
    char *cert = NULL;
    char *key = NULL;
    char *routes = NULL;
    char *lzo = NULL;
    char *dev = NULL;
    char *proto = NULL;
    char *connection_type = NULL;
    char *shared_key = NULL;
    char *local_ip = NULL;
    char *remote_ip = NULL;
    char *username = NULL;
    gboolean should_expand;

    connectionname  = g_key_file_get_string (keyfile, "openvpn", "description", NULL);
    connection_type = g_key_file_get_string (keyfile, "openvpn", "connection-type", NULL);
    remote          = g_key_file_get_string (keyfile, "openvpn", "remote", NULL);
    dev             = g_key_file_get_string (keyfile, "openvpn", "dev", NULL);
    proto           = g_key_file_get_string (keyfile, "openvpn", "proto", NULL);
    ca              = g_key_file_get_string (keyfile, "openvpn", "ca", NULL);
    cert            = g_key_file_get_string (keyfile, "openvpn", "cert", NULL);
    key             = g_key_file_get_string (keyfile, "openvpn", "key", NULL);
    lzo             = g_key_file_get_string (keyfile, "openvpn", "comp-lzo", NULL);
    shared_key      = g_key_file_get_string (keyfile, "openvpn", "shared-key", NULL);
    local_ip        = g_key_file_get_string (keyfile, "openvpn", "local-ip", NULL);
    remote_ip       = g_key_file_get_string (keyfile, "openvpn", "remote-ip", NULL);
    username        = g_key_file_get_string (keyfile, "openvpn", "username", NULL);

    /* may not exist */
    if ((routes = g_key_file_get_string (keyfile, "openvpn", "routes", NULL)) == NULL)
      routes = g_strdup ("");

    /* sanity check data */
    if ( (connectionname != NULL) &&
	 (remote != NULL ) &&
	 (dev != NULL) &&
	 (proto != NULL) &&
	 (connection_type != NULL) &&
	 (strlen (remote) > 0) &&
	 (strlen (dev) > 0) &&
	 (strlen (proto) > 0) &&
	 (strlen (connectionname) > 0) ) {

      // Basics ok, now check per poosible mode

      if (strcmp (connection_type, "x509") == 0) {
	if ( (ca != NULL ) &&
	     (cert != NULL ) &&
	     (key != NULL ) &&
	     (strlen(ca) > 0) &&
	     (strlen(cert) > 0) &&
	     (strlen(key) > 0) ) {

	  gtk_entry_set_text (impl->w_ca, ca);
	  gtk_entry_set_text (impl->w_password_ca, ca);
	  gtk_entry_set_text (impl->w_cert, cert);
	  gtk_entry_set_text (impl->w_key, key);
	} else {
	  file_is_good = FALSE;
	}
      } else if (strcmp (connection_type, "shared-key") == 0) {
	if ( (shared_key != NULL ) &&
	     (local_ip != NULL ) &&
	     (remote_ip != NULL ) &&
	     (strlen(shared_key) > 0) &&
	     (strlen(local_ip) > 0) &&
	     (strlen(remote_ip) > 0) &&
	     check_ip (local_ip) &&
	     check_ip (remote_ip) ) {

	  gtk_entry_set_text (impl->w_shared_key, shared_key);
	  gtk_entry_set_text (impl->w_local_ip, local_ip);
	  gtk_entry_set_text (impl->w_remote_ip, remote_ip);
	} else {
	  file_is_good = FALSE;
	}
      } else if (strcmp (connection_type, "password") == 0) {
	if ( (username != NULL ) &&
	     (strlen(username) > 0) ) {

	  gtk_entry_set_text (impl->w_username, username);
	  gtk_entry_set_text (impl->w_password_ca, ca);
	  gtk_entry_set_text (impl->w_ca, ca);
	} else {
	  file_is_good = FALSE;
	}	
      } else {
	// no connection type given in config
	file_is_good = FALSE;
      }
    } else {
      // invlid basic data
      file_is_good = FALSE;
    }

    if (file_is_good) {
      should_expand = FALSE;

      gtk_entry_set_text (impl->w_connection_name, connectionname);
      gtk_entry_set_text (impl->w_remote, remote);

      if ( (lzo != NULL) && (strcmp(lzo, "yes") == 0) ) {
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (impl->w_use_lzo), TRUE);
	should_expand = TRUE;
      }

      if ( strcmp (dev, "tap") == 0 ) {
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (impl->w_use_tap), TRUE);
	should_expand = TRUE;
      }

      if ( strcmp (proto, "tcp") == 0 ) {
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (impl->w_use_tcp), TRUE);
	should_expand = TRUE;
      }

      if ( strlen (routes) > 0 ) {
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (impl->w_use_routes), TRUE);
	should_expand = TRUE;
	gtk_entry_set_text (impl->w_routes, routes);
	gtk_widget_set_sensitive (GTK_WIDGET (impl->w_routes), TRUE);
      }

      gtk_expander_set_expanded (impl->w_opt_info_expander, should_expand);
    } else {
      GtkWidget *dialog;
		
      dialog = gtk_message_dialog_new (NULL,
				       GTK_DIALOG_DESTROY_WITH_PARENT,
				       GTK_MESSAGE_WARNING,
				       GTK_BUTTONS_CLOSE,
				       _("Cannot import settings"));
      gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
						_("The VPN settings file '%s' does not contain valid data."), basename);
      gtk_dialog_run (GTK_DIALOG (dialog));
      gtk_widget_destroy (dialog);
    }

    g_key_file_free (keyfile);

    g_free (connectionname);
    g_free (connection_type);
    g_free (remote);
    g_free (dev);
    g_free (proto);
    g_free (ca);
    g_free (cert);
    g_free (key);
    g_free (lzo);
    g_free (shared_key);
    g_free (local_ip);
    g_free (remote_ip);
    g_free (username);
  }

  g_free (basename);

  return file_is_good;
}

static void
import_button_clicked (GtkButton *button, gpointer user_data)
{
  char *filename = NULL;
  GtkWidget *dialog;
  NetworkManagerVpnUIImpl *impl = (NetworkManagerVpnUIImpl *) user_data;

  dialog = gtk_file_chooser_dialog_new (_("Select file to import"),
					NULL,
					GTK_FILE_CHOOSER_ACTION_OPEN,
					GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
					NULL);
  
  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
		
    filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
    /*printf ("User selected '%s'\n", filename);*/

  }
	
  gtk_widget_destroy (dialog);

  if (filename != NULL) {
    import_from_file (impl, filename);
    g_free (filename);
  }      
}


static void
connection_type_changed (GtkComboBox *box, gpointer user_data)
{
  int i;
  NetworkManagerVpnUIImpl *impl = (NetworkManagerVpnUIImpl *) user_data;
  gint sel = gtk_combo_box_get_active( box );

  switch ( sel ) {
  case NM_OPENVPN_CONTYPE_X509:
  case NM_OPENVPN_CONTYPE_SHAREDKEY:
  case NM_OPENVPN_CONTYPE_PASSWORD:
    {
      gtk_notebook_set_current_page( impl->w_settings_notebook, sel );
      for (i = NM_OPENVPN_CONTYPE_X509; i <= NM_OPENVPN_CONTYPE_PASSWORD; ++i) {
	GtkWidget *tab = GTK_WIDGET ( gtk_notebook_get_nth_page( GTK_NOTEBOOK (impl->w_settings_notebook), i));
	gtk_widget_set_sensitive( tab, (i == sel));
	gtk_widget_set_sensitive( GTK_WIDGET ( gtk_notebook_get_tab_label( GTK_NOTEBOOK (impl->w_settings_notebook), tab) ), (i == sel));
      }
    }
    break;
  case NM_OPENVPN_CONTYPE_X509USERPASS:
    {
      GtkWidget *tab;

      tab = GTK_WIDGET ( gtk_notebook_get_nth_page( GTK_NOTEBOOK (impl->w_settings_notebook),
						    NM_OPENVPN_CONTYPE_X509));
      gtk_widget_set_sensitive( tab, TRUE);
      gtk_widget_set_sensitive( GTK_WIDGET ( gtk_notebook_get_tab_label( GTK_NOTEBOOK (impl->w_settings_notebook), tab) ), TRUE);

      tab = GTK_WIDGET ( gtk_notebook_get_nth_page( GTK_NOTEBOOK (impl->w_settings_notebook),
						    NM_OPENVPN_CONTYPE_SHAREDKEY));
      gtk_widget_set_sensitive( tab, FALSE);
      gtk_widget_set_sensitive( GTK_WIDGET ( gtk_notebook_get_tab_label( GTK_NOTEBOOK (impl->w_settings_notebook), tab) ), FALSE);

      tab = GTK_WIDGET ( gtk_notebook_get_nth_page( GTK_NOTEBOOK (impl->w_settings_notebook),
						    NM_OPENVPN_CONTYPE_PASSWORD));
      gtk_widget_set_sensitive( tab, TRUE);
      gtk_widget_set_sensitive( GTK_WIDGET ( gtk_notebook_get_tab_label( GTK_NOTEBOOK (impl->w_settings_notebook), tab) ), TRUE);

    }
    gtk_notebook_set_current_page( impl->w_settings_notebook, NM_OPENVPN_CONTYPE_X509 );
    break;
  }
}

static void
open_button_clicked (GtkButton *button, gpointer user_data)
{

  NetworkManagerVpnUIImpl *impl = (NetworkManagerVpnUIImpl *)user_data;
  GtkWidget *dialog;

  const char *msg;
  GtkEntry *entry;

  gchar *dir;

  if ( button == impl->w_button_ca ) {
    msg = _("Select CA to use");
    entry = impl->w_ca;
  } else if ( button == impl->w_button_cert ) {
    msg = _("Select certificate to use");
    entry = impl->w_cert;
  } else if ( button == impl->w_button_key ) {
    msg = _("Select key to use");
    entry = impl->w_key;
  } else if ( button == impl->w_button_shared_key ) {
    msg = _("Select shared key to use");
    entry = impl->w_shared_key;
  } else if ( button == impl->w_button_password_ca ) {
    msg = _("Select CA to use");
    entry = impl->w_password_ca;
  } else {
    return;
  }

  dialog = gtk_file_chooser_dialog_new (msg,
					NULL,
					GTK_FILE_CHOOSER_ACTION_OPEN,
					GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
					NULL);
  
  if ( impl->last_fc_dir != NULL ) {
    gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), impl->last_fc_dir);
  }
				  
  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
    gtk_entry_set_text (entry, gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog)));
    dir = gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (dialog));
    g_free( impl->last_fc_dir );
    impl->last_fc_dir = dir;
  }

  gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
  gtk_widget_destroy (dialog);

}

static gboolean 
impl_can_export (NetworkManagerVpnUI *self)
{
  return TRUE;
}

static gboolean 
impl_import_file (NetworkManagerVpnUI *self, const char *path)
{
  NetworkManagerVpnUIImpl *impl = (NetworkManagerVpnUIImpl *) self->data;

  return import_from_file (impl, path);
}

static gboolean
export_to_file (NetworkManagerVpnUIImpl *impl, const char *path, 
		GSList *properties, GSList *routes, const char *connection_name)
{
  FILE *f;
  GSList *i;
  const char *connection_type = NULL;
  const char *remote = NULL;
  const char *dev = NULL;
  const char *proto = NULL;
  const char *ca = NULL;
  const char *cert = NULL;
  const char *key = NULL;
  const char *lzo = NULL;
  const char *shared_key = NULL;
  const char *local_ip = NULL;
  const char *remote_ip = NULL;
  const char *username = NULL;
  char *routes_str = NULL;
  gboolean ret;

  /*printf ("in export_to_file; path='%s'\n", path);*/

  for (i = properties; i != NULL && g_slist_next (i) != NULL; i = g_slist_next (g_slist_next (i))) {
    const char *key;
    const char *value;

    key = i->data;
    value = (g_slist_next (i))->data;

    if (strcmp (key, "remote") == 0) {
      remote = value;
    } else if (strcmp (key, "dev") == 0) {
      dev = value;
    } else if (strcmp (key, "proto") == 0) {
      proto = value;
    } else if (strcmp (key, "ca") == 0) {
      ca = value;
    } else if (strcmp (key, "cert") == 0) {
      cert = value;
    } else if (strcmp (key, "key") == 0) {
      key = value;
    } else if (strcmp (key, "comp-lzo") == 0) {
      lzo = value;
    } else if (strcmp (key, "shared-key") == 0) {
      shared_key = value;
    } else if (strcmp (key, "local-ip") == 0) {
      local_ip = value;
    } else if (strcmp (key, "remote-ip") == 0) {
      remote_ip = value;
    } else if (strcmp (key, "username") == 0) {
      username = value;
    } else if (strcmp (key, "connection-type") == 0) {
      connection_type = value;
    }
  }


  if (routes != NULL) {
    GString *str;

    str = g_string_new ("routes=");
    for (i = routes; i != NULL; i = g_slist_next (i)) {
      const char *route;
      
      if (i != routes)
	g_string_append_c (str, ' ');
			
      route = (const char *) i->data;
      g_string_append (str, route);
    }

    g_string_append_c (str, '\n');

    routes_str = g_string_free (str, FALSE);
  }

  f = fopen (path, "w");
  if (f != NULL) {

    fprintf (f, 
	     "[openvpn]\n"
	     "description=%s\n"
	     "connection-type=%s\n"
	     "remote=%s\n"
	     "dev=%s\n"
	     "proto=%s\n"
	     "ca=%s\n"
	     "cert=%s\n"
	     "key=%s\n"
	     "comp-lzo=%s\n"
	     "shared-key=%s\n"
	     "local-ip=%s\n"
	     "remote-ip=%s\n"
	     "username=%s\n"
	     "routes=%s\n",
	     /* Description */ connection_name,
	     /* conn type */   connection_type,
	     /* Host */        remote,
	     /* TUN or TAP */  dev,
	     /* TCP or UDP */  proto,
	     /* CA */          ca,
	     /* Cert */        cert,
	     /* Key */         key,
	     /* Comp-LZO */    lzo,
	     /* Shared key */  shared_key,
	     /* local ip */    local_ip,
	     /* remote ip */   remote_ip,
	     /* username */    username,
	     /* X-NM-Routes */ routes_str != NULL ? routes_str : "");

    fclose (f);
    ret = TRUE;
  }
  	else
		ret = FALSE;
  g_free (routes_str);
  return ret;
}


static gboolean 
impl_export (NetworkManagerVpnUI *self, GSList *properties, GSList *routes, const char *connection_name)
{
  char *suggested_name;
  char *path = NULL;
  GtkWidget *dialog;
  NetworkManagerVpnUIImpl *impl = (NetworkManagerVpnUIImpl *) self->data;

  /*printf ("in impl_export\n");*/

  dialog = gtk_file_chooser_dialog_new (_("Save as..."),
					NULL,
					GTK_FILE_CHOOSER_ACTION_SAVE,
					GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
					NULL);

  suggested_name = g_strdup_printf ("%s.pcf", connection_name);
  gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog), suggested_name);
  g_free (suggested_name);

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
    {
      
      path = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
      /*printf ("User selected '%s'\n", path);*/
      
    }
	
  gtk_widget_destroy (dialog);

  if (path != NULL) {
    if (g_file_test (path, G_FILE_TEST_EXISTS)) {
      int response;
      GtkWidget *dialog;

      dialog = gtk_message_dialog_new (NULL,
				       GTK_DIALOG_DESTROY_WITH_PARENT,
				       GTK_MESSAGE_QUESTION,
				       GTK_BUTTONS_CANCEL,
				       _("A file named \"%s\" already exists."), path);
      gtk_dialog_add_buttons (GTK_DIALOG (dialog), "_Replace", GTK_RESPONSE_OK, NULL);
      gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
						_("Do you want to replace it with the one you are saving?"));
      response = gtk_dialog_run (GTK_DIALOG (dialog));
      gtk_widget_destroy (dialog);
      if (response == GTK_RESPONSE_OK)
          if (!export_to_file (impl, path, properties, routes, connection_name)) {
			GtkWidget *dialog;

			dialog = gtk_message_dialog_new (NULL,
									   GTK_DIALOG_DESTROY_WITH_PARENT,
									   GTK_MESSAGE_WARNING,
									   GTK_BUTTONS_CLOSE,
									   _("Failed to export configuration"));
			gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
											  _("Failed to save file %s"), path);
			gtk_dialog_run (GTK_DIALOG (dialog));
			gtk_widget_destroy (dialog);
		}
    }
  }      

  g_free (path);

  return TRUE;
}


static NetworkManagerVpnUI* 
impl_get_object (void)
{
  char *glade_file;
  NetworkManagerVpnUIImpl *impl;

  impl = g_new0 (NetworkManagerVpnUIImpl, 1);

  impl->last_fc_dir = NULL;

  glade_file = g_strdup_printf ("%s/%s", GLADEDIR, "nm-openvpn-dialog.glade");
  impl->xml = glade_xml_new (glade_file, NULL, GETTEXT_PACKAGE);
  g_free( glade_file );
  if (impl->xml != NULL) {

    impl->widget = glade_xml_get_widget(impl->xml, "nm-openvpn-widget");

    impl->w_connection_name        = GTK_ENTRY (glade_xml_get_widget (impl->xml, "openvpn-connection-name"));
    impl->w_remote                = GTK_ENTRY (glade_xml_get_widget (impl->xml, "openvpn-remote"));
    impl->w_use_routes             = GTK_CHECK_BUTTON (glade_xml_get_widget (impl->xml, "openvpn-use-routes"));
    impl->w_routes                 = GTK_ENTRY (glade_xml_get_widget (impl->xml, "openvpn-routes"));
    impl->w_opt_info_expander      = GTK_EXPANDER (glade_xml_get_widget (impl->xml, 
									 "openvpn-optional-information-expander"));
    impl->w_import_button          = GTK_BUTTON (glade_xml_get_widget (impl->xml, 
								       "openvpn-import-button"));

    impl->w_ca                     = GTK_ENTRY( glade_xml_get_widget( impl->xml, "openvpn-ca" ) );
    impl->w_cert                   = GTK_ENTRY( glade_xml_get_widget( impl->xml, "openvpn-cert" ) );
    impl->w_key                    = GTK_ENTRY( glade_xml_get_widget( impl->xml, "openvpn-key" ) );

    impl->w_button_ca              = GTK_BUTTON( glade_xml_get_widget( impl->xml, "openvpn-but-ca" ) );
    impl->w_button_cert            = GTK_BUTTON( glade_xml_get_widget( impl->xml, "openvpn-but-cert" ) );
    impl->w_button_key             = GTK_BUTTON( glade_xml_get_widget( impl->xml, "openvpn-but-key" ) );

    impl->w_use_lzo                = GTK_CHECK_BUTTON (glade_xml_get_widget (impl->xml, "openvpn-use-lzo"));
    impl->w_use_tap                = GTK_CHECK_BUTTON (glade_xml_get_widget (impl->xml, "openvpn-use-tap"));
    impl->w_use_tcp                = GTK_CHECK_BUTTON (glade_xml_get_widget (impl->xml, "openvpn-use-tcp"));

    impl->w_connection_type        = GTK_COMBO_BOX (glade_xml_get_widget (impl->xml, "openvpn-connection-type"));
    impl->w_settings_notebook      = GTK_NOTEBOOK (glade_xml_get_widget (impl->xml, "openvpn-settings"));

    impl->w_button_shared_key      = GTK_BUTTON( glade_xml_get_widget( impl->xml, "openvpn-but-shared-key" ) );
    impl->w_shared_key             = GTK_ENTRY( glade_xml_get_widget( impl->xml, "openvpn-shared-key" ) );
    impl->w_local_ip               = GTK_ENTRY( glade_xml_get_widget( impl->xml, "openvpn-local-ip" ) );
    impl->w_remote_ip              = GTK_ENTRY( glade_xml_get_widget( impl->xml, "openvpn-remote-ip" ) );

    impl->w_username               = GTK_ENTRY( glade_xml_get_widget( impl->xml, "openvpn-username" ) );
    impl->w_password_ca            = GTK_ENTRY( glade_xml_get_widget( impl->xml, "openvpn-password-ca" ) );
    impl->w_button_password_ca     = GTK_BUTTON( glade_xml_get_widget( impl->xml, "openvpn-password-but-ca" ) );

    impl->callback                 = NULL;


    gtk_signal_connect (GTK_OBJECT (impl->w_use_routes), 
			"toggled", GTK_SIGNAL_FUNC (use_routes_toggled), impl);

    gtk_signal_connect (GTK_OBJECT (impl->w_connection_name), 
			"changed", GTK_SIGNAL_FUNC (editable_changed), impl);
    gtk_signal_connect (GTK_OBJECT (impl->w_remote), 
			"changed", GTK_SIGNAL_FUNC (editable_changed), impl);
    gtk_signal_connect (GTK_OBJECT (impl->w_routes), 
			"changed", GTK_SIGNAL_FUNC (editable_changed), impl);
    gtk_signal_connect (GTK_OBJECT (impl->w_ca), 
			"changed", GTK_SIGNAL_FUNC (editable_changed), impl);
    gtk_signal_connect (GTK_OBJECT (impl->w_cert), 
			"changed", GTK_SIGNAL_FUNC (editable_changed), impl);
    gtk_signal_connect (GTK_OBJECT (impl->w_key), 
			"changed", GTK_SIGNAL_FUNC (editable_changed), impl);
    gtk_signal_connect (GTK_OBJECT (impl->w_shared_key), 
			"changed", GTK_SIGNAL_FUNC (editable_changed), impl);
    gtk_signal_connect (GTK_OBJECT (impl->w_local_ip), 
			"changed", GTK_SIGNAL_FUNC (editable_changed), impl);
    gtk_signal_connect (GTK_OBJECT (impl->w_remote_ip), 
			"changed", GTK_SIGNAL_FUNC (editable_changed), impl);
    gtk_signal_connect (GTK_OBJECT (impl->w_username), 
			"changed", GTK_SIGNAL_FUNC (editable_changed), impl);
    gtk_signal_connect (GTK_OBJECT (impl->w_password_ca), 
			"changed", GTK_SIGNAL_FUNC (editable_changed), impl);
    
    
    gtk_signal_connect (GTK_OBJECT (impl->w_button_ca), 
			"clicked", GTK_SIGNAL_FUNC (open_button_clicked), impl);
    gtk_signal_connect (GTK_OBJECT (impl->w_button_cert), 
			"clicked", GTK_SIGNAL_FUNC (open_button_clicked), impl);
    gtk_signal_connect (GTK_OBJECT (impl->w_button_key), 
			"clicked", GTK_SIGNAL_FUNC (open_button_clicked), impl);
    gtk_signal_connect (GTK_OBJECT (impl->w_button_shared_key), 
			"clicked", GTK_SIGNAL_FUNC (open_button_clicked), impl);
    gtk_signal_connect (GTK_OBJECT (impl->w_button_password_ca), 
			"clicked", GTK_SIGNAL_FUNC (open_button_clicked), impl);

    gtk_signal_connect (GTK_OBJECT (impl->w_import_button), 
			"clicked", GTK_SIGNAL_FUNC (import_button_clicked), impl);

    gtk_signal_connect (GTK_OBJECT (impl->w_connection_type),
			"changed", GTK_SIGNAL_FUNC (connection_type_changed), impl);

    /* make the widget reusable */
    gtk_signal_connect (GTK_OBJECT (impl->widget), "delete-event", 
			GTK_SIGNAL_FUNC (gtk_widget_hide_on_delete), NULL);
    
    openvpn_clear_widget (impl);

    impl->parent.get_display_name              = impl_get_display_name;
    impl->parent.get_service_name              = impl_get_service_name;
    impl->parent.get_widget                    = impl_get_widget;
    impl->parent.get_connection_name           = impl_get_connection_name;
    impl->parent.get_properties                = impl_get_properties;
    impl->parent.get_routes                    = impl_get_routes;
    impl->parent.set_validity_changed_callback = impl_set_validity_changed_callback;
    impl->parent.is_valid                      = impl_is_valid;
    impl->parent.get_confirmation_details      = impl_get_confirmation_details;
    impl->parent.can_export                    = impl_can_export;
    impl->parent.import_file                   = impl_import_file;
    impl->parent.export                        = impl_export;
    impl->parent.data                          = impl;
    
    return &(impl->parent);
  } else {
    g_free (impl);
    return NULL;
  }
}

NetworkManagerVpnUI* 
nm_vpn_properties_factory (void)
{
  return impl_get_object();
}
